#ifndef BinaryExpressionSignedRightShiftNode_h
#define BinaryExpressionSignedRightShiftNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionSignedRightShiftNode : public ExpressionNode {
public:
    BinaryExpressionSignedRightShiftNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionSignedRightShift)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(SignedRightShift(), context, this);
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
