#ifndef ByteCodeOperations_h
#define ByteCodeOperations_h

#include "runtime/ESValue.h"

namespace escargot {

ALWAYS_INLINE ESValue* getByIdOperation(ESVMInstance* instance, ExecutionContext* ec, GetById* code)
{
    if (LIKELY(code->m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
        ASSERT(ec->resolveBinding(code->m_name) == code->m_cachedSlot);
#ifdef ENABLE_ESJIT
        code->m_profile.addProfile(*code->m_cachedSlot);
#endif
        return code->m_cachedSlot;
    } else {
        ESValue* slot = ec->resolveBinding(code->m_name);
        if (LIKELY(slot != NULL)) {
            code->m_cachedSlot = slot;
            code->m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
#ifdef ENABLE_ESJIT
            code->m_profile.addProfile(*code->m_cachedSlot);
#endif
            return code->m_cachedSlot;
        } else {
            ReferenceError* receiver = ReferenceError::create();
            std::vector<ESValue> arguments;
            u16string err_msg;
            err_msg.append(code->m_name.string()->data());
            err_msg.append(u" is not defined");

            // TODO call constructor
            // ESFunctionObject::call(fn, receiver, &arguments[0], arguments.size(), instance);
            receiver->set(strings->message.string(), ESString::create(std::move(err_msg)));
            throw ESValue(receiver);
        }
    }
}

ALWAYS_INLINE ESValue getByGlobalIndexOperation(GlobalObject* globalObject, GetByGlobalIndex* code)
{
    ESValue val = globalObject->hiddenClass()->read(globalObject, globalObject, code->m_index);
    ASSERT(code->m_orgOpcode == GetByGlobalIndexOpcode);
    if (UNLIKELY(val.isDeleted())) {
        size_t idx = globalObject->hiddenClass()->findProperty(code->m_name);
        if (UNLIKELY(idx == SIZE_MAX)) {
            throw ESValue(ReferenceError::create());
        } else {
            code->m_index = idx;
            return globalObject->hiddenClass()->read(globalObject, globalObject, idx);
        }
    } else {
        ASSERT(globalObject->hiddenClass()->findProperty(code->m_name) == code->m_index);
        return val;
    }
}

ALWAYS_INLINE void setByGlobalIndexOperation(GlobalObject* globalObject, SetByGlobalIndex* code, const ESValue& value)
{
    const ESHiddenClassPropertyInfo& info = globalObject->hiddenClass()->propertyInfo(code->m_index);
    ASSERT(code->m_orgOpcode == SetByGlobalIndexOpcode);
    if (LIKELY(!info.m_flags.m_isDeletedValue)) {
        ASSERT(globalObject->hiddenClass()->findProperty(code->m_name) == code->m_index);
        globalObject->hiddenClass()->write(globalObject, globalObject, code->m_index, value);
    } else {
        size_t idx = globalObject->hiddenClass()->findProperty(code->m_name);
        if (UNLIKELY(idx == SIZE_MAX)) {
            throw ESValue(ReferenceError::create());
        } else {
            code->m_index = idx;
            globalObject->hiddenClass()->write(globalObject, globalObject, code->m_index, value);
        }
    }
}

NEVER_INLINE ESValue getByGlobalIndexOperationWithNoInline(GlobalObject* globalObject, GetByGlobalIndex* code);
NEVER_INLINE void setByGlobalIndexOperationWithNoInline(GlobalObject* globalObject, SetByGlobalIndex* code, const ESValue& value);

NEVER_INLINE ESValue* getByIdOperationWithNoInline(ESVMInstance* instance, ExecutionContext* ec, GetById* code);

NEVER_INLINE ESValue plusOperationSlowCase(const ESValue& left, const ESValue& right);
ALWAYS_INLINE ESValue plusOperation(const ESValue& left, const ESValue& right)
{
    ESValue ret(ESValue::ESForceUninitialized);
    if (left.isInt32() && right.isInt32()) {
        int64_t a = left.asInt32();
        int64_t b = right.asInt32();
        a = a + b;

        if (a > std::numeric_limits<int32_t>::max() || a < std::numeric_limits<int32_t>::min()) {
            ret = ESValue(ESValue::EncodeAsDouble, a);
        } else {
            ret = ESValue((int32_t)a);
        }
        return ret;
    } else if (left.isNumber() && right.isNumber()) {
        ret = ESValue(left.asNumber() + right.asNumber());
        return ret;
    } else {
        return plusOperationSlowCase(left, right);
    }

}

ALWAYS_INLINE ESValue minusOperation(const ESValue& left, const ESValue& right)
{
    // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.2
    ESValue ret(ESValue::ESForceUninitialized);
    if (left.isInt32() && right.isInt32()) {
        int64_t a = left.asInt32();
        int64_t b = right.asInt32();
        a = a - b;

        if (a > std::numeric_limits<int32_t>::max() || a < std::numeric_limits<int32_t>::min()) {
            ret = ESValue(ESValue::EncodeAsDouble, a);
        } else {
            ret = ESValue((int32_t)a);
        }
    } else {
        ret = ESValue(left.toNumber() - right.toNumber());
    }
    return ret;
}

NEVER_INLINE ESValue modOperation(const ESValue& left, const ESValue& right);

// http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
NEVER_INLINE ESValue abstractRelationalComparisonSlowCase(const ESValue& left, const ESValue& right, bool leftFirst);
ALWAYS_INLINE ESValue abstractRelationalComparison(const ESValue& left, const ESValue& right, bool leftFirst)
{
    // consume very fast case
    if (LIKELY(left.isInt32() && right.isInt32())) {
        return ESValue(left.asInt32() < right.asInt32());
    }

    return abstractRelationalComparisonSlowCase(left, right, leftFirst);
}

// d = {}. d[0]
NEVER_INLINE ESValue getObjectOperationSlowCase(ESValue* willBeObject, ESValue* property, GlobalObject* globalObject);
ALWAYS_INLINE ESValue getObjectOperation(ESValue* willBeObject, ESValue* property, GlobalObject* globalObject)
{
    ASSERT(!ESVMInstance::currentInstance()->globalObject()->didSomePrototypeObjectDefineIndexedProperty());
    if (LIKELY(willBeObject->isESPointer() && willBeObject->asESPointer()->isESArrayObject())) {
        ESArrayObject* arr = willBeObject->asESPointer()->asESArrayObject();
        if (LIKELY(arr->isFastmode())) {
            uint32_t idx = property->toIndex();
            if (LIKELY(idx < arr->length())) {
                ASSERT(idx != ESValue::ESInvalidIndexValue);
                const ESValue& v = arr->data()[idx];
                if (LIKELY(!v.isEmpty())) {
                    return v;
                } else {
                    return ESValue();
                }
            }
        }
    }
    return getObjectOperationSlowCase(willBeObject, property, globalObject);
}

// d = {}. d.foo
ALWAYS_INLINE ESValue getObjectPreComputedCaseOperation(ESValue* willBeObject, ESString* keyString, GlobalObject* globalObject
    , ESHiddenClassChain* cachedHiddenClassChain, size_t* cachedHiddenClassIndex)
{
    ASSERT(!ESVMInstance::currentInstance()->globalObject()->didSomePrototypeObjectDefineIndexedProperty());
    ESObject* obj;
    ESObject* targetObj;
    if (LIKELY(willBeObject->isESPointer())) {
        if (LIKELY(willBeObject->asESPointer()->isESObject())) {
            targetObj = obj = willBeObject->asESPointer()->asESObject();
GetObjectPreComputedCaseInlineCacheOperation:
            size_t cSiz = cachedHiddenClassChain->size();
            bool miss = !cSiz;
            if (!miss) {
                for (int i = 0; i < cSiz-1; i ++) {
                    if ((*cachedHiddenClassChain)[i] != obj->hiddenClass()) {
                        miss = true;
                        break;
                    }
                    const ESValue& proto = obj->__proto__();
                    if (LIKELY(proto.isObject())) {
                        obj = proto.asESPointer()->asESObject();
                    } else {
                        miss = true;
                        break;
                    }
                }
            }
            if (!miss) {
                if ((*cachedHiddenClassChain)[cSiz - 1] == obj->hiddenClass()) {
                    if (*cachedHiddenClassIndex != SIZE_MAX) {
                        return obj->hiddenClass()->read(obj, targetObj, *cachedHiddenClassIndex);
                    } else {
                        return ESValue();
                    }
                }
            }

            // cache miss.
            obj = targetObj;

            *cachedHiddenClassIndex = SIZE_MAX;
            cachedHiddenClassChain->clear();
            while (true) {
                cachedHiddenClassChain->push_back(obj->hiddenClass());
                size_t idx = obj->hiddenClass()->findProperty(keyString);
                if (idx != SIZE_MAX) {
                    *cachedHiddenClassIndex = idx;
                    break;
                }
                const ESValue& proto = obj->__proto__();
                if (proto.isObject()) {
                    obj = proto.asESPointer()->asESObject();
                } else
                    break;
            }

            if (*cachedHiddenClassIndex != SIZE_MAX) {
                return obj->hiddenClass()->read(obj, targetObj, *cachedHiddenClassIndex);
            } else {
                return ESValue();
            }
        } else {
            ASSERT(willBeObject->asESPointer()->isESString());
            if (*keyString == *strings->length.string()) {
                return ESValue(willBeObject->asESString()->length());
            }
            globalObject->stringObjectProxy()->setStringData(willBeObject->asESString(), false);
            targetObj = obj = globalObject->stringObjectProxy();
            goto GetObjectPreComputedCaseInlineCacheOperation;
        }
    } else {
        // number
        if (willBeObject->isNumber()) {
            globalObject->numberObjectProxy()->setNumberData(willBeObject->asNumber());
            targetObj = obj = globalObject->numberObjectProxy();
            goto GetObjectPreComputedCaseInlineCacheOperation;
        }
        return willBeObject->toObject()->get(keyString);
    }
}

NEVER_INLINE ESValue getObjectPreComputedCaseOperationWithNeverInline(ESValue* willBeObject, ESString* property, GlobalObject* globalObject
    , ESHiddenClassChain* cachedHiddenClassChain, size_t* cachedHiddenClassIndex);

NEVER_INLINE void throwObjectWriteError();

// d = {}. d[0] = 1
NEVER_INLINE void setObjectOperationSlowCase(ESValue* willBeObject, ESValue* property, const ESValue& value);
NEVER_INLINE void setObjectOperationExpandLengthCase(ESArrayObject* arr, uint32_t idx, const ESValue& value);
ALWAYS_INLINE void setObjectOperation(ESValue* willBeObject, ESValue* property, const ESValue& value)
{
    ASSERT(!ESVMInstance::currentInstance()->globalObject()->didSomePrototypeObjectDefineIndexedProperty());
    if (LIKELY(willBeObject->isESPointer())) {
        if (LIKELY(willBeObject->asESPointer()->isESArrayObject())) {
            ESArrayObject* arr = willBeObject->asESPointer()->asESArrayObject();
            if (LIKELY(arr->isFastmode())) {
                uint32_t idx = property->toIndex();
                if (LIKELY(idx < arr->length())) {
                    ASSERT(idx != ESValue::ESInvalidIndexValue);
                    arr->data()[idx] = value;
                    return;
                } else {
                    if (UNLIKELY(!arr->isExtensible()))
                        return;
                    setObjectOperationExpandLengthCase(arr, idx, value);
                    return;
                }
            }
        }
    }
    setObjectOperationSlowCase(willBeObject, property, value);
}

// d = {}. d.foo = 1
NEVER_INLINE void setObjectPreComputedCaseOperationSlowCase(ESValue* willBeObject, ESString* keyString, const ESValue& value);
ALWAYS_INLINE void setObjectPreComputedCaseOperation(ESValue* willBeObject, ESString* keyString, const ESValue& value
    , ESHiddenClassChain* cachedHiddenClassChain, size_t* cachedHiddenClassIndex, ESHiddenClass** hiddenClassWillBe)
{
    ASSERT(!ESVMInstance::currentInstance()->globalObject()->didSomePrototypeObjectDefineIndexedProperty());

    if (LIKELY(willBeObject->isESPointer())) {
        if (LIKELY(willBeObject->asESPointer()->isESObject())) {
            ESObject* obj = willBeObject->asESPointer()->asESObject();
            if (*cachedHiddenClassIndex != SIZE_MAX && (*cachedHiddenClassChain)[0] == obj->hiddenClass()) {
                ASSERT((*cachedHiddenClassChain).size() == 1);
                // cache hit!
                if (!obj->hiddenClass()->write(obj, obj, *cachedHiddenClassIndex, value)) {
                    throwObjectWriteError();
                }
                return;
            } else if (*hiddenClassWillBe) {
                size_t cSiz = cachedHiddenClassChain->size();
                bool miss = false;
                for (int i = 0; i < cSiz - 1; i ++) {
                    if ((*cachedHiddenClassChain)[i] != obj->hiddenClass()) {
                        miss = true;
                        break;
                    } else {
                        ESValue o = obj->__proto__();
                        if (!o.isObject()) {
                            miss = true;
                            break;
                        }
                        obj = o.asESPointer()->asESObject();
                    }
                }
                if (!miss) {
                    if ((*cachedHiddenClassChain)[cSiz - 1] == obj->hiddenClass()) {
                        // cache hit!
                        obj = willBeObject->asESPointer()->asESObject();
                        obj->m_hiddenClassData.push_back(value);
                        obj->m_hiddenClass = *hiddenClassWillBe;
                        return;
                    }
                }
            }

            // cache miss
            *cachedHiddenClassIndex = SIZE_MAX;
            *hiddenClassWillBe = NULL;
            cachedHiddenClassChain->clear();

            obj = willBeObject->asESPointer()->asESObject();
            size_t idx = obj->hiddenClass()->findProperty(keyString);
            if (idx != SIZE_MAX) {
                // own property
                *cachedHiddenClassIndex = idx;
                cachedHiddenClassChain->push_back(obj->hiddenClass());

                if (!obj->hiddenClass()->write(obj, obj, idx, value))
                    throwObjectWriteError();
            } else {
                cachedHiddenClassChain->push_back(obj->hiddenClass());
                ESValue proto = obj->__proto__();
                while (proto.isObject()) {
                    obj = proto.asESPointer()->asESObject();
                    cachedHiddenClassChain->push_back(obj->hiddenClass());

                    if (obj->hiddenClass()->hasReadOnlyProperty()) {
                        size_t idx = obj->hiddenClass()->findProperty(keyString);
                        if (idx != SIZE_MAX) {
                            if (!obj->hiddenClass()->propertyInfo(idx).m_flags.m_isWritable) {
                                *cachedHiddenClassIndex = SIZE_MAX;
                                *hiddenClassWillBe = NULL;
                                cachedHiddenClassChain->clear();
                                throwObjectWriteError();
                            }
                        }
                    }
                    proto = obj->__proto__();
                }

                ASSERT(!willBeObject->asESPointer()->asESObject()->hasOwnProperty(keyString));
                ESHiddenClass* before = willBeObject->asESPointer()->asESObject()->hiddenClass();
                bool res = willBeObject->asESPointer()->asESObject()->defineDataProperty(keyString, true, true, true, value);

                // only cache vector mode object.
                if (res && willBeObject->asESPointer()->asESObject()->hiddenClass()->isVectorMode()) {
                    *hiddenClassWillBe = willBeObject->asESPointer()->asESObject()->hiddenClass();
                } else {
                    cachedHiddenClassChain->clear();
                    *hiddenClassWillBe = NULL;
                }
            }
            return;
        }
    }
    setObjectPreComputedCaseOperationSlowCase(willBeObject, keyString, value);
}

NEVER_INLINE ESValue getObjectOperationSlowMode(ESValue* willBeObject, ESValue* property, GlobalObject* globalObject);
NEVER_INLINE void setObjectOperationSlowMode(ESValue* willBeObject, ESValue* property, const ESValue& value);

NEVER_INLINE bool instanceOfOperation(ESValue* lval, ESValue* rval);
NEVER_INLINE ESValue typeOfOperation(ESValue* v);
NEVER_INLINE ESValue newOperation(ESVMInstance* instance, GlobalObject* globalObject, ESValue fn, ESValue* arguments, size_t argc);
NEVER_INLINE bool inOperation(ESValue* obj, ESValue* key);
NEVER_INLINE void tryOperation(ESVMInstance* instance, CodeBlock* codeBlock, char* codeBuffer, ExecutionContext* ec, size_t programCounter, Try* code);
NEVER_INLINE EnumerateObjectData* executeEnumerateObject(ESObject* obj);

}
#endif
