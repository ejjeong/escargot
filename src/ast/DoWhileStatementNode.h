#ifndef DoWhileStatementNode_h
#define DoWhileStatementNode_h

#include "StatementNode.h"
#include "ExpressionNode.h"

namespace escargot {

class DoWhileStatementNode : public StatementNode, public ControlFlowNode {
public:
    friend class ESScriptParser;
    DoWhileStatementNode(Node *test, Node *body)
            : StatementNode(NodeType::DoWhileStatement)
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
                    bool start = false;
                    int r = setjmp(cont.m_buffer);
                    if (r != 1) {
                        instance->currentExecutionContext()->pushContinuePosition(cont);
                        start = true;
                    } else {
                        test = m_test->executeExpression(instance);
                    }
                    while (start || test.toBoolean()) {
                        start = false;
                        m_body->executeStatement(instance);
                        test = m_test->executeExpression(instance);
                    }
                    instance->currentExecutionContext()->popContinuePosition();
            });
        } else {
            do {
                m_body->executeStatement(instance);
            } while(m_test->executeExpression(instance).toBoolean());
        }
    }

protected:
    ExpressionNode *m_test;
    StatementNode *m_body;
};

}

#endif
