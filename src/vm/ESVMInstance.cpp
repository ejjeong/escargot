#include "Escargot.h"
#include "ESVMInstance.h"
#include "runtime/Environment.h"
#include "runtime/ExecutionContext.h"
#include "runtime/GlobalObject.h"
#include "parser/ESScriptParser.h"

namespace escargot {

ESVMInstance::ESVMInstance()
{
    std::setlocale(LC_ALL, "en_US.utf8");

    m_globalObject = new GlobalObject();
    LexicalEnvironment* a = new LexicalEnvironment(new GlobalEnvironmentRecord(m_globalObject), NULL);

    m_globalExecutionContext = new ExecutionContext(a, a);
    m_currentExecutionContext = m_globalExecutionContext;
}

void ESVMInstance::evaluate(const std::string& source)
{
    Node* node = ESScriptParser::parseScript(source.c_str());
    node->execute(this);
}

}
