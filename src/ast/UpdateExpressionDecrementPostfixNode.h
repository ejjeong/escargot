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
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESSlotAccessor slot = m_argument->executeForWrite(instance);
        ESValue argval = slot.value();
        ESValue ret = argval;

        if (LIKELY(argval.isInt32())) {
            //FIXME check overflow
            argval = ESValue(argval.asInt32() - 1);
        } else {
            double argnum = argval.toNumber();
            argval = ESValue(argnum - 1);
        }

        slot.setValue(argval);

        return ret;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateResolveAddressByteCode(codeBlock, context);
        m_argument->generateReferenceResolvedAddressByteCode(codeBlock, context);
        codeBlock->pushCode(DuplicateTopOfStackValue(), this);
        codeBlock->pushCode(PushIntoTempStack(), this);
        codeBlock->pushCode(Decrement(), this);
        m_argument->generatePutByteCode(codeBlock, context);
        codeBlock->pushCode(Pop(), this);
        codeBlock->pushCode(PopFromTempStack(), this);
    }
protected:
    ExpressionNode* m_argument;
};

}

#endif
