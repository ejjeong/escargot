#ifndef BinaryExpressionStrictEqualNode_h
#define BinaryExpressionStrictEqualNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionStrictEqualNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    BinaryExpressionStrictEqualNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionStrictEqual)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        return ESValue(m_left->executeExpression(instance).equalsTo(m_right->executeExpression(instance)));
    }

    virtual void generateByteCode(CodeBlock* codeBlock)
    {
        m_left->generateByteCode(codeBlock);
        m_right->generateByteCode(codeBlock);
        codeBlock->pushCode(StrictEqual(), this);
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
