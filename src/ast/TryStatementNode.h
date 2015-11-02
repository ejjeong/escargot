#ifndef TryStatementNode_h
#define TryStatementNode_h

#include "StatementNode.h"
#include "runtime/ExecutionContext.h"
#include "CatchClauseNode.h"

namespace escargot {

class TryStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    TryStatementNode(Node *block, Node *handler, CatchClauseNodeVector&& guardedHandlers,  Node *finalizer)
        : StatementNode(NodeType::TryStatement)
    {
        m_block = (BlockStatementNode*) block;
        m_handler = (CatchClauseNode*) handler;
        m_guardedHandlers = guardedHandlers;
        m_finalizer = (BlockStatementNode*) finalizer;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        context.m_tryStatementScopeCount++;
        codeBlock->pushCode(Try(), context, this);
        size_t pos = codeBlock->lastCodePosition<Try>();
        codeBlock->peekCode<Try>(pos)->m_tryDupCount = context.m_tryStatementScopeCount;
        m_block->generateStatementByteCode(codeBlock, context);

        codeBlock->pushCode(TryCatchBodyEnd(), context, this);
        size_t catchPos = codeBlock->currentCodeSize();
        if (m_handler) {
            m_handler->generateStatementByteCode(codeBlock, context);
        }
        codeBlock->pushCode(TryCatchBodyEnd(), context, this);

        context.registerJumpPositionsToComplexCase();
        context.m_tryStatementScopeCount--;

        size_t endPos = codeBlock->currentCodeSize();
        codeBlock->peekCode<Try>(pos)->m_catchPosition = catchPos;
        codeBlock->peekCode<Try>(pos)->m_statementEndPosition = endPos;
        if (m_handler) {
            codeBlock->peekCode<Try>(pos)->m_name = m_handler->param()->name();
        } else {
            codeBlock->peekCode<Try>(pos)->m_name = strings->emptyString;
        }
        if (m_finalizer)
            m_finalizer->generateStatementByteCode(codeBlock, context);
        codeBlock->pushCode(FinallyEnd(), context, this);
    }

protected:
    BlockStatementNode *m_block;
    CatchClauseNode *m_handler;
    CatchClauseNodeVector m_guardedHandlers;
    BlockStatementNode *m_finalizer;
};

}

#endif



