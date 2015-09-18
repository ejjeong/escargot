#ifndef BreakStatmentNode_h
#define BreakStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class BreakStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    BreakStatementNode()
            : StatementNode(NodeType::BreakStatement)
    {
    }

    void executeStatement(ESVMInstance* instance)
    {
        instance->currentExecutionContext()->doBreak();
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenereateContext& context)
    {
        codeBlock->pushCode(Jump(SIZE_MAX), this);
        context.pushBreakPositions(codeBlock->lastCodePosition<Jump>());
    }
};

}

#endif
