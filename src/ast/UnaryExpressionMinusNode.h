#ifndef UnaryExpressionMinusNode_h
#define UnaryExpressionMinusNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionMinusNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    UnaryExpressionMinusNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionMinus)
    {
        m_argument = argument;
    }


    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateExpressionByteCode(codeBlock, context);
        updateNodeIndex(context);
        codeBlock->pushCode(UnaryMinus(), this);
        WRITE_LAST_INDEX(m_nodeIndex, m_argument->nodeIndex(), -1);
    }
protected:
    Node* m_argument;
};

}

#endif
