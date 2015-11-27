#ifndef UpdateExpressionIncrementPrefixNode_h
#define UpdateExpressionIncrementPrefixNode_h

#include "ExpressionNode.h"

namespace escargot {

class UpdateExpressionIncrementPrefixNode : public ExpressionNode {
public:
    friend class ScriptParser;

    UpdateExpressionIncrementPrefixNode(Node *argument)
        : ExpressionNode(NodeType::UpdateExpressionIncrementPrefix)
    {
        m_argument = (ExpressionNode*)argument;
    }

    virtual NodeType type() { return NodeType::UpdateExpressionIncrementPrefix; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateResolveAddressByteCode(codeBlock, context);
        m_argument->generateReferenceResolvedAddressByteCode(codeBlock, context);
        codeBlock->pushCode(ToNumber(), context, this);
        codeBlock->pushCode(Increment(), context, this);
        m_argument->generatePutByteCode(codeBlock, context);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 2;
        m_argument->computeRoughCodeBlockSizeInWordSize(result);
    }
protected:
    ExpressionNode* m_argument;
};

}

#endif
