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
        instance->unregisterCheckedObjectAll();
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
    ESBindingSlot slot;
    LexicalEnvironment* env = nullptr;

    if (code->m_onlySearchGlobal)
        slot = instance->globalObject()->addressOfProperty(code->m_name.string());
    else
        slot = ec->resolveBinding(code->m_name, env);

    if (LIKELY(slot)) {
        if (LIKELY(slot.isMutable())) {
            if (LIKELY(slot.isDataBinding())) {
                if (LIKELY(slot.isGlobalBinding())) {
                    code->m_cachedSlot = slot.getSlot();
                    code->m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
                }
                *slot.getSlot() = *value;
            } else {
                // must be global binding
                ASSERT(!env || env->record()->isGlobalEnvironmentRecord());
                if (!instance->globalObject()->set(code->m_name.string(), *value))
                    throwObjectWriteError();
            }
        } else {
            throwObjectWriteError();
        }
    } else {
        if (UNLIKELY(code->m_name == strings->arguments)) {
            if (ESValue* ret = ec->resolveArgumentsObjectBinding()) {
                *ret = *value;
                return;
            }
        }
        if (!ec->isStrictMode()) {
            globalObject->defineDataProperty(code->m_name.string(), true, true, true, *value);
        } else {
            UTF16String err_msg;
            err_msg.append(u"assignment to undeclared variable ");
            err_msg.append(code->m_name.string()->toUTF16String());
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
    ASSERT(globalObject->hiddenClass()->findProperty(code->m_name) == SIZE_MAX);
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
            double r = fmod(lvalue, rvalue);
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
                return willBeObject->toTransientObject(globalObject)->get(*property, willBeObject);
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
        return willBeObject->toTransientObject(globalObject)->get(*property, willBeObject);
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

NEVER_INLINE ESValue getObjectPrecomputedCaseOperationSlowMode(ESValue* willBeObject, ESValue* property, GlobalObject* globalObject)
{
    ASSERT(ESVMInstance::currentInstance()->globalObject()->didSomePrototypeObjectDefineIndexedProperty());
    if (willBeObject->isESPointer()) {
        if (willBeObject->asESPointer()->isESArrayObject()) {
            return willBeObject->toTransientObject(globalObject)->get(*property);
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
                return willBeObject->toTransientObject(globalObject)->get(*property, willBeObject);
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
            return willBeObject->toTransientObject(globalObject)->get(*property, willBeObject);
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
        return willBeObject->toTransientObject(globalObject)->get(*property, willBeObject);
    }
}

NEVER_INLINE void throwObjectWriteError(const char* msg)
{
    if (ESVMInstance::currentInstance()->currentExecutionContext()->isStrictMode())
        ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create(msg))));
}

NEVER_INLINE void throwUndefinedReferenceError(const ESString* name)
{
    UTF16String err_msg;
    err_msg.append(name->toUTF16String());
    err_msg.append(u" is not defined");
    ReferenceError* receiver = ReferenceError::create(ESString::create(std::move(err_msg)));
    // std::vector<ESValue, gc_allocator<ESValue> > arguments;

    // TODO call constructor
    // ESFunctionObject::call(fn, receiver, &arguments[0], arguments.size(), instance);
    // receiver->set(strings->message.string(), ESString::create(std::move(err_msg)));
    ESVMInstance::currentInstance()->throwError(receiver);
}

NEVER_INLINE void setObjectPrecomputedCaseOperationSlowMode(ESValue* willBeObject, ESValue* property, const ESValue& value)
{
    ASSERT(ESVMInstance::currentInstance()->globalObject()->didSomePrototypeObjectDefineIndexedProperty());
    if (!willBeObject->toTransientObject()->setSlowPath(*property, value, willBeObject)) {
        throwObjectWriteError();
    }
}

