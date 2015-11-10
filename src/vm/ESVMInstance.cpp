#include "Escargot.h"
#include "parser/ScriptParser.h"
#include "ESVMInstance.h"
#include "runtime/Environment.h"
#include "runtime/ExecutionContext.h"
#include "runtime/GlobalObject.h"
#include "bytecode/ByteCode.h"

#include "BumpPointerAllocator.h"

namespace escargot {

__thread ESVMInstance* currentInstance;

ESVMInstance::ESVMInstance()
{
    GC_add_roots(this, (void *)(((size_t)this + sizeof(ESVMInstance)) + 1));
#ifndef NDEBUG
    m_dumpByteCode = false;
    m_dumpExecuteByteCode = false;
    m_verboseJIT = false;
    m_reportUnsupportedOpcode = false;
#endif
    m_useExprFilter = false;
    m_useCseFilter = false;
    m_jitThreshold = 2;
    m_osrExitThreshold = 1;
    enter();

    m_scriptParser = new(GC) ScriptParser();
    std::srand(std::time(0));

    m_table = new OpcodeTable();
    // init goto table
    interpret(this, NULL, 0);

    clock_gettime(CLOCK_REALTIME, &m_cachedTimeOrigin);
    m_cachedTime = localtime(&m_cachedTimeOrigin.tv_sec);

    /*
    GC_set_on_collection_event([](GC_EventType type){
        if (type == GC_EVENT_RECLAIM_END && ESVMInstance::currentInstance()) {
            ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
        }
    });
    */

    m_identifierCacheInvalidationCheckCount = 0;

    std::setlocale(LC_ALL, "en_US.utf8");
    m_strings.initStaticStrings(this);

    // TODO call destructor
    m_bumpPointerAllocator = new(GC) WTF::BumpPointerAllocator();

    m_globalFunctionPrototype = NULL;

    m_object__proto__AccessorData.setGetter([](ESObject* obj, ESObject* originalObj) -> ESValue {
        return obj->__proto__();
    });

    m_object__proto__AccessorData.setSetter([](::escargot::ESObject* self, ESObject* originalObj, const ESValue& value) -> void {
        if (value.isESPointer() && value.asESPointer()->isESObject()) {
            self->set__proto__(value.asESPointer()->asESObject());
        } else if (value.isUndefined()) {
            self->set__proto__(ESValue());
        } else if (value.isNull()) {
            self->set__proto__(ESValue(ESValue::ESNull));
        }
    });

    m_initialHiddenClassForObject.m_propertyIndexHashMapInfo.insert(std::make_pair(
        m_strings.__proto__,
        0
        ));
    m_initialHiddenClassForObject.m_propertyInfo.push_back(ESHiddenClassPropertyInfo(m_strings.__proto__.string(), false, true, false, false));

    // $19.2.4 Function Instances
    m_initialHiddenClassForFunctionObject = m_initialHiddenClassForObject.defineProperty(m_strings.length, true, false, false, false);
    m_initialHiddenClassForFunctionObject = m_initialHiddenClassForFunctionObject->defineProperty(m_strings.prototype, false, true, false, false);
    m_initialHiddenClassForFunctionObject = m_initialHiddenClassForFunctionObject->defineProperty(m_strings.name, true, false, false, true);

    m_initialHiddenClassForFunctionObjectWithoutPrototype = m_initialHiddenClassForObject.defineProperty(m_strings.length, true, false, false, false);
    m_initialHiddenClassForFunctionObjectWithoutPrototype = m_initialHiddenClassForFunctionObjectWithoutPrototype->defineProperty(m_strings.name, true, false, false, false);

    m_initialHiddenClassForPrototypeObject = m_initialHiddenClassForObject.defineProperty(m_strings.constructor, true, true, false, true);

    m_initialHiddenClassForArrayObject = m_initialHiddenClassForObject.defineProperty(m_strings.length, false, true, false, false);

    m_functionPrototypeAccessorData.setGetter([](ESObject* self, ESObject* originalObj) -> ESValue {
        return self->asESFunctionObject()->protoType();
    });

    m_functionPrototypeAccessorData.setSetter([](::escargot::ESObject* self, ESObject* originalObj, const ESValue& value) {
        self->asESFunctionObject()->setProtoType(value);
    });

    m_arrayLengthAccessorData.setGetter([](ESObject* self, ESObject* originalObj) -> ESValue {
        return ESValue(self->asESArrayObject()->length());
    });

    m_arrayLengthAccessorData.setSetter([](::escargot::ESObject* self, ESObject* originalObj, const ESValue& value) {
        if (!value.isNumber() || value.toNumber() != value.toUint32())
            throw ESValue(RangeError::create());
        self->asESArrayObject()->setLength(value.toInt32());
    });

    m_stringObjectLengthAccessorData.setGetter([](ESObject* self, ESObject* originalObj) -> ESValue {
        return ESValue(self->asESStringObject()->stringData()->length());
    });


    m_globalObject = new GlobalObject();
    m_globalObject->initGlobalObject();

    LexicalEnvironment* a = new LexicalEnvironment(new GlobalEnvironmentRecord(m_globalObject), NULL);

    m_globalExecutionContext = new ExecutionContext(a, true, false, NULL);
    m_currentExecutionContext = m_globalExecutionContext;
    exit();

    GC_gcollect();
}

ESVMInstance::~ESVMInstance()
{
}

ESValue ESVMInstance::evaluate(u16string& source, bool isForGlobalScope)
{
    m_lastExpressionStatementValue = ESValue();
    CodeBlock* block = m_scriptParser->parseScript(this, source, isForGlobalScope);
    interpret(this, block);
    return m_lastExpressionStatementValue;
}

void ESVMInstance::enter()
{
    ASSERT(!escargot::currentInstance);
    escargot::currentInstance = this;
    escargot::strings = &m_strings;
}

void ESVMInstance::exit()
{
    escargot::currentInstance = NULL;
    escargot::strings = NULL;
}

int ESVMInstance::timezoneOffset()
{
    return -m_cachedTime->tm_gmtoff / 60;
}

const tm* ESVMInstance::computeLocalTime(const timespec& ts)
{
    time_t t = ts.tv_sec + m_cachedTime->tm_gmtoff;
    return gmtime(&t);
    // return localtime(&ts.tv_sec);
}

void ESVMInstance::printValue(ESValue val)
{
    std::string str;
    std::function<void(ESValue v)> toString = [&str, &toString](ESValue v)
    {
        if (v.isEmpty()) {
            str.append("[Empty Value]");
        } else if (v.isInt32()) {
            str.append(v.toString()->utf8Data());
        } else if (v.isNumber()) {
            str.append(v.toString()->utf8Data());
        } else if (v.isUndefined()) {
            str.append(v.toString()->utf8Data());
        } else if (v.isNull()) {
            str.append(v.toString()->utf8Data());
        } else if (v.isBoolean()) {
            str.append(v.toString()->utf8Data());
        } else if (v.isESPointer()) {
            ESPointer* o = v.asESPointer();
            if (o->isESString()) {
                str.append(o->asESString()->utf8Data());
            } else if (o->isESFunctionObject()) {
                str.append(v.toString()->utf8Data());
            } else if (o->isESArrayObject()) {
                str.append("[");
                bool isFirst = true;
                o->asESObject()->enumeration([&str, &isFirst, o, &toString](escargot::ESValue key) {
                    if (!isFirst)
                        str.append(",");
                    str.append(key.toString()->utf8Data());
                    str.append(": ");
                    str.append(o->asESObject()->getOwnProperty(key).toString()->utf8Data());
                    isFirst = false;
                });
                str.append("]");
            } else if (o->isESErrorObject()) {
                str.append(v.toString()->utf8Data());
            } else if (o->isESObject()) {
                if (o->asESObject()->get(ESValue(currentInstance()->strings().constructor)).isESPointer() && o->asESObject()->get(ESValue(currentInstance()->strings().constructor)).asESPointer()->isESObject())
                    str.append(o->asESObject()->get(ESValue(currentInstance()->strings().constructor)).asESPointer()->asESObject()->get(ESValue(currentInstance()->strings().name)).toString()->utf8Data());
                str.append(" {");
                bool isFirst = true;
                o->asESObject()->enumeration([&str, &isFirst, o, &toString](escargot::ESValue key) {
                    if (!isFirst)
                        str.append(", ");
                    str.append(key.toString()->utf8Data());
                    str.append(": ");
                    str.append(o->asESObject()->getOwnProperty(key).toString()->utf8Data());
                    // toString(slot.value(o->asESObject()));
                    isFirst = false;
                });
                if (o->isESStringObject()) {
                    str.append(", [[PrimitiveValue]]: \"");
                    str.append(o->asESStringObject()->stringData()->utf8Data());
                    str.append("\"");
                }
                str.append("}");
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
        } else {
#if ESCARGOT_64
            printf("Invalid ESValue Format : 0x%lx\n", v.asRawData());
#else
            printf("Invalid ESValue Format : 0x%llx\n", v.asRawData());
#endif
            ASSERT(false);
        }
    };
    toString(val);

    printf("%s\n", str.data());
    fflush(stdout);
}

}
