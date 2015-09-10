#ifndef AssignmentExpressionMinusNode_h
#define AssignmentExpressionMinusNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"

namespace escargot {

//An assignment operator expression.
class AssignmentExpressionMinusNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    AssignmentExpressionMinusNode(Node* left, Node* right)
            : ExpressionNode(NodeType::AssignmentExpressionMinus)
    {
        m_left = left;
        m_right = right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESSlotAccessor slot;

        slot = m_left->executeForWrite(instance);
        ESValue lval = slot.value();
        ESValue rval = m_right->executeExpression(instance);
        ESValue ret(ESValue::ESForceUninitialized);
        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.1
        lval = lval.toPrimitive();
        rval = rval.toPrimitive();
        if (lval.isESString() || rval.isESString()) {
            ESString* lstr;
            ESString* rstr;

            lstr = lval.toString();
            rstr = rval.toString();
            ret = ESString::concatTwoStrings(lstr, rstr);
        } else {
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
        }
        slot.setValue(ret);
        return ret;
    }

protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
};

}

#endif
