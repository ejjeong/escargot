#include "Escargot.h"
#include "ThisExpressionNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue ThisExpressionNode::execute(ESVMInstance* instance)
{
    return instance->currentExecutionContext()->resolveThisBinding();
}

}
