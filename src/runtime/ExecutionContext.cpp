#include "Escargot.h"
#include "ExecutionContext.h"
#include "Environment.h"

namespace escargot {

ExecutionContext::ExecutionContext(LexicalEnvironment* varEnv, bool needsActivation, bool isNewExpression, ExecutionContext* callerContext, ESValue* arguments, size_t argumentsCount)
{
    m_lexicalEnvironment = varEnv;
    m_variableEnvironment = varEnv;
    m_function = NULL;
    resetLastESObjectMetInMemberExpressionNode();
    m_needsActivation = needsActivation;
    m_isNewExpression = isNewExpression;
    m_callerContext = callerContext;
    m_arguments = arguments;
    m_argumentCount = argumentsCount;
    m_inWriteMode = false;
}

ESSlot* ExecutionContext::resolveBinding(const InternalAtomicString& atomicName, const InternalString& name)
{
    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvebinding
    LexicalEnvironment* env = environment();

    while(env) {
        ESSlot* slot = env->record()->hasBinding(atomicName, name);
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
