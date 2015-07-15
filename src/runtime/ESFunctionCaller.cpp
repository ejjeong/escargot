#include "Escargot.h"
#include "ESFunctionCaller.h"

namespace escargot {

ESValue* ESFunctionCaller::call(ESValue callee, ESValue receiver, ESValue* arguments, size_t argumentCount, ESVMInstance* ESVMInstance)
{
    //assert(callee == JSFunction)
    //assert(receiver == JSObject)
    /*
    ExcutionContext* currentCtx = ESVMInstance->currentExecutionCtx;
    ESVMInstance->currentExecutionCtx = new ExecutionContext();
    //ESVMInstance->currentExecutionCtx->outerEnv = callee.asJSFunction().m_outerEnvironment;
    ESVMInstance->currentExecutionCtx->record = new FunctionEnvironmentRecord();
    //TODO process thisValue
    //callee.asFunction().m_body->execute(ESVMInstance);
    ESVMInstance->currentExecutionCtx = currentCtx;
    */

    return undefined;
}

}
