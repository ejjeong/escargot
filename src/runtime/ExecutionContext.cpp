#include "Escargot.h"
#include "ExecutionContext.h"

namespace escargot {

ExecutionContext::ExecutionContext(LexicalEnvironment* lexEnv, LexicalEnvironment* varEnv) {
    m_lexicalEnv = lexEnv;
    m_variableEnv = varEnv;
    m_function = NULL;
}

LexicalEnvironment* ExecutionContext::currentEnvironment() {
    return m_variableEnv;
}

}
