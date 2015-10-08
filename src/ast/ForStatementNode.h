#ifndef ForStatementNode_h
#define ForStatementNode_h

#include "StatementNode.h"
#include "ExpressionNode.h"

namespace escargot {

class ForStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    ForStatementNode(Node *init, Node *test, Node *update, Node *body)
            : StatementNode(NodeType::ForStatement)
    {
        m_init = (ExpressionNode*) init;
        m_test = (ExpressionNode*) test;
        m_update = (ExpressionNode*) update;
        m_body = (StatementNode*) body;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        ByteCodeGenerateContext newContext(context);

        if (m_init) {
            size_t start = codeBlock->currentCodeSize();
            m_init->generateExpressionByteCode(codeBlock, newContext);
            size_t end = codeBlock->currentCodeSize();
            if(start != end)
                codeBlock->pushCode(Pop(), this);
        }

#ifdef ENABLE_ESJIT
        codeBlock->pushCode(LoopStart(), this);
#endif

        size_t forStart = codeBlock->currentCodeSize();

        if(m_test) {
            m_test->generateExpressionByteCode(codeBlock, newContext);
        } else {
            updateNodeIndex(newContext);
            codeBlock->pushCode(Push(ESValue(true)), this);
            WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
        }

        updateNodeIndex(newContext);
        codeBlock->pushCode(JumpIfTopOfStackValueIsFalse(SIZE_MAX), this);
        if (m_test)
            WRITE_LAST_INDEX(m_nodeIndex, m_test->nodeIndex(), -1);
        else
            WRITE_LAST_INDEX(m_nodeIndex, m_nodeIndex-1, -1);
        size_t testPos = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsFalse>();

        m_body->generateStatementByteCode(codeBlock, newContext);

        size_t updatePosition = codeBlock->currentCodeSize();
        if(m_update) {
            m_update->generateExpressionByteCode(codeBlock, newContext);
            codeBlock->pushCode(Pop(), this);
        }
        codeBlock->pushCode(Jump(forStart), this);

        size_t forEnd = codeBlock->currentCodeSize();
        codeBlock->peekCode<JumpIfTopOfStackValueIsFalse>(testPos)->m_jumpPosition = forEnd;

        newContext.consumeBreakPositions(codeBlock, forEnd);
        newContext.consumeContinuePositions(codeBlock, updatePosition);
        newContext.m_positionToContinue = updatePosition;
        newContext.propagateInformationTo(context);
    }

protected:
    ExpressionNode *m_init;
    ExpressionNode *m_test;
    ExpressionNode *m_update;
    StatementNode *m_body;
};

}

#endif
