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


//http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
ALWAYS_INLINE ESValue abstractRelationalComparison(const ESValue& left, const ESValue& right, bool leftFirst)
{
    ESValue lval(ESValue::ESForceUninitialized);
    ESValue rval(ESValue::ESForceUninitialized);
    if(leftFirst) {
        lval = left.toPrimitive();
        rval = right.toPrimitive();
    } else {
        rval = right.toPrimitive();
        lval = left.toPrimitive();
    }

    // http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
    if(lval.isInt32() && rval.isInt32()) {
        return ESValue(lval.asInt32() < rval.asInt32());
    } else if (lval.isESString() && rval.isESString()) {
        return ESValue(lval.toString()->string() < rval.toString()->string());
    } else {
        double n1 = lval.toNumber();
        double n2 = rval.toNumber();
        if(isnan(n1) || isnan(n2)) {
            return ESValue();
        } else if(n1 == n2) {
            return ESValue(false);
        } else if(n1 == 0.0 && n2 == -0.0) {
            return ESValue(false);
        } else if(n1 == -0.0 && n2 == 0.0) {
            return ESValue(false);
        } else if(isinf(n1) == 1) {
            return ESValue(false);
        } else if(isinf(n2) == 1) {
            return ESValue(true);
        } else if(isinf(n2) == -1) {
            return ESValue(false);
        } else if(isinf(n1) == -1) {
            return ESValue(true);
        } else {
            return ESValue(n1 < n2);
        }
    }
}

inline ESValue* contextResolveBinding(ExecutionContext* context, InternalAtomicString* atomicName, ESString* name)
{
    return context->resolveBinding(*atomicName, name);
}

inline void objectDefinePropertyOrThrow(ESObject* object, ESString* key,
        /*bool isWritable, bool isEnumarable, bool isConfigurable,*/
        ESValueInDouble initial)
{
    ESValue initialVal = ESValue::fromDouble(initial);
    object->definePropertyOrThrow(key, /*isWritable, isEnumarable, isConfigurable,*/
            true, true, true, initialVal);
}

#ifndef NDEBUG
inline void jitLogIntOperation(int arg)
{
    printf("Logging in JIT code: int 0x%x\n", bitwise_cast<unsigned>(arg));
}
inline void jitLogDoubleOperation(ESValueInDouble arg)
{
    printf("Logging in JIT code: double 0x%lx\n", bitwise_cast<uint64_t>(arg));
}
inline void jitLogPointerOperation(void* arg)
{
    printf("Logging in JIT code: pointer 0x%lx\n", bitwise_cast<uint64_t>(arg));
}
#endif

}
#endif
