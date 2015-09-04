#ifndef WhileStatementNode_h
#define WhileStatementNode_h

#include "StatementNode.h"
#include "ExpressionNode.h"

namespace escargot {

class WhileStatementNode : public StatementNode, public ControlFlowNode {
public:
    friend class ESScriptParser;
    WhileStatementNode(Node *test, Node *body)
            : StatementNode(NodeType::WhileStatement)
    {
        m_test = (ExpressionNode*) test;
        m_body = (StatementNode*) body;
    }

    void executeStatement(ESVMInstance* instance)
    {
        if(UNLIKELY(m_isSlowCase)) {
            ESValue test = m_test->executeExpression(instance);
            instance->currentExecutionContext()->setJumpPositionAndExecute([&](){
                    jmpbuf_wrapper cont;
                    int r = setjmp(cont.m_buffer);
                    if (r != 1) {
                        instance->currentExecutionContext()->pushContinuePosition(cont);
                    } else {
                        test = m_test->executeExpression(instance);
                    }
                    while (test.toBoolean()) {
                        m_body->executeStatement(instance);
                        test = m_test->executeExpression(instance);
                    }
                    instance->currentExecutionContext()->popContinuePosition();
            });
        } else {
            while (m_test->executeExpression(instance).toBoolean()) {
                m_body->executeStatement(instance);
            }
        }
    }

    void markAsSlowCase()
    {
        m_isSlowCase = true;
    }

protected:
    ExpressionNode *m_test;
    StatementNode *m_body;
};

}

#endif
