#include "Escargot.h"
#include "ESVMInstance.h"
#include "runtime/Environment.h"
#include "runtime/ExecutionContext.h"
#include "runtime/GlobalObject.h"
#include "parser/ESScriptParser.h"

namespace escargot {

__thread ESVMInstance* currentInstance;

ESVMInstance::ESVMInstance()
{
    GC_set_on_collection_event([](GC_EventType type){
        if(type == GC_EVENT_RECLAIM_END && ESVMInstance::currentInstance())
            ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
    });

    m_identifierCacheInvalidationCheckCount = 0;

    std::setlocale(LC_ALL, "en_US.utf8");
    m_strings.initStaticStrings(this);

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
    LexicalEnvironment* a = new LexicalEnvironment(new GlobalEnvironmentRecord(m_globalObject), NULL);

    m_globalExecutionContext = new ExecutionContext(a, true, false, NULL);
    m_currentExecutionContext = m_globalExecutionContext;
    exit();

    escargot::ESScriptParser::enter();

    GC_gcollect();
}

ESVMInstance::~ESVMInstance()
{
    escargot::ESScriptParser::exit();
}

ESValue ESVMInstance::evaluate(const std::string& source)
{
    ESValue ret;
    try {
        Node* node = ESScriptParser::parseScript(this, source);
        ret = node->execute(this);
    } catch(ReferenceError& err) {
        wprintf(L"ReferenceError: %ls\n", err.message()->data());
    } catch(TypeError& err) {
        wprintf(L"TypeError: %ls\n", err.message()->data());
    } catch(SyntaxError& err) {
        wprintf(L"SyntaxError: %ls\n", err.message()->data());
    } catch(const ESValue& err) {
        wprintf(L"Uncaught %ls\n", err.toString()->data());
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

}
