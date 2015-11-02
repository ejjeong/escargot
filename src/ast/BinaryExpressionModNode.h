#ifndef BinaryExpressionModNode_h
#define BinaryExpressionModNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionModNode : public ExpressionNode {
public:
    BinaryExpressionModNode(Node *left, Node* right)
        : ExpressionNode(NodeType::BinaryExpressionMod)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;

    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(Mod(), context, this);
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif



