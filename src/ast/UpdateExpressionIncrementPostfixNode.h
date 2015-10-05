#ifndef UpdateExpressionIncrementPostfixNode_h
#define UpdateExpressionIncrementPostfixNode_h

#include "ExpressionNode.h"

namespace escargot {

class UpdateExpressionIncrementPostfixNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    UpdateExpressionIncrementPostfixNode(Node *argument)
            : ExpressionNode(NodeType::UpdateExpressionIncrementPostfix)
    {
        m_argument = (ExpressionNode*)argument;
        m_isSimpleCase = false;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if(m_isSimpleCase) {
            m_argument->generateResolveAddressByteCode(codeBlock, context);
            m_argument->generateReferenceResolvedAddressByteCode(codeBlock, context);
            updateNodeIndex(context);
            codeBlock->pushCode(ToNumber(), this);
            WRITE_LAST_INDEX(m_nodeIndex, m_argument->nodeIndex(), -1);
            updateNodeIndex(context);
            codeBlock->pushCode(Increment(), this);
            WRITE_LAST_INDEX(m_nodeIndex, m_argument->nodeIndex() + 1, -1);
            m_argument->updateNodeIndex(context);
            m_argument->generatePutByteCode(codeBlock, context);
            WRITE_LAST_INDEX(m_argument->nodeIndex(), m_nodeIndex, -1);
            return ;
        }
        m_argument->generateResolveAddressByteCode(codeBlock, context);
        m_argument->generateReferenceResolvedAddressByteCode(codeBlock, context);
        codeBlock->pushCode(ToNumber(), this);
        codeBlock->pushCode(DuplicateTopOfStackValue(), this);
        codeBlock->pushCode(PushIntoTempStack(), this);
        codeBlock->pushCode(Increment(), this);
        m_argument->generatePutByteCode(codeBlock, context);
        codeBlock->pushCode(Pop(), this);
        codeBlock->pushCode(PopFromTempStack(), this);
    }
protected:
    ExpressionNode* m_argument;
    bool m_isSimpleCase;
};

}

#endif
