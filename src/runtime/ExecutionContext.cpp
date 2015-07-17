#include "Escargot.h"
#include "ExecutionContext.h"
#include "Environment.h"

namespace escargot {

ExecutionContext::ExecutionContext(LexicalEnvironment* varEnv)
{
    m_lexicalEnvironment = varEnv;
    m_variableEnvironment = varEnv;
    m_function = NULL;
    resetLastJSObjectMetInMemberExpressionNode();
}

JSObjectSlot* ExecutionContext::resolveBinding(const ESString& name)
{
    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvebinding
    LexicalEnvironment* env = environment();

    while(env) {
        JSObjectSlot* slot = env->record()->hasBinding(name);
        if(slot)
            return slot;
        env = env->outerEnvironment();
    }

    return NULL;
}

}
