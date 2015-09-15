#ifndef BinaryExpressionLogicalAndNode_h
#define BinaryExpressionLogicalAndNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionLogicalAndNode : public ExpressionNode {
public:
    BinaryExpressionLogicalAndNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionLogicalAnd)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESValue lval = m_left->executeExpression(instance);

        if (lval.toBoolean() == false)
            return lval;
        else
            return m_right->executeExpression(instance);
    }

    virtual void generateByteCode(CodeBlock* codeBlock)
    {
        m_left->generateByteCode(codeBlock);
        codeBlock->pushCode<JumpIfTopOfStackValueIsFalseWithPeeking>(JumpIfTopOfStackValueIsFalseWithPeeking(SIZE_MAX), this);
        size_t pos = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsFalseWithPeeking>();
        codeBlock->pushCode(Pop(), this);
        m_right->generateByteCode(codeBlock);
        codeBlock->peekCode<JumpIfTopOfStackValueIsFalseWithPeeking>(pos)->m_jumpPosition = codeBlock->currentCodeSize();
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
