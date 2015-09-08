#ifndef BinaryExpressionInNode_h
#define BinaryExpressionInNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionInNode : public ExpressionNode {
public:
    BinaryExpressionInNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionIn)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESValue willBeObj = m_right->executeExpression(instance);
        ESObject* obj = willBeObj.toObject();
        ESValue key = m_left->executeExpression(instance);
        return ESValue(obj->find(key, true).hasData());
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