NEVER_INLINE void setObjectOperationSlowCase(ESValue* willBeObject, ESValue* property, const ESValue& value)
{
    if (!willBeObject->toTransientObject()->set(*property, value, willBeObject)) {
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
    if (!willBeObject->toTransientObject()->set(keyString, value, willBeObject)) {
        throwObjectWriteError();
    }
}

NEVER_INLINE bool instanceOfOperation(ESValue* lval, ESValue* rval)
{
    if (!(rval->isESPointer() && rval->asESPointer()->isESFunctionObject()) || !rval->isObject())
        ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Invalid operand to 'instanceof': Callee is not a function object"))));
    if (rval->isESPointer() && rval->asESPointer()->isESFunctionObject() && lval->isESPointer() && lval->asESPointer()->isESObject()) {
        ESFunctionObject* C = rval->asESPointer()->asESFunctionObject();
        while (C->isBoundFunc())
            C = C->codeBlock()->peekCode<CallBoundFunction>(0)->m_boundTargetFunction;
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
            ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("instanceof called on an object with an invalid prototype property"))));
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
    ESFunctionObject* finalTargetFunction = function;
    if (function->nonConstructor()) {
        UTF16String str;
        str.append(function->name()->toUTF16String());
        str.append(u" is not a constructor");
        instance->throwError(ESValue(TypeError::create(ESString::create(std::move(str)))));
    }
    CallBoundFunction* callBoundFunctionCode = nullptr;
    if (function->isBoundFunc()) {
        callBoundFunctionCode = function->codeBlock()->peekCode<CallBoundFunction>(0);
        function = callBoundFunctionCode->m_boundTargetFunction;
        finalTargetFunction = function;
        while (finalTargetFunction->isBoundFunc())
            finalTargetFunction = finalTargetFunction->codeBlock()->peekCode<CallBoundFunction>(0)->m_boundTargetFunction;
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
    } else if (function == globalObject->uriError()) {
        receiver = URIError::create();
    } else if (function == globalObject->evalError()) {
        receiver = EvalError::create();
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

    if (finalTargetFunction->protoType().isObject())
        receiver->set__proto__(finalTargetFunction->protoType());
    else
        receiver->set__proto__(ESObject::create());

    ESValue res;
    if (callBoundFunctionCode) {
        size_t targetFuncArgCount = argc + callBoundFunctionCode->m_boundArgumentsCount;
        ESValue* targetFuncArgs;
        ALLOCA_WRAPPER(instance, targetFuncArgs, ESValue*, sizeof(ESValue) * targetFuncArgCount, false);
        memcpy(targetFuncArgs, callBoundFunctionCode->m_boundArguments, sizeof(ESValue) * callBoundFunctionCode->m_boundArgumentsCount);
        memcpy(targetFuncArgs + callBoundFunctionCode->m_boundArgumentsCount, arguments, sizeof(ESValue) * argc);
        res = ESFunctionObject::call(instance, function, receiver, targetFuncArgs, targetFuncArgCount, true);
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
    ESValue target = obj->toTransientObject();
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
//        ec->tryOrCatchBodyResult() = ESValue(ESValue::ESEmptyValue);
        ec->tryOrCatchBodyResult().push_back(ESValue(ESValue::ESEmptyValue));
        ESValue ret = interpret(instance, codeBlock, resolveProgramCounter(codeBuffer, programCounter + sizeof(Try)), stackStorage, heapStorage);
        if (!ret.isEmpty()) {
            ec->tryOrCatchBodyResult()[ec->tryOrCatchBodyResult().size() - 1] = (ESControlFlowRecord::create(ESControlFlowRecord::ControlFlowReason::NeedsReturn, ret, ESValue((int32_t)code->m_tryDupCount)));
        }
        instance->unregisterTryPos(&tryPosition);
        instance->unregisterCheckedObjectAll();
    } else {
        ESValue err = instance->getCatchedError();
        tryOperationThrowCase(err, oldEnv, backupedEC, instance, codeBlock, codeBuffer, ec, programCounter, code, stackStorage, heapStorage);
    }
}

