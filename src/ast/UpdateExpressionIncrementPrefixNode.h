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

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateResolveAddressByteCode(codeBlock, context);
        m_argument->generateReferenceResolvedAddressByteCode(codeBlock, context);
        updateNodeIndex(context);
        codeBlock->pushCode(ToNumber(), context, this);
        WRITE_LAST_INDEX(m_nodeIndex, m_argument->nodeIndex(), -1);
        updateNodeIndex(context);
        codeBlock->pushCode(Increment(), context, this);
        WRITE_LAST_INDEX(m_nodeIndex, m_argument->nodeIndex() + 1, -1);
        m_argument->generatePutByteCode(codeBlock, context);
        WRITE_LAST_INDEX(m_argument->nodeIndex(), m_nodeIndex, -1);
    }
protected:
    ExpressionNode* m_argument;
};

}

#endif
