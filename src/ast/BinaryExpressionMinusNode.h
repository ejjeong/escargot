#ifndef BinaryExpressionMinusNode_h
#define BinaryExpressionMinusNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionMinusNode : public ExpressionNode {
public:
    BinaryExpressionMinusNode(Node *left, Node* right)
        : ExpressionNode(NodeType::BinaryExpressionMinus)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    virtual NodeType type() { return NodeType::BinaryExpressionMinus; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(Minus(), context, this);
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