NEVER_INLINE void tryOperationThrowCase(const ESValue& err, LexicalEnvironment* oldEnv, ExecutionContext* backupedEC, ESVMInstance* instance, CodeBlock* codeBlock, char* codeBuffer, ExecutionContext* ec, size_t programCounter, Try* code, ESValue* stackStorage, ESValueVector* heapStorage)
{
    instance->invalidateIdentifierCacheCheckCount();
    instance->m_currentExecutionContext = backupedEC;
    if (code->m_catchPosition == 0) {
        instance->currentExecutionContext()->setEnvironment(oldEnv);
        ec->tryOrCatchBodyResult()[ec->tryOrCatchBodyResult().size() - 1] = (ESControlFlowRecord::create(ESControlFlowRecord::ControlFlowReason::NeedsThrow, err, ESValue((int32_t)code->m_tryDupCount)));
        return;
    }
    LexicalEnvironment* catchEnv = new LexicalEnvironment(new DeclarativeEnvironmentRecordForCatchClause(0, 0, InternalAtomicStringVector(), true, SIZE_MAX, code->m_name, oldEnv->record()), oldEnv);
    instance->enterCatchClause();
    instance->currentExecutionContext()->setEnvironment(catchEnv);
    // instance->currentExecutionContext()->environment()->record()->createMutableBinding(code->m_name);
    instance->currentExecutionContext()->environment()->record()->setMutableBinding(code->m_name, err, false);
    std::jmp_buf tryPosition;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        ESValue ret = interpret(instance, codeBlock, code->m_catchPosition, stackStorage, heapStorage);
        instance->currentExecutionContext()->setEnvironment(oldEnv);
        if (ret.isEmpty()) {
            if (!ec->tryOrCatchBodyResult().empty() && !ec->tryOrCatchBodyResult().back().isEmpty() && ec->tryOrCatchBodyResult().back().isESPointer() && ec->tryOrCatchBodyResult().back().asESPointer()->isESControlFlowRecord()) {
                ESControlFlowRecord* record = ec->tryOrCatchBodyResult().back().asESPointer()->asESControlFlowRecord();
                if (record->reason() == ESControlFlowRecord::ControlFlowReason::NeedsThrow) {
                    ec->tryOrCatchBodyResult().pop_back();
                }
            }
        } else {
            ec->tryOrCatchBodyResult()[ec->tryOrCatchBodyResult().size() - 1] = (ESControlFlowRecord::create(ESControlFlowRecord::ControlFlowReason::NeedsReturn, ret, ESValue((int32_t)code->m_tryDupCount)));
        }
        instance->unregisterTryPos(&tryPosition);
        instance->unregisterCheckedObjectAll();
    } else {
        ESValue e = instance->getCatchedError();
        instance->currentExecutionContext()->setEnvironment(oldEnv);
        ec->tryOrCatchBodyResult()[ec->tryOrCatchBodyResult().size() - 1] = (ESControlFlowRecord::create(ESControlFlowRecord::ControlFlowReason::NeedsThrow, e, ESValue((int32_t)code->m_tryDupCount - 1)));
    }
    instance->exitCatchClause();
}

NEVER_INLINE EnumerateObjectData* executeEnumerateObject(ESObject* obj)
{
    EnumerateObjectData* data = new EnumerateObjectData();
    data->m_object = obj;
    data->m_keys.reserve(obj->keyCount());
    std::vector<ESValue, gc_allocator<ESValue> > nonIntKeys;
    nonIntKeys.reserve(obj->keyCount());

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
        target->enumerationWithNonEnumerable([&data, &nonIntKeys, &keyStringSet](ESValue key, ESHiddenClassPropertyInfo* propertyInfo) {
            if (propertyInfo->m_flags.m_isEnumerable) {
                uint32_t index = key.toIndex();
                if (index != ESValue::ESInvalidIndexValue) {
                    data->m_keys.push_back(key);
                } else {
                    nonIntKeys.push_back(key);
                }
            }
            keyStringSet.insert(key.toString());
        });
        data->m_hiddenClassChain.push_back(target->hiddenClass());
        proto = target->__proto__();
        while (proto.isESPointer() && proto.asESPointer()->isESObject()) {
            target = proto.asESPointer()->asESObject();
            target->enumeration([&data, &nonIntKeys, &keyStringSet](ESValue key) {
                ESString* str = key.toString();
                if (keyStringSet.find(str) == keyStringSet.end()) {
                    uint32_t index = key.toIndex();
                    if (index != ESValue::ESInvalidIndexValue) {
                        data->m_keys.push_back(key);
                    } else {
                        nonIntKeys.push_back(key);
                    }
                    keyStringSet.insert(str);
                }
            });
            data->m_hiddenClassChain.push_back(target->hiddenClass());
            proto = target->__proto__();
        }
    } else {
        target->enumeration([&data, &nonIntKeys](ESValue key) {
            uint32_t index = key.toIndex();
            if (index != ESValue::ESInvalidIndexValue) {
                data->m_keys.push_back(key);
            } else {
                nonIntKeys.push_back(key);
                data->m_hasNonIntKey = true;
            }
        });
        data->m_hiddenClassChain.push_back(target->hiddenClass());
    }

    // TODO: Temporarily block this below code for performance
    /*std::sort(data->m_keys.begin(), data->m_keys.end(), [](ESValue i, ESValue j) {
        return i.toInt32() < j.toInt32();
    });*/
    data->m_keys.insert(data->m_keys.end(), nonIntKeys.begin(), nonIntKeys.end());

    return data;
}

