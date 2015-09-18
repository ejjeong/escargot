#ifndef UnaryExpressionBitwiseNotNode_h
#define UnaryExpressionBitwiseNotNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionBitwiseNotNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    UnaryExpressionBitwiseNotNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionBitwiseNot)
    {
        m_argument = argument;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        return ESValue(~m_argument->executeExpression(instance).toInt32());
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(BitwiseNot(), this);
    }
protected:
    Node* m_argument;
};

}

#endif
