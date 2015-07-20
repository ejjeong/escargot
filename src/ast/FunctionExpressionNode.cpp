#include "Escargot.h"
#include "FunctionExpressionNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue* FunctionExpressionNode::execute(ESVMInstance* instance)
{
    JSFunction* function = JSFunction::create(instance->currentExecutionContext()->environment(), this);
    return function;
}

}
