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

    m_functionPrototypeAccessorData.m_getter = [](ESObject* self) -> ESValue {
        return self->asESFunctionObject()->protoType();
    };

    m_functionPrototypeAccessorData.m_setter = [](::escargot::ESObject* self, ESValue value){
        if(value.isESPointer() && value.asESPointer()->isESObject())
            self->asESFunctionObject()->setProtoType(value.asESPointer()->asESObject());
    };

    m_arrayLengthAccessorData.m_getter = [](ESObject* self) -> ESValue {
        return self->asESArrayObject()->length();
    };

    m_arrayLengthAccessorData.m_setter = [](::escargot::ESObject* self, ESValue value) {
        ESValue len = ESValue(value.asInt32());
        self->asESArrayObject()->setLength(len);
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
    ESValue ret;
    try {
        Node* node = ESScriptParser::parseScript(this, source);
        ret = node->execute(this);
    } catch(ReferenceError& err) {
        printf("ReferenceError: %s\n", err.message()->utf8Data());
    } catch(TypeError& err) {
        printf("TypeError: %s\n", err.message()->utf8Data());
    } catch(SyntaxError& err) {
        printf("SyntaxError: %s\n", err.message()->utf8Data());
    } catch(const ESValue& err) {
        try{
            printf("Uncaught %s\n", err.toString()->utf8Data());
        } catch(...) {
            printf("an error occur in catch-block\n");
        }
    }

    return ret;
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
    time_t diff = ts.tv_sec - m_cachedTimeOrigin.tv_sec;
    if(diff < 0)
        return localtime(&ts.tv_sec);
    m_time = *m_cachedTime;
    m_time.tm_hour += diff / 3600;
    diff = diff % 3600;
    m_time.tm_min += diff / 60;
    diff = diff % 60;
    m_time.tm_sec += diff;
    if(m_time.tm_sec > 60) {
        m_time.tm_sec = m_time.tm_sec % 60;
        m_time.tm_min++;
    }
    if(m_time.tm_min > 60) {
        m_time.tm_min = m_time.tm_min % 60;
        m_time.tm_hour++;
    }
    if(m_time.tm_hour > 24) {
        //slow case
        return localtime(&ts.tv_sec);
    }
    return &m_time;
}

}
