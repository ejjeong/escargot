#ifndef UnaryExpressionLogicalNotNode_h
#define UnaryExpressionLogicalNotNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionLogicalNotNode : public ExpressionNode {
public:
    friend class ScriptParser;
    UnaryExpressionLogicalNotNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionLogicalNot)
    {
        m_argument = argument;
    }

    virtual NodeType type() { return NodeType::UnaryExpressionLogicalNot; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(LogicalNot(), context, this);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 1;
        m_argument->computeRoughCodeBlockSizeInWordSize(result);
    }

protected:
    Node* m_argument;
};

}

#endif
