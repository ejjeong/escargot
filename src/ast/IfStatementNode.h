#ifndef IfStatementNode_h
#define IfStatementNode_h

#include "StatementNode.h"

namespace escargot {

class IfStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    IfStatementNode(Node *test, Node *consequente, Node *alternate)
            : StatementNode(NodeType::IfStatement)
    {
        m_test = (ExpressionNode*) test;
        m_consequente = (StatementNode*) consequente;
        m_alternate = (StatementNode*) alternate;
    }

    void executeStatement(ESVMInstance* instance)
    {
        if (m_test->executeExpression(instance).toBoolean())
            m_consequente->executeStatement(instance);
        else if (m_alternate)
            m_alternate->executeStatement(instance);
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenereateContext& context)
    {
        if(!m_alternate) {
            m_test->generateExpressionByteCode(codeBlock, context);
            codeBlock->pushCode(JumpIfTopOfStackValueIsFalse(SIZE_MAX), this);
            size_t jPos = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsFalse>();
            m_consequente->generateStatementByteCode(codeBlock, context);
            JumpIfTopOfStackValueIsFalse* j = codeBlock->peekCode<JumpIfTopOfStackValueIsFalse>(jPos);
            j->m_jumpPosition = codeBlock->currentCodeSize();
        } else {
            m_test->generateExpressionByteCode(codeBlock, context);
            codeBlock->pushCode(JumpIfTopOfStackValueIsFalse(SIZE_MAX), this);
            size_t jPos = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsFalse>();
            m_consequente->generateStatementByteCode(codeBlock, context);
            JumpIfTopOfStackValueIsFalse* j = codeBlock->peekCode<JumpIfTopOfStackValueIsFalse>(jPos);
            codeBlock->pushCode(Jump(SIZE_MAX), this);
            size_t jPos2 = codeBlock->lastCodePosition<Jump>();
            j->m_jumpPosition = codeBlock->currentCodeSize();

            m_alternate->generateStatementByteCode(codeBlock, context);
            Jump* j2 = codeBlock->peekCode<Jump>(jPos2);
            j2->m_jumpPosition = codeBlock->currentCodeSize();
        }

    }

protected:
    ExpressionNode *m_test;
    StatementNode *m_consequente;
    StatementNode *m_alternate;
};

}

#endif
