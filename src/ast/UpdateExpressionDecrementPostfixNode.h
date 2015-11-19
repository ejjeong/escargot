#ifndef UpdateExpressionDecrementPostfixNode_h
#define UpdateExpressionDecrementPostfixNode_h

#include "ExpressionNode.h"

namespace escargot {

class UpdateExpressionDecrementPostfixNode : public ExpressionNode {
public:
    friend class ScriptParser;

    UpdateExpressionDecrementPostfixNode(Node *argument)
        : ExpressionNode(NodeType::UpdateExpressionDecrementPostfix)
    {
        m_argument = (ExpressionNode*)argument;
        m_isSimpleCase = false;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if (m_isSimpleCase) {
            m_argument->generateResolveAddressByteCode(codeBlock, context);
            m_argument->generateReferenceResolvedAddressByteCode(codeBlock, context);
            codeBlock->pushCode(ToNumber(), context, this);
            codeBlock->pushCode(Decrement(), context, this);
            m_argument->generatePutByteCode(codeBlock, context);
            return;
        }
        m_argument->generateResolveAddressByteCode(codeBlock, context);
        m_argument->generateReferenceResolvedAddressByteCode(codeBlock, context);
        codeBlock->pushCode(ToNumber(), context, this);
        codeBlock->pushCode(DuplicateTopOfStackValue(), context, this);
        size_t pushPos = codeBlock->currentCodeSize();
        codeBlock->pushCode(PushIntoTempStack(), context, this);
        codeBlock->pushCode(Decrement(), context, this);
        m_argument->generatePutByteCode(codeBlock, context);
        codeBlock->pushCode(Pop(), context, this);
        codeBlock->pushCode(PopFromTempStack(pushPos), context, this);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 7;
        m_argument->computeRoughCodeBlockSizeInWordSize(result);
    }

protected:
    ExpressionNode* m_argument;
    bool m_isSimpleCase;
};

}

#endif
