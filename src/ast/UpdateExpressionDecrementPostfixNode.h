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

    ESValue execute(ESVMInstance* instance)
    {
        ExecutionContext* ec = instance->currentExecutionContext();
        ESSlotWriterForAST::prepareExecuteForWriteASTNode(ec);
        ESSlotAccessor slot = m_argument->executeForWrite(instance);
        ESValue argval = ESSlotWriterForAST::readValue(slot, ec);
        ESValue ret(ESValue::ESForceUninitialized);
        ret = argval;

        if (argval.isInt32()) {
            //FIXME check overflow
            argval = ESValue(argval.asInt32() - 1);
        } else {
            double argnum = argval.toNumber();
            argval = ESValue(argnum - 1);
        }

        ESSlotWriterForAST::setValue(slot, ec, argval);

        return ret;
    }
protected:
    ExpressionNode* m_argument;
};

}

#endif
