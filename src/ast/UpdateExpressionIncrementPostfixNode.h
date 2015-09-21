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
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateResolveAddressByteCode(codeBlock, context);
        m_argument->generateReferenceResolvedAddressByteCode(codeBlock, context);
        codeBlock->pushCode(DuplicateTopOfStackValue(), this);
        codeBlock->pushCode(PushIntoTempStack(), this);
        codeBlock->pushCode(Increment(), this);
        m_argument->generatePutByteCode(codeBlock, context);
        codeBlock->pushCode(Pop(), this);
        codeBlock->pushCode(PopFromTempStack(), this);
    }
protected:
    ExpressionNode* m_argument;
};

}

#endif
