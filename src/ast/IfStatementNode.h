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

    virtual ESValue execute(ESVMInstance* instance)
    {
        ESValue test = m_test->execute(instance).ensureValue();
        if (test.toBoolean())
            m_consequente->execute(instance);
        else if (m_alternate)
            m_alternate->execute(instance);
        return ESValue();
    }

protected:
    ExpressionNode *m_test;
    StatementNode *m_consequente;
    StatementNode *m_alternate;
};

}

#endif
