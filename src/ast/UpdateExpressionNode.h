#ifndef UpdateExpressionNode_h
#define UpdateExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class UpdateExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    enum UpdateExpressionOperator {
        INCREMENT, //"++"
        DECREMENT, //"--"
    };

    UpdateExpressionNode(Node *argument, ESString* oper, bool prefix)
            : ExpressionNode(NodeType::UpdateExpression)
    {
        m_argument = (ExpressionNode*)argument;

        if (*oper == u"++")
            m_operator = INCREMENT;
        else if (*oper == u"--")
            m_operator = DECREMENT;
        else
            RELEASE_ASSERT_NOT_REACHED();

        m_prefix = prefix;
    }

    ESValue execute(ESVMInstance* instance)
    {
        ExecutionContext* ec = instance->currentExecutionContext();
        ESSlotWriterForAST::prepareExecuteForWriteASTNode(ec);
        ESSlotAccessor slot = m_argument->executeForWrite(instance);
        ESValue argval = ESSlotWriterForAST::readValue(slot, ec);
        ESValue ret(ESValue::ESForceUninitialized);
        if (!m_prefix)
            ret = argval;

        if(LIKELY(m_operator == INCREMENT)) {
            if (argval.isInt32()) {
                //FIXME check overflow
                argval = ESValue(argval.asInt32() + 1);
            } else {
                double argnum = argval.toNumber();
                argval = ESValue(argnum + 1);
            }
        } else {
            if (argval.isInt32()) {
                //FIXME check overflow
                argval = ESValue(argval.asInt32() - 1);
            } else {
                double argnum = argval.toNumber();
                argval = ESValue(argnum - 1);
            }
        }

        ESSlotWriterForAST::setValue(slot, ec, argval);

        if (m_prefix)
            ret = argval;
        return ret;
    }
protected:
    ExpressionNode* m_argument;
    UpdateExpressionOperator m_operator;
    bool m_prefix;
};

}

#endif
