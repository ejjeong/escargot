#include "Escargot.h"
#include "ContinueStatementNode.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue ContinueStatementNode::execute(ESVMInstance* instance)
{
    instance->currentExecutionContext()->doContinue();
    RELEASE_ASSERT_NOT_REACHED();
    return ESValue();
}

}
