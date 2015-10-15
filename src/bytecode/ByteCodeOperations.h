#ifndef ByteCodeOperations_h
#define ByteCodeOperations_h

#include "runtime/ESValue.h"

namespace escargot {

ALWAYS_INLINE ESValue* getByIdOperation(ESVMInstance* instance, ExecutionContext* ec, GetById* code)
{
    if (LIKELY(code->m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
        ASSERT(ec->resolveBinding(code->m_name, code->m_nonAtomicName) == code->m_cachedSlot);
        return code->m_cachedSlot;
#ifdef ENABLE_ESJIT
        code->m_profile.addProfile(*code->m_cachedSlot);
#endif
    } else {
        ESValue* slot = ec->resolveBinding(code->m_name, code->m_nonAtomicName);
        if(LIKELY(slot != NULL)) {
            code->m_cachedSlot = slot;
            code->m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
            return code->m_cachedSlot;
#ifdef ENABLE_ESJIT
            code->m_profile.addProfile(*code->m_cachedSlot);
#endif
        } else {
            ReferenceError* receiver = ReferenceError::create();
            std::vector<ESValue> arguments;
            u16string err_msg;
            err_msg.append(code->m_nonAtomicName->data());
            err_msg.append(u" is not defined");

            //TODO call constructor
            //ESFunctionObject::call(fn, receiver, &arguments[0], arguments.size(), instance);
            receiver->set(strings->message.string(), ESString::create(std::move(err_msg)));
            throw ESValue(receiver);
        }
    }
}

NEVER_INLINE ESValue* getByIdOperationWithNoInline(ESVMInstance* instance, ExecutionContext* ec, GetById* code);

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

NEVER_INLINE ESValue modOperation(const ESValue& left, const ESValue& right);
//http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
ALWAYS_INLINE ESValue abstractRelationalComparison(const ESValue& left, const ESValue& right, bool leftFirst)
{
    //consume very fast case
    if(LIKELY(left.isInt32() && right.isInt32())) {
        return ESValue(left.asInt32() < right.asInt32());
    }

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
//d = {}. d[0]
ALWAYS_INLINE ESValue getObjectOperation(ESValue* willBeObject, ESValue* property, ESValue* lastObjectValueMetInMemberExpression, GlobalObject* globalObject)
{
    ASSERT(!ESVMInstance::currentInstance()->globalObject()->didSomeObjectDefineIndexedReadOnlyOrAccessorProperty());
    *lastObjectValueMetInMemberExpression = *willBeObject;
    if(LIKELY(willBeObject->isESPointer())) {
        if(LIKELY(willBeObject->asESPointer()->isESArrayObject())) {
            ESArrayObject* arr = willBeObject->asESPointer()->asESArrayObject();
            if(arr->isFastmode()) {
                size_t idx = property->toIndex();
                if(idx != SIZE_MAX && idx < arr->length()) {
                    const ESValue& v = arr->data()[idx];
                    if(!v.isEmpty()) {
                        return v;
                    } else {
                        //TODO search prototo directly
                    }
                }
            }

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

NEVER_INLINE ESValue getObjectOperationWithNeverInline(ESValue* willBeObject, ESValue* property, ESValue* lastObjectValueMetInMemberExpression, GlobalObject* globalObject);

//d = {}. d.foo
ALWAYS_INLINE ESValue getObjectPreComputedCaseOperation(ESValue* willBeObject, ESString* property, ESValue* lastObjectValueMetInMemberExpression, GlobalObject* globalObject
        ,ESHiddenClassChain* cachedHiddenClassChain, size_t* cachedHiddenClassIndex)
{
    ASSERT(!ESVMInstance::currentInstance()->globalObject()->didSomeObjectDefineIndexedReadOnlyOrAccessorProperty());
    *lastObjectValueMetInMemberExpression = *willBeObject;
    if(LIKELY(willBeObject->isESPointer())) {
        if(LIKELY(willBeObject->asESPointer()->isESObject())) {
            ESObject* obj = willBeObject->asESPointer()->asESObject();
            ESString* keyString = property;
            size_t cSiz = cachedHiddenClassChain->size();
            bool miss = !cSiz;
            if(!miss) {
                for(int i = 0; i < cSiz-1 ; i ++) {
                    if((*cachedHiddenClassChain)[i] != obj->hiddenClass()) {
                        miss = true;
                        break;
                    }
                    const ESValue& proto = obj->__proto__();
                    if(LIKELY(proto.isObject())) {
                        obj = proto.asESPointer()->asESObject();
                    } else {
                        miss = true;
                        break;
                    }
                }
            }
            if(!miss) {
                if((*cachedHiddenClassChain)[cSiz - 1] == obj->hiddenClass()) {
                    if(*cachedHiddenClassIndex != SIZE_MAX) {
                        return obj->hiddenClass()->read(obj, willBeObject->asESPointer()->asESObject(), *cachedHiddenClassIndex);
                    } else {
                        return ESValue();
                    }
                }
            }

            //cache miss.
            obj = willBeObject->asESPointer()->asESObject();

            *cachedHiddenClassIndex = SIZE_MAX;
            cachedHiddenClassChain->clear();
            while(true) {
                cachedHiddenClassChain->push_back(obj->hiddenClass());
                size_t idx = obj->hiddenClass()->findProperty(keyString);
                if(idx != SIZE_MAX) {
                    *cachedHiddenClassIndex = idx;
                    break;
                }
                const ESValue& proto = obj->__proto__();
                if(proto.isObject()) {
                    obj = proto.asESPointer()->asESObject();
                } else
                    break;
            }

            if(*cachedHiddenClassIndex != SIZE_MAX) {
                return obj->hiddenClass()->read(obj, willBeObject->asESPointer()->asESObject(), *cachedHiddenClassIndex);
            } else {
                return ESValue();
            }
        } else {
            ASSERT(willBeObject->asESPointer()->isESString());
            if(*property == *strings->length) {
                return ESValue(willBeObject->asESString()->length());
            }
            globalObject->stringObjectProxy()->setStringData(willBeObject->asESString());
            ESValue ret = globalObject->stringObjectProxy()->get(property);
            return ret;
        }
    } else {
        //number
        if(willBeObject->isNumber()) {
            globalObject->numberObjectProxy()->setNumberData(willBeObject->asNumber());
            return globalObject->numberObjectProxy()->get(property);
        }
        return willBeObject->toObject()->get(property);
    }
}

NEVER_INLINE ESValue getObjectPreComputedCaseOperationWithNeverInline(ESValue* willBeObject, ESString* property, ESValue* lastObjectValueMetInMemberExpression, GlobalObject* globalObject
        ,ESHiddenClassChain* cachedHiddenClassChain, size_t* cachedHiddenClassIndex);

//d = {}. d[0] = 1
ALWAYS_INLINE void setObjectOperation(ESValue* willBeObject, ESValue* property, const ESValue& value)
{
    ASSERT(!ESVMInstance::currentInstance()->globalObject()->didSomeObjectDefineIndexedReadOnlyOrAccessorProperty());
    if(LIKELY(willBeObject->isESPointer())) {
        if(LIKELY(willBeObject->asESPointer()->isESArrayObject())) {
            ESArrayObject* arr = willBeObject->asESPointer()->asESArrayObject();
            if(arr->isFastmode()) {
                size_t idx = property->toIndex();
                if(/*idx != SIZE_MAX && */idx < arr->length()) {
                    ASSERT(idx != SIZE_MAX);
                    arr->data()[idx] = value;
                    return ;
                }
            }
            willBeObject->toObject()->set(*property, value, true);
        } else if(willBeObject->asESPointer()->isESString()) {
            willBeObject->toObject()->set(*property, value, true);
        } else {
            ASSERT(willBeObject->asESPointer()->isESObject());
            willBeObject->asESPointer()->asESObject()->set(*property, value, true);
        }
    } else {
        willBeObject->toObject()->set(*property, value, true);
    }
}
//d = {}. d.foo = 1
ALWAYS_INLINE void setObjectPreComputedCaseOperation(ESValue* willBeObject, ESString* keyString, const ESValue& value
        , ESHiddenClassChain* cachedHiddenClassChain, size_t* cachedHiddenClassIndex, ESHiddenClass** hiddenClassWillBe)
{
    ASSERT(!ESVMInstance::currentInstance()->globalObject()->didSomeObjectDefineIndexedReadOnlyOrAccessorProperty());

    if(LIKELY(willBeObject->isESPointer())) {
        if(LIKELY(willBeObject->asESPointer()->isESObject())) {
            ESObject* obj = willBeObject->asESPointer()->asESObject();
            if(*cachedHiddenClassIndex != SIZE_MAX && (*cachedHiddenClassChain)[0] == obj->hiddenClass()) {
                ASSERT((*cachedHiddenClassChain).size() == 1);
                if(!obj->hiddenClass()->write(obj, obj, keyString, value))
                    throw ESValue(TypeError::create());
                return ;
            } else if(*hiddenClassWillBe) {
                size_t cSiz = cachedHiddenClassChain->size();
                bool miss = false;
                for(int i = 0; i < cSiz - 1; i ++) {
                    if((*cachedHiddenClassChain)[i] != obj->hiddenClass()) {
                        miss = true;
                        break;
                    } else {
                        ESValue o = obj->__proto__();
                        if(!o.isObject()) {
                            miss = true;
                            break;
                        }
                        obj = o.asESPointer()->asESObject();
                    }
                }
                if(!miss) {
                    if((*cachedHiddenClassChain)[cSiz - 1] == obj->hiddenClass()) {
                        //cache hit!
                        obj = willBeObject->asESPointer()->asESObject();
                        obj->m_hiddenClassData.push_back(value);
                        obj->m_hiddenClass = *hiddenClassWillBe;
                        return ;
                    }
                }
            }

            //cache miss
            *cachedHiddenClassIndex = SIZE_MAX;
            *hiddenClassWillBe = NULL;
            cachedHiddenClassChain->clear();

            obj = willBeObject->asESPointer()->asESObject();
            size_t idx = obj->hiddenClass()->findProperty(keyString);
            if(idx != SIZE_MAX) {
                //own property
                *cachedHiddenClassIndex = idx;
                cachedHiddenClassChain->push_back(obj->hiddenClass());

                if(!obj->hiddenClass()->write(obj, obj, idx, value))
                    throw ESValue(TypeError::create());
            } else {
                cachedHiddenClassChain->push_back(obj->hiddenClass());
                ESValue proto = obj->__proto__();
                while(proto.isObject()) {
                    obj = proto.asESPointer()->asESObject();
                    cachedHiddenClassChain->push_back(obj->hiddenClass());

                    if(obj->hiddenClass()->hasReadOnlyProperty()) {
                        size_t idx = obj->hiddenClass()->findProperty(keyString);
                        if(idx != SIZE_MAX) {
                            if(!obj->hiddenClass()->propertyInfo(idx).m_flags.m_isWritable) {
                                *cachedHiddenClassIndex = SIZE_MAX;
                                *hiddenClassWillBe = NULL;
                                cachedHiddenClassChain->clear();
                                throw ESValue(TypeError::create());
                            }
                        }
                    }
                    proto = obj->__proto__();
                }

                ASSERT(!willBeObject->asESPointer()->asESObject()->hasOwnProperty(keyString));
                ESHiddenClass* before = willBeObject->asESPointer()->asESObject()->hiddenClass();
                willBeObject->asESPointer()->asESObject()->defineDataProperty(keyString, true, true, true, value);

                //only cache vector mode object.
                if(willBeObject->asESPointer()->asESObject()->hiddenClass() != before)
                    *hiddenClassWillBe = willBeObject->asESPointer()->asESObject()->hiddenClass();
            }
        } else {
            willBeObject->toObject()->set(keyString, value, true);
        }
    } else {
        willBeObject->toObject()->set(keyString, value, true);
    }
}

NEVER_INLINE ESValue getObjectOperationSlowMode(ESValue* willBeObject, ESValue* property, ESValue* lastObjectValueMetInMemberExpression, GlobalObject* globalObject);
NEVER_INLINE void setObjectOperationSlowMode(ESValue* willBeObject, ESValue* property, const ESValue& value);

NEVER_INLINE bool instanceOfOperation(ESValue* lval, ESValue* rval);
NEVER_INLINE ESValue typeOfOperation(ESValue* v);
NEVER_INLINE ESValue newOperation(ESVMInstance* instance, GlobalObject* globalObject, ESValue fn, ESValue* arguments, size_t argc);
NEVER_INLINE bool inOperation(ESValue* obj, ESValue* key);
NEVER_INLINE void tryOperation(ESVMInstance* instance, CodeBlock* codeBlock, char* codeBuffer, ExecutionContext* ec, size_t programCounter, Try* code);
NEVER_INLINE EnumerateObjectData* executeEnumerateObject(ESObject* obj);

}
#endif
