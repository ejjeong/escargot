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

    UpdateExpressionNode(Node *argument, const InternalString& oper, bool prefix)
            : ExpressionNode(NodeType::UpdateExpression)
    {
        m_argument = (ExpressionNode*)argument;

        if (oper == L"++")
            m_operator = INCREMENT;
        else if (oper == L"--")
            m_operator = DECREMENT;
        else
            RELEASE_ASSERT_NOT_REACHED();

        m_prefix = prefix;
    }

    ESValue execute(ESVMInstance* instance)
    {
        ESValue argval = m_argument->execute(instance);
        ESValue ret;
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

        AssignmentExpressionNode::writeValue(instance, m_argument, argval);

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
