#ifndef LabeledStatementNode_h
#define LabeledStatementNode_h

#include "StatementNode.h"

namespace escargot {

class LabeledStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    LabeledStatementNode(StatementNode* statementNode, ESString* label)
            : StatementNode(NodeType::LabeledStatement)
    {
        m_statementNode = statementNode;
        m_label = label;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        size_t position = codeBlock->currentCodeSize();
        codeBlock->pushCode(SaveStackPointer(), this);
        size_t start = codeBlock->currentCodeSize();
        m_statementNode->generateStatementByteCode(codeBlock, context);
        size_t position2 = codeBlock->currentCodeSize();
        codeBlock->pushCode(LoadStackPointer(position), this);
        context.consumeLabeledBreakPositions(codeBlock, position2, m_label);
        context.consumeLabeledContinuePositions(codeBlock, start, m_label);
    }

protected:
    StatementNode* m_statementNode;
    ESString* m_label;
};

}

#endif
