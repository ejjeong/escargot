#include "Escargot.h"
#include "bytecode/ByteCode.h"
#include "ByteCodeOperations.h"

namespace escargot {

NEVER_INLINE ESValue getByIdOperationWithNoInline(ESVMInstance* instance, ExecutionContext* ec, GetById* code)
{
    std::jmp_buf tryPosition;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        ESValue ret = *getByIdOperation(instance, ec, code);
        instance->unregisterTryPos(&tryPosition);
        return ret;
    } else {
        ESValue err = instance->getCatchedError();
        (void)err;
        return ESValue();
    }
}

NEVER_INLINE void setByIdSlowCase(ESVMInstance* instance, GlobalObject* globalObject, SetById* code, ESValue* value)
{
    ExecutionContext* ec = instance->currentExecutionContext();
    // TODO
    // Object.defineProperty(this, "asdf", {value:1}) //this == global
    // asdf = 2
    ESValue* slot;

    if (code->m_onlySearchGlobal)
        slot = instance->globalObject()->addressOfProperty(code->m_name.string());
    else
        slot = ec->resolveBinding(code->m_name);

    if (LIKELY(slot != NULL)) {
        code->m_cachedSlot = slot;
        code->m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
        *code->m_cachedSlot = *value;
    } else {
        if (!ec->isStrictMode()) {
            globalObject->defineDataProperty(code->m_name.string(), true, true, true, *value);
        } else {
            UTF16String err_msg;
            err_msg.append(u"assignment to undeclared variable ");
            err_msg.append(code->m_name.string()->toNullableUTF16String().m_buffer);
            instance->throwError(ESValue(ReferenceError::create(ESString::create(std::move(err_msg)))));
        }
    }
}

NEVER_INLINE ESValue getByGlobalIndexOperationWithNoInline(GlobalObject* globalObject, GetByGlobalIndex* code)
{
    return getByGlobalIndexOperation(globalObject, code);
}


NEVER_INLINE void setByGlobalIndexOperationWithNoInline(GlobalObject* globalObject, SetByGlobalIndex* code, const ESValue& value)
{
    setByGlobalIndexOperation(globalObject, code, value);
}

NEVER_INLINE ESValue getByGlobalIndexOperationSlowCase(GlobalObject* globalObject, GetByGlobalIndex* code)
{
    ESStringBuilder builder;
    builder.appendString(code->m_name);
    builder.appendString("is not defined");
    ESVMInstance::currentInstance()->throwError(ESValue(ReferenceError::create(builder.finalize())));
    RELEASE_ASSERT_NOT_REACHED();
}

NEVER_INLINE void setByGlobalIndexOperationSlowCase(GlobalObject* globalObject, SetByGlobalIndex* code, const ESValue& value)
{
    ASSERT(globalObject->hiddenClass()->findPropertyCheckDeleted(code->m_name) == SIZE_MAX);
    if (ESVMInstance::currentInstance()->currentExecutionContext()->isStrictMode()) {
        ESVMInstance::currentInstance()->throwError(ESValue(ReferenceError::create()));
        RELEASE_ASSERT_NOT_REACHED();
    } else {
        globalObject->defineDataProperty(InternalAtomicString(code->m_name->toNullableUTF8String().m_buffer, code->m_name->toNullableUTF8String().m_bufferSize), true, true, true, value, true);
    }
}


NEVER_INLINE ESValue plusOperationSlowCase(ESValue* left, ESValue* right)
{
    ESValue ret(ESValue::ESForceUninitialized);
    ESValue lval(ESValue::ESForceUninitialized);
    ESValue rval(ESValue::ESForceUninitialized);

    // http://www.ecma-international.org/ecma-262/5.1/#sec-8.12.8
    // No hint is provided in the calls to ToPrimitive in steps 5 and 6.
    // All native ECMAScript objects except Date objects handle the absence of a hint as if the hint Number were given;
    // Date objects handle the absence of a hint as if the hint String were given.
    // Host objects may handle the absence of a hint in some other manner.
    if (left->isESPointer() && left->asESPointer()->isESDateObject()) {
        lval = left->toPrimitive(ESValue::PreferString);
    } else {
        lval = left->toPrimitive();
    }

    if (right->isESPointer() && right->asESPointer()->isESDateObject()) {
        rval = right->toPrimitive(ESValue::PreferString);
    } else {
        rval = right->toPrimitive();
    }

    if (lval.isESString() || rval.isESString()) {
        ret = ESString::concatTwoStrings(lval.toString(), rval.toString());
    } else {
        ret = ESValue(lval.toNumber() + rval.toNumber());
    }

    return ret;
}

