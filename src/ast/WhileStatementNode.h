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

    virtual ESValue execute(ESVMInstance* instance);

protected:
    ExpressionNode *m_test;
    StatementNode *m_body;
};

}

#endif
