#ifndef ESJITOperations_h
#define ESJITOperations_h

#include "runtime/ESValue.h"
#include "bytecode/ByteCodeOperations.h"

namespace escargot {

inline ESValueInDouble workaroundForSavingDouble(double d)
{
    return ESValue::toRawDouble(ESValue(d));
}

inline ESValueInDouble getByGlobalIndexOp(GlobalObject* globalObject, GetByGlobalIndex* code)
{
    return ESValue::toRawDouble(getByGlobalIndexOperation(globalObject, code));
}

inline void setByGlobalIndexOp(GlobalObject* globalObject, SetByGlobalIndex* code, ESValueInDouble v)
{
    setByGlobalIndexOperation(globalObject, code, ESValue::fromRawDouble(v));
}

inline ESValueInDouble getByIndexWithActivationOp(ExecutionContext* ec, int32_t upCount, int32_t index)
{
    LexicalEnvironment* env = ec->environment();
    for (int i = 0; i < upCount; i ++) {
        env = env->outerEnvironment();
    }
    return ESValue::toRawDouble(*env->record()->toDeclarativeEnvironmentRecord()->bindingValueForHeapAllocatedData((unsigned)index));
}

inline void setByIndexWithActivationOp(ExecutionContext* ec, int32_t upCount, int32_t index, ESValueInDouble val)
{
    LexicalEnvironment* env = ec->environment();
    for (int i = 0; i < upCount; i ++) {
        env = env->outerEnvironment();
    }
    *env->record()->toDeclarativeEnvironmentRecord()->bindingValueForHeapAllocatedData((unsigned)index) = ESValue::fromRawDouble(val);
}

inline ESValueInDouble getByIdWithoutExceptionOp(ESVMInstance* instance, ExecutionContext* ec, ByteCode* bytecode)
{
    GetById* code = (GetById*)bytecode;
    std::jmp_buf tryPosition;
    if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
        ESValue* res = getByIdOperation(instance, ec, code);
        instance->unregisterTryPos(&tryPosition);
        return ESValue::toRawDouble(*res);
    } else {
        return ESValue::toRawDouble(ESValue());
    }
}

inline ESValueInDouble plusOp(ESValueInDouble left, ESValueInDouble right)
{
    ESValue leftVal = ESValue::fromRawDouble(left);
    ESValue rightVal = ESValue::fromRawDouble(right);
    ESValueInDouble ret = ESValue::toRawDouble(plusOperation(&leftVal, &rightVal));
    // printf("plusop %lx = %lx + %lx\n", bitwise_cast<uint64_t>(ret), bitwise_cast<uint64_t>(left), bitwise_cast<uint64_t>(right));
    return ret;
}

inline ESValueInDouble minusOp(ESValueInDouble left, ESValueInDouble right)
{
    ESValue leftVal = ESValue::fromRawDouble(left);
    ESValue rightVal = ESValue::fromRawDouble(right);
    ESValueInDouble ret = ESValue::toRawDouble(minusOperation(&leftVal, &rightVal));
    // printf("plusop %lx = %lx + %lx\n", bitwise_cast<uint64_t>(ret), bitwise_cast<uint64_t>(left), bitwise_cast<uint64_t>(right));
    return ret;
}

inline ESValueInDouble bitwiseOrOp(ESValueInDouble leftInDouble, ESValueInDouble rightInDouble)
{
    ESValue left = ESValue::fromRawDouble(leftInDouble);
    ESValue right = ESValue::fromRawDouble(rightInDouble);
    return ESValue::toRawDouble(ESValue(left.toInt32() | right.toInt32()));
}

inline ESValueInDouble bitwiseXorOp(ESValueInDouble leftInDouble, ESValueInDouble rightInDouble)
{
    ESValue left = ESValue::fromRawDouble(leftInDouble);
    ESValue right = ESValue::fromRawDouble(rightInDouble);
    return ESValue::toRawDouble(ESValue(left.toInt32() ^ right.toInt32()));
}

inline ESValueInDouble bitwiseNotOp(ESValueInDouble valueInDouble)
{
    ESValue value = ESValue::fromRawDouble(valueInDouble);
    return ESValue::toRawDouble(ESValue(~value.toInt32()));
}

inline ESValueInDouble logicalNotOp(ESValueInDouble valueInDouble)
{
    ESValue value = ESValue::fromRawDouble(valueInDouble);
    return ESValue::toRawDouble(ESValue(!value.toBoolean()));
}

inline ESValueInDouble leftShiftOp(ESValueInDouble leftInDouble, ESValueInDouble rightInDouble)
{
    ESValue left = ESValue::fromRawDouble(leftInDouble);
    ESValue right = ESValue::fromRawDouble(rightInDouble);
    int32_t lnum = left.toInt32();
    int32_t rnum = right.toInt32();
    lnum <<= ((unsigned int)rnum) & 0x1F;
    return ESValue::toRawDouble(ESValue(lnum));
}

