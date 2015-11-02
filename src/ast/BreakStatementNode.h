#ifndef BreakStatmentNode_h
#define BreakStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class BreakStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    BreakStatementNode()
        : StatementNode(NodeType::BreakStatement)
    {
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(Jump(SIZE_MAX), context, this);
        context.pushBreakPositions(codeBlock->lastCodePosition<Jump>());
    }
};

}

#endif


