#include "Escargot.h"
#include "bytecode/ByteCode.h"
#include "ByteCodeOperations.h"

namespace escargot {

NEVER_INLINE ESValue* getByIdOperationWithNoInline(ESVMInstance* instance, ExecutionContext* ec, GetById* code)
{
    return getByIdOperation(instance, ec, code);
}

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

NEVER_INLINE ESValue getObjectOperationSlowCase(ESValue* willBeObject, ESValue* property, ESValue* lastObjectValueMetInMemberExpression, GlobalObject* globalObject)
{
    if(LIKELY(willBeObject->isESPointer())) {
        if(willBeObject->asESPointer()->isESString()) {
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
                ESValue ret = globalObject->stringObjectProxy()->get(val);
                return ret;
            }
        } else {
            ASSERT(willBeObject->asESPointer()->isESObject());
            return willBeObject->asESPointer()->asESObject()->get(*property);
        }
    } else {
        if(willBeObject->isNumber()) {
            globalObject->numberObjectProxy()->setNumberData(willBeObject->asNumber());
            return globalObject->numberObjectProxy()->get(*property);
        }
        return willBeObject->toObject()->get(*property);
    }
}

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

NEVER_INLINE void throwObjectWriteError()
{
    if(ESVMInstance::currentInstance()->currentExecutionContext()->isStrictMode())
        throw ESValue(TypeError::create());
}

NEVER_INLINE void setObjectOperationSlowMode(ESValue* willBeObject, ESValue* property, const ESValue& value)
{
    ASSERT(ESVMInstance::currentInstance()->globalObject()->didSomeObjectDefineIndexedReadOnlyOrAccessorProperty());
    if(!willBeObject->toObject()->set(*property, value)) {
        throwObjectWriteError();
    }
}

NEVER_INLINE void setObjectOperationSlowCase(ESValue* willBeObject, ESValue* property, const ESValue& value)
{
    if(!willBeObject->toObject()->set(*property, value)) {
        throwObjectWriteError();
    }
}

NEVER_INLINE void setObjectPreComputedCaseOperationSlowCase(ESValue* willBeObject, ESString* keyString, const ESValue& value)
{
    if(!willBeObject->toObject()->set(keyString, value)) {
        throwObjectWriteError();
    }
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

NEVER_INLINE void tryOperation(ESVMInstance* instance, CodeBlock* codeBlock, char* codeBuffer, ExecutionContext* ec, size_t programCounter, Try* code)
{
    LexicalEnvironment* oldEnv = ec->environment();
    ExecutionContext* backupedEC = ec;
    try {
        ESValue ret = interpret(instance, codeBlock, resolveProgramCounter(codeBuffer, programCounter + sizeof(Try)));
        if(!ret.isEmpty()) {
            ec->tryOrCatchBodyResult() = ESControlFlowRecord::create(ESControlFlowRecord::ControlFlowReason::NeedsReturn, ret, ESValue((int32_t)code->m_tryDupCount));
        }
    } catch(const ESValue& err) {
        instance->invalidateIdentifierCacheCheckCount();
        instance->m_currentExecutionContext = backupedEC;
        LexicalEnvironment* catchEnv = new LexicalEnvironment(new DeclarativeEnvironmentRecord(), oldEnv);
        instance->currentExecutionContext()->setEnvironment(catchEnv);
        instance->currentExecutionContext()->environment()->record()->createMutableBinding(code->m_name,
                code->m_nonAtomicName);
        instance->currentExecutionContext()->environment()->record()->setMutableBinding(code->m_name,
                code->m_nonAtomicName
                , err, false);
        try{
            ESValue ret = interpret(instance, codeBlock, code->m_catchPosition);
            instance->currentExecutionContext()->setEnvironment(oldEnv);
            if(ret.isEmpty()) {
                if(!ec->tryOrCatchBodyResult().isEmpty() && ec->tryOrCatchBodyResult().isESPointer() && ec->tryOrCatchBodyResult().asESPointer()->isESControlFlowRecord()) {
                    ESControlFlowRecord* record = ec->tryOrCatchBodyResult().asESPointer()->asESControlFlowRecord();
                    if(record->reason() == ESControlFlowRecord::ControlFlowReason::NeedsThrow) {
                        ec->tryOrCatchBodyResult() = ESValue(ESValue::ESEmptyValue);
                    }
                }
            } else {
                ec->tryOrCatchBodyResult() = ESControlFlowRecord::create(ESControlFlowRecord::ControlFlowReason::NeedsReturn, ret, ESValue((int32_t)code->m_tryDupCount));
            }
        } catch(ESValue e) {
            instance->currentExecutionContext()->setEnvironment(oldEnv);
            ec->tryOrCatchBodyResult() = ESControlFlowRecord::create(ESControlFlowRecord::ControlFlowReason::NeedsThrow, e, ESValue((int32_t)code->m_tryDupCount - 1));
        }
    }
}

NEVER_INLINE EnumerateObjectData* executeEnumerateObject(ESObject* obj)
{
    EnumerateObjectData* data = new EnumerateObjectData();
    data->m_object = obj;
    data->m_keys.reserve(obj->keyCount());
    ESObject* target = obj;
    std::unordered_set<ESString*, std::hash<ESString*>, std::equal_to<ESString*>, gc_allocator<ESString *> > keyStringSet;
    target->enumeration([&data, &keyStringSet](ESValue key) {
        data->m_keys.push_back(key);
        keyStringSet.insert(key.toString());
    });
    ESValue proto = target->__proto__();
    while(proto.isESPointer() && proto.asESPointer()->isESObject()) {
        target = proto.asESPointer()->asESObject();
        target->enumeration([&data, &keyStringSet](ESValue key) {
            ESString* str = key.toString();
            if(keyStringSet.find(str) != keyStringSet.end()) {
                data->m_keys.push_back(key);
                keyStringSet.insert(str);
            }
        });
        proto = target->__proto__();
    }

    return data;
}

}
