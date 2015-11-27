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

    virtual NodeType type() { return NodeType::ContinueLabelStatement; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(Jump(SIZE_MAX), context, this);
        context.pushLabeledContinuePositions(codeBlock->lastCodePosition<Jump>(), m_label);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 2;
    }

protected:
    size_t m_upIndex;
    ESString* m_label; // for debug
};

}

#endif
