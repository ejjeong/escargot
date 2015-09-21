#ifndef BinaryExpressionGreaterThanNode_h
#define BinaryExpressionGreaterThanNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionGreaterThanNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    BinaryExpressionGreaterThanNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionGreaterThan)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(GreaterThan(), this);
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
