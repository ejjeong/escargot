#include "Escargot.h"
#include "ThrowStatementNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue ThrowStatementNode::execute(ESVMInstance* instance)
{
    /*
    ESValue* arg = m_argument->execute(instance);
    if (arg->isESSlot())
        throw arg->toESSlot()->value();
    throw arg;
    RELEASE_ASSERT_NOT_REACHED();
    */
    return ESValue();
}

}

