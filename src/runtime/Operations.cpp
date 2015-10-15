#include "Escargot.h"
#include "Operations.h"

namespace escargot {

NEVER_INLINE ESValue modOperation(const ESValue& left, const ESValue& right)
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

/*
//http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
NEVER_INLINE ESValue abstractRelationalComparison(const ESValue& left, const ESValue& right, bool leftFirst)
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
*/

NEVER_INLINE ESValue getObjectPreComputedCaseOperationWithNeverInline(ESValue* willBeObject, ESString* property, ESValue* lastObjectValueMetInMemberExpression, GlobalObject* globalObject
        ,ESHiddenClassChain* cachedHiddenClassChain, size_t* cachedHiddenClassIndex)
{
    return getObjectPreComputedCaseOperation(willBeObject, property, lastObjectValueMetInMemberExpression, globalObject, cachedHiddenClassChain, cachedHiddenClassIndex);
}

NEVER_INLINE ESValue getObjectOperationSlowMode(ESValue* willBeObject, ESValue* property, ESValue* lastObjectValueMetInMemberExpression, GlobalObject* globalObject)
{
    ASSERT(ESVMInstance::currentInstance()->globalObject()->didSomeObjectDefineIndexedReadOnlyOrAccessorProperty());
    *lastObjectValueMetInMemberExpression = *willBeObject;
    if(willBeObject->isESPointer()) {
        if(willBeObject->asESPointer()->isESArrayObject()) {
            return willBeObject->toObject()->get(*property);
        } else if(willBeObject->asESPointer()->isESString()) {
            size_t idx = property->toIndex();
            if(idx != SIZE_MAX) {
                if(LIKELY(0 <= idx && idx < willBeObject->asESString()->length())) {
                    char16_t c = willBeObject->asESString()->string().data()[idx];
                    if(LIKELY(c < ESCARGOT_ASCII_TABLE_MAX)) {
                        return strings->asciiTable[c].string();
                    } else {
                        return ESString::create(c);
                    }
                }
                return willBeObject->toObject()->get(*property);
            } else {
                ESString* val = property->toString();
                if(*val == *strings->length) {
                    return ESValue(willBeObject->asESString()->length());
                }
                globalObject->stringObjectProxy()->setStringData(willBeObject->asESString());
                ESValue ret = globalObject->stringObjectProxy()->get(val);
                return ret;
            }
        } else {
            ASSERT(willBeObject->asESPointer()->isESObject());
            return willBeObject->asESPointer()->asESObject()->get(*property);
        }
    } else {
        //number
        if(willBeObject->isNumber()) {
            globalObject->numberObjectProxy()->setNumberData(willBeObject->asNumber());
            return globalObject->numberObjectProxy()->get(*property);
        }
        return willBeObject->toObject()->get(*property);
    }
}


NEVER_INLINE void setObjectOperationSlowMode(ESValue* willBeObject, ESValue* property, const ESValue& value)
{
    ASSERT(ESVMInstance::currentInstance()->globalObject()->didSomeObjectDefineIndexedReadOnlyOrAccessorProperty());
    willBeObject->toObject()->set(*property, value, true);
}

NEVER_INLINE bool instanceOfOperation(ESValue* lval, ESValue* rval)
{
    if(rval->isESPointer() && rval->asESPointer()->isESFunctionObject() && lval->isESPointer() && lval->asESPointer()->isESObject()) {
        ESFunctionObject* C = rval->asESPointer()->asESFunctionObject();
        ESValue P = C->protoType();
        ESValue O = lval->asESPointer()->asESObject()->__proto__();
        if(P.isESPointer() && P.asESPointer()->isESObject()) {
            while (!O.isUndefinedOrNull()) {
                if(P == O) {
                    return true;
                }
                O = O.asESPointer()->asESObject()->__proto__();
            }
        }
        else {
            throw ReferenceError::create(ESString::create(u""));
        }
    }
    return false;
}

NEVER_INLINE ESValue typeOfOperation(ESValue* v)
{
    if(v->isUndefined() || v->isEmpty())
        return strings->undefined.string();
    else if(v->isNull())
        return strings->object.string();
    else if(v->isBoolean())
        return strings->boolean.string();
    else if(v->isNumber())
        return strings->number.string();
    else if(v->isESString())
        return strings->string.string();
    else {
        ASSERT(v->isESPointer());
        ESPointer* p = v->asESPointer();
        if(p->isESFunctionObject()) {
            return strings->function.string();
        } else {
            return strings->object.string();
        }
    }
}

NEVER_INLINE ESValue newOperation(ESVMInstance* instance, GlobalObject* globalObject, ESValue fn, ESValue* arguments, size_t argc)
{
    if(!fn.isESPointer() || !fn.asESPointer()->isESFunctionObject())
        throw ESValue(TypeError::create(ESString::create(u"constructor is not an function object")));
    ESFunctionObject* function = fn.asESPointer()->asESFunctionObject();
    ESObject* receiver;
    if (function == globalObject->date()) {
        receiver = ESDateObject::create();
    } else if (function == globalObject->array()) {
        receiver = ESArrayObject::create(0);
    } else if (function == globalObject->string()) {
        receiver = ESStringObject::create();
    } else if (function == globalObject->regexp()) {
        receiver = ESRegExpObject::create(strings->emptyString.string(),ESRegExpObject::Option::None);
    } else if (function == globalObject->boolean()) {
        receiver = ESBooleanObject::create(false);
    } else if (function == globalObject->number()) {
        receiver = ESNumberObject::create(0);
    } else if (function == globalObject->error()) {
        receiver = ESErrorObject::create();
    } else if (function == globalObject->referenceError()) {
        receiver = ReferenceError::create();
    } else if (function == globalObject->typeError()) {
        receiver = TypeError::create();
    } else if (function == globalObject->syntaxError()) {
        receiver = SyntaxError::create();
    } else if (function == globalObject->rangeError()) {
        receiver = RangeError::create();
    }
    // TypedArray
    else if (function == globalObject->int8Array()) {
        receiver = ESTypedArrayObject<Int8Adaptor>::create();
    } else if (function == globalObject->uint8Array()) {
        receiver = ESTypedArrayObject<Uint8Adaptor>::create();
    } else if (function == globalObject->int16Array()) {
        receiver = ESTypedArrayObject<Int16Adaptor>::create();
    } else if (function == globalObject->uint16Array()) {
        receiver = ESTypedArrayObject<Uint16Adaptor>::create();
    } else if (function == globalObject->int32Array()) {
        receiver = ESTypedArrayObject<Int32Adaptor>::create();
    } else if (function == globalObject->uint32Array()) {
        receiver = ESTypedArrayObject<Uint32Adaptor>::create();
    } else if (function == globalObject->uint8ClampedArray()) {
        receiver = ESTypedArrayObject<Uint8ClampedAdaptor>::create();
    } else if (function == globalObject->float32Array()) {
        receiver = ESTypedArrayObject<Float32Adaptor>::create();
    } else if (function == globalObject->float64Array()) {
        receiver = ESTypedArrayObject<Float64Adaptor>::create();
    } else if (function == globalObject->arrayBuffer()) {
        receiver = ESArrayBufferObject::create();
    } else {
        receiver = ESObject::create();
    }

    if(function->protoType().isObject())
        receiver->set__proto__(function->protoType());
    else
        receiver->set__proto__(ESObject::create());

    ESValue res = ESFunctionObject::call(instance, fn, receiver, arguments, argc, true);
    if (res.isObject())
        return res;
    else
        return receiver;
}

NEVER_INLINE bool inOperation(ESValue* obj, ESValue* key)
{
    bool result = false;
    ESValue target = obj->toObject();
    while(true) {
        if(!target.isObject()) {
            break;
        }
        result = target.asESPointer()->asESObject()->hasOwnProperty(*key);
        if(result)
            break;
        target = target.asESPointer()->asESObject()->__proto__();
    }

    return result;
}

}
