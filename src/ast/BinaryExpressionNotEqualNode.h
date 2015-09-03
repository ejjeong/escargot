#ifndef BinaryExpressionNotEqualNode_h
#define BinaryExpressionNotEqualNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionNotEqualNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    BinaryExpressionNotEqualNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionNotEqual)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue execute(ESVMInstance* instance)
    {
        return ESValue(!m_left->execute(instance).abstractEqualsTo(m_right->execute(instance)));
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif