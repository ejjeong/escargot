#ifndef LogicalExpressionNode_h
#define LogicalExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class LogicalExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    enum LogicalExpressionOperator {
        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.11
        // Binary Logical Operators
        LogicalAnd, //"&&"
        LogicalOr,  //"||"
    };

    LogicalExpressionNode(Node *left, Node* right, const InternalString& oper)
            : ExpressionNode(NodeType::LogicalExpression)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;

        // Binary Logical Operator
        if (oper == L"&&")
            m_operator = LogicalAnd;
        else if (oper == L"||")
            m_operator = LogicalOr;

        // TODO
        else
            RELEASE_ASSERT_NOT_REACHED();
    }

    virtual ESValue execute(ESVMInstance* instance)
    {
        ESValue lval = m_left->execute(instance).ensureValue();
        ESValue rval = m_right->execute(instance).ensureValue();
        return execute(instance, lval, rval, m_operator);
    }

    static ESValue execute(ESVMInstance* instance, ESValue lval, ESValue rval, LogicalExpressionOperator oper) {
        ESValue ret;
        switch(oper) {
            case LogicalAnd:
                if (lval.toBoolean() == false) ret = lval;
                else ret = rval;
                break;
            case LogicalOr:
                if (lval.toBoolean() == true) ret = lval;
                else ret = rval;
                break;
            default:
                // TODO
                wprintf(L"unsupport operator is->%d\n",(int)oper);
                RELEASE_ASSERT_NOT_REACHED();
                break;
        }
        return ret;
    }
protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
    LogicalExpressionOperator m_operator;
};

}

#endif
