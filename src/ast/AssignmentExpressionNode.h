#ifndef AssignmentExpressionNode_h
#define AssignmentExpressionNode_h

#include "BinaryExpressionNode.h"
#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"

namespace escargot {

//An assignment operator expression.
class AssignmentExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    /*
    enum AssignmentOperator {
        "=" | "+=" | "-=" | "*=" | "/=" | "%="
            | "<<=" | ">>=" | ">>>="
            | "|=" | "^=" | "&="
    }*/
    enum AssignmentOperator {
        SimpleAssignment, //"="
        CompoundAssignment
    };

    AssignmentExpressionNode(Node* left, Node* right, const InternalString& oper)
            : ExpressionNode(NodeType::AssignmentExpression)
    {
        m_left = left;
        m_right = right;

        if (oper == L"=")
            m_operator = SimpleAssignment;
        else {
            m_operator = CompoundAssignment;
            if (oper == L"<<=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::LeftShift;
            else if (oper == L">>=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::SignedRightShift;
            else if (oper == L">>>=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::UnsignedRightShift;
            else if (oper == L"+=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::Plus;
            else if (oper == L"-=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::Minus;
            else if (oper == L"*=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::Mult;
            else if (oper == L"/=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::Div;
            else if (oper == L"&=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::BitwiseAnd;
            else if (oper == L"|=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::BitwiseOr;
            else if (oper == L"^=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::BitwiseXor;
            else //TODO
                RELEASE_ASSERT_NOT_REACHED();
        }
    }

    ESValue execute(ESVMInstance* instance)
    {
        ESValue rvalue(ESValue::ESForceUninitialized);
        switch(m_operator) {
        case SimpleAssignment:
        {
            //http://www.ecma-international.org/ecma-262/5.1/#sec-11.13.1
            rvalue = m_right->execute(instance);
            writeValue(instance, m_left, rvalue);
            break;
        }
        case CompoundAssignment:
        {
            rvalue = BinaryExpressionNode::execute(instance, m_left->execute(instance), m_right->execute(instance), m_compoundOperator);
            writeValue(instance, m_left, rvalue);
            break;
        }
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }

        return rvalue;
    }

    ALWAYS_INLINE static void writeValue(ESVMInstance* instance, Node* leftHandNode, const ESValue& rvalue)
    {
        if(leftHandNode->type() == NodeType::Identifier) {
            IdentifierNode* idNode = (IdentifierNode *)leftHandNode;
            try {
                idNode->executeForWrite(instance)->setDataProperty(rvalue);
            } catch(ESValue& err) {
                if(err.isESPointer() && err.asESPointer()->isESObject() &&
                        (err.asESPointer()->asESObject()->constructor().asESPointer() == instance->globalObject()->referenceError())) {
                    //TODO set proper flags
                    instance->globalObject()->set(idNode->nonAtomicName(), rvalue);
                } else {
                    throw err;
                }
            }

        } else {
            ExecutionContext* ec = instance->currentExecutionContext();
            //ec->resetLastESObjectMetInMemberExpressionNode();
            leftHandNode->execute(instance);
            ESObject* obj = ec->lastESObjectMetInMemberExpressionNode();
            if(UNLIKELY(!obj)) {
                throw ESValue(ESString::create(L"could not assign to left hand node lastESObjectMetInMemberExpressionNode==NULL"));
            }

            if(ec->isLastUsedPropertyValueInMemberExpressionNodeSetted()) {
                if(obj->isESArrayObject()) {
                    obj->asESArrayObject()->set(ec->lastUsedPropertyValueInMemberExpressionNode(), rvalue);
                } else {
                    obj->set(ec->lastUsedPropertyValueInMemberExpressionNode(), rvalue);
                }
            } else {
                ec->lastUsedPropertySlotInMemberExpressionNode()->setValue(rvalue, obj);
            }

        }
    }
protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
    AssignmentOperator m_operator; //operator: AssignmentOperator
    BinaryExpressionNode::BinaryExpressionOperator m_compoundOperator;
};

}

#endif
