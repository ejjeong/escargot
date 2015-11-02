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
        codeBlock->pushCode(AllocPhi(), context, this);
        int allocPhiIndex = context.lastUsedSSAIndex();
        int srcIndex0 = -1;
        int srcIndex1 = -1;
#endif

        m_test->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(JumpIfTopOfStackValueIsFalse(SIZE_MAX), context, this);

        size_t jumpPosForTestIsFalse = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsFalse>();
        int savedBaseRegisterCounter = context.m_baseRegisterCount;
        m_consequente->generateExpressionByteCode(codeBlock, context);

#ifdef ENABLE_ESJIT
        codeBlock->pushCode(StorePhi(allocPhiIndex), context, this);
        srcIndex0 = context.lastUsedSSAIndex();
#endif

        codeBlock->pushCode(Jump(SIZE_MAX), context, this);
        JumpIfTopOfStackValueIsFalse* jumpForTestIsFalse = codeBlock->peekCode<JumpIfTopOfStackValueIsFalse>(jumpPosForTestIsFalse);
        size_t jumpPosForEndOfConsequence = codeBlock->lastCodePosition<Jump>();

        jumpForTestIsFalse->m_jumpPosition = codeBlock->currentCodeSize();
#ifdef ENABLE_ESJIT
        context.m_ssaComputeStack.pop_back();
#endif
        context.m_baseRegisterCount = savedBaseRegisterCounter;
        m_alternate->generateExpressionByteCode(codeBlock, context);

#ifdef ENABLE_ESJIT
        codeBlock->pushCode(StorePhi(allocPhiIndex), context, this);
        srcIndex1 = context.lastUsedSSAIndex();
#endif

        Jump* jumpForEndOfConsequence = codeBlock->peekCode<Jump>(jumpPosForEndOfConsequence);
        jumpForEndOfConsequence->m_jumpPosition = codeBlock->currentCodeSize();

#ifdef ENABLE_ESJIT
        codeBlock->pushCode(LoadPhi(allocPhiIndex, srcIndex0, srcIndex1), context, this);
        context.m_ssaComputeStack.back() = context.lastUsedSSAIndex();
#endif
    }

protected:
    ExpressionNode* m_test;
    ExpressionNode* m_consequente;
    ExpressionNode* m_alternate;
};

}

#endif

