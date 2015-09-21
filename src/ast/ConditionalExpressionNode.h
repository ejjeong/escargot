#ifndef ConditionalExpressionNode_h
#define ConditionalExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class ConditionalExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    ConditionalExpressionNode(Node *test, Node *consequente, Node *alternate)
            : ExpressionNode(NodeType::ConditionalExpression)
    {
        m_test = (ExpressionNode*) test;
        m_consequente = (ExpressionNode*) consequente;
        m_alternate = (ExpressionNode*) alternate;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_test->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(JumpIfTopOfStackValueIsFalse(SIZE_MAX), this);
        size_t jumpPosForTestIsFalse = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsFalse>();

        m_consequente->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(Jump(SIZE_MAX), this);
        JumpIfTopOfStackValueIsFalse* jumpForTestIsFalse = codeBlock->peekCode<JumpIfTopOfStackValueIsFalse>(jumpPosForTestIsFalse);
        size_t jumpPosForEndOfConsequence = codeBlock->lastCodePosition<Jump>();

        jumpForTestIsFalse->m_jumpPosition = codeBlock->currentCodeSize();
        m_alternate->generateExpressionByteCode(codeBlock, context);
        Jump* jumpForEndOfConsequence = codeBlock->peekCode<Jump>(jumpPosForEndOfConsequence);
        jumpForEndOfConsequence->m_jumpPosition = codeBlock->currentCodeSize();
    }

protected:
    ExpressionNode* m_test;
    ExpressionNode* m_consequente;
    ExpressionNode* m_alternate;
};

}

#endif
