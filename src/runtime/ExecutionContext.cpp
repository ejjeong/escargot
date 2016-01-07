#include "Escargot.h"
#include "ExecutionContext.h"
#include "Environment.h"

namespace escargot {

ESValue* ExecutionContext::resolveBinding(const InternalAtomicString& atomicName)
{
    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvebinding
    LexicalEnvironment* env = environment();

    while (env) {
        ESValue* slot = env->record()->hasBinding(atomicName);
        if (slot)
            return slot;
        env = env->outerEnvironment();
    }

    return NULL;
}

ESValue* ExecutionContext::resolveBinding(const InternalAtomicString& atomicName, LexicalEnvironment*& env)
{
    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvebinding
    env = environment();

    while (env) {
        ESValue* slot = env->record()->hasBinding(atomicName);
        if (slot)
            return slot;
        env = env->outerEnvironment();
    }

    return NULL;
}

ESValue* ExecutionContext::resolveArgumentsObjectBinding()
{
    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvebinding
    LexicalEnvironment* env = environment();

    while (env) {
        ESValue* slot = env->record()->hasBindingForArgumentsObject();
        if (slot)
            return slot;
        env = env->outerEnvironment();
    }

    RELEASE_ASSERT_NOT_REACHED();
}
/*
// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvethisbinding
ESValue ExecutionContext::resolveThisBinding()
{
    return getThisEnvironment()->record()->getThisBinding();
}

// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-getthisenvironment
LexicalEnvironment* ExecutionContext::getThisEnvironment()
{
    LexicalEnvironment* lex = environment();
    while (true) {
        bool exists = lex->record()->hasThisBinding();
        if (exists)
            break;
        lex = lex->outerEnvironment();
    }
    return lex;

}
*/
}
