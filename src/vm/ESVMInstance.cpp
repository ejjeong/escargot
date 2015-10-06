#include "Escargot.h"
#include "ESVMInstance.h"
#include "runtime/Environment.h"
#include "runtime/ExecutionContext.h"
#include "runtime/GlobalObject.h"
#include "parser/ESScriptParser.h"
#include "bytecode/ByteCode.h"

#include "BumpPointerAllocator.h"

namespace escargot {

__thread ESVMInstance* currentInstance;

ESVMInstance::ESVMInstance()
{
#ifndef NDEBUG
    m_dumpByteCode = false;
    m_dumpExecuteByteCode = false;
    m_verboseJIT = false;
    m_reportUnsupportedOpcode = false;
#endif
    enter();

    std::srand(std::time(0));

    m_table = new OpcodeTable();
    //init goto table
    interpret(this, NULL, 0);

    clock_gettime(CLOCK_REALTIME,&m_cachedTimeOrigin);
    m_cachedTime = localtime(&m_cachedTimeOrigin.tv_sec);

    GC_set_on_collection_event([](GC_EventType type){
        if(type == GC_EVENT_RECLAIM_END && ESVMInstance::currentInstance())
            ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
    });

    m_identifierCacheInvalidationCheckCount = 0;

    std::setlocale(LC_ALL, "en_US.utf8");
    m_strings.initStaticStrings(this);

    //TODO call destructor
    m_bumpPointerAllocator = new(GC) WTF::BumpPointerAllocator();

    m_globalFunctionPrototype = NULL;

    m_object__proto__AccessorData.setGetter([](ESObject* obj) -> ESValue {
        return obj->__proto__();
    });

    m_object__proto__AccessorData.setSetter([](::escargot::ESObject* self, const ESValue& value) -> void {
        if(value.isESPointer() && value.asESPointer()->isESObject()) {
            self->set__proto__(value.asESPointer()->asESObject());
        } else {
            self->set__proto__(ESValue());
        }
    });

    //FIXME set proper flags(is...)
    m_initialHiddenClassForObject.m_propertyInfo.insert(std::make_pair(
            m_strings.constructor,
            0
            ));
    m_initialHiddenClassForObject.m_propertyFlagInfo.push_back(ESHiddenClassPropertyInfo(true, true, false, false));

    //FIXME set proper flags(is...)
    m_initialHiddenClassForObject.m_propertyInfo.insert(std::make_pair(
            m_strings.__proto__,
            1
            ));
    m_initialHiddenClassForObject.m_propertyFlagInfo.push_back(ESHiddenClassPropertyInfo(false, true, false, false));

    //FIXME set proper flags(is...)
    m_initialHiddenClassForFunction.m_propertyInfo.insert(std::make_pair(
            m_strings.constructor,
            0
            ));
    m_initialHiddenClassForFunction.m_propertyFlagInfo.push_back(ESHiddenClassPropertyInfo(true, true, false, false));

    //FIXME set proper flags(is...)
    m_initialHiddenClassForFunction.m_propertyInfo.insert(std::make_pair(
            m_strings.__proto__,
            1
            ));
    m_initialHiddenClassForFunction.m_propertyFlagInfo.push_back(ESHiddenClassPropertyInfo(false, true, false, false));

    //FIXME set proper flags(is...)
    m_initialHiddenClassForFunction.m_propertyInfo.insert(std::make_pair(
            m_strings.prototype,
            2
            ));
    m_initialHiddenClassForFunction.m_propertyFlagInfo.push_back(ESHiddenClassPropertyInfo(false, true, false, false));

    //FIXME set proper flags(is...)
    m_initialHiddenClassForFunction.m_propertyInfo.insert(std::make_pair(
            m_strings.name,
            3
            ));
    m_initialHiddenClassForFunction.m_propertyFlagInfo.push_back(ESHiddenClassPropertyInfo(true, true, false, false));


    //FIXME set proper flags(is...)
    m_initialHiddenClassForArrayObject.m_propertyInfo.insert(std::make_pair(
            m_strings.constructor,
            0
            ));
    m_initialHiddenClassForArrayObject.m_propertyFlagInfo.push_back(ESHiddenClassPropertyInfo(true, true, false, false));

    //FIXME set proper flags(is...)
    m_initialHiddenClassForArrayObject.m_propertyInfo.insert(std::make_pair(
            m_strings.__proto__,
            1
            ));
    m_initialHiddenClassForArrayObject.m_propertyFlagInfo.push_back(ESHiddenClassPropertyInfo(false, true, false, false));

    //FIXME set proper flags(is...)
    m_initialHiddenClassForArrayObject.m_propertyInfo.insert(std::make_pair(
            m_strings.length,
            2
            ));
    m_initialHiddenClassForArrayObject.m_propertyFlagInfo.push_back(ESHiddenClassPropertyInfo(false, true, false, false));

    m_functionPrototypeAccessorData.setGetter([](ESObject* self) -> ESValue {
        return self->asESFunctionObject()->protoType();
    });

    m_functionPrototypeAccessorData.setSetter([](::escargot::ESObject* self, const ESValue& value){
        self->asESFunctionObject()->setProtoType(value);
    });

    m_arrayLengthAccessorData.setGetter([](ESObject* self) -> ESValue {
        return ESValue(self->asESArrayObject()->length());
    });

    m_arrayLengthAccessorData.setSetter([](::escargot::ESObject* self, const ESValue& value) {
        self->asESArrayObject()->setLength(value.toInt32());
    });

    m_stringObjectLengthAccessorData.setGetter([](ESObject* self) -> ESValue {
        return ESValue(self->asESStringObject()->getStringData()->length());
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

ESValue ESVMInstance::evaluate(u16string& source)
{
    try {
        m_lastExpressionStatementValue = ESValue();
        CodeBlock* block = ESScriptParser::parseScript(this, source);
        interpret(this, block);
    } catch(const ESValue& err) {
        try{
            printf("Uncaught %s\n", err.toString()->utf8Data());
        } catch(...) {
            printf("an error occur in catch-block\n");
        }
        fflush(stdout);
        throw;
    }

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
    return -m_cachedTime->tm_gmtoff/60;
}

const tm* ESVMInstance::computeLocalTime(const timespec& ts)
{
    time_t t = ts.tv_sec + m_cachedTime->tm_gmtoff;
    return gmtime(&t);
    //return localtime(&ts.tv_sec);
}

void ESVMInstance::printValue(ESValue val)
{
    std::string str;
    std::function<void (ESValue v)> toString = [&str, &toString](ESValue v) {
        if(v.isInt32()) {
            str.append(v.toString()->utf8Data());
        } else if(v.isNumber()) {
            str.append(v.toString()->utf8Data());
        } else if(v.isUndefined()) {
            str.append(v.toString()->utf8Data());
        } else if(v.isNull()) {
            str.append(v.toString()->utf8Data());
        } else if(v.isBoolean()) {
            str.append(v.toString()->utf8Data());
        } else if(v.isESPointer()){
            ESPointer* o = v.asESPointer();
            if(o->isESString()) {
                str.append(o->asESString()->utf8Data());
            } else if(o->isESFunctionObject()) {
                str.append(v.toString()->utf8Data());
            } else if(o->isESArrayObject()) {
                str.append("[");
                bool isFirst = true;
                o->asESObject()->enumeration([&str, &isFirst, o, &toString](escargot::ESValue key) {
                    if(!isFirst)
                        str.append(", ");
                    str.append(key.toString()->utf8Data());
                    str.append(": ");
                    str.append(o->asESObject()->get(key, false).toString()->utf8Data());
                    //toString(slot.value(o->asESObject()));
                    isFirst = false;
                    });
                str.append("]");
            } else if(o->isESErrorObject()) {
                str.append(v.toString()->utf8Data());
            } else if(o->isESObject()) {
                if(o->asESObject()->constructor().isESPointer() && o->asESObject()->constructor().asESPointer()->isESObject())
                    str.append(o->asESObject()->constructor().asESPointer()->asESObject()->get(ESValue(currentInstance()->strings().name), true).toString()->utf8Data());
                str.append(" {");
                bool isFirst = true;
                o->asESObject()->enumeration([&str, &isFirst, o, &toString](escargot::ESValue key) {
                    if(!isFirst)
                        str.append(", ");
                        str.append(key.toString()->utf8Data());
                        str.append(": ");
                        str.append(o->asESObject()->get(key, false).toString()->utf8Data());
                        //toString(slot.value(o->asESObject()));
                        isFirst = false;
                    });
                if(o->isESStringObject()) {
                    str.append(", [[PrimitiveValue]]: \"");
                    str.append(o->asESStringObject()->getStringData()->utf8Data());
                    str.append("\"");
                }
                str.append("}");
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
        } else {
            printf("Invalid ESValue Format : 0x%lx\n", v.asRawData());
            ASSERT(false);
        }
    };
    toString(val);

    printf("%s\n", str.data());
    fflush(stdout);
}

}
