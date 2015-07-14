#include "Escargot.h"
#include "ESVMInstance.h"
#include "runtime/Environment.h"
#include "runtime/ExecutionContext.h"
#include "parser/ESScriptParser.h"

namespace escargot {

ESVMInstance::ESVMInstance()
{
    auto a = new LexicalEnvironment(new GlobalEnvironmentRecord(), NULL);
    
    m_globalExecutionContext = new ExecutionContext(a, a);
    m_currentExecutionContext = m_globalExecutionContext;
}

void ESVMInstance::evaluate(const std::string& source)
{
    ESScriptParser::parseScript(source.c_str());
}

}
