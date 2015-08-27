#ifndef BinaryExpressionLogicalAndNode_h
#define BinaryExpressionLogicalAndNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionLogicalAndNode : public ExpressionNode {
public:
    BinaryExpressionLogicalAndNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionLogicalAnd)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue execute(ESVMInstance* instance)
    {
        ESValue lval = m_left->execute(instance);

        if (lval.toBoolean() == false)
            return lval;
        else
            return m_right->execute(instance);
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
