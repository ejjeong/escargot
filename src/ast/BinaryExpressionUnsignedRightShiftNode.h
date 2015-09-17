#ifndef BinaryExpressionUnsignedRightShiftNode_h
#define BinaryExpressionUnsignedRightShiftNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionUnsignedRightShiftNode : public ExpressionNode {
public:
    BinaryExpressionUnsignedRightShiftNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionUnsignedRightShift)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        int32_t lnum = m_left->executeExpression(instance).toInt32();
        unsigned int shiftCount = ((unsigned int)m_right->executeExpression(instance).toInt32()) & 0x1F;
        lnum = ((unsigned int)lnum) >> shiftCount;

        return ESValue(lnum);
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock)
    {
        m_left->generateExpressionByteCode(codeBlock);
        m_right->generateExpressionByteCode(codeBlock);
        codeBlock->pushCode(UnsignedRightShift(), this);
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
