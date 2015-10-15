#ifndef Operations_h
#define Operations_h

#include "ESValue.h"

namespace escargot {

ALWAYS_INLINE ESValue plusOperation(const ESValue& left, const ESValue& right)
{
    // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.1

    ESValue ret(ESValue::ESForceUninitialized);
    if(left.isInt32() && right.isInt32()) {
        int64_t a = left.asInt32();
        int64_t b = right.asInt32();
        a = a + b;

        if(a > std::numeric_limits<int32_t>::max() || a < std::numeric_limits<int32_t>::min()) {
            ret = ESValue(ESValue::EncodeAsDouble, a);
        } else {
            ret = ESValue((int32_t)a);
        }
        return ret;
    }

    ESValue lval(ESValue::ESForceUninitialized);
    ESValue rval(ESValue::ESForceUninitialized);

    //http://www.ecma-international.org/ecma-262/5.1/#sec-8.12.8
    //No hint is provided in the calls to ToPrimitive in steps 5 and 6.
    //All native ECMAScript objects except Date objects handle the absence of a hint as if the hint Number were given;
    //Date objects handle the absence of a hint as if the hint String were given.
    //Host objects may handle the absence of a hint in some other manner.
    if(left.isESPointer() && left.asESPointer()->isESDateObject()) {
        lval = left.toPrimitive(ESValue::PreferString);
    } else {
        lval = left.toPrimitive();
    }

    if(right.isESPointer() && right.asESPointer()->isESDateObject()) {
        rval = right.toPrimitive(ESValue::PreferString);
    } else {
        rval = right.toPrimitive();
    }
    if (lval.isESString() || rval.isESString()) {
        ret = ESString::concatTwoStrings(lval.toString(), rval.toString());
    } else {
        ret = ESValue(lval.toNumber() + rval.toNumber());
    }

    return ret;
}

inline ESValueInDouble plusOp(ESValueInDouble left, ESValueInDouble right)
{
    ESValue leftVal = ESValue::fromRawDouble(left);
    ESValue rightVal = ESValue::fromRawDouble(right);
    ESValueInDouble ret = ESValue::toRawDouble(plusOperation(leftVal, rightVal));
    //printf("plusop %lx = %lx + %lx\n", bitwise_cast<uint64_t>(ret), bitwise_cast<uint64_t>(left), bitwise_cast<uint64_t>(right));
    return ret;
}

inline ESValueInDouble ESObjectSetOp(ESValueInDouble obj, ESValueInDouble property, ESValueInDouble source)
{
    ESValue objVal = ESValue::fromRawDouble(obj);
    if (objVal.isESPointer()) {
        ESPointer* objP = objVal.asESPointer();
        if (objP->isESArrayObject()) {
            ESArrayObject* arrObj = objP->asESArrayObject();

            ESValue propVal = ESValue::fromRawDouble(property);
            ESValue srcVal = ESValue::fromRawDouble(source);
            arrObj->set(propVal.asInt32(), srcVal);
        }
    }
    return source;
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

inline ESValueInDouble minusOp(ESValueInDouble left, ESValueInDouble right)
{
    ESValue leftVal = ESValue::fromRawDouble(left);
    ESValue rightVal = ESValue::fromRawDouble(right);
    ESValueInDouble ret = ESValue::toRawDouble(minusOperation(leftVal, rightVal));
    //printf("plusop %lx = %lx + %lx\n", bitwise_cast<uint64_t>(ret), bitwise_cast<uint64_t>(left), bitwise_cast<uint64_t>(right));
    return ret;
}

ALWAYS_INLINE ESValue modOperation(const ESValue& left, const ESValue& right)
{
    ESValue ret(ESValue::ESForceUninitialized);

    int32_t intLeft;
    int32_t intRight;
    if (left.isInt32() && ((intLeft = left.asInt32()) > 0) && right.isInt32() && (intRight = right.asInt32())) {
        ret = ESValue(intLeft % intRight);
    } else {
        double lvalue = left.toNumber();
        double rvalue = right.toNumber();
        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.5.3
        if (std::isnan(lvalue) || std::isnan(rvalue))
            ret = ESValue(std::numeric_limits<double>::quiet_NaN());
        else if (isinf(lvalue) || rvalue == 0 || rvalue == -0.0)
            ret = ESValue(std::numeric_limits<double>::quiet_NaN());
        else if (isinf(rvalue))
            ret = ESValue(lvalue);
        else if (lvalue == 0.0) {
            if(std::signbit(lvalue))
                ret = ESValue(ESValue::EncodeAsDouble, -0.0);
            else
                ret = ESValue(0);
        }
        else {
            bool isLNeg = lvalue < 0.0;
            lvalue = std::abs(lvalue);
            rvalue = std::abs(rvalue);
            int d = lvalue / rvalue;
            double r = lvalue - (d * rvalue);
            if(isLNeg)
                r = -r;
            ret = ESValue(r);
        }
    }

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
        bool sign1 = std::signbit(n1);
        bool sign2 = std::signbit(n2);
        if(isnan(n1) || isnan(n2)) {
            return ESValue();
        } else if(n1 == n2) {
            return ESValue(false);
        } else if(n1 == 0.0 && n2 == 0.0 && sign2) {
            return ESValue(false);
        } else if(n1 == 0.0 && n2 == 0.0 && sign1) {
            return ESValue(false);
        } else if(isinf(n1) && !sign1) {
            return ESValue(false);
        } else if(isinf(n2) && !sign2) {
            return ESValue(true);
        } else if(isinf(n2) && sign2) {
            return ESValue(false);
        } else if(isinf(n1) && sign1) {
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
    ESValue initialVal = ESValue::fromRawDouble(initial);
    object->definePropertyOrThrow(key, /*isWritable, isEnumarable, isConfigurable,*/
            true, true, true, initialVal);
}

inline ESValueInDouble esFunctionObjectCall(ESVMInstance* instance,
        ESValueInDouble callee, ESValueInDouble receiverInput,
        ESValue* arguments, size_t argumentCount, int isNewExpression)
{
    ESValue calleeVal = ESValue::fromRawDouble(callee);
    ESValue receiverInputVal = ESValue::fromRawDouble(receiverInput);
    ESValue ret = ESFunctionObject::call(instance, calleeVal,
            receiverInputVal, arguments, argumentCount, isNewExpression);
    return ESValue::toRawDouble(ret);
}

inline ESValueInDouble resolveNonDataProperty(ESObject* object, ESPointer* hiddenClassIdxData)
{
    // printf("[resolveNonDataProperty] (void*)object : %p\n", (void*)object);
    // printf("[resolveNonDataProperty] (void*)hiddenClassIdxData : %p\n", (void*)hiddenClassIdxData);
    return ESValue::toRawDouble(((ESAccessorData *)hiddenClassIdxData)->value(object));
}

#ifndef NDEBUG
inline void jitLogIntOperation(int arg, const char* msg)
{
    printf("[JIT_LOG] %s : int 0x%x\n", msg, bitwise_cast<unsigned>(arg));
}
inline void jitLogDoubleOperation(ESValueInDouble arg, const char* msg)
{
    printf("[JIT_LOG] %s : double 0x%lx\n", msg, bitwise_cast<uint64_t>(arg));
}
inline void jitLogPointerOperation(void* arg, const char* msg)
{
    printf("[JIT_LOG] %s : pointer 0x%lx\n", msg, bitwise_cast<uint64_t>(arg));
}
inline void jitLogStringOperation(const char* arg, const char* msg)
{
    printf("[JIT_LOG] %s : string %s\n", msg, arg);
}
#endif

}
#endif