inline ESValueInDouble signedRightShiftOp(ESValueInDouble leftInDouble, ESValueInDouble rightInDouble)
{
    ESValue left = ESValue::fromRawDouble(leftInDouble);
    ESValue right = ESValue::fromRawDouble(rightInDouble);
    int32_t lnum = left.toInt32();
    int32_t rnum = right.toInt32();
    lnum >>= ((unsigned int)rnum) & 0x1F;
    return ESValue::toRawDouble(ESValue(lnum));
}

inline ESValueInDouble unsignedRightShiftOp(ESValueInDouble leftInDouble, ESValueInDouble rightInDouble)
{
    ESValue left = ESValue::fromRawDouble(leftInDouble);
    ESValue right = ESValue::fromRawDouble(rightInDouble);
    int32_t lnum = left.toInt32();
    int32_t rnum = right.toInt32();
    lnum = (lnum) >> ((rnum) & 0x1F);
    return ESValue::toRawDouble(ESValue(lnum));
}

inline ESValueInDouble typeOfOp(ESValueInDouble valueInDouble)
{
    ESValue value = ESValue::fromRawDouble(valueInDouble);
    return ESValue::toRawDouble(typeOfOperation(&value));
}

inline ESValueInDouble getObjectOp(ESValueInDouble willBeObject, ESValueInDouble property, GlobalObject* globalObject)
{
    ESValue obj = ESValue::fromRawDouble(willBeObject);
    ESValue p = ESValue::fromRawDouble(property);
    return ESValue::toRawDouble(getObjectOperation(&obj, &p, globalObject));
}

inline ESValueInDouble getObjectPreComputedCaseOp(ESValueInDouble willBeObject, GlobalObject* globalObject, GetObjectPreComputedCase* bytecode)
{
    ESValue obj = ESValue::fromRawDouble(willBeObject);
    return ESValue::toRawDouble(getObjectPreComputedCaseOperation(&obj, bytecode->m_propertyValue, globalObject,
        &bytecode->m_inlineCache));
}

inline ESValueInDouble getObjectPreComputedCaseOpLastPart(ESObject* protoObj, ESObject* orgObj, int idx)
{
    return ESValue::toRawDouble(protoObj->hiddenClass()->read(protoObj, orgObj, (size_t)idx));
}

inline void setObjectOp(ESValueInDouble willBeObject, ESValueInDouble property, ESValueInDouble value)
{
    ESValue obj = ESValue::fromRawDouble(willBeObject);
    ESValue p = ESValue::fromRawDouble(property);
    setObjectOperation(&obj, &p, ESValue::fromRawDouble(value));
}

inline void setObjectPreComputedOp(ESValueInDouble willBeObject, GlobalObject* globalObject, SetObjectPreComputedCase* bytecode, ESValueInDouble value)
{
    ESValue obj = ESValue::fromRawDouble(willBeObject);
    ESValue v = ESValue::fromRawDouble(value);
    setObjectPreComputedCaseOperation(&obj, bytecode->m_propertyValue, v,
        &bytecode->m_cachedhiddenClassChain, &bytecode->m_cachedIndex, &bytecode->m_hiddenClassWillBe);
}

inline ESValue* contextResolveBinding(ExecutionContext* context, ByteCode* currentCode)
{
    GetById *code = (GetById*)currentCode;
    return context->resolveBinding(code->m_name);
}

inline ESValue* setVarContextResolveBinding(ExecutionContext* ec, ByteCode* currentCode)
{
    SetById* code = (SetById*)currentCode;
    return ec->resolveBinding(code->m_name);
}

inline ESValueInDouble contextResolveThisBinding(ExecutionContext* ec)
{
    ESValue thisValue = ec->resolveThisBinding();
    return ESValue::toRawDouble(thisValue);
}

inline void setVarDefineDataProperty(ExecutionContext* ec, GlobalObject* globalObj, ByteCode* currentCode, ESValueInDouble rawValue)
{
    SetById* code = (SetById*)currentCode;
    ESValue value = ESValue::fromRawDouble(rawValue);

    if (!ec->isStrictMode()) {
        globalObj->defineDataProperty(code->m_name.string(), true, true, true, value);
    } else {
        UTF16String err_msg;
        err_msg.append(u"assignment to undeclared variable ");
        err_msg.append(code->m_name.string()->toUTF16String());
        ESVMInstance::currentInstance()->throwError(ESValue(ReferenceError::create(ESString::create(std::move(err_msg)))));
    }
}

inline void objectDefineDataProperty(ESObject* object, ESString* key,
    /*bool isWritable, bool isEnumarable, bool isConfigurable, */
    ESValueInDouble initial)
{
    ESValue initialVal = ESValue::fromRawDouble(initial);
    object->defineDataProperty(key, /*isWritable, isEnumarable, isConfigurable, */
        true, true, true, initialVal);
}

