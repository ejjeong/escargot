#include "Escargot.h"
#include "ESVMInstance.h"
#include "runtime/Environment.h"
#include "runtime/ExecutionContext.h"
#include "runtime/GlobalObject.h"
#include "parser/ESScriptParser.h"

#include "BumpPointerAllocator.h"

namespace escargot {

__thread ESVMInstance* currentInstance;

ESVMInstance::ESVMInstance()
{
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


    m_object__proto__AccessorData.m_getter = [](ESObject* obj) -> ESValue {
        return obj->__proto__();
    };

    m_object__proto__AccessorData.m_setter = [](::escargot::ESObject* self, ESValue value){
        if(value.isESPointer() && value.asESPointer()->isESObject()) {
            self->set__proto__(value.asESPointer()->asESObject());
        }
    };

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

    m_functionPrototypeAccessorData.m_getter = [](ESObject* self) -> ESValue {
        return self->asESFunctionObject()->protoType();
    };

    m_functionPrototypeAccessorData.m_setter = [](::escargot::ESObject* self, ESValue value){
        if(value.isESPointer() && value.asESPointer()->isESObject())
            self->asESFunctionObject()->setProtoType(value.asESPointer()->asESObject());
    };

    m_arrayLengthAccessorData.m_getter = [](ESObject* self) -> ESValue {
        return ESValue(self->asESArrayObject()->length());
    };

    m_arrayLengthAccessorData.m_setter = [](::escargot::ESObject* self, ESValue value) {
        self->asESArrayObject()->setLength(value.toInt32());
    };

    m_stringObjectLengthAccessorData.m_getter = [](ESObject* self) -> ESValue {
        return ESValue(self->asESStringObject()->getStringData()->length());
    };

    enter();
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
        ProgramNode* node = ESScriptParser::parseScript(this, source);
        node->execute(this);
    } catch(const ESValue& err) {
        try{
            printf("Uncaught %s\n", err.toString()->utf8Data());
        } catch(...) {
            printf("an error occur in catch-block\n");
        }
        fflush(stdout);
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
        } else {
            ESPointer* o = v.asESPointer();
            if(o->isESString()) {
                str.append("\"");
                str.append(o->asESString()->utf8Data());
                str.append("\"");
            } else if(o->isESFunctionObject()) {
                str.append(v.toString()->utf8Data());
            } else if(o->isESArrayObject()) {
                str.append("[");
                bool isFirst = true;
                o->asESObject()->enumeration([&str, &isFirst, o, &toString](escargot::ESValue key, const ::escargot::ESSlotAccessor& slot) {
                    if(!isFirst)
                        str.append(", ");
                        str.append(key.toString()->utf8Data());
                        str.append(": ");
                        str.append(slot.value(o->asESObject()).toString()->utf8Data());
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
                o->asESObject()->enumeration([&str, &isFirst, o, &toString](escargot::ESValue key, const ::escargot::ESSlotAccessor& slot) {
                    if(!isFirst)
                        str.append(", ");
                        str.append(key.toString()->utf8Data());
                        str.append(": ");
                        str.append(slot.value(o->asESObject()).toString()->utf8Data());
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
        }
    };
    toString(val);

    printf("%s\n", str.data());
    fflush(stdout);
}

}
