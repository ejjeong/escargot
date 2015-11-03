#ifndef BinaryExpressionInNode_h
#define BinaryExpressionInNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionInNode : public ExpressionNode {
public:
    BinaryExpressionInNode(Node *left, Node* right)
        : ExpressionNode(NodeType::BinaryExpressionIn)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(StringIn(), context, this);
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
