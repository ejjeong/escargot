#include "Escargot.h"
#include "ExecutionContext.h"
#include "Environment.h"

namespace escargot {

ExecutionContext::ExecutionContext(LexicalEnvironment* varEnv, bool needsActivation, ESValue** arguments, size_t argumentsCount)
{
#if 0
    m_lexicalEnvironment = varEnv;
    m_variableEnvironment = varEnv;
    m_function = NULL;
    resetLastESObjectMetInMemberExpressionNode();
    m_returnValue = esUndefined;
    m_needsActivation = needsActivation;
    m_arguments = arguments;
    m_argumentCount = argumentsCount;
#endif
}

ESSlot* ExecutionContext::resolveBinding(const InternalAtomicString& name)
{
    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvebinding
    LexicalEnvironment* env = environment();

    while(env) {
        ESSlot* slot = env->record()->hasBinding(name);
        if(slot)
            return slot;
        env = env->outerEnvironment();
    }

    return NULL;
}

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvethisbinding
ESObject* ExecutionContext::resolveThisBinding()
{
    return getThisEnvironment()->record()->getThisBinding();
}

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-getthisenvironment
LexicalEnvironment* ExecutionContext::getThisEnvironment()
{
    LexicalEnvironment* lex = environment();
    while(true) {
        bool exists = lex->record()->hasThisBinding();
        if(exists)
            break;
        lex = lex->outerEnvironment();
    }
    return lex;

}

}