inline ESValueInDouble esFunctionObjectCall(ESVMInstance* instance,
    ESValueInDouble callee, ESValue* arguments, uint32_t argumentCount, int32_t isNewExpression)
{
    ESValue calleeVal = ESValue::fromRawDouble(callee);
    ESValue ret = ESFunctionObject::call(instance, calleeVal,
        ESValue(), arguments, argumentCount, isNewExpression);
    return ESValue::toRawDouble(ret);
}

inline ESValueInDouble esFunctionObjectCallWithReceiver(ESVMInstance* instance,
    ESValueInDouble callee, ESValueInDouble receiver, ESValue* arguments, uint32_t argumentCount, int32_t isNewExpression)
{
    ESValue calleeVal = ESValue::fromRawDouble(callee);
    ESValue receiverVal = ESValue::fromRawDouble(receiver);
    ESValue ret = ESFunctionObject::call(instance, calleeVal,
        receiverVal, arguments, argumentCount, isNewExpression);
    return ESValue::toRawDouble(ret);
}

inline ESValueInDouble evalCall(ESVMInstance* instance, ExecutionContext* ec, size_t argc, ESValue* arguments)
{
    ESValue callee = *ec->resolveBinding(strings->eval);
    if (callee.isESPointer() && (void *)callee.asESPointer() == (void *)instance->globalObject()->eval()) {
        ESValue ret = instance->runOnEvalContext([instance, &arguments, &argc]() {
            ESValue ret;
            if (argc)
                ret = instance->evaluate(arguments[0].asESString(), false);
            return ret;
        }, true);
        return ESValue::toRawDouble(ret);
    } else {
        ESObject* receiver = instance->globalObject();
        return ESValue::toRawDouble(ESFunctionObject::call(instance, callee, receiver, arguments, argc, false));
    }
}

inline ESString* generateToString(ESValueInDouble rawValue)
{
    ESValue value = ESValue::fromRawDouble(rawValue);
    return value.toStringSlowCase();
}

inline ESString* concatTwoStrings(ESString* left, ESString* right)
{
    return ESString::concatTwoStrings(left, right);
}

NEVER_INLINE ESValue newOperation(ESVMInstance* instance, GlobalObject* globalObject, ESValue fn, ESValue* arguments, size_t argc);
inline ESValueInDouble newOp(ESVMInstance* instance, GlobalObject* globalObject, ESValueInDouble fn, ESValue* arguments, size_t argc)
{
    ESValue fnVal = ESValue::fromRawDouble(fn);
    return ESValue::toRawDouble(newOperation(instance, globalObject, fnVal, arguments, argc));
}

// inline bool equalOp(ESValueInDouble left, ESValueInDouble right)
// {
// ESValue leftVal = ESValue::fromRawDouble(left);
// ESValue rightVal = ESValue::fromRawDouble(right);
// bool ret = leftVal.abstractEqualsTo(rightVal);
// return ret;
// }

// inline bool lessThanOp(ESValueInDouble left, ESValueInDouble right)
// {
// ESValue leftVal = ESValue::fromRawDouble(left);
// ESValue rightVal = ESValue::fromRawDouble(right);
// ESValue ret = abstractRelationalComparison(leftVal, rightVal, true);
// if (ret.isUndefined()) return false;
// return ret.asBoolean();
// }

inline ESValueInDouble createObject(int keyCount)
{
    ESObject* obj = ESObject::create((size_t)keyCount + 1);
    return ESValue::toRawDouble(obj);
}

inline ESValueInDouble createArr(int keyCount)
{
    ESArrayObject* arrObj = ESArrayObject::create(keyCount);
    return ESValue::toRawDouble(arrObj);
}

inline void initObject(ESValueInDouble objectIndouble, ESValueInDouble keyInDouble, ESValueInDouble valueInDouble)
{
    ESObject* object = ESValue::fromRawDouble(objectIndouble).asESPointer()->asESObject();
    ESValue key = ESValue::fromRawDouble(keyInDouble);
    ESValue value = ESValue::fromRawDouble(valueInDouble);
    object->set(key, value);
}

