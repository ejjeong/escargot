#ifndef BinaryExpressionLeftShiftNode_h
#define BinaryExpressionLeftShiftNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionLeftShiftNode : public ExpressionNode {
public:
    BinaryExpressionLeftShiftNode(Node *left, Node* right)
        : ExpressionNode(NodeType::BinaryExpressionLeftShift)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    virtual NodeType type() { return NodeType::BinaryExpressionLeftShift; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(LeftShift(), context, this);
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