NEVER_INLINE ESValue modOperation(ESValue* left, ESValue* right)
{
    ESValue ret(ESValue::ESForceUninitialized);

    int32_t intLeft;
    int32_t intRight;
    if (left->isInt32() && ((intLeft = left->asInt32()) > 0) && right->isInt32() && (intRight = right->asInt32())) {
        ret = ESValue(intLeft % intRight);
    } else {
        double lvalue = left->toNumber();
        double rvalue = right->toNumber();
        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.5.3
        if (std::isnan(lvalue) || std::isnan(rvalue))
            ret = ESValue(std::numeric_limits<double>::quiet_NaN());
        else if (std::isinf(lvalue) || rvalue == 0 || rvalue == -0.0)
            ret = ESValue(std::numeric_limits<double>::quiet_NaN());
        else if (std::isinf(rvalue))
            ret = ESValue(lvalue);
        else if (lvalue == 0.0) {
            if (std::signbit(lvalue))
                ret = ESValue(ESValue::EncodeAsDouble, -0.0);
            else
                ret = ESValue(0);
        } else {
            bool isLNeg = lvalue < 0.0;
            lvalue = std::abs(lvalue);
            rvalue = std::abs(rvalue);
            int d = lvalue / rvalue;
            double r = lvalue - (d * rvalue);
            if (isLNeg)
                r = -r;
            ret = ESValue(r);
        }
    }

    return ret;
}

NEVER_INLINE bool abstractRelationalComparisonSlowCase(ESValue* left, ESValue* right, bool leftFirst)
{
    ESValue lval(ESValue::ESForceUninitialized);
    ESValue rval(ESValue::ESForceUninitialized);
    if (leftFirst) {
        lval = left->toPrimitive();
        rval = right->toPrimitive();
    } else {
        rval = right->toPrimitive();
        lval = left->toPrimitive();
    }

    // http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
    if (lval.isInt32() && rval.isInt32()) {
        return lval.asInt32() < rval.asInt32();
    } else if (lval.isESString() && rval.isESString()) {
        return *lval.asESPointer()->asESString() < *rval.asESPointer()->asESString();
    } else {
        double n1 = lval.toNumber();
        double n2 = rval.toNumber();
        return n1 < n2;
    }
}

NEVER_INLINE bool abstractRelationalComparisonOrEqualSlowCase(ESValue* left, ESValue* right, bool leftFirst)
{
    ESValue lval(ESValue::ESForceUninitialized);
    ESValue rval(ESValue::ESForceUninitialized);
    if (leftFirst) {
        lval = left->toPrimitive();
        rval = right->toPrimitive();
    } else {
        rval = right->toPrimitive();
        lval = left->toPrimitive();
    }

    if (lval.isInt32() && rval.isInt32()) {
        return lval.asInt32() <= rval.asInt32();
    } else if (lval.isESString() && rval.isESString()) {
        return *lval.asESPointer()->asESString() <= *rval.asESPointer()->asESString();
    } else {
        double n1 = lval.toNumber();
        double n2 = rval.toNumber();
        return n1 <= n2;
    }
}

