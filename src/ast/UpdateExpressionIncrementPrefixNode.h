#ifndef UpdateExpressionIncrementPrefixNode_h
#define UpdateExpressionIncrementPrefixNode_h

#include "ExpressionNode.h"

namespace escargot {

class UpdateExpressionIncrementPrefixNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    UpdateExpressionIncrementPrefixNode(Node *argument)
            : ExpressionNode(NodeType::UpdateExpressionIncrementPrefix)
    {
        m_argument = (ExpressionNode*)argument;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateResolveAddressByteCode(codeBlock, context);
        m_argument->generateReferenceResolvedAddressByteCode(codeBlock, context);
        codeBlock->pushCode(Increment(), this);
        m_argument->generatePutByteCode(codeBlock, context);
    }
protected:
    ExpressionNode* m_argument;
};

}

#endif
