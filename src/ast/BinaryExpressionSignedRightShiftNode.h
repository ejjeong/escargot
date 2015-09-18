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

    ESValue executeExpression(ESVMInstance* instance)
    {
        int32_t lnum = m_left->executeExpression(instance).toInt32();
        lnum >>= ((unsigned int)m_right->executeExpression(instance).toInt32()) & 0x1F;
        return ESValue(lnum);
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenereateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(SignedRightShift(), this);
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