NEVER_INLINE ESValue getObjectOperationSlowCase(ESValue* willBeObject, ESValue* property, GlobalObject* globalObject)
{
    if (LIKELY(willBeObject->isESPointer())) {
        if (willBeObject->asESPointer()->isESString()) {
            uint32_t idx = property->toIndex();
            if (idx != ESValue::ESInvalidIndexValue) {
                if (LIKELY(idx < willBeObject->asESString()->length())) {
                    char16_t c = willBeObject->asESString()->charAt(idx);
                    if (LIKELY(c < ESCARGOT_ASCII_TABLE_MAX)) {
                        return strings->asciiTable[c].string();
                    } else {
                        return ESString::create(c);
                    }
                }
                return willBeObject->toObject()->get(*property, willBeObject);
            } else {
                ESString* val = property->toString();
                if (*val == *strings->length) {
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
        if (willBeObject->isNumber()) {
            globalObject->numberObjectProxy()->setNumberData(willBeObject->asNumber());
            return globalObject->numberObjectProxy()->get(*property);
        }
        return willBeObject->toObject()->get(*property, willBeObject);
    }
}

NEVER_INLINE ESValue getObjectPreComputedCaseOperationWithNeverInline(ESValue* willBeObject, ESString* property, GlobalObject* globalObject
    , ESHiddenClassInlineCache* inlineCache)
{
    return getObjectPreComputedCaseOperation(willBeObject, property, globalObject, inlineCache);
}

NEVER_INLINE void setObjectPreComputedCaseOperationWithNeverInline(ESValue* willBeObject, ESString* keyString, const ESValue& value
    , ESHiddenClassChain* cachedHiddenClassChain, size_t* cachedHiddenClassIndex, ESHiddenClass** hiddenClassWillBe)
{
    return setObjectPreComputedCaseOperation(willBeObject, keyString, value, cachedHiddenClassChain, cachedHiddenClassIndex, hiddenClassWillBe);
}

NEVER_INLINE ESValue getObjectOperationSlowMode(ESValue* willBeObject, ESValue* property, GlobalObject* globalObject)
{
    ASSERT(ESVMInstance::currentInstance()->globalObject()->didSomePrototypeObjectDefineIndexedProperty());
    if (willBeObject->isESPointer()) {
        if (willBeObject->asESPointer()->isESArrayObject()) {
            return willBeObject->toObject()->get(*property);
        } else if (willBeObject->asESPointer()->isESString()) {
            uint32_t idx = property->toIndex();
            if (idx != ESValue::ESInvalidIndexValue) {
                if (LIKELY(idx < willBeObject->asESString()->length())) {
                    char16_t c = willBeObject->asESString()->charAt(idx);
                    if (LIKELY(c < ESCARGOT_ASCII_TABLE_MAX)) {
                        return strings->asciiTable[c].string();
                    } else {
                        return ESString::create(c);
                    }
                }
                return willBeObject->toObject()->get(*property, willBeObject);
            } else {
                ESString* val = property->toString();
                if (*val == *strings->length) {
                    return ESValue(willBeObject->asESString()->length());
                }
                globalObject->stringObjectProxy()->setStringData(willBeObject->asESString());
                ESValue ret = globalObject->stringObjectProxy()->get(val);
                return ret;
            }
        } else if (willBeObject->asESPointer()->isESStringObject()) {
            return willBeObject->toObject()->get(*property, willBeObject);
        } else {
            ASSERT(willBeObject->asESPointer()->isESObject());
            return willBeObject->asESPointer()->asESObject()->get(*property);
        }
    } else {
        // number
        if (willBeObject->isNumber()) {
            globalObject->numberObjectProxy()->setNumberData(willBeObject->asNumber());
            return globalObject->numberObjectProxy()->get(*property, willBeObject);
        }
        return willBeObject->toObject()->get(*property, willBeObject);
    }
}

NEVER_INLINE void throwObjectWriteError(const char* msg)
{
    if (ESVMInstance::currentInstance()->currentExecutionContext()->isStrictMode())
        ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create(msg))));
}

NEVER_INLINE void setObjectOperationSlowMode(ESValue* willBeObject, ESValue* property, const ESValue& value)
{
    ASSERT(ESVMInstance::currentInstance()->globalObject()->didSomePrototypeObjectDefineIndexedProperty());
    if (!willBeObject->toObject()->set(*property, value, willBeObject)) {
        throwObjectWriteError();
    }
}

NEVER_INLINE void setObjectOperationSlowCase(ESValue* willBeObject, ESValue* property, const ESValue& value)
{
    if (!willBeObject->toObject()->set(*property, value, willBeObject)) {
        throwObjectWriteError();
    }
}

NEVER_INLINE void setObjectOperationExpandLengthCase(ESArrayObject* arr, uint32_t idx, const ESValue& value)
{
    if (UNLIKELY(arr->shouldConvertToSlowMode(idx))) {
        ESValue obj(arr);
        ESValue pro((double)idx);
        setObjectOperationSlowCase(&obj, &pro, value);
    } else {
        arr->setLength(idx + 1);
        arr->data()[idx] = value;
    }
}

NEVER_INLINE void setObjectPreComputedCaseOperationSlowCase(ESValue* willBeObject, ESString* keyString, const ESValue& value)
{
    if (!willBeObject->toObject()->set(keyString, value, willBeObject)) {
        throwObjectWriteError();
    }
}

NEVER_INLINE bool instanceOfOperation(ESValue* lval, ESValue* rval)
{
    if (!(rval->isESPointer() && rval->asESPointer()->isESFunctionObject()) || !rval->isObject())
        ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(strings->emptyString)));
    if (rval->isESPointer() && rval->asESPointer()->isESFunctionObject() && lval->isESPointer() && lval->asESPointer()->isESObject()) {
        ESFunctionObject* C = rval->asESPointer()->asESFunctionObject();
        ESValue P = C->protoType();
        ESValue O = lval->asESPointer()->asESObject()->__proto__();
        if (P.isESPointer() && P.asESPointer()->isESObject()) {
            while (!O.isUndefinedOrNull()) {
                if (P == O) {
                    return true;
                }
                O = O.asESPointer()->asESObject()->__proto__();
            }
        } else {
            ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(strings->emptyString)));
        }
    }
    return false;
}

