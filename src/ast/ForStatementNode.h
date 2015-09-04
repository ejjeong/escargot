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
        if(UNLIKELY(m_isSlowCase)) {
            if (m_init)
                m_init->executeExpression(instance);
            ESValue test = m_test->executeExpression(instance);
            instance->currentExecutionContext()->setJumpPositionAndExecute([&](){
                jmpbuf_wrapper cont;
                int r = setjmp(cont.m_buffer);
                if (r != 1) {
                    instance->currentExecutionContext()->pushContinuePosition(cont);
                } else {
                    m_update->executeExpression(instance);
                    test = m_test->executeExpression(instance);
                }
                while (test.toBoolean()) {
                    m_body->executeStatement(instance);
                    m_update->executeExpression(instance);
                    test = m_test->executeExpression(instance);
                }
                instance->currentExecutionContext()->popContinuePosition();
            });
        } else {
            if (m_init)
                m_init->executeExpression(instance);
            while(m_test->executeExpression(instance).toBoolean()) {
                m_body->executeStatement(instance);
                m_update->executeExpression(instance);
            }
        }
    }

protected:
    ExpressionNode *m_init;
    ExpressionNode *m_test;
    ExpressionNode *m_update;
    StatementNode *m_body;
};

}

#endif
