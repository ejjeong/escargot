#ifndef ContinueLabelStatmentNode_h
#define ContinueLabelStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class ContinueLabelStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    ContinueLabelStatementNode(size_t upIndex, ESString* label)
        : StatementNode(NodeType::ContinueLabelStatement)
    {
        m_upIndex = upIndex;
        m_label = label;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(Jump(SIZE_MAX), context, this);
        context.pushLabeledContinuePositions(codeBlock->lastCodePosition<Jump>(), m_label);
    }

protected:
    size_t m_upIndex;
    ESString* m_label; // for debug
};

}

#endif



