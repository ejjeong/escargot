#include "Escargot.h"
#include "FunctionDeclarationNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue* FunctionDeclarationNode::execute(ESVMInstance* instance)
{
    JSFunction* function = JSFunction::create(instance->currentExecutionContext()->environment(), this);
    instance->currentExecutionContext()->environment()->record()->createMutableBindingForAST(m_id, false);
    instance->currentExecutionContext()->environment()->record()->setMutableBinding(m_id, function, false);
    return undefined;
}

}
