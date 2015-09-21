#ifndef BinaryExpressionLogicalOrNode_h
#define BinaryExpressionLogicalOrNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionLogicalOrNode : public ExpressionNode {
public:
    BinaryExpressionLogicalOrNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionLogicalOr)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode<JumpIfTopOfStackValueIsTrueWithPeeking>(JumpIfTopOfStackValueIsTrueWithPeeking(SIZE_MAX), this);
        size_t pos = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsTrueWithPeeking>();
        codeBlock->pushCode(Pop(), this);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->peekCode<JumpIfTopOfStackValueIsTrueWithPeeking>(pos)->m_jumpPosition = codeBlock->currentCodeSize();
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
