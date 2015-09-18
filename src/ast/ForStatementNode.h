#ifndef ForStatementNode_h
#define ForStatementNode_h

#include "StatementNode.h"
#include "ExpressionNode.h"

namespace escargot {

class ForStatementNode : public StatementNode, public ControlFlowNode {
public:
    friend class ESScriptParser;
    ForStatementNode(Node *init, Node *test, Node *update, Node *body)
            : StatementNode(NodeType::ForStatement)
    {
        m_init = (ExpressionNode*) init;
        m_test = (ExpressionNode*) test;
        m_update = (ExpressionNode*) update;
        m_body = (StatementNode*) body;
    }

    void executeStatement(ESVMInstance* instance)
    {
        ESValue test;
        if(UNLIKELY(m_isSlowCase)) {
            if (m_init)
                m_init->executeExpression(instance);
            test = m_test ? m_test->executeExpression(instance) : ESValue(true);
            instance->currentExecutionContext()->setJumpPositionAndExecute([&](){
                jmpbuf_wrapper cont;
                int r = setjmp(cont.m_buffer);
                if (r != 1) {
                    instance->currentExecutionContext()->pushContinuePosition(cont);
                } else {
                    if (m_update)
                        m_update->executeExpression(instance);
                    test = m_test ? m_test->executeExpression(instance) : ESValue(true);
                  }
                while (test.toBoolean()) {
                    m_body->executeStatement(instance);
                    if (m_update)
                        m_update->executeExpression(instance);
                    test = m_test ? m_test->executeExpression(instance) : ESValue(true);
                  }
                instance->currentExecutionContext()->popContinuePosition();
            });
        } else {
            if (m_init)
                m_init->executeExpression(instance);
            test = m_test ? m_test->executeExpression(instance) : ESValue(true);
            while(test.toBoolean()) {
                m_body->executeStatement(instance);
                if (m_update)
                    m_update->executeExpression(instance);
                test = m_test ? m_test->executeExpression(instance) : ESValue(true);
            }
        }
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenereateContext& context)
    {
        if (m_init) {
            m_init->generateExpressionByteCode(codeBlock, context);
            codeBlock->pushCode(Pop(), this);
        }

        size_t forStart = codeBlock->currentCodeSize();
        if(m_test) {
            m_test->generateExpressionByteCode(codeBlock, context);
        } else {
            codeBlock->pushCode(Push(ESValue(true)), this);
        }

        codeBlock->pushCode(JumpIfTopOfStackValueIsFalse(SIZE_MAX), this);
        size_t testPos = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsFalse>();

        {
            ByteCodeGenereateContextStatePusher c(context.m_lastContinuePosition);
            context.m_lastContinuePosition = forStart;
            m_body->generateStatementByteCode(codeBlock, context);
        }

        if(m_update) {
            m_update->generateExpressionByteCode(codeBlock, context);
            codeBlock->pushCode(Pop(), this);
        }
        codeBlock->pushCode(Jump(forStart), this);
        context.consumeBreakPositions(codeBlock);
        size_t forEnd = codeBlock->currentCodeSize();
        codeBlock->peekCode<JumpIfTopOfStackValueIsFalse>(testPos)->m_jumpPosition = forEnd;
    }

protected:
    ExpressionNode *m_init;
    ExpressionNode *m_test;
    ExpressionNode *m_update;
    StatementNode *m_body;
};

}

#endif
