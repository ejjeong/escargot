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

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESValue test = m_test->executeExpression(instance);
        if (test.toBoolean())
            return m_consequente->executeExpression(instance);
        else
            return m_alternate->executeExpression(instance);
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenereateContext& context)
    {
        m_test->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(JumpIfTopOfStackValueIsFalse(SIZE_MAX), this);
        size_t jumpPosForTestIsFalse = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsFalse>();

        m_consequente->generateExpressionByteCode(codeBlock, context);
        JumpIfTopOfStackValueIsFalse* jumpForTestIsFalse = codeBlock->peekCode<JumpIfTopOfStackValueIsFalse>(jumpPosForTestIsFalse);
        codeBlock->pushCode(Jump(SIZE_MAX), this);
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
