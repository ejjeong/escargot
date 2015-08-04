#include "Escargot.h"
#include "ReturnStatmentNode.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue ReturnStatmentNode::execute(ESVMInstance* instance)
{
    instance->currentExecutionContext()->doReturn(m_argument ? m_argument->execute(instance) : ESValue());
    RELEASE_ASSERT_NOT_REACHED();
    return ESValue();
}

}
