#include "Escargot.h"
#include "ESFunctionCaller.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#include "runtime/ESFunctionCaller.h"
#include "ast/AST.h"

namespace escargot {

ESValue* functionCallerInnerProcess(JSFunction* fn, ESValue* callee, ESValue* receiver, ESValue* arguments[], size_t argumentCount, bool needsArgumentsObject, ESVMInstance* ESVMInstance)
{
    ((FunctionEnvironmentRecord *)ESVMInstance->currentExecutionContext()->environment()->record())->bindThisValue(receiver->toHeapObject()->toJSObject());
    DeclarativeEnvironmentRecord* functionRecord = ESVMInstance->currentExecutionContext()->environment()->record()->toDeclarativeEnvironmentRecord();
    if(needsArgumentsObject) {
        JSObject* argumentsObject = JSObject::create();
        unsigned i = 0;
        argumentsObject->set(strings->length, Smi::fromInt(argumentCount));
        for(; i < argumentCount && i < ESCARGOT_STRINGS_NUMBERS_MAX ; i ++) {
            argumentsObject->set(strings->numbers[i], arguments[i]);
        }
        for( ; i < argumentCount ; i ++) {
            argumentsObject->set(ESAtomicString(ESString((int)i).data()), arguments[i]);
        }

        functionRecord->createMutableBinding(strings->arguments,false);
        functionRecord->setMutableBinding(strings->arguments, argumentsObject, true);
    }

    const ESAtomicStringVector& params = fn->functionAST()->params();

    for(unsigned i = 0; i < params.size() ; i ++) {
        functionRecord->createMutableBinding(params[i],false);
        if(i < argumentCount) {
            functionRecord->setMutableBinding(params[i], arguments[i], true);
        }
    }

    int r = setjmp(ESVMInstance->currentExecutionContext()->returnPosition());
    if(r != 1) {
        fn->functionAST()->body()->execute(ESVMInstance);
    }
    return ESVMInstance->currentExecutionContext()->returnValue();
}

ESValue* ESFunctionCaller::call(ESValue* callee, ESValue* receiver, ESValue* arguments[], size_t argumentCount, ESVMInstance* ESVMInstance)
{
    ESValue* result = esUndefined;
    if(callee->isHeapObject() && callee->toHeapObject()->isJSFunction()) {
        ExecutionContext* currentContext = ESVMInstance->currentExecutionContext();
        JSFunction* fn = callee->toHeapObject()->toJSFunction();
        if(fn->functionAST()->needsActivation()) {
            ESVMInstance->m_currentExecutionContext = new ExecutionContext(LexicalEnvironment::newFunctionEnvironment(fn, receiver));
            result = functionCallerInnerProcess(fn, callee, receiver, arguments, argumentCount, true, ESVMInstance);
            ESVMInstance->m_currentExecutionContext = currentContext;
        } else {
            bool needsArgumentsObject = false;
            ESAtomicStringVector& v = fn->functionAST()->innerIdentifiers();
            for(unsigned i = 0; i < v.size() ; i ++) {
                if(v[i] == strings->arguments) {
                    needsArgumentsObject = true;
                    break;
                }
            }

            FunctionEnvironmentRecord envRec(true,
                    (std::pair<ESAtomicString, JSSlot>*)alloca(sizeof(std::pair<ESAtomicString, JSSlot>) * fn->functionAST()->innerIdentifiers().size()),
                    fn->functionAST()->innerIdentifiers().size());

            envRec.m_functionObject = fn;
            envRec.m_newTarget = receiver;

            LexicalEnvironment env(&envRec, fn->outerEnvironment());
            ExecutionContext ec(&env);
            ESVMInstance->m_currentExecutionContext = &ec;
            result = functionCallerInnerProcess(fn, callee, receiver, arguments, argumentCount, needsArgumentsObject, ESVMInstance);
            ESVMInstance->m_currentExecutionContext = currentContext;
        }
    } else {
        throw TypeError();
    }

    return result;
}

}
