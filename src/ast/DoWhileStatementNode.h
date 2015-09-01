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

    ESValue execute(ESVMInstance* instance)
    {
        if(UNLIKELY(m_isSlowCase)) {
            ESValue test = m_test->execute(instance);
            instance->currentExecutionContext()->setJumpPositionAndExecute([&](){
                    jmpbuf_wrapper cont;
                    bool start = false;
                    int r = setjmp(cont.m_buffer);
                    if (r != 1) {
                        instance->currentExecutionContext()->pushContinuePosition(cont);
                        start = true;
                    } else {
                        test = m_test->execute(instance);
                    }
                    while (start || test.toBoolean()) {
                        start = false;
                        m_body->execute(instance);
                        test = m_test->execute(instance);
                    }
                    instance->currentExecutionContext()->popContinuePosition();
            });
            return ESValue();
        } else {
            ESValue test(ESValue::ESForceUninitialized);
            do {
                m_body->execute(instance);
                test = m_test->execute(instance);
            } while(test.toBoolean());
            return ESValue();
        }
    }

protected:
    ExpressionNode *m_test;
    StatementNode *m_body;
};

}

#endif
