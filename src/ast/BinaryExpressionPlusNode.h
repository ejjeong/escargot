#ifndef BinaryExpressionPlusNode_h
#define BinaryExpressionPlusNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionPlusNode : public ExpressionNode {
public:
    BinaryExpressionPlusNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionPlus)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        updateNodeIndex();
        codeBlock->pushCode(Plus(m_nodeIndex, m_left->nodeIndex(), m_right->nodeIndex()), this);
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
