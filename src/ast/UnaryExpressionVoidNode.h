#ifndef UnaryExpressionVoidNode_h
#define UnaryExpressionVoidNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionVoidNode : public ExpressionNode {
public:
    friend class ScriptParser;
    UnaryExpressionVoidNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionVoid)
    {
        m_argument = argument;
    }

    virtual NodeType type() { return NodeType::UnaryExpressionVoid; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(UnaryVoid(), context, this);
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
