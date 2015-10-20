#ifndef ReturnStatmentNode_h
#define ReturnStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class ReturnStatmentNode : public StatementNode {
public:
    friend class ScriptParser;
    ReturnStatmentNode(Node* argument)
            : StatementNode(NodeType::ReturnStatement)
    {
        m_argument = argument;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if(m_argument) {
            m_argument->generateExpressionByteCode(codeBlock, context);
            codeBlock->pushCode(ReturnFunctionWithValue(), context, this);
            WRITE_LAST_INDEX(-1, m_argument->nodeIndex(), -1);
        } else {
            codeBlock->pushCode(ReturnFunction(), context, this);
        }
    }
protected:
    Node* m_argument;
};

}

#endif
