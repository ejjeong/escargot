#include "Escargot.h"
#include "ReturnStatmentNode.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue ReturnStatmentNode::execute(ESVMInstance* instance)
{
    /*
    ESValue* v = m_argument->execute(instance)->ensureValue();
    instance->currentExecutionContext()->doReturn(v);
    RELEASE_ASSERT_NOT_REACHED();
    */
    return ESValue();
}

}
