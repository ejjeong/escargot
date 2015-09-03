#ifndef BinaryExpressionDivisionNode_h
#define BinaryExpressionDivisionNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionDivisionNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    BinaryExpressionDivisionNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionDivison)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue execute(ESVMInstance* instance)
    {
        return ESValue(m_left->execute(instance).toNumber() / m_right->execute(instance).toNumber());
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif