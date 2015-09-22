#ifndef Operations_h
#define Operations_h

#include "ESValue.h"

namespace escargot {

ALWAYS_INLINE ESValue plusOperation(const ESValue left, const ESValue right)
{
    ESValue lval = left.toPrimitive();
    ESValue rval = right.toPrimitive();
    // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.1

    ESValue ret(ESValue::ESForceUninitialized);
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

inline ESValueInDouble plusOp(ESValueInDouble left, ESValueInDouble right)
{
    ESValue leftVal = ESValue::fromDouble(left);
    ESValue rightVal = ESValue::fromDouble(right);
    ESValueInDouble ret = ESValue::toDouble(plusOperation(leftVal, rightVal));
    //printf("plusop %lx = %lx + %lx\n", bitwise_cast<uint64_t>(ret), bitwise_cast<uint64_t>(left), bitwise_cast<uint64_t>(right));
    return ret;
}

ALWAYS_INLINE ESValue minusOperation(const ESValue left, const ESValue right)
{
    // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.2
    ESValue ret(ESValue::ESForceUninitialized);
    if (left.isInt32() && right.isInt32()) {
        int a = left.asInt32(), b = right.asInt32();
        if (UNLIKELY((a > 0 && b < 0 && b < a - std::numeric_limits<int32_t>::max()))) {
            //overflow
            ret = ESValue((double)left.asInt32() - (double)right.asInt32());
        } else if (UNLIKELY(a < 0 && b > 0 && b > a - std::numeric_limits<int32_t>::min())) {
            //underflow
            ret = ESValue((double)left.asInt32() - (double)right.asInt32());
        } else {
            ret = ESValue(left.asInt32() - right.asInt32());
        }
    }
    else
        ret = ESValue(left.toNumber() - right.toNumber());
    return ret;
}

#ifndef NDEBUG
inline void jitLogOperation(ESValueInDouble arg)
{
    printf("Logging in JIT code: 0x%lx\n", bitwise_cast<uint64_t>(arg));
}
#endif

}
#endif
