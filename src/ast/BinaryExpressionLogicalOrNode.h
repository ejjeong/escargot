#ifndef BinaryExpressionLogicalOrNode_h
#define BinaryExpressionLogicalOrNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionLogicalOrNode : public ExpressionNode {
public:
    BinaryExpressionLogicalOrNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionLogicalOr)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESValue lval = m_left->executeExpression(instance);

        if (lval.toBoolean() == true)
            return lval;
        else
            return m_right->executeExpression(instance);
    }

    virtual void generateByteCode(CodeBlock* codeBlock)
    {
        m_left->generateByteCode(codeBlock);
        codeBlock->pushCode<JumpIfTopOfStackValueIsTrueWithPeeking>(JumpIfTopOfStackValueIsTrueWithPeeking(SIZE_MAX), this);
        size_t pos = codeBlock->lastCodePosition<JumpIfTopOfStackValueIsTrueWithPeeking>();
        codeBlock->pushCode(Pop(), this);
        m_right->generateByteCode(codeBlock);
        codeBlock->peekCode<JumpIfTopOfStackValueIsTrueWithPeeking>(pos)->m_jumpPosition = codeBlock->currentCodeSize();
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
