#ifndef BreakLabelStatmentNode_h
#define BreakLabelStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class BreakLabelStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    BreakLabelStatementNode(size_t upIndex, ESString* label)
        : StatementNode(NodeType::BreakLabelStatement)
    {
        m_upIndex = upIndex;
        m_label = label;
    }

    virtual NodeType type() { return NodeType::BreakLabelStatement; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(Jump(SIZE_MAX), context, this);
        context.pushLabeledBreakPositions(codeBlock->lastCodePosition<Jump>(), m_label);
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
