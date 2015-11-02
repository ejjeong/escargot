#ifndef BinaryExpressionBitwiseOrNode_h
#define BinaryExpressionBitwiseOrNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionBitwiseOrNode: public ExpressionNode {
public:
    friend class ScriptParser;

    BinaryExpressionBitwiseOrNode(Node *left, Node* right)
        : ExpressionNode(NodeType::BinaryExpressionBitwiseOr)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(BitwiseOr(), context, this);
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif


