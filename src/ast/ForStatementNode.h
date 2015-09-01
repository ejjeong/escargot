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

    ESValue execute(ESVMInstance* instance)
    {
        if(UNLIKELY(m_isSlowCase)) {
            if (m_init)
                m_init->execute(instance);
            ESValue test = m_test->execute(instance);
            instance->currentExecutionContext()->setJumpPositionAndExecute([&](){
                jmpbuf_wrapper cont;
                int r = setjmp(cont.m_buffer);
                if (r != 1) {
                    instance->currentExecutionContext()->pushContinuePosition(cont);
                } else {
                    m_update->execute(instance);
                    test = m_test->execute(instance);
                }
                while (test.toBoolean()) {
                    m_body->execute(instance);
                    m_update->execute(instance);
                    test = m_test->execute(instance);
                }
                instance->currentExecutionContext()->popContinuePosition();
            });
            return ESValue();
        } else {
            if (m_init)
                m_init->execute(instance);
            while(m_test->execute(instance).toBoolean()) {
                m_body->execute(instance);
                m_update->execute(instance);
            }
            return ESValue();
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
