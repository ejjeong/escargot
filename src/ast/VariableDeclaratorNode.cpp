#include "Escargot.h"
#include "VariableDeclaratorNode.h"
#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue* VariableDeclaratorNode::execute(ESVMInstance* instance)
{
    //TODO implement hoisting(process init AST later)
    ASSERT(m_id->type() == NodeType::Identifier);
    instance->currentExecutionContext()->variableEnvironment()->record()->createMutableBinding(((IdentifierNode *)m_id)->name());
    return undefined;
}

}
