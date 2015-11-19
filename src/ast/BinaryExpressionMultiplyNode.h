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

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(Multiply(), context, this);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 1;
        m_left->computeRoughCodeBlockSizeInWordSize(result);
        m_right->computeRoughCodeBlockSizeInWordSize(result);
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
