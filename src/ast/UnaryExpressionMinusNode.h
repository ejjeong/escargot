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


    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(UnaryMinus(), context, this);
    }
protected:
    Node* m_argument;
};

}

#endif



