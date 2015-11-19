#ifndef UnaryExpressionPlusNode_h
#define UnaryExpressionPlusNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionPlusNode : public ExpressionNode {
public:
    friend class ScriptParser;
    UnaryExpressionPlusNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionPlus)
    {
        m_argument = argument;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(UnaryPlus(), context, this);
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