NEVER_INLINE EnumerateObjectData* updateEnumerateObjectData(EnumerateObjectData* data)
{
    EnumerateObjectData* newData = executeEnumerateObject(data->m_object);
    std::vector<ESValue, gc_allocator<ESValue> > differenceKeys;
    if (!data->m_hasNonIntKey) {
        for (ESValue& key : newData->m_keys) {
            if (std::find(data->m_keys.begin(), data->m_keys.begin() + data->m_idx, key) == data->m_keys.begin() + data->m_idx) {
                // If a property that has not yet been visited during enumeration is deleted, then it will not be visited.
                if (std::find(data->m_keys.begin() + data->m_idx, data->m_keys.end(), key) != data->m_keys.end()) {
                    // If new properties are added to the object being enumerated during enumeration,
                    // the newly added properties are not guaranteed to be visited in the active enumeration.
                    differenceKeys.push_back(key);
                }
            }
        }
    } else {
        for (ESValue& keyValue : newData->m_keys) {
            ESString* key = keyValue.toString();
            for (size_t i = data->m_idx; i < data->m_keys.size(); i++) {
                ESString* oldKey = data->m_keys[i].toString();
                if (*key == *oldKey) {
                    differenceKeys.push_back(key);
                    break;
                }
            }
        }
    }
    data = newData;
    data->m_keys = std::move(differenceKeys);
    return data;
}

NEVER_INLINE bool deleteBindingOperation(UnaryDelete* code, ExecutionContext* ec, GlobalObject* globalObject)
{
    InternalAtomicString str(code->m_name->utf8Data(), code->m_name->length());
    if (UNLIKELY(str == strings->arguments && !ec->environment()->record()->isGlobalEnvironmentRecord())) {
        return false;
    } else {
        LexicalEnvironment* env = nullptr;
        ESBindingSlot binding = ec->resolveBinding(str, env);
        if (binding) {
            ASSERT(env);
            if (env->record()->isGlobalEnvironmentRecord()) {
                bool res = globalObject->deleteProperty(code->m_name);
                return res;
            } else if (env->record()->isDeclarativeEnvironmentRecord()) {
                DeclarativeEnvironmentRecord* record = (DeclarativeEnvironmentRecord*)env->record();
                ASSERT(record->needsActivation());
                if (binding.isConfigurable()) {
                    binding.setSlot(ESValue::ESDeletedValueTag::ESDeletedValue);
                    return true;
                } else {
                    return false;
                }
            } else {
                return false;
            }
        } else {
            return true;
        }
    }
}

NEVER_INLINE void initializeFunctionDeclaration(CreateFunction* code, ExecutionContext* ec, ESFunctionObject* function)
{
    function->set(strings->name.string(), code->m_nonAtomicName);
    if (UNLIKELY(code->m_name == strings->arguments && !ec->environment()->record()->isGlobalEnvironmentRecord())) {
        *ec->resolveArgumentsObjectBinding() = function;
    } if (code->m_idIndex == std::numeric_limits<size_t>::max()) {
        ESBindingSlot binding = ec->resolveBinding(code->m_name);
        if (!binding) {
            ec->environment()->record()->createMutableBinding(code->m_name);
        } else if (ec->environment()->record()->isGlobalEnvironmentRecord()) {
            if (binding.isConfigurable()) {
                if (binding.isDataBinding()) {
                    ec->environment()->record()->setMutableBinding(code->m_name, function, false);
                } else {
                    GlobalEnvironmentRecord* record = ec->environment()->record()->asGlobalEnvironmentRecord();
                    record->bindingObject()->defineOwnProperty(code->m_name.string(), PropertyDescriptor {function, Writable | Enumerable}, true);
                    return;
                }
            } else if (!binding.isDataBinding() || !binding.isMutable()) {
                throwObjectWriteError();
            }
        }
        ec->environment()->record()->setMutableBinding(code->m_name, function, false);
    } else {
        if (code->m_isIdIndexOnHeapStorage) {
            *ec->environment()->record()->toDeclarativeEnvironmentRecord()->bindingValueForHeapAllocatedData(code->m_idIndex) = function;
        } else {
            *ec->environment()->record()->toDeclarativeEnvironmentRecord()->bindingValueForStackAllocatedData(code->m_idIndex) = function;
        }
    }
}

}
