#ifndef BinaryExpressionEqualNode_h
#define BinaryExpressionEqualNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionEqualNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    BinaryExpressionEqualNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionEqual)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue execute(ESVMInstance* instance)
    {
        return ESValue(m_left->execute(instance).abstractEqualsTo(m_right->execute(instance)));
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
