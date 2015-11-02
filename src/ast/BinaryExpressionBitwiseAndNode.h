#ifndef BinaryExpressionBitwiseAndNode_h
#define BinaryExpressionBitwiseAndNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionBitwiseAndNode: public ExpressionNode {
public:
    friend class ScriptParser;

    BinaryExpressionBitwiseAndNode(Node *left, Node* right)
        : ExpressionNode(NodeType::BinaryExpressionBitwiseAnd)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(BitwiseAnd(), context, this);
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif

