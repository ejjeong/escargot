#include "Escargot.h"
#include "ESFunctionCaller.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#include "runtime/ESFunctionCaller.h"
#include "ast/AST.h"

namespace escargot {

ESValue* ESFunctionCaller::call(ESValue* callee, ESValue* receiver, ESValue* arguments[], size_t argumentCount, ESVMInstance* ESVMInstance)
{
    if(callee->isHeapObject() && callee->toHeapObject()->isJSFunction()) {
        ExecutionContext* currentContext = ESVMInstance->currentExecutionContext();
        JSFunction* fn = callee->toHeapObject()->toJSFunction();

        //TODO process receiver
        ESVMInstance->m_currentExecutionContext = new ExecutionContext(LexicalEnvironment::newFunctionEnvironment(fn, receiver));
        DeclarativeEnvironmentRecord* functionRecord = ESVMInstance->m_currentExecutionContext->environment()->record()->toDeclarativeEnvironmentRecord();
        JSObject* innerObject = functionRecord->innerObject();
        JSObject* argumentsObject = JSObject::create();
        for(unsigned i = 0; i < argumentCount ; i ++) {
            argumentsObject->set(ESString((int)i), arguments[i]);
        }
        innerObject->set(L"arguments", argumentsObject);
        innerObject->set(L"length", Smi::fromInt(argumentCount));

        const ESStringVector& params = fn->functionAST()->params();

        for(unsigned i = 0; i < params.size() ; i ++) {
            functionRecord->createMutableBinding(params[i],false);
            if(i < argumentCount) {
                functionRecord->setMutableBinding(params[i], arguments[i], true);
            }
        }

        fn->functionAST()->body()->execute(ESVMInstance);
        ESVMInstance->m_currentExecutionContext = currentContext;
    } else {
        throw "TypeError";
    }
    //assert(callee == JSFunction)
    //assert(receiver == JSObject)
    /*

    */

    return undefined;
}

}
