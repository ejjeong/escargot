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
    m_identifierCacheInvalidationCheckCount = 0;

    std::setlocale(LC_ALL, "en_US.utf8");
    emptyStringData.initHash();
    m_strings.initStaticStrings(this);

    enter();
    m_globalObject = new GlobalObject();
    LexicalEnvironment* a = new LexicalEnvironment(new GlobalEnvironmentRecord(m_globalObject), NULL);

    m_globalExecutionContext = new ExecutionContext(a, true);
    m_currentExecutionContext = m_globalExecutionContext;

    exit();
}

void ESVMInstance::evaluate(const std::string& source)
{
    try {
        Node* node = ESScriptParser::parseScript(this, source.c_str());
        node->execute(this);
    } catch(ReferenceError& err) {
        wprintf(L"ReferenceError - %ls\n", err.identifier().data());
    } catch(TypeError& err) {
        wprintf(L"TypeError\n");
    } catch(const ESValue& err) {
        wprintf(L"Uncaught %ls\n", err.toInternalString().data());
    }

    /*
    //test/basic_ctx1.js
    ESValue* v = m_globalObject->get("a");
    ASSERT(v->toSmi()->value() == 1);
    */
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

//$7.1.13 ToObject()
ESObject* ESVMInstance::ToObject(ESValue argument) {
    if (argument.isNumber()) {
        ESFunctionObject* function = this->globalObject()->number();
        ESObject* receiver = ESNumberObject::create(argument);
        receiver->setConstructor(function);
        receiver->set__proto__(function->protoType());
        std::vector<ESValue, gc_allocator<ESValue>> arguments;
        arguments.push_back(argument);
        ESFunctionObject::call((ESValue) function, receiver, &arguments[0], arguments.size(), this);
        return receiver;
    }
   return ESObject::create();
}

}
