#include "Escargot.h"
#include "AssignmentExpressionNode.h"

#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue* AssignmentExpressionNode::execute(ESVMInstance* instance)
{
    ESValue* ret;
    switch(m_operator) {
    case SimpleAssignment:
    {
        //http://www.ecma-international.org/ecma-262/5.1/#sec-11.13.1
        //TODO
        ESValue* rval = m_right->execute(instance)->ensureValue();
        instance->currentExecutionContext()->resetLastJSObjectMetInMemberExpressionNode();
        ESValue* lref = esUndefined;
        try {
            lref = m_left->execute(instance);
        } catch(ReferenceError& err) {
        }

        //TODO
        if(lref == esUndefined) {
            JSObject* obj = instance->currentExecutionContext()->lastJSObjectMetInMemberExpressionNode();
            if(obj == NULL && m_left->type() == NodeType::Identifier) {
                IdentifierNode* n = (IdentifierNode *)m_left;
                instance->globalObject()->set(n->name(), rval);
            } else if(obj) {
                ESValue* propertyVal = instance->currentExecutionContext()->lastLastUsedPropertyValueInMemberExpressionNode();
                if(obj->isJSArray() && propertyVal != NULL) {
                    obj->toJSArray()->set(propertyVal, rval);
                } else {
                    obj->set(instance->currentExecutionContext()->lastLastUsedPropertyNameInMemberExpressionNode(), rval);
                }
            } else {
                throw ReferenceError();
            }

        } else {
            JSObjectSlot* slot = lref->toHeapObject()->toJSObjectSlot();
            slot->setValue(rval);
        }
        ret = rval;
        break;
    }
    case CompoundAssignment:
    {
        ESValue* lref = m_left->execute(instance);
        ESValue* lval = lref->ensureValue();
        ESValue* rval = m_right->execute(instance)->ensureValue();
        ESValue* r = BinaryExpressionNode::execute(instance, lval, rval, m_compoundOperator);

        // TODO 6. Throw a SyntaxError

        JSObjectSlot* slot = lref->toHeapObject()->toJSObjectSlot();
        slot->setValue(r);
        ret = r;
        break;
    }
    default:
        RELEASE_ASSERT_NOT_REACHED();
        break;
    }

    return ret;
}

}

