#ifndef BinaryExpressionEqualNode_h
#define BinaryExpressionEqualNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionEqualNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    BinaryExpressionEqualNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionEqual)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        return ESValue(m_left->executeExpression(instance).abstractEqualsTo(m_right->executeExpression(instance)));
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenereateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(Equal(), this);
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
