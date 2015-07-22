#include "Escargot.h"
#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue* IdentifierNode::execute(ESVMInstance* instance)
{
    //TODO throw Ref..Error
    JSObjectSlot* slot = instance->currentExecutionContext()->resolveBinding(name());
    if(slot)
        return slot;
    throw ReferenceError(m_name);
    return undefined;
}

}
