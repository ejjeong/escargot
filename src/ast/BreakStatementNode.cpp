#include "Escargot.h"
#include "BreakStatementNode.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue BreakStatmentNode::execute(ESVMInstance* instance)
{
    instance->currentExecutionContext()->doBreak();
    RELEASE_ASSERT_NOT_REACHED();
    return ESValue();
}

}
