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

    AssignmentExpressionNode(Node* left, Node* right, ESString* oper)
            : ExpressionNode(NodeType::AssignmentExpression)
    {
        m_left = left;
        m_right = right;

        if (*oper == u"=")
            m_operator = SimpleAssignment;
        else {
            m_operator = CompoundAssignment;
            if (*oper == u"<<=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::LeftShift;
            else if (*oper == u">>=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::SignedRightShift;
            else if (*oper == u">>>=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::UnsignedRightShift;
            else if (*oper == u"+=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::Plus;
            else if (*oper == u"-=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::Minus;
            else if (*oper == u"*=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::Mult;
            else if (*oper == u"/=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::Div;
            else if (*oper == u"&=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::BitwiseAnd;
            else if (*oper == u"|=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::BitwiseOr;
            else if (*oper == u"^=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::BitwiseXor;
            else //TODO
                RELEASE_ASSERT_NOT_REACHED();
        }
    }

    ESValue execute(ESVMInstance* instance)
    {
        ESValue rvalue(ESValue::ESForceUninitialized);
        ESSlotAccessor slot;
        ExecutionContext* ec = instance->currentExecutionContext();
        ESSlotWriterForAST::prepareExecuteForWriteASTNode(ec);

        if(LIKELY(m_operator == SimpleAssignment)) {
            //http://www.ecma-international.org/ecma-262/5.1/#sec-11.13.1
            rvalue = m_right->execute(instance);
            slot = m_left->executeForWrite(instance);
        } else { //CompoundAssignment
            ASSERT(m_operator == CompoundAssignment);
            slot = m_left->executeForWrite(instance);
            rvalue = BinaryExpressionNode::execute(instance, slot.value(ec->lastESObjectMetInMemberExpressionNode()), m_right->execute(instance), m_compoundOperator);
        }

        ESSlotWriterForAST::setValue(slot, ec, rvalue);
        return rvalue;
    }

protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
    AssignmentOperator m_operator; //operator: AssignmentOperator
    BinaryExpressionNode::BinaryExpressionOperator m_compoundOperator;
};

}

#endif
