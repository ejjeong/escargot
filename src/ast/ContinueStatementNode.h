#ifndef ContinueStatmentNode_h
#define ContinueStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class ContinueStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    ContinueStatementNode()
            : StatementNode(NodeType::ContinueStatement)
    {
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenereateContext& context)
    {
        codeBlock->pushCode(Jump(SIZE_MAX), this);
        context.pushContinuePositions(codeBlock->lastCodePosition<Jump>());
    }
};

}

#endif
