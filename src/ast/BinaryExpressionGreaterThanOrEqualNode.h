#ifndef BinaryExpressionGreaterThanOrEqualNode_h
#define BinaryExpressionGreaterThanOrEqualNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionGreaterThanOrEqualNode : public ExpressionNode {
public:
    friend class ScriptParser;

    BinaryExpressionGreaterThanOrEqualNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionGreaterThanOrEqual)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        updateNodeIndex(context);
        codeBlock->pushCode(GreaterThanOrEqual(), context, this);
        WRITE_LAST_INDEX(m_nodeIndex, m_left->nodeIndex(), m_right->nodeIndex());
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
