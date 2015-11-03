#ifndef SwitchCaseNode_h
#define SwitchCaseNode_h

#include "StatementNode.h"
#include "ExpressionNode.h"

namespace escargot {

class SwitchCaseNode : public StatementNode {
public:
    friend class ScriptParser;
    friend class SwitchStatementNode;
    SwitchCaseNode(Node* test, StatementNodeVector&& consequent)
        : StatementNode(NodeType::SwitchCase)
    {
        m_test = (ExpressionNode*) test;
        m_consequent = consequent;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        for (unsigned i = 0; i < m_consequent.size(); i++)
            m_consequent[i]->generateStatementByteCode(codeBlock, context);
    }

    bool isDefaultNode()
    {
        return !m_test;
    }

protected:
    ExpressionNode* m_test;
    StatementNodeVector m_consequent;
};

}

#endif
