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

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        ByteCodeGenerateContext newContext(context);

#ifdef ENABLE_ESJIT
        codeBlock->pushCode(LoopStart(), this);
#endif

        size_t whileStart = codeBlock->currentCodeSize();
        m_test->generateExpressionByteCode(codeBlock, newContext);

        updateNodeIndex(newContext);

        codeBlock->pushCode(JumpIfTopOfStackValueIsFalse(SIZE_MAX), this);
        WRITE_LAST_INDEX(m_nodeIndex, m_test->nodeIndex(), -1);
        size_t testPos = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsFalse>();

        m_body->generateStatementByteCode(codeBlock, newContext);

        codeBlock->pushCode(Jump(whileStart), this);
        newContext.consumeContinuePositions(codeBlock, whileStart);
        size_t whileEnd = codeBlock->currentCodeSize();
        newContext.consumeBreakPositions(codeBlock, whileEnd);
        codeBlock->peekCode<JumpIfTopOfStackValueIsFalse>(testPos)->m_jumpPosition = whileEnd;
        newContext.m_positionToContinue = context.m_positionToContinue;
        newContext.propagateInformationTo(context);
    }

protected:
    ExpressionNode *m_test;
    StatementNode *m_body;
};

}

#endif
