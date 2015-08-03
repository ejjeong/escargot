#ifndef AssignmentExpressionNode_h
#define AssignmentExpressionNode_h

#include "BinaryExpressionNode.h"
#include "ExpressionNode.h"
#include "PatternNode.h"

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
            else if (oper == L"+=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::Plus;
            else if (oper == L"-=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::Minus;
            else if (oper == L"*=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::Mult;
            else if (oper == L"/=")
                m_compoundOperator = BinaryExpressionNode::BinaryExpressionOperator::Div;
            else //TODO
                RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual ESValue execute(ESVMInstance* instance);
protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
    AssignmentOperator m_operator; //operator: AssignmentOperator
    BinaryExpressionNode::BinaryExpressionOperator m_compoundOperator;
};

}

#endif
