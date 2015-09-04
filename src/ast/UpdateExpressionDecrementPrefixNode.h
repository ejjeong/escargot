#ifndef UpdateExpressionDecrementPrefixNode_h
#define UpdateExpressionDecrementPrefixNode_h

#include "ExpressionNode.h"

namespace escargot {

class UpdateExpressionDecrementPrefixNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    UpdateExpressionDecrementPrefixNode(Node *argument)
            : ExpressionNode(NodeType::UpdateExpressionDecrementPrefix)
    {
        m_argument = (ExpressionNode*)argument;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ExecutionContext* ec = instance->currentExecutionContext();
        ESSlotWriterForAST::prepareExecuteForWriteASTNode(ec);
        ESSlotAccessor slot = m_argument->executeForWrite(instance);
        ESValue argval = ESSlotWriterForAST::readValue(slot, ec);
        ESValue ret(ESValue::ESForceUninitialized);

        if (LIKELY(argval.isInt32())) {
            //FIXME check overflow
            argval = ESValue(argval.asInt32() - 1);
        } else {
            double argnum = argval.toNumber();
            argval = ESValue(argnum - 1);
        }

        ESSlotWriterForAST::setValue(slot, ec, argval);

        ret = argval;
        return ret;
    }
protected:
    ExpressionNode* m_argument;
};

}

#endif
