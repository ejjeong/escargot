#ifndef ReturnStatmentNode_h
#define ReturnStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class ReturnStatmentNode : public StatementNode {
public:
    friend class ESScriptParser;
    ReturnStatmentNode(Node* argument)
            : StatementNode(NodeType::ReturnStatement)
    {
        m_argument = argument;
    }

    void executeStatement(ESVMInstance* instance)
    {
        instance->currentExecutionContext()->doReturn(m_argument ? m_argument->executeExpression(instance) : ESValue());
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenereateContext& context)
    {
        if(m_argument) {
            m_argument->generateExpressionByteCode(codeBlock, context);
            codeBlock->pushCode(ReturnFunctionWithValue(), this);
        } else {
            codeBlock->pushCode(ReturnFunction(), this);
        }
    }
protected:
    Node* m_argument;
};

}

#endif
