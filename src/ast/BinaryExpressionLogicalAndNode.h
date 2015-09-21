#ifndef BinaryExpressionLogicalAndNode_h
#define BinaryExpressionLogicalAndNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionLogicalAndNode : public ExpressionNode {
public:
    BinaryExpressionLogicalAndNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionLogicalAnd)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode<JumpIfTopOfStackValueIsFalseWithPeeking>(JumpIfTopOfStackValueIsFalseWithPeeking(SIZE_MAX), this);
        size_t pos = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsFalseWithPeeking>();
        codeBlock->pushCode(Pop(), this);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->peekCode<JumpIfTopOfStackValueIsFalseWithPeeking>(pos)->m_jumpPosition = codeBlock->currentCodeSize();
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
