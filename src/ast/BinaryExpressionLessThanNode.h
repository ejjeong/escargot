#ifndef BinaryExpressionLessThanNode_h
#define BinaryExpressionLessThanNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionLessThanNode : public ExpressionNode {
public:
    friend class ScriptParser;

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
        codeBlock->pushCode(LessThan(), context, this);
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif

