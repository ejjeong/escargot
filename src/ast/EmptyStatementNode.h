#ifndef EmptyStatmentNode_h
#define EmptyStatmentNode_h

#include "StatementNode.h"

namespace escargot {

//An empty statement, i.e., a solitary semicolon.
class EmptyStatementNode : public StatementNode {
public:
    EmptyStatementNode()
            : StatementNode(NodeType::EmptyStatement)
    {
    }

    void executeStatement(ESVMInstance* instance)
    {
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenereateContext& context)
    {
    }
protected:
};

}

#endif
