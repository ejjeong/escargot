#ifndef DoWhileStatementNode_h
#define DoWhileStatementNode_h

#include "StatementNode.h"
#include "ExpressionNode.h"

namespace escargot {

class DoWhileStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    DoWhileStatementNode(Node *test, Node *body)
        : StatementNode(NodeType::DoWhileStatement)
    {
        m_test = (ExpressionNode*) test;
        m_body = (StatementNode*) body;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        ByteCodeGenerateContext newContext(context);
#ifdef ENABLE_ESJIT
        codeBlock->pushCode(LoopStart(), newContext, this);
#endif
        size_t doStart = codeBlock->currentCodeSize();
        m_body->generateStatementByteCode(codeBlock, newContext);

        size_t testPos = codeBlock->currentCodeSize();
        m_test->generateExpressionByteCode(codeBlock, newContext);
        codeBlock->pushCode(JumpIfTopOfStackValueIsTrue(doStart), newContext, this);

        size_t doEnd = codeBlock->currentCodeSize();

        newContext.consumeContinuePositions(codeBlock, testPos);
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

