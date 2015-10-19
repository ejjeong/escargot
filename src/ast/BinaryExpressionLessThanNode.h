#ifndef BinaryExpressionLessThanNode_h
#define BinaryExpressionLessThanNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionLessThanNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    BinaryExpressionLessThanNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionLessThan)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        updateNodeIndex(context);
        codeBlock->pushCode(LessThan(), context, this);
        WRITE_LAST_INDEX(m_nodeIndex, m_left->nodeIndex(), m_right->nodeIndex());
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
