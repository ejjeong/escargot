#ifndef BinaryExpressionMinusNode_h
#define BinaryExpressionMinusNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionMinusNode : public ExpressionNode {
public:
    BinaryExpressionMinusNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionMinus)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESValue lval = m_left->executeExpression(instance);
        ESValue rval = m_right->executeExpression(instance);
        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.2
        ESValue ret(ESValue::ESForceUninitialized);
        if (lval.isInt32() && rval.isInt32()) {
            int a = lval.asInt32(), b = rval.asInt32();
            if (UNLIKELY((a > 0 && b < 0 && b < a - std::numeric_limits<int32_t>::max()))) {
                //overflow
                ret = ESValue((double)lval.asInt32() - (double)rval.asInt32());
            } else if (UNLIKELY(a < 0 && b > 0 && b > a - std::numeric_limits<int32_t>::min())) {
                //underflow
                ret = ESValue((double)lval.asInt32() - (double)rval.asInt32());
            } else {
                ret = ESValue(lval.asInt32() - rval.asInt32());
            }
        }
        else
            ret = ESValue(lval.toNumber() - rval.toNumber());
        return ret;
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
