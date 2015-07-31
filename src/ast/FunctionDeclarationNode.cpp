#include "Escargot.h"
#include "FunctionDeclarationNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue* FunctionDeclarationNode::execute(ESVMInstance* instance)
{
    ESFunctionObject* function = ESFunctionObject::create(instance->currentExecutionContext()->environment(), this);
    //FIXME these lines duplicate with FunctionExpressionNode::execute
    function->set__proto__(instance->globalObject()->functionPrototype());
    ESObject* prototype = ESObject::create();
    prototype->setConstructor(function);
    prototype->set__proto__(instance->globalObject()->object());
    function->setProtoType(prototype);
    function->set(strings->name, PString::create(m_id.data()));
    /////////////////////////////////////////////
    instance->currentExecutionContext()->environment()->record()->createMutableBindingForAST(m_id, false);
    instance->currentExecutionContext()->environment()->record()->setMutableBinding(m_id, function, false);
    return esUndefined;
}

}
