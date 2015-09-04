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
        m_consequente = (ExpressionNode*) consequente;
        m_alternate = (ExpressionNode*) alternate;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESValue test = m_test->executeExpression(instance);
        if (test.toBoolean())
            return m_consequente->executeExpression(instance);
        else
            return m_alternate->executeExpression(instance);
    }

protected:
    ExpressionNode* m_test;
    ExpressionNode* m_consequente;
    ExpressionNode* m_alternate;
};

}

#endif
