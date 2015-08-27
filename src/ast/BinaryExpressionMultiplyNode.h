#ifndef BinaryExpressionMultiplyNode_h
#define BinaryExpressionMultiplyNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionMultiplyNode : public ExpressionNode {
public:
    BinaryExpressionMultiplyNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionMultiply)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue execute(ESVMInstance* instance)
    {
        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.5.1
        return ESValue(m_left->execute(instance).toNumber() * m_right->execute(instance).toNumber());
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