inline ESValueInDouble createFunction(ExecutionContext* ec, ByteCode* bytecode)
{
    CreateFunction* code = (CreateFunction*)bytecode;
    ASSERT(((size_t)code->m_codeBlock % sizeof(size_t)) == 0);
    ESFunctionObject* function = ESFunctionObject::create(ec->environment(), code->m_codeBlock, code->m_nonAtomicName == NULL ? strings->emptyString.string() : code->m_nonAtomicName, code->m_codeBlock->m_params.size());
    function->set(strings->name.string(), code->m_nonAtomicName);
    if (code->m_isDeclaration) {
        if (code->m_idIndex == std::numeric_limits<size_t>::max()) {
            ec->environment()->record()->setMutableBinding(code->m_name, function, false);
        } else {
            if (ec->environment()->record()->toDeclarativeEnvironmentRecord()->useHeapAllocatedStorage()) {
                *ec->environment()->record()->toDeclarativeEnvironmentRecord()->bindingValueForHeapAllocatedData(code->m_idIndex) = function;
            } else {
                *ec->environment()->record()->toDeclarativeEnvironmentRecord()->bindingValueForStackAllocatedData(code->m_idIndex) = function;
            }
        }
    }
    return ESValue::toRawDouble(ESValue((ESPointer*)function));
}

inline ESValueInDouble equalOp(ESValueInDouble left, ESValueInDouble right)
{
    ESValue leftVal = ESValue::fromRawDouble(left);
    ESValue rightVal = ESValue::fromRawDouble(right);
    bool ret = leftVal.abstractEqualsTo(rightVal);
    return ESValue::toRawDouble(ESValue(ret));
}

inline ESValueInDouble strictEqualOp(ESValueInDouble left, ESValueInDouble right)
{
    ESValue leftVal = ESValue::fromRawDouble(left);
    ESValue rightVal = ESValue::fromRawDouble(right);
    bool ret = leftVal.equalsTo(rightVal);
    return ESValue::toRawDouble(ESValue(ret));
}

inline ESValueInDouble lessThanOp(ESValueInDouble left, ESValueInDouble right)
{
    ESValue leftVal = ESValue::fromRawDouble(left);
    ESValue rightVal = ESValue::fromRawDouble(right);
    ESValue ret = ESValue(abstractRelationalComparison(&leftVal, &rightVal, true));
    if (ret.isUndefined())
        return false;
    return ESValue::toRawDouble(ret);
}

inline void throwOp(ESValueInDouble err)
{
    ESValue error = ESValue::fromRawDouble(err);
    ESVMInstance::currentInstance()->throwError(error);
}

inline ESPointer* getEnumerablObjectData(ESValueInDouble value)
{
    ESObject* obj = ESValue::fromRawDouble(value).toObject();
    return (ESPointer *)executeEnumerateObject(obj);
}

inline int keySize(EnumerateObjectData* data)
{
    return data->m_keys.size();
}

inline ESValueInDouble getEnumerationKey(EnumerateObjectData* data)
{
    ESValue value = data->m_keys[data->m_idx - 1];
    return ESValue::toRawDouble(value);
}

inline bool toBoolean(ESValueInDouble value)
{
    ESValue valueToBoolean = ESValue::fromRawDouble(value);
    return valueToBoolean.toBoolean();
}

#if 0
ALWAYS_INLINE ESValueInDouble resolveNonDataProperty(ESObject* object, ESPointer* hiddenClassIdxData)
{
    // printf("[resolveNonDataProperty] (void*)object : %p\n", (void*)object);
    // printf("[resolveNonDataProperty] (void*)hiddenClassIdxData : %p\n", (void*)hiddenClassIdxData);
    return ESValue::toRawDouble(((ESAccessorData *)hiddenClassIdxData)->value(object));
}
#else
ALWAYS_INLINE ESValueInDouble resolveNonDataProperty(ESObject* object, size_t idx)
{
    // printf("[resolveNonDataProperty] (void*)object : %p\n", (void*)object);
#ifdef EJJEONG_MERGING
    return ESValue::toRawDouble(object->readHiddenClass(idx));
#else
    return 0;
#endif
}
#endif

#ifndef NDEBUG
inline void jitLogIntOperation(int arg, const char* msg)
{
    if (!ESVMInstance::currentInstance()->m_reportCompiledFunction) {
        printf("[JIT_LOG] %s : int 0x%x (%d)\n", msg, bitwise_cast<unsigned>(arg), arg);
        fflush(stdout);
    }
}
inline void jitLogDoubleOperation(double arg, const char* msg)
{
    if (!ESVMInstance::currentInstance()->m_reportCompiledFunction) {
        printf("[JIT_LOG] %s : double 0x%jx(%lf)\n", msg, bitwise_cast<uint64_t>(arg), arg);
        fflush(stdout);
    }
}
inline void jitLogPointerOperation(void* arg, const char* msg)
{
    if (!ESVMInstance::currentInstance()->m_reportCompiledFunction) {
        printf("[JIT_LOG] %s : pointer 0x%jx\n", msg, bitwise_cast<uint64_t>(arg));
        fflush(stdout);
    }
}
inline void jitLogStringOperation(const char* arg, const char* msg)
{
    if (!ESVMInstance::currentInstance()->m_reportCompiledFunction) {
        printf("[JIT_LOG] %s : string %s\n", msg, arg);
        fflush(stdout);
    }
}
#endif


}

#endif
