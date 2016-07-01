/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "Escargot.h"
#include "ExecutionContext.h"
#include "Environment.h"

namespace escargot {

ESBindingSlot ExecutionContext::resolveBinding(const InternalAtomicString& atomicName)
{
    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvebinding
    LexicalEnvironment* env = environment();

    while (env) {
        ESBindingSlot slot = env->record()->hasBinding(atomicName);
        if (slot)
            return slot;
        env = env->outerEnvironment();
    }

    return NULL;
}

ESBindingSlot ExecutionContext::resolveBinding(const InternalAtomicString& atomicName, LexicalEnvironment*& env)
{
    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvebinding
    env = environment();

    while (env) {
        ESBindingSlot slot = env->record()->hasBinding(atomicName);
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

    // TODO ASSERT this is eval context
    // RELEASE_ASSERT_NOT_REACHED();
    return NULL;
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
