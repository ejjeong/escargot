#ifndef UpdateExpressionDecrementPrefixNode_h
#define UpdateExpressionDecrementPrefixNode_h

#include "ExpressionNode.h"

namespace escargot {

class UpdateExpressionDecrementPrefixNode : public ExpressionNode {
public:
    friend class ScriptParser;

    UpdateExpressionDecrementPrefixNode(Node *argument)
        : ExpressionNode(NodeType::UpdateExpressionDecrementPrefix)
    {
        m_argument = (ExpressionNode*)argument;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateResolveAddressByteCode(codeBlock, context);
        m_argument->generateReferenceResolvedAddressByteCode(codeBlock, context);
        codeBlock->pushCode(ToNumber(), context, this);
        codeBlock->pushCode(Decrement(), context, this);
        m_argument->generatePutByteCode(codeBlock, context);
    }
protected:
    ExpressionNode* m_argument;
};

}

#endif



