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

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateExpressionByteCode(codeBlock, context);
        updateNodeIndex(context);
        codeBlock->pushCode(BitwiseNot(), context, this);
        WRITE_LAST_INDEX(m_nodeIndex, m_argument->nodeIndex(), -1);
    }
protected:
    Node* m_argument;
};

}

#endif
