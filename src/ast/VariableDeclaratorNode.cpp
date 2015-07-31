#include "Escargot.h"
#include "VariableDeclaratorNode.h"
#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue* VariableDeclaratorNode::execute(ESVMInstance* instance)
{
    ASSERT(m_id->type() == NodeType::Identifier);
    if(instance->currentExecutionContext()->needsActivation()) {
        instance->currentExecutionContext()->environment()->record()->createMutableBindingForAST(((IdentifierNode *)m_id)->name(), false);
    }
    return esUndefined;
}

}
