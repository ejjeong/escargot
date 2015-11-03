#ifndef BinaryExpressionNotEqualNode_h
#define BinaryExpressionNotEqualNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionNotEqualNode : public ExpressionNode {
public:
    friend class ScriptParser;

    BinaryExpressionNotEqualNode(Node *left, Node* right)
        : ExpressionNode(NodeType::BinaryExpressionNotEqual)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(NotEqual(), context, this);
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
