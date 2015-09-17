#ifndef BinaryExpressionDivisionNode_h
#define BinaryExpressionDivisionNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionDivisionNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    BinaryExpressionDivisionNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionDivison)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        return ESValue(m_left->executeExpression(instance).toNumber() / m_right->executeExpression(instance).toNumber());
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock)
    {
        m_left->generateExpressionByteCode(codeBlock);
        m_right->generateExpressionByteCode(codeBlock);
        codeBlock->pushCode(Division(), this);
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
