#ifndef ContinueStatmentNode_h
#define ContinueStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class ContinueStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    ContinueStatementNode()
        : StatementNode(NodeType::ContinueStatement)
    {
    }

    virtual NodeType type() { return NodeType::ContinueStatement; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(Jump(SIZE_MAX), context, this);
        context.pushContinuePositions(codeBlock->lastCodePosition<Jump>());
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 2;
    }
};

}

#endif
