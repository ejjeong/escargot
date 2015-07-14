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

    m_global = NULL; //new GlobalObject();
    LexicalEnvironment* a = new LexicalEnvironment(new GlobalEnvironmentRecord(m_global), NULL);
    
    m_globalExecutionContext = new ExecutionContext(a, a);
    m_currentExecutionContext = m_globalExecutionContext;
}

void ESVMInstance::evaluate(const std::string& source)
{
    ESScriptParser::parseScript(source.c_str());
}

}