NEVER_INLINE ESValue typeOfOperation(ESValue* v)
{
    if (v->isUndefined() || v->isEmpty())
        return strings->undefined.string();
    else if (v->isNull())
        return strings->object.string();
    else if (v->isBoolean())
        return strings->boolean.string();
    else if (v->isNumber())
        return strings->number.string();
    else if (v->isESString())
        return strings->string.string();
    else {
        ASSERT(v->isESPointer());
        ESPointer* p = v->asESPointer();
        if (p->isESFunctionObject()) {
            return strings->function.string();
        } else {
            return strings->object.string();
        }
    }
}

NEVER_INLINE ESValue newOperation(ESVMInstance* instance, GlobalObject* globalObject, ESValue fn, ESValue* arguments, size_t argc)
{
    if (!fn.isESPointer() || !fn.asESPointer()->isESFunctionObject())
        instance->throwError(ESValue(TypeError::create(ESString::create(u"constructor is not an function object"))));
    ESFunctionObject* function = fn.asESPointer()->asESFunctionObject();
    if (function->nonConstructor()) {
        UTF16String str;
        str.append(function->name()->toNullableUTF16String().m_buffer);
        str.append(u" is not a constructor");
        instance->throwError(ESValue(TypeError::create(ESString::create(std::move(str)))));
    }
    CallBoundFunction* callBoundFunctionCode = nullptr;
    if (function->isBoundFunc()) {
        callBoundFunctionCode = function->codeBlock()->peekCode<CallBoundFunction>(0);
        function = callBoundFunctionCode->m_boundTargetFunction;
    }
    ESObject* receiver;
    if (function == globalObject->date()) {
        receiver = ESDateObject::create();
    } else if (function == globalObject->array()) {
        receiver = ESArrayObject::create(0);
    } else if (function == globalObject->string()) {
        receiver = ESStringObject::create();
    } else if (function == globalObject->regexp()) {
        receiver = ESRegExpObject::create(strings->emptyString.string(), ESRegExpObject::Option::None);
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
    } else if (function == globalObject->int8Array()) {
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

    if (function->protoType().isObject())
        receiver->set__proto__(function->protoType());
    else
        receiver->set__proto__(ESObject::create());

    ESValue res;
    if (callBoundFunctionCode) {
        size_t targetFuncArgCount = argc + callBoundFunctionCode->m_boundArgumentsCount;
        ESValue* targetFuncArgs = (ESValue *)alloca(sizeof(ESValue) * targetFuncArgCount);
        memcpy(targetFuncArgs, callBoundFunctionCode->m_boundArguments, sizeof(ESValue) * callBoundFunctionCode->m_boundArgumentsCount);
        memcpy(targetFuncArgs + callBoundFunctionCode->m_boundArgumentsCount, arguments, sizeof(ESValue) * argc);
        res = ESFunctionObject::call(instance, callBoundFunctionCode->m_boundTargetFunction, receiver, targetFuncArgs, targetFuncArgCount, true);
    } else
        res = ESFunctionObject::call(instance, fn, receiver, arguments, argc, true);
    if (res.isObject())
        return res;
    else
        return receiver;
}

NEVER_INLINE bool inOperation(ESValue* obj, ESValue* key)
{
    bool result = false;
    if (!obj->isObject())
        ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create(u"Type(rval) is not Object"))));
    ESValue target = obj->toObject();
    while (true) {
        if (!target.isObject()) {
            break;
        }
        result = target.asESPointer()->asESObject()->hasOwnProperty(*key);
        if (result)
            break;
        target = target.asESPointer()->asESObject()->__proto__();
    }

    return result;
}

NEVER_INLINE void tryOperation(ESVMInstance* instance, CodeBlock* codeBlock, char* codeBuffer, ExecutionContext* ec, size_t programCounter, Try* code, ESValue* stackStorage, ESValueVector* heapStorage)
{
    LexicalEnvironment* oldEnv = ec->environment();
    ExecutionContext* backupedEC = ec;
    std::jmp_buf tryPosition;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        ec->tryOrCatchBodyResult() = ESValue(ESValue::ESEmptyValue);
        ESValue ret = interpret(instance, codeBlock, resolveProgramCounter(codeBuffer, programCounter + sizeof(Try)), stackStorage, heapStorage);
        if (!ret.isEmpty()) {
            ec->tryOrCatchBodyResult() = ESControlFlowRecord::create(ESControlFlowRecord::ControlFlowReason::NeedsReturn, ret, ESValue((int32_t)code->m_tryDupCount));
        }
        instance->unregisterTryPos(&tryPosition);
    } else {
        ESValue err = instance->getCatchedError();
        tryOperationThrowCase(err, oldEnv, backupedEC, instance, codeBlock, codeBuffer, ec, programCounter, code, stackStorage, heapStorage);
    }
}

