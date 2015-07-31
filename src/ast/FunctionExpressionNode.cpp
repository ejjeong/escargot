#include "Escargot.h"
#include "FunctionExpressionNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue FunctionExpressionNode::execute(ESVMInstance* instance)
{
    ESFunctionObject* function = ESFunctionObject::create(instance->currentExecutionContext()->environment(), this);
    //FIXME these lines duplicate with FunctionDeclarationNode::execute
    function->set__proto__(instance->globalObject()->functionPrototype());
    ESObject* prototype = ESObject::create();
    prototype->setConstructor(function);
    prototype->set__proto__(instance->globalObject()->object());
    function->setProtoType(prototype);
    function->set(strings->name, ESString::create(m_id.data()));
    /////////////////////////////////////////////////////////////////////

    return function;
}

}
