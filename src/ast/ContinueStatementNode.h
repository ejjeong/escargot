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

    void executeStatement(ESVMInstance* instance)
    {
        instance->currentExecutionContext()->doContinue();
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenereateContext& context)
    {
        ASSERT(context.m_lastContinuePosition != SIZE_MAX);
        codeBlock->pushCode(Jump(context.m_lastContinuePosition), this);
    }
};

}

#endif
