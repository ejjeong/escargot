#ifndef ThrowStatementNode_h
#define ThrowStatementNode_h

#include "StatementNode.h"

namespace escargot {

//interface ThrowStatement <: Statement {
class ThrowStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    ThrowStatementNode(Node *argument)
            : StatementNode(NodeType::ThrowStatement)
    {
        m_argument = argument;
    }


    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(Throw(), this);
    }

protected:
    Node* m_argument;
};

}

#endif
