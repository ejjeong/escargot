#ifndef BinaryExpressionPlusNode_h
#define BinaryExpressionPlusNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionPlusNode : public ExpressionNode {
public:
    BinaryExpressionPlusNode(Node *left, Node* right)
            : ExpressionNode(NodeType::BinaryExpressionPlus)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    ESValue execute(ESVMInstance* instance)
    {
        ESValue ret(ESValue::ESForceUninitialized);
        ESValue lval = m_left->execute(instance).toPrimitive();
        ESValue rval = m_right->execute(instance).toPrimitive();
        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.1

        if(lval.isInt32() && rval.isInt32()) {
            int a = lval.asInt32(), b = rval.asInt32();
            if (UNLIKELY(a > 0 && b > std::numeric_limits<int32_t>::max() - a)) {
                //overflow
                ret = ESValue((double)lval.asInt32() + (double)rval.asInt32());
            } else if (UNLIKELY(a < 0 && b < std::numeric_limits<int32_t>::min() - a)) {
                //underflow
                ret = ESValue((double)lval.asInt32() + (double)rval.asInt32());
            } else {
                ret = ESValue(lval.asInt32() + rval.asInt32());
            }
        } else if (lval.isESString() || rval.isESString()) {
            ret = ESString::concatTwoStrings(lval.toString(), rval.toString());
        } else {
            ret = ESValue(lval.toNumber() + rval.toNumber());
        }

        return ret;
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
