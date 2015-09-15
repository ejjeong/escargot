#ifndef BinaryExpressionLeftShiftNode_h
#define BinaryExpressionLeftShiftNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionLeftShiftNode : public ExpressionNode {
public:
    BinaryExpressionLeftShiftNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionLeftShift)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        int32_t lnum = m_left->executeExpression(instance).toInt32();
        int32_t rnum = m_right->executeExpression(instance).toInt32();
        lnum <<= ((unsigned int)rnum) & 0x1F;
        return ESValue(lnum);
    }

    virtual void generateByteCode(CodeBlock* codeBlock)
    {
        m_left->generateByteCode(codeBlock);
        m_right->generateByteCode(codeBlock);
        codeBlock->pushCode(LeftShift(), this);
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