NEVER_INLINE void tryOperationThrowCase(const ESValue& err, LexicalEnvironment* oldEnv, ExecutionContext* backupedEC, ESVMInstance* instance, CodeBlock* codeBlock, char* codeBuffer, ExecutionContext* ec, size_t programCounter, Try* code, ESValue* stackStorage, ESValueVector* heapStorage)
{
    instance->invalidateIdentifierCacheCheckCount();
    instance->m_currentExecutionContext = backupedEC;
    LexicalEnvironment* catchEnv = new LexicalEnvironment(new DeclarativeEnvironmentRecord(0, 0, InternalAtomicStringVector(), true), oldEnv);
    instance->currentExecutionContext()->setEnvironment(catchEnv);
    instance->currentExecutionContext()->environment()->record()->createMutableBinding(code->m_name);
    instance->currentExecutionContext()->environment()->record()->setMutableBinding(code->m_name, err, false);
    std::jmp_buf tryPosition;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        ESValue ret = interpret(instance, codeBlock, code->m_catchPosition, stackStorage, heapStorage);
        instance->currentExecutionContext()->setEnvironment(oldEnv);
        if (ret.isEmpty()) {
            if (!ec->tryOrCatchBodyResult().isEmpty() && ec->tryOrCatchBodyResult().isESPointer() && ec->tryOrCatchBodyResult().asESPointer()->isESControlFlowRecord()) {
                ESControlFlowRecord* record = ec->tryOrCatchBodyResult().asESPointer()->asESControlFlowRecord();
                if (record->reason() == ESControlFlowRecord::ControlFlowReason::NeedsThrow) {
                    ec->tryOrCatchBodyResult() = ESValue(ESValue::ESEmptyValue);
                }
            }
        } else {
            ec->tryOrCatchBodyResult() = ESControlFlowRecord::create(ESControlFlowRecord::ControlFlowReason::NeedsReturn, ret, ESValue((int32_t)code->m_tryDupCount));
        }
        instance->unregisterTryPos(&tryPosition);
    } else {
        ESValue e = instance->getCatchedError();
        instance->currentExecutionContext()->setEnvironment(oldEnv);
        ec->tryOrCatchBodyResult() = ESControlFlowRecord::create(ESControlFlowRecord::ControlFlowReason::NeedsThrow, e, ESValue((int32_t)code->m_tryDupCount - 1));
    }
}

NEVER_INLINE EnumerateObjectData* executeEnumerateObject(ESObject* obj)
{
    EnumerateObjectData* data = new EnumerateObjectData();
    data->m_object = obj;
    data->m_keys.reserve(obj->keyCount());

    ESObject* target = obj;
    bool shouldSearchProto = false;
    ESValue proto = target->__proto__();

    while (proto.isESPointer() && proto.asESPointer()->isESObject()) {
        target = proto.asESPointer()->asESObject();
        target->enumeration([&shouldSearchProto](ESValue key) {
            shouldSearchProto = true;
        });

        if (shouldSearchProto) {
            break;
        }
        proto = target->__proto__();
    }

    target = obj;
    if (shouldSearchProto) {
        std::unordered_set<ESString*, std::hash<ESString*>, std::equal_to<ESString*>, gc_allocator<ESString *> > keyStringSet;
        target->enumerationWithNonEnumerable([&data, &keyStringSet](ESValue key, ESHiddenClassPropertyInfo* propertyInfo) {
            if (propertyInfo->m_flags.m_isEnumerable)
                data->m_keys.push_back(key);
            keyStringSet.insert(key.toString());
        });
        proto = target->__proto__();
        while (proto.isESPointer() && proto.asESPointer()->isESObject()) {
            target = proto.asESPointer()->asESObject();
            target->enumeration([&data, &keyStringSet](ESValue key) {
                ESString* str = key.toString();
                if (keyStringSet.find(str) == keyStringSet.end()) {
                    data->m_keys.push_back(key);
                    keyStringSet.insert(str);
                }
            });
            proto = target->__proto__();
        }
    } else {
        target->enumeration([&data](ESValue key) {
            data->m_keys.push_back(key);
        });
    }

    return data;
}

}
