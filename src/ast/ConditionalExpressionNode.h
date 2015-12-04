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

    virtual NodeType type() { return NodeType::ConditionalExpression; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_test->generateExpressionByteCode(codeBlock, context);
#ifdef ENABLE_ESJIT
        size_t phiIndex = context.m_phiIndex++;
        codeBlock->pushCode(AllocPhi(phiIndex), context, this);
#endif
        codeBlock->pushCode(JumpIfTopOfStackValueIsFalse(SIZE_MAX), context, this);

        size_t jumpPosForTestIsFalse = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsFalse>();
        int savedBaseRegisterCounter = context.m_baseRegisterCount;
        m_consequente->generateExpressionByteCode(codeBlock, context);

#ifdef ENABLE_ESJIT
        codeBlock->pushCode(StorePhi(phiIndex, true, true), context, this);
#endif

        codeBlock->pushCode(Jump(SIZE_MAX), context, this);
        JumpIfTopOfStackValueIsFalse* jumpForTestIsFalse = codeBlock->peekCode<JumpIfTopOfStackValueIsFalse>(jumpPosForTestIsFalse);
        size_t jumpPosForEndOfConsequence = codeBlock->lastCodePosition<Jump>();

        jumpForTestIsFalse->m_jumpPosition = codeBlock->currentCodeSize();
        context.m_baseRegisterCount = savedBaseRegisterCounter;
#ifdef ENABLE_ESJIT
        codeBlock->pushCode(FakePop(), context, this);
#endif
        m_alternate->generateExpressionByteCode(codeBlock, context);

#ifdef ENABLE_ESJIT
        codeBlock->pushCode(StorePhi(phiIndex, true, false), context, this);
#endif

        Jump* jumpForEndOfConsequence = codeBlock->peekCode<Jump>(jumpPosForEndOfConsequence);
        jumpForEndOfConsequence->m_jumpPosition = codeBlock->currentCodeSize();

#ifdef ENABLE_ESJIT
        codeBlock->pushCode(LoadPhi(phiIndex), context, this);
#endif
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 6;
        m_test->computeRoughCodeBlockSizeInWordSize(result);
        m_consequente->computeRoughCodeBlockSizeInWordSize(result);
        m_alternate->computeRoughCodeBlockSizeInWordSize(result);
    }

protected:
    ExpressionNode* m_test;
    ExpressionNode* m_consequente;
    ExpressionNode* m_alternate;
};

}

#endif
