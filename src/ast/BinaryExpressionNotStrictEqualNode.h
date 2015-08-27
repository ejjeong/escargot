#ifndef BinaryExpressionNotStrictEqualNode_h
#define BinaryExpressionNotStrictEqualNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionNotStrictEqualNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    BinaryExpressionNotStrictEqualNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionNotStrictEqual)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue execute(ESVMInstance* instance)
    {
        return ESValue(!m_left->execute(instance).equalsTo(m_right->execute(instance)));
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
