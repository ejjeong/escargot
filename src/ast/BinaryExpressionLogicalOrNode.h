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

    virtual NodeType type() { return NodeType::BinaryExpressionLogicalOr; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
#ifdef ENABLE_ESJIT
        size_t phiIndex = context.m_phiIndex++;
        codeBlock->pushCode(AllocPhi(phiIndex), context, this);
#endif
        m_left->generateExpressionByteCode(codeBlock, context);

#ifdef ENABLE_ESJIT
        codeBlock->pushCode(StorePhi(phiIndex, false, true), context, this);
#endif

        codeBlock->pushCode<JumpIfTopOfStackValueIsTrueWithPeeking>(JumpIfTopOfStackValueIsTrueWithPeeking(SIZE_MAX), context, this);
        size_t pos = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsTrueWithPeeking>();
        codeBlock->pushCode(Pop(), context, this);
        m_right->generateExpressionByteCode(codeBlock, context);
#ifdef ENABLE_ESJIT
        codeBlock->pushCode(StorePhi(phiIndex, true, false), context, this);
#endif
        codeBlock->peekCode<JumpIfTopOfStackValueIsTrueWithPeeking>(pos)->m_jumpPosition = codeBlock->currentCodeSize();
#ifdef ENABLE_ESJIT
        codeBlock->pushCode(LoadPhi(phiIndex), context, this);
#endif
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 6;
        m_left->computeRoughCodeBlockSizeInWordSize(result);
        m_right->computeRoughCodeBlockSizeInWordSize(result);
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
