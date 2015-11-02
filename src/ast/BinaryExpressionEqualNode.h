#ifndef BinaryExpressionEqualNode_h
#define BinaryExpressionEqualNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionEqualNode : public ExpressionNode {
public:
    friend class ScriptParser;

    BinaryExpressionEqualNode(Node *left, Node* right)
        : ExpressionNode(NodeType::BinaryExpressionEqual)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(Equal(), context, this);
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif

