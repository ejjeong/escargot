#ifndef WhileStatementNode_h
#define WhileStatementNode_h

#include "StatementNode.h"

namespace escargot {

class WhileStatementNode : public StatementNode {
public:
    WhileStatementNode(Node *test, Node *body)
            : StatementNode(NodeType::WhileStatement)
    {
        m_test = (ExpressionNode*) test;
        m_body = (StatementNode*) body;
    }

    virtual ESValue* execute(ESVMInstance* instance)
    {
        ESValue *test = m_test->execute(instance)->ensureValue();
        while (test->isSmi()? test->toSmi()->value() : test->toHeapObject()->toPBoolean()->get()) {
            m_body->execute(instance);
            test = m_test->execute(instance)->ensureValue();
        }
        return esUndefined;
    }

protected:
    ExpressionNode *m_test;
    StatementNode *m_body;
};

}

#endif
