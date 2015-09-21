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

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESSlotAccessor slot = m_argument->executeForWrite(instance);
        ESValue argval = slot.value();
        ESValue ret(ESValue::ESForceUninitialized);

        if (LIKELY(argval.isInt32())) {
            //FIXME check overflow
            argval = ESValue(argval.asInt32() + 1);
        } else {
            double argnum = argval.toNumber();
            argval = ESValue(argnum + 1);
        }

        slot.setValue(argval);

        ret = argval;
        return ret;
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
