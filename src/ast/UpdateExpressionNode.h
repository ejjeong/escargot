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

    virtual ESValue execute(ESVMInstance* instance)
    {
        ESValue argref = m_argument->execute(instance);
        ESValue argval = argref.ensureValue();
        ESValue ret;
        switch(m_operator) {
            case INCREMENT:
            {
                if (!m_prefix)
                    ret = argval;
                ESSlot* slot = argref.asESPointer()->asESSlot();
                if (argval.isInt32()) {
                    slot->setValue(ESValue(argval.asInt32() + 1));
                } else {
                    double argnum = argval.toNumber();
                    slot->setValue(ESValue(argnum + 1));
                }
                if (m_prefix)
                    ret = argref.ensureValue();
                break;
            }
            case DECREMENT:
            default:
                // TODO
                RELEASE_ASSERT_NOT_REACHED();
                break;
        }
        return ret;
    }
protected:
    ExpressionNode* m_argument;
    UpdateExpressionOperator m_operator;
    bool m_prefix;
};

}

#endif
