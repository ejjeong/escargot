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

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateExpressionByteCode(codeBlock, context);
        updateNodeIndex(context);
        codeBlock->pushCode(LogicalNot(), context, this);
        WRITE_LAST_INDEX(m_nodeIndex, m_argument->nodeIndex(), -1);
    }

protected:
    Node* m_argument;
};

}

#endif
