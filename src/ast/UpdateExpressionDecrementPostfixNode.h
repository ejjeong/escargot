#ifndef UpdateExpressionDecrementPostfixNode_h
#define UpdateExpressionDecrementPostfixNode_h

#include "ExpressionNode.h"

namespace escargot {

class UpdateExpressionDecrementPostfixNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    UpdateExpressionDecrementPostfixNode(Node *argument)
            : ExpressionNode(NodeType::UpdateExpressionDecrementPostfix)
    {
        m_argument = (ExpressionNode*)argument;
        m_isSimpleCase = false;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if(m_isSimpleCase) {
            m_argument->generateResolveAddressByteCode(codeBlock, context);
            m_argument->generateReferenceResolvedAddressByteCode(codeBlock, context);
            codeBlock->pushCode(ToNumber(), context, this);
            codeBlock->pushCode(Decrement(), context, this);
            m_argument->generatePutByteCode(codeBlock, context);
            return ;
        }
        m_argument->generateResolveAddressByteCode(codeBlock, context);
        m_argument->generateReferenceResolvedAddressByteCode(codeBlock, context);
        codeBlock->pushCode(ToNumber(), context, this);
        codeBlock->pushCode(DuplicateTopOfStackValue(), context, this);
        codeBlock->pushCode(PushIntoTempStack(), context, this);
        codeBlock->pushCode(Decrement(), context, this);
        m_argument->generatePutByteCode(codeBlock, context);
        codeBlock->pushCode(Pop(), context, this);
        codeBlock->pushCode(PopFromTempStack(), context, this);
    }

protected:
    ExpressionNode* m_argument;
    bool m_isSimpleCase;
};

}

#endif
