#include "Escargot.h"
#include "CallExpressionNode.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#include "runtime/ESFunctionCaller.h"

namespace escargot {

ESValue* CallExpressionNode::execute(ESVMInstance* instance)
{
    instance->currentExecutionContext()->resetLastJSObjectMetInMemberExpressionNode();
    ESValue* slot = m_callee->execute(instance);
    ESValue* fn = undefined;
    if(slot->isHeapObject() && slot->toHeapObject()->isJSObjectSlot()) {
        fn = slot->toHeapObject()->toJSObjectSlot()->value();
    }
    ESValue* receiver = instance->currentExecutionContext()->lastJSObjectMetInMemberExpressionNode();
    if(receiver == NULL)
        receiver = instance->globalObject();

    std::vector<ESValue*, gc_allocator<ESValue*>> arguments;
    for(unsigned i = 0; i < m_arguments.size() ; i ++) {
        ESValue* result = m_arguments[i]->execute(instance)->ensureValue();
        arguments.push_back(result);
    }

    return ESFunctionCaller::call(fn, receiver, &arguments[0], arguments.size(), instance);
}

}

