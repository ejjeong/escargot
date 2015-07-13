#include "ExecutionContext.h"

ExecutionContext::ExecutionContext(LexicalEnvironment* lexEnv, LexicalEnvironment* varEnv) {
    lexicalEnv = lexEnv;
    variableEnv = varEnv;
}

LexicalEnvironment* ExecutionContext::currentEnvironment() {
    return variableEnv;
}
