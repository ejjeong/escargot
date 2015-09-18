#ifndef DoWhileStatementNode_h
#define DoWhileStatementNode_h

#include "StatementNode.h"
#include "ExpressionNode.h"

namespace escargot {

class DoWhileStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    DoWhileStatementNode(Node *test, Node *body)
            : StatementNode(NodeType::DoWhileStatement)
    {
        m_test = (ExpressionNode*) test;
        m_body = (StatementNode*) body;
    }

protected:
    ExpressionNode *m_test;
    StatementNode *m_body;
};

}

#endif
