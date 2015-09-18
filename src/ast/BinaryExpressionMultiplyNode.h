#ifndef BinaryExpressionMultiplyNode_h
#define BinaryExpressionMultiplyNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionMultiplyNode : public ExpressionNode {
public:
    BinaryExpressionMultiplyNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionMultiply)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.5.1
        return ESValue(m_left->executeExpression(instance).toNumber() * m_right->executeExpression(instance).toNumber());
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenereateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(Multiply(), this);
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
