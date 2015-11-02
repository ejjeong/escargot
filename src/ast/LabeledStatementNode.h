#ifndef LabeledStatementNode_h
#define LabeledStatementNode_h

#include "StatementNode.h"

namespace escargot {

class LabeledStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    LabeledStatementNode(StatementNode* statementNode, ESString* label)
        : StatementNode(NodeType::LabeledStatement)
    {
        m_statementNode = statementNode;
        m_label = label;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        size_t start = codeBlock->currentCodeSize();
        context.m_positionToContinue = start;
        m_statementNode->generateStatementByteCode(codeBlock, context);
        size_t end = codeBlock->currentCodeSize();
        codeBlock->pushCode(LoadStackPointer(context.m_offsetToBasePointer), context, this);
        context.consumeLabeledBreakPositions(codeBlock, end, m_label);
        context.consumeLabeledContinuePositions(codeBlock, context.m_positionToContinue, m_label);
    }

protected:
    StatementNode* m_statementNode;
    ESString* m_label;
};

}

#endif


