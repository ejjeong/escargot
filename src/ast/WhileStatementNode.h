#ifndef WhileStatementNode_h
#define WhileStatementNode_h

#include "StatementNode.h"
#include "ExpressionNode.h"

namespace escargot {

class WhileStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    WhileStatementNode(Node *test, Node *body)
            : StatementNode(NodeType::WhileStatement)
    {
        m_test = (ExpressionNode*) test;
        m_body = (StatementNode*) body;
    }

    ESValue execute(ESVMInstance* instance)
    {
        ESValue test = m_test->execute(instance);
        instance->currentExecutionContext()->setJumpPositionAndExecute([&](){
                jmpbuf_wrapper cont;
                int r = setjmp(cont.m_buffer);
                if (r != 1) {
                    instance->currentExecutionContext()->pushContinuePosition(cont);
                } else {
                    test = m_test->execute(instance);
                }
                while (test.toBoolean()) {
                    m_body->execute(instance);
                    test = m_test->execute(instance);
                }
                instance->currentExecutionContext()->popContinuePosition();
        });
        return ESValue();
    }

protected:
    ExpressionNode *m_test;
    StatementNode *m_body;
};

}

#endif
