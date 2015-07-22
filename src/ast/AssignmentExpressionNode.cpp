#include "Escargot.h"
#include "AssignmentExpressionNode.h"

#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue* AssignmentExpressionNode::execute(ESVMInstance* instance)
{
    if(m_operator == Equal) {
        //http://www.ecma-international.org/ecma-262/5.1/#sec-11.13.1
        //TODO
        ESValue* rval = m_right->execute(instance)->ensureValue();
        instance->currentExecutionContext()->resetLastJSObjectMetInMemberExpressionNode();
        ESValue* lref = undefined;
        try {
            lref = m_left->execute(instance);
        } catch(ReferenceError& err) {
        }


        //TODO
        if(lref == undefined) {
            JSObject* obj = instance->currentExecutionContext()->lastJSObjectMetInMemberExpressionNode();
            if(obj == NULL && m_left->type() == NodeType::Identifier) {
                IdentifierNode* n = (IdentifierNode *)m_left;
                instance->globalObject()->set(n->name(), rval);
            } else if(obj) {
                obj->set(instance->currentExecutionContext()->lastLastUsedPropertyNameInMemberExpressionNode(), rval);
            } else {
                throw ReferenceError();
            }

        } else {
            JSObjectSlot* slot = lref->toHeapObject()->toJSObjectSlot();
            slot->setValue(rval);
        }
    }

    return undefined;
}

}

