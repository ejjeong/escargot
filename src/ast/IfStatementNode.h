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

    void executeStatement(ESVMInstance* instance)
    {
        if (m_test->executeExpression(instance).toBoolean())
            m_consequente->executeStatement(instance);
        else if (m_alternate)
            m_alternate->executeStatement(instance);
    }

protected:
    ExpressionNode *m_test;
    StatementNode *m_consequente;
    StatementNode *m_alternate;
};

}

#endif
