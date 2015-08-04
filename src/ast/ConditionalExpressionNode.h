#ifndef ConditionalExpressionNode_h
#define ConditionalExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class ConditionalExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    ConditionalExpressionNode(Node *test, Node *consequente, Node *alternate)
            : ExpressionNode(NodeType::ConditionalExpression)
    {
        m_test = (ExpressionNode*) test;
        m_consequente = (StatementNode*) consequente;
        m_alternate = (StatementNode*) alternate;
    }

    virtual ESValue execute(ESVMInstance* instance)
    {
        ESValue test = m_test->execute(instance);
        if (test.toBoolean())
            return m_consequente->execute(instance);
        else
            return m_alternate->execute(instance);
    }

protected:
    ExpressionNode* m_test;
    StatementNode* m_consequente;
    StatementNode* m_alternate;
};

}

#endif
