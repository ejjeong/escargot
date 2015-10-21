#ifndef ConditionalExpressionNode_h
#define ConditionalExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class ConditionalExpressionNode : public ExpressionNode {
public:
    friend class ScriptParser;
    ConditionalExpressionNode(Node *test, Node *consequente, Node *alternate)
            : ExpressionNode(NodeType::ConditionalExpression)
    {
        m_test = (ExpressionNode*) test;
        m_consequente = (ExpressionNode*) consequente;
        m_alternate = (ExpressionNode*) alternate;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
#ifdef ENABLE_ESJIT
        updateNodeIndex(context);
        codeBlock->pushCode(AllocPhi(), context, this);
        int allocPhiIndex = m_nodeIndex;
        WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
#endif

        m_test->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(JumpIfTopOfStackValueIsFalse(SIZE_MAX), context, this);
        size_t jumpPosForTestIsFalse = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsFalse>();
        WRITE_LAST_INDEX(-1, m_test->nodeIndex(), -1);

        m_consequente->generateExpressionByteCode(codeBlock, context);

#ifdef ENABLE_ESJIT
        updateNodeIndex(context);
        codeBlock->pushCode(StorePhi(), context, this);
        WRITE_LAST_INDEX(m_nodeIndex, m_consequente->nodeIndex(), allocPhiIndex);
#endif

        codeBlock->pushCode(Jump(SIZE_MAX), context, this);
        WRITE_LAST_INDEX(-1, -1, -1);
        JumpIfTopOfStackValueIsFalse* jumpForTestIsFalse = codeBlock->peekCode<JumpIfTopOfStackValueIsFalse>(jumpPosForTestIsFalse);
        size_t jumpPosForEndOfConsequence = codeBlock->lastCodePosition<Jump>();

        jumpForTestIsFalse->m_jumpPosition = codeBlock->currentCodeSize();
        m_alternate->generateExpressionByteCode(codeBlock, context);

#ifdef ENABLE_ESJIT
        updateNodeIndex(context);
        codeBlock->pushCode(StorePhi(), context, this);
        WRITE_LAST_INDEX(m_nodeIndex, m_alternate->nodeIndex(), allocPhiIndex);
#endif

        Jump* jumpForEndOfConsequence = codeBlock->peekCode<Jump>(jumpPosForEndOfConsequence);
        jumpForEndOfConsequence->m_jumpPosition = codeBlock->currentCodeSize();

#ifdef ENABLE_ESJIT
        updateNodeIndex(context);
        codeBlock->pushCode(LoadPhi(), context, this);
        WRITE_LAST_INDEX(m_nodeIndex, allocPhiIndex, m_consequente->nodeIndex());
#endif
    }

protected:
    ExpressionNode* m_test;
    ExpressionNode* m_consequente;
    ExpressionNode* m_alternate;
};

}

#endif
