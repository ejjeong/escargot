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

    void executeStatement(ESVMInstance* instance)
    {
        throw m_argument->executeExpression(instance);
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void generateByteCode(CodeBlock* codeBlock)
    {
        m_argument->generateByteCode(codeBlock);
        codeBlock->pushCode(Throw(), this);
    }

protected:
    Node* m_argument;
};

}

#endif
