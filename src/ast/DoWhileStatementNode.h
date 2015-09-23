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

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        size_t doStart = codeBlock->currentCodeSize();
        m_body->generateStatementByteCode(codeBlock, context);

        m_test->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(JumpIfTopOfStackValueIsTrue(doStart), this);

        size_t doEnd = codeBlock->currentCodeSize();

        context.consumeContinuePositions(codeBlock, doStart);
        context.consumeBreakPositions(codeBlock, doEnd);
    }

protected:
    ExpressionNode *m_test;
    StatementNode *m_body;
};

}

#endif
