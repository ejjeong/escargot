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
        ByteCodeGenerateContext newContext;
        size_t doStart = codeBlock->currentCodeSize();
        m_body->generateStatementByteCode(codeBlock, newContext);

        m_test->generateExpressionByteCode(codeBlock, newContext);
        codeBlock->pushCode(JumpIfTopOfStackValueIsTrue(doStart), this);

        size_t doEnd = codeBlock->currentCodeSize();

        newContext.consumeContinuePositions(codeBlock, doStart);
        newContext.consumeBreakPositions(codeBlock, doEnd);
        newContext.m_positionToContinue = context.m_positionToContinue;
        newContext.propagateInformationTo(context);
    }

protected:
    ExpressionNode *m_test;
    StatementNode *m_body;
};

}

#endif
