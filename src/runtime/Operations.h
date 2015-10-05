#ifndef Operations_h
#define Operations_h

#include "ESValue.h"

namespace escargot {

ALWAYS_INLINE ESValue plusOperation(const ESValue& left, const ESValue& right)
{
    ESValue lval = left.toPrimitive();
    ESValue rval = right.toPrimitive();
    // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.1

    ESValue ret(ESValue::ESForceUninitialized);
    if(lval.isInt32() && rval.isInt32()) {
        int64_t a = lval.asInt32();
        int64_t b = rval.asInt32();
        a = a + b;

        if(a > std::numeric_limits<int32_t>::max() || a < std::numeric_limits<int32_t>::min()) {
            ret = ESValue(ESValue::EncodeAsDouble, a);
        } else {
            ret = ESValue((int32_t)a);
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

ALWAYS_INLINE ESValue minusOperation(const ESValue& left, const ESValue& right)
{
    // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.2
    ESValue ret(ESValue::ESForceUninitialized);
    if (left.isInt32() && right.isInt32()) {
        int64_t a = left.asInt32();
        int64_t b = right.asInt32();
        a = a - b;

        if(a > std::numeric_limits<int32_t>::max() || a < std::numeric_limits<int32_t>::min()) {
            ret = ESValue(ESValue::EncodeAsDouble, a);
        } else {
            ret = ESValue((int32_t)a);
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
