#ifndef UnaryExpressionBitwiseNotNode_h
#define UnaryExpressionBitwiseNotNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionBitwiseNotNode : public ExpressionNode {
public:
    friend class ScriptParser;
    UnaryExpressionBitwiseNotNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionBitwiseNot)
    {
        m_argument = argument;
    }

    virtual NodeType type() { return NodeType::UnaryExpressionBitwiseNot; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(BitwiseNot(), context, this);
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
