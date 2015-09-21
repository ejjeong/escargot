#ifndef UnaryExpressionLogicalNotNode_h
#define UnaryExpressionLogicalNotNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionLogicalNotNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    UnaryExpressionLogicalNotNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionLogicalNot)
    {
        m_argument = argument;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(LogicalNot(), this);
    }

protected:
    Node* m_argument;
};

}

#endif
