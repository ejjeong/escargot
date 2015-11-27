#ifndef UnaryExpressionMinusNode_h
#define UnaryExpressionMinusNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionMinusNode : public ExpressionNode {
public:
    friend class ScriptParser;
    UnaryExpressionMinusNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionMinus)
    {
        m_argument = argument;
    }

    virtual NodeType type() { return NodeType::UnaryExpressionMinus; }


    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(UnaryMinus(), context, this);
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
