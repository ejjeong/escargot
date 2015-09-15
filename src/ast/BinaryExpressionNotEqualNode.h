#ifndef BinaryExpressionNotEqualNode_h
#define BinaryExpressionNotEqualNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionNotEqualNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    BinaryExpressionNotEqualNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionNotEqual)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        return ESValue(!m_left->executeExpression(instance).abstractEqualsTo(m_right->executeExpression(instance)));
    }

    virtual void generateByteCode(CodeBlock* codeBlock)
    {
        m_left->generateByteCode(codeBlock);
        m_right->generateByteCode(codeBlock);
        codeBlock->pushCode(NotEqual(), this);
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
