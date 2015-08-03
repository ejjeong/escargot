#include "Escargot.h"
#include "AssignmentExpressionNode.h"

#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue AssignmentExpressionNode::execute(ESVMInstance* instance)
{
    ESValue ret;
    switch(m_operator) {
    case SimpleAssignment:
    {
        //http://www.ecma-international.org/ecma-262/5.1/#sec-11.13.1
        //TODO
        ESValue rval = m_right->execute(instance).ensureValue();
        instance->currentExecutionContext()->resetLastESObjectMetInMemberExpressionNode();
        ESValue lref;
        try {
            lref = m_left->execute(instance);
        } catch(ReferenceError& err) {
        } catch(ESValue& err) {
            if(err.isESPointer() && err.asESPointer()->isESObject() &&
                    (err.asESPointer()->asESObject()->constructor().asESPointer() == instance->globalObject()->referenceError())) {

            } else {
                throw err;
            }
        }

        //TODO
        if(lref.isUndefined()) {
            ESObject* obj = instance->currentExecutionContext()->lastESObjectMetInMemberExpressionNode();
            if(obj == NULL && m_left->type() == NodeType::Identifier) {
                IdentifierNode* n = (IdentifierNode *)m_left;
                instance->globalObject()->set(n->name(), rval);
            } else if(obj) {
                ESValue propertyVal = instance->currentExecutionContext()->lastUsedPropertyValueInMemberExpressionNode();
                if(obj->isESArrayObject()) {
                    obj->asESArrayObject()->set(propertyVal, rval);
                } else {
                    obj->set(instance->currentExecutionContext()->lastUsedPropertyNameInMemberExpressionNode(), rval);
                }
            } else {
                throw ReferenceError(L"");
            }

        } else if(lref.asESPointer() && lref.asESPointer()->isESSlot()) {
            if(instance->currentExecutionContext()->lastESObjectMetInMemberExpressionNode()) {
                instance->currentExecutionContext()->
                    lastESObjectMetInMemberExpressionNode()->set(instance->currentExecutionContext()->lastUsedPropertyNameInMemberExpressionNode(), rval);
            } else {
                ESSlot* slot = lref.asESPointer()->asESSlot();
                slot->setValue(rval);
            }
        } else {
            throw ReferenceError(L"");
        }
        ret = rval;
        break;
    }
    case CompoundAssignment:
    {
        ESValue lref = m_left->execute(instance);
        ESValue lval = lref.ensureValue();
        ESValue rval = m_right->execute(instance).ensureValue();
        ESValue r = BinaryExpressionNode::execute(instance, lval, rval, m_compoundOperator);

        // TODO 6. Throw a SyntaxError

        ESSlot* slot = lref.asESPointer()->asESSlot();
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

