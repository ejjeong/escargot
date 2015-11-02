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
#ifdef ENABLE_ESJIT
        codeBlock->pushCode(AllocPhi(), context, this);
        int allocPhiIndex = context.lastUsedSSAIndex();
        int srcIndex0 = -1;
        int srcIndex1 = -1;
#endif
        m_left->generateExpressionByteCode(codeBlock, context);

#ifdef ENABLE_ESJIT
        codeBlock->pushCode(StorePhi(allocPhiIndex), context, this);
        srcIndex0 = context.lastUsedSSAIndex();
#endif

        codeBlock->pushCode<JumpIfTopOfStackValueIsTrueWithPeeking>(JumpIfTopOfStackValueIsTrueWithPeeking(SIZE_MAX), context, this);
        size_t pos = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsTrueWithPeeking>();
        codeBlock->pushCode(Pop(), context, this);
        m_right->generateExpressionByteCode(codeBlock, context);
#ifdef ENABLE_ESJIT
        codeBlock->pushCode(StorePhi(allocPhiIndex), context, this);
        srcIndex1 = context.lastUsedSSAIndex();
#endif
        codeBlock->peekCode<JumpIfTopOfStackValueIsTrueWithPeeking>(pos)->m_jumpPosition = codeBlock->currentCodeSize();
#ifdef ENABLE_ESJIT
        codeBlock->pushCode(LoadPhi(allocPhiIndex, srcIndex0, srcIndex1), context, this);
        context.m_ssaComputeStack.back() = context.lastUsedSSAIndex();
#endif
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif


