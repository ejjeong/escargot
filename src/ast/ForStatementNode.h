#ifndef ForStatementNode_h
#define ForStatementNode_h

#include "StatementNode.h"

namespace escargot {

class ForStatementNode : public StatementNode {
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

    virtual ESValue execute(ESVMInstance* instance)
    {
        /*
        m_init->execute(instance)->ensureValue();
        ESValue *test = m_test->execute(instance)->ensureValue();
        while (test->isSmi()? test->toSmi()->value() : test->toHeapObject()->toESBoolean()->get()) {
            m_body->execute(instance);
            m_update->execute(instance)->ensureValue();
            test = m_test->execute(instance)->ensureValue();
        }
        return esUndefined;
        */
        return ESValue();
    }

protected:
    ExpressionNode *m_init;
    ExpressionNode *m_test;
    ExpressionNode *m_update;
    StatementNode *m_body;
};

}

#endif
