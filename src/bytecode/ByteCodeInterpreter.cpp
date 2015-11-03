#include "Escargot.h"
#include "bytecode/ByteCode.h"

#include "ByteCodeOperations.h"

namespace escargot {

ESValue interpret(ESVMInstance* instance, CodeBlock* codeBlock, size_t programCounter, unsigned maxStackPos)
{
    if (codeBlock == NULL) {
#define REGISTER_TABLE(opcode, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
        instance->opcodeTable()->m_table[opcode##Opcode] = &&opcode##OpcodeLbl;
        FOR_EACH_BYTECODE_OP(REGISTER_TABLE);
        return ESValue();
    }

#define NEXT_INSTRUCTION() \
            goto NextInstruction

    ExecutionContext* ec = instance->currentExecutionContext();

    unsigned stackSiz = codeBlock->m_requiredStackSizeInESValueSize * sizeof(ESValue);
    char* stackBuf;
    void* bp;
    void* stack;
    if (maxStackPos == 0) {
        stackBuf = (char *)alloca(stackSiz);
        bp = stackBuf;
        stack = bp;
    } else {
#ifdef ENABLE_ESJIT
        size_t offset = maxStackPos*sizeof(ESValue);
        stackBuf = ec->getBp();
        bp = stackBuf;
        stack = (void*)(((size_t)bp) + offset);
#endif
    }

    void* topOfStack = stackBuf + stackSiz;
    char tmpStackBuf[32];
    void* tmpStack = tmpStackBuf;
    void* tmpBp = tmpStack;
    void* tmpTopOfStack = tmpStackBuf + 32;
    char* codeBuffer = codeBlock->m_code.data();
#ifdef ENABLE_ESJIT
    size_t numParams = codeBlock->m_params.size();
#endif
    GlobalObject* globalObject = instance->globalObject();
    ESValue* lastExpressionStatementValue = &instance->m_lastExpressionStatementValue;
    ESValue* nonActivitionModeLocalValuePointer = ec->cachedDeclarativeEnvironmentRecordESValue();
    ESValue thisValue(ESValue::ESEmptyValue);
    ASSERT(((size_t)stack % sizeof(size_t)) == 0);
    ASSERT(((size_t)tmpStack % sizeof(size_t)) == 0);
    // resolve programCounter into address
    programCounter = (size_t)(&codeBuffer[programCounter]);
    ByteCode* currentCode;

    NextInstruction:
    currentCode = (ByteCode *)programCounter;
    ASSERT(((size_t)currentCode % sizeof(size_t)) == 0);

#ifndef NDEBUG
    if (instance->m_dumpExecuteByteCode) {
        size_t tt = (size_t)currentCode;
        ASSERT(tt % sizeof(size_t) == 0);
        if (currentCode->m_node)
            printf("execute %p %u \t(nodeinfo %d)\t",currentCode, (unsigned)(programCounter-(size_t)codeBuffer), (int)currentCode->m_node->sourceLocation().m_lineNumber);
        else
            printf("execute %p %u \t(nodeinfo null)\t",currentCode, (unsigned)(programCounter-(size_t)codeBuffer));
        currentCode->dump();
        fflush(stdout);
    }

    if (currentCode->m_orgOpcode < 0 || currentCode->m_orgOpcode > OpcodeKindEnd) {
        printf("Error: unknown opcode\n");
        RELEASE_ASSERT_NOT_REACHED();
    } else {


    }
#endif
    goto *(currentCode->m_opcodeInAddress);

    PushOpcodeLbl:
    {
        Push* pushCode = (Push*)currentCode;
        push<ESValue>(stack, topOfStack, pushCode->m_value);
        executeNextCode<Push>(programCounter);
        NEXT_INSTRUCTION();
    }

    PopOpcodeLbl:
    {
        Pop* popCode = (Pop*)currentCode;
        pop<ESValue>(stack, bp);
        executeNextCode<Pop>(programCounter);
        NEXT_INSTRUCTION();
    }

    DuplicateTopOfStackValueOpcodeLbl:
    {
        push<ESValue>(stack, topOfStack, peek<ESValue>(stack, bp));
        executeNextCode<DuplicateTopOfStackValue>(programCounter);
        NEXT_INSTRUCTION();
    }

    PopExpressionStatementOpcodeLbl:
    {
        ESValue* t = pop<ESValue>(stack, bp);
        *lastExpressionStatementValue = *t;
        executeNextCode<PopExpressionStatement>(programCounter);
        NEXT_INSTRUCTION();
    }

    PushIntoTempStackOpcodeLbl:
    {
        push<ESValue>(tmpStack, tmpTopOfStack, pop<ESValue>(stack, bp));
        executeNextCode<PushIntoTempStack>(programCounter);
        NEXT_INSTRUCTION();
    }

    PopFromTempStackOpcodeLbl:
    {
        push<ESValue>(stack, topOfStack, pop<ESValue>(tmpStack, tmpBp));
        executeNextCode<PopFromTempStack>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetByIdOpcodeLbl:
    {
        GetById* code = (GetById*)currentCode;
        ESValue* value = getByIdOperation(instance, ec, code);
        push<ESValue>(stack, topOfStack, value);
#ifdef ENABLE_ESJIT
        code->m_profile.addProfile(*value);
#endif
        executeNextCode<GetById>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetByIndexOpcodeLbl:
    {
        GetByIndex* code = (GetByIndex*)currentCode;
        ASSERT(code->m_index < ec->environment()->record()->toDeclarativeEnvironmentRecord()->innerIdentifiers()->size());
#ifndef ENABLE_ESJIT
        push<ESValue>(stack, topOfStack, &nonActivitionModeLocalValuePointer[code->m_index]);
#else
        ESValue v = nonActivitionModeLocalValuePointer[code->m_index];
        push<ESValue>(stack, topOfStack, v);
        code->m_profile.addProfile(v);
#endif
        executeNextCode<GetByIndex>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetByIndexWithActivationOpcodeLbl:
    {
        GetByIndexWithActivation* code = (GetByIndexWithActivation*)currentCode;
        LexicalEnvironment* env = ec->environment();
        for (unsigned i = 0; i < code->m_upIndex; i ++) {
            env = env->outerEnvironment();
        }
        ASSERT(env->record()->isDeclarativeEnvironmentRecord());

#ifndef ENABLE_ESJIT
        push<ESValue>(stack, topOfStack, env->record()->toDeclarativeEnvironmentRecord()->bindingValueForActivationMode(code->m_index));
#else
        ESValue v = *env->record()->toDeclarativeEnvironmentRecord()->bindingValueForActivationMode(code->m_index);
        push<ESValue>(stack, topOfStack, v);
        code->m_profile.addProfile(v);
#endif
        executeNextCode<GetByIndexWithActivation>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetByGlobalIndexOpcodeLbl:
    {
        GetByGlobalIndex* code = (GetByGlobalIndex*)currentCode;
#ifndef ENABLE_ESJIT
        push<ESValue>(stack, topOfStack, getByGlobalIndexOperation(globalObject, code));
#else
        ESValue value = getByGlobalIndexOperation(globalObject, code);
        push<ESValue>(stack, topOfStack, value);
        code->m_profile.addProfile(value);
#endif
        executeNextCode<GetByGlobalIndex>(programCounter);
        NEXT_INSTRUCTION();
    }

    SetByIdOpcodeLbl:
    {
        SetById* code = (SetById*)currentCode;
        ESValue* value = peek<ESValue>(stack, bp);

        if (LIKELY(code->m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
            ASSERT(ec->resolveBinding(code->m_name) == code->m_cachedSlot);
            *code->m_cachedSlot = *value;
        } else {
            ExecutionContext* ec = instance->currentExecutionContext();
            // TODO
            // Object.defineProperty(this,"asdf",{value:1}) //this == global
            // asdf = 2
            ESValue* slot = ec->resolveBinding(code->m_name);

            if (LIKELY(slot != NULL)) {
                code->m_cachedSlot = slot;
                code->m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
                *code->m_cachedSlot = *value;
            } else {
                if (!ec->isStrictMode()) {
                    globalObject->defineDataProperty(code->m_name.string(), true, true, true, *value);
                } else {
                    u16string err_msg;
                    err_msg.append(u"assignment to undeclared variable ");
                    err_msg.append(code->m_name.string()->data());
                    throw ESValue(ReferenceError::create(ESString::create(std::move(err_msg))));
                }
            }
        }
        executeNextCode<SetById>(programCounter);
        NEXT_INSTRUCTION();
    }

    SetByGlobalIndexOpcodeLbl:
    {
        SetByGlobalIndex* code = (SetByGlobalIndex*)currentCode;
        setByGlobalIndexOperation(globalObject, code, *peek<ESValue>(stack, bp));
        executeNextCode<SetByGlobalIndex>(programCounter);
        NEXT_INSTRUCTION();
    }

    SetByIndexOpcodeLbl:
    {
        SetByIndex* code = (SetByIndex*)currentCode;
        nonActivitionModeLocalValuePointer[code->m_index] = *peek<ESValue>(stack, bp);
        executeNextCode<SetByIndex>(programCounter);
        NEXT_INSTRUCTION();
    }

    SetByIndexWithActivationOpcodeLbl:
    {
        SetByIndexWithActivation* code = (SetByIndexWithActivation*)currentCode;
        LexicalEnvironment* env = ec->environment();
        for (unsigned i = 0; i < code->m_upIndex; i ++) {
            env = env->outerEnvironment();
        }
        ASSERT(env->record()->isDeclarativeEnvironmentRecord());
        *env->record()->toDeclarativeEnvironmentRecord()->bindingValueForActivationMode(code->m_index) = *peek<ESValue>(stack, bp);
        executeNextCode<SetByIndexWithActivation>(programCounter);
        NEXT_INSTRUCTION();
    }

    CreateBindingOpcodeLbl:
    {
        CreateBinding* code = (CreateBinding*)currentCode;
        ec->environment()->record()->createMutableBindingForAST(code->m_name, false);
        executeNextCode<CreateBinding>(programCounter);
        NEXT_INSTRUCTION();
    }

    EqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, ESValue(left->abstractEqualsTo(*right)));
        executeNextCode<Equal>(programCounter);
        NEXT_INSTRUCTION();
    }

    NotEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, ESValue(!left->abstractEqualsTo(*right)));
        executeNextCode<NotEqual>(programCounter);
        NEXT_INSTRUCTION();
    }

    StrictEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, ESValue(left->equalsTo(*right)));
        executeNextCode<StrictEqual>(programCounter);
        NEXT_INSTRUCTION();
    }

    NotStrictEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, ESValue(!left->equalsTo(*right)));
        executeNextCode<NotStrictEqual>(programCounter);
        NEXT_INSTRUCTION();
    }

    BitwiseAndOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, ESValue(left->toInt32() & right->toInt32()));
        executeNextCode<BitwiseAnd>(programCounter);
        NEXT_INSTRUCTION();
    }

    BitwiseOrOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, ESValue(left->toInt32() | right->toInt32()));
        executeNextCode<BitwiseOr>(programCounter);
        NEXT_INSTRUCTION();
    }

    BitwiseXorOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, ESValue(left->toInt32() ^ right->toInt32()));
        executeNextCode<BitwiseXor>(programCounter);
        NEXT_INSTRUCTION();
    }

    LeftShiftOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        int32_t lnum = left->toInt32();
        int32_t rnum = right->toInt32();
        lnum <<= ((unsigned int)rnum) & 0x1F;
        push<ESValue>(stack, topOfStack, ESValue(lnum));
        executeNextCode<LeftShift>(programCounter);
        NEXT_INSTRUCTION();
    }

    SignedRightShiftOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        int32_t lnum = left->toInt32();
        int32_t rnum = right->toInt32();
        lnum >>= ((unsigned int)rnum) & 0x1F;
        push<ESValue>(stack, topOfStack, ESValue(lnum));
        executeNextCode<SignedRightShift>(programCounter);
        NEXT_INSTRUCTION();
    }

    UnsignedRightShiftOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        uint32_t lnum = left->toUint32();
        uint32_t rnum = right->toUint32();
        lnum = (lnum) >> ((rnum) & 0x1F);
        push<ESValue>(stack, topOfStack, ESValue(lnum));
        executeNextCode<UnsignedRightShift>(programCounter);
        NEXT_INSTRUCTION();
    }

    LessThanOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        ESValue r = abstractRelationalComparison(*left, *right, true);
        if (r.isUndefined())
            push<ESValue>(stack, topOfStack, ESValue(false));
        else
            push<ESValue>(stack, topOfStack, r);
        executeNextCode<LessThan>(programCounter);
        NEXT_INSTRUCTION();
    }

    LessThanOrEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        ESValue r = abstractRelationalComparison(*right, *left, false);
        if (r == ESValue(true) || r.isUndefined())
            push<ESValue>(stack, topOfStack, ESValue(false));
        else
            push<ESValue>(stack, topOfStack, ESValue(true));
        executeNextCode<LessThanOrEqual>(programCounter);
        NEXT_INSTRUCTION();
    }

    GreaterThanOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        ESValue r = abstractRelationalComparison(*right, *left, false);
        if (r.isUndefined())
            push<ESValue>(stack, topOfStack, ESValue(false));
        else
            push<ESValue>(stack, topOfStack, r);
        executeNextCode<GreaterThan>(programCounter);
        NEXT_INSTRUCTION();
    }

    GreaterThanOrEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        ESValue r = abstractRelationalComparison(*left, *right, true);
        if (r == ESValue(true) || r.isUndefined())
            push<ESValue>(stack, topOfStack, ESValue(false));
        else
            push<ESValue>(stack, topOfStack, ESValue(true));
        executeNextCode<GreaterThanOrEqual>(programCounter);
        NEXT_INSTRUCTION();
    }

    PlusOpcodeLbl:
    {
        ESValue right = *pop<ESValue>(stack, bp);
        ESValue left = *pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, plusOperation(left, right));
        executeNextCode<Plus>(programCounter);
        NEXT_INSTRUCTION();
    }

    MinusOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, minusOperation(*left, *right));
        executeNextCode<Minus>(programCounter);
        NEXT_INSTRUCTION();
    }

    MultiplyOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, ESValue(left->toNumber() * right->toNumber()));
        executeNextCode<Multiply>(programCounter);
        NEXT_INSTRUCTION();
    }

    DivisionOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, ESValue(left->toNumber() / right->toNumber()));
        executeNextCode<Division>(programCounter);
        NEXT_INSTRUCTION();
    }

    ModOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, modOperation(*left, *right));
        executeNextCode<Mod>(programCounter);
        NEXT_INSTRUCTION();
    }

    IncrementOpcodeLbl:
    {
        ESValue* src = pop<ESValue>(stack, bp);
        ESValue ret(ESValue::ESForceUninitialized);
        if (src->isInt32()) {
            int32_t a = src->asInt32();
            if (a == std::numeric_limits<int32_t>::max())
                ret = ESValue(ESValue::EncodeAsDouble, ((double)a) + 1);
            else
                ret = ESValue(a + 1);
        } else {
            ret = ESValue(ESValue::EncodeAsDouble, src->asNumber() + 1);
        }
        push<ESValue>(stack, topOfStack, ret);
        executeNextCode<Increment>(programCounter);
        NEXT_INSTRUCTION();
    }

    DecrementOpcodeLbl:
    {
        ESValue* src = pop<ESValue>(stack, bp);
        ESValue ret(ESValue::ESForceUninitialized);
        if (src->isInt32()) {
            int32_t a = src->asInt32();
            if (a == std::numeric_limits<int32_t>::min())
                ret = ESValue(ESValue::EncodeAsDouble, ((double)a) - 1);
            else
                ret = ESValue(a - 1);
        } else {
            ret = ESValue(ESValue::EncodeAsDouble, src->asNumber() - 1);
        }
        push<ESValue>(stack, topOfStack, ret);
        executeNextCode<Decrement>(programCounter);
        NEXT_INSTRUCTION();
    }

    BitwiseNotOpcodeLbl:
    {
        push<ESValue>(stack, topOfStack, ESValue(~pop<ESValue>(stack, bp)->toInt32()));
        executeNextCode<BitwiseNot>(programCounter);
        NEXT_INSTRUCTION();
    }

    LogicalNotOpcodeLbl:
    {
        push<ESValue>(stack, topOfStack, ESValue(!pop<ESValue>(stack, bp)->toBoolean()));
        executeNextCode<LogicalNot>(programCounter);
        NEXT_INSTRUCTION();
    }

    UnaryMinusOpcodeLbl:
    {
        push<ESValue>(stack, topOfStack, ESValue(-pop<ESValue>(stack, bp)->toNumber()));
        executeNextCode<UnaryMinus>(programCounter);
        NEXT_INSTRUCTION();
    }

    UnaryPlusOpcodeLbl:
    {
        push<ESValue>(stack, topOfStack, ESValue(pop<ESValue>(stack, bp)->toNumber()));
        executeNextCode<UnaryPlus>(programCounter);
        NEXT_INSTRUCTION();
    }

    ToNumberOpcodeLbl:
    {
        ESValue* v = peek<ESValue>(stack, bp);
        if (!v->isNumber()) {
            v = pop<ESValue>(stack, bp);
            push<ESValue>(stack, topOfStack, ESValue(v->toNumber()));
        }
        executeNextCode<ToNumber>(programCounter);
        NEXT_INSTRUCTION();
    }

    ThisOpcodeLbl:
    {
        if (UNLIKELY(thisValue.isEmpty())) {
            thisValue = ec->resolveThisBinding();
        }
#ifdef ENABLE_ESJIT
        This* code = (This*)currentCode;
        code->m_profile.addProfile(thisValue);
#endif
        push<ESValue>(stack, topOfStack, thisValue);
        executeNextCode<This>(programCounter);
        NEXT_INSTRUCTION();
    }

    ReturnFunctionOpcodeLbl:
    {
        ASSERT(bp == stack);
        return ESValue();
    }

    ReturnFunctionWithValueOpcodeLbl:
    {
        ESValue* ret = pop<ESValue>(stack, bp);
        ASSERT(bp == stack);
        return *ret;
    }

    CreateObjectOpcodeLbl:
    {
        CreateObject* code = (CreateObject*)currentCode;
        ESObject* obj = ESObject::create(code->m_keyCount + 1);
        push<ESValue>(stack, topOfStack, obj);
        executeNextCode<CreateObject>(programCounter);
        NEXT_INSTRUCTION();
    }

    CreateArrayOpcodeLbl:
    {
        CreateArray* code = (CreateArray*)currentCode;
        ESArrayObject* arr = ESArrayObject::create(code->m_keyCount);
        push<ESValue>(stack, topOfStack, arr);
        executeNextCode<CreateArray>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetObjectOpcodeLbl:
    {
        GetObject* code = (GetObject*)currentCode;

        ESValue* property = pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        ESValue value = getObjectOperation(willBeObject, property, globalObject);
        push<ESValue>(stack, topOfStack, value);
#ifdef ENABLE_ESJIT
        code->m_profile.addProfile(value);
#endif
        executeNextCode<GetObject>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetObjectAndPushObjectOpcodeLbl:
    {
        GetObject* code = (GetObject*)currentCode;
        ESValue* property = pop<ESValue>(stack, bp);
        ESValue willBeObject = *pop<ESValue>(stack, bp);
        ESValue value = getObjectOperation(&willBeObject, property, globalObject);
        push<ESValue>(stack, topOfStack, value);
        push<ESValue>(stack, topOfStack, willBeObject);
#ifdef ENABLE_ESJIT
        code->m_profile.addProfile(value);
#endif
        executeNextCode<GetObjectAndPushObject>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetObjectPreComputedCaseOpcodeLbl:
    {
        GetObjectPreComputedCase* code = (GetObjectPreComputedCase*)currentCode;
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        ESValue value = getObjectPreComputedCaseOperation(willBeObject, code->m_propertyValue, globalObject,
            &code->m_cachedhiddenClassChain, &code->m_cachedIndex);
        push<ESValue>(stack, topOfStack, value);
#ifdef ENABLE_ESJIT
        code->m_profile.addProfile(value);
#endif
        executeNextCode<GetObjectPreComputedCase>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetObjectPreComputedCaseAndPushObjectOpcodeLbl:
    {
        GetObjectPreComputedCaseAndPushObject* code = (GetObjectPreComputedCaseAndPushObject*)currentCode;
        ESValue willBeObject = *pop<ESValue>(stack, bp);
        ESValue value = getObjectPreComputedCaseOperation(&willBeObject, code->m_propertyValue, globalObject,
            &code->m_cachedhiddenClassChain, &code->m_cachedIndex);
        push<ESValue>(stack, topOfStack, value);
        push<ESValue>(stack, topOfStack, willBeObject);
#ifdef ENABLE_ESJIT
        code->m_profile.addProfile(value);
#endif
        executeNextCode<GetObjectPreComputedCaseAndPushObject>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetObjectWithPeekingOpcodeLbl:
    {
        GetObjectWithPeeking* code = (GetObjectWithPeeking*)currentCode;
        ESValue* property = (ESValue *)((size_t)stack - sizeof(ESValue));
        ESValue* willBeObject = (ESValue *)((size_t)stack - sizeof(ESValue) * 2);
        ESValue value = getObjectOperation(willBeObject, property, globalObject);
        push<ESValue>(stack, topOfStack, value);
#ifdef ENABLE_ESJIT
        code->m_profile.addProfile(value);
#endif
        executeNextCode<GetObjectWithPeeking>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetObjectWithPeekingPreComputedCaseOpcodeLbl:
    {
        GetObjectWithPeekingPreComputedCase* code = (GetObjectWithPeekingPreComputedCase*)currentCode;
        ESValue* willBeObject = peek<ESValue>(stack, bp);
        ESValue value = getObjectPreComputedCaseOperationWithNeverInline(willBeObject, code->m_propertyValue, globalObject,
            &code->m_cachedhiddenClassChain, &code->m_cachedIndex);
        push<ESValue>(stack, topOfStack, value);
#ifdef ENABLE_ESJIT
        code->m_profile.addProfile(value);
#endif
        executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
        NEXT_INSTRUCTION();
    }

    SetObjectOpcodeLbl:
    {
        SetObject* code = (SetObject*)currentCode;
        ESValue value = *pop<ESValue>(stack, bp);
        ESValue* property = pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        setObjectOperation(willBeObject, property, value);
        push<ESValue>(stack, topOfStack, value);
        executeNextCode<SetObject>(programCounter);
        NEXT_INSTRUCTION();
    }

    SetObjectPreComputedCaseOpcodeLbl:
    {
        SetObjectPreComputedCase* code = (SetObjectPreComputedCase*)currentCode;
        ESValue value = *pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        setObjectPreComputedCaseOperation(willBeObject, code->m_propertyValue, value, &code->m_cachedhiddenClassChain
            , &code->m_cachedIndex, &code->m_hiddenClassWillBe);
        push<ESValue>(stack, topOfStack, value);
        executeNextCode<SetObjectPreComputedCase>(programCounter);
        NEXT_INSTRUCTION();
    }

    CreateFunctionOpcodeLbl:
    {
        CreateFunction* code = (CreateFunction*)currentCode;
        ASSERT(((size_t)code->m_codeBlock % sizeof(size_t)) == 0);
        ESFunctionObject* function = ESFunctionObject::create(ec->environment(), code->m_codeBlock, code->m_nonAtomicName == NULL ? strings->emptyString.string() : code->m_nonAtomicName, code->m_codeBlock->m_params.size());
        if (code->m_isDeclaration) { // FD
            function->set(strings->name.string(), code->m_nonAtomicName);
            ec->environment()->record()->setMutableBinding(code->m_name, function, false);
        }
        else { // FE
            function->set(strings->name.string(), code->m_nonAtomicName);
            push<ESValue>(stack, topOfStack, function);
        }
        executeNextCode<CreateFunction>(programCounter);
        NEXT_INSTRUCTION();
    }

    CallFunctionOpcodeLbl:
    {
        CallFunction* code = (CallFunction*)currentCode;
        const unsigned& argc = code->m_argmentCount;
        stack = (void *)((size_t)stack - argc * sizeof(ESValue));
        ESValue* arguments = (ESValue *)stack;
        ESValue result = ESFunctionObject::call(instance, *pop<ESValue>(stack, bp), ESValue(), arguments, argc, false);
#ifdef ENABLE_ESJIT
        code->m_profile.addProfile(result);
#endif
        push<ESValue>(stack, topOfStack, result);
        executeNextCode<CallFunction>(programCounter);
        NEXT_INSTRUCTION();
    }

    CallFunctionWithReceiverOpcodeLbl:
    {
        CallFunctionWithReceiver* code = (CallFunctionWithReceiver*)currentCode;
        const unsigned& argc = code->m_argmentCount;
        stack = (void *)((size_t)stack - argc * sizeof(ESValue));
        ESValue* arguments = (ESValue *)stack;
        ESValue* receiver = pop<ESValue>(stack, bp);
        ESValue result = ESFunctionObject::call(instance, *pop<ESValue>(stack, bp), *receiver, arguments, argc, false);
#ifdef ENABLE_ESJIT
        code->m_profile.addProfile(result);
#endif
        push<ESValue>(stack, topOfStack, result);
        executeNextCode<CallFunctionWithReceiver>(programCounter);
        NEXT_INSTRUCTION();
    }

    NewFunctionCallOpcodeLbl:
    {
        NewFunctionCall* code = (NewFunctionCall*)currentCode;
        const unsigned& argc = code->m_argmentCount;
        stack = (void *)((size_t)stack - argc * sizeof(ESValue));
        ESValue* arguments = (ESValue *)stack;
        ESValue fn = *pop<ESValue>(stack, bp);
        ESValue result = newOperation(instance, globalObject, fn, arguments, argc);
#ifdef ENABLE_ESJIT
        code->m_profile.addProfile(result);
#endif
        push<ESValue>(stack, topOfStack, result);
        executeNextCode<NewFunctionCall>(programCounter);
        NEXT_INSTRUCTION();
    }

    JumpOpcodeLbl:
    {
        Jump* code = (Jump *)currentCode;
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
        NEXT_INSTRUCTION();
    }

    JumpComplexCaseOpcodeLbl:
    {
        JumpComplexCase* code = (JumpComplexCase*)currentCode;
        ec->tryOrCatchBodyResult() = code->m_controlFlowRecord->clone();
        // TODO add check stack pointer;
        return ESValue(ESValue::ESEmptyValue);
    }

    JumpIfTopOfStackValueIsFalseOpcodeLbl:
    {
        JumpIfTopOfStackValueIsFalse* code = (JumpIfTopOfStackValueIsFalse *)currentCode;
        ESValue* top = pop<ESValue>(stack, bp);
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        if (!top->toBoolean())
            programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
        else
            executeNextCode<JumpIfTopOfStackValueIsFalse>(programCounter);
        NEXT_INSTRUCTION();
    }

    JumpIfTopOfStackValueIsTrueOpcodeLbl:
    {
        JumpIfTopOfStackValueIsTrue* code = (JumpIfTopOfStackValueIsTrue *)currentCode;
        ESValue* top = pop<ESValue>(stack, bp);
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        if (top->toBoolean())
            programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
        else
            executeNextCode<JumpIfTopOfStackValueIsTrue>(programCounter);
        NEXT_INSTRUCTION();
    }

    JumpAndPopIfTopOfStackValueIsTrueOpcodeLbl:
    {
        JumpAndPopIfTopOfStackValueIsTrue* code = (JumpAndPopIfTopOfStackValueIsTrue *)currentCode;
        ESValue* top = pop<ESValue>(stack, bp);
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        if (top->toBoolean()) {
            programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
            pop<ESValue>(stack, bp);
        }
        else
            executeNextCode<JumpAndPopIfTopOfStackValueIsTrue>(programCounter);
        NEXT_INSTRUCTION();
    }

    JumpIfTopOfStackValueIsFalseWithPeekingOpcodeLbl:
    {
        JumpIfTopOfStackValueIsFalseWithPeeking* code = (JumpIfTopOfStackValueIsFalseWithPeeking *)currentCode;
        ESValue* top = peek<ESValue>(stack, bp);
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        if (!top->toBoolean())
            programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
        else
            executeNextCode<JumpIfTopOfStackValueIsFalseWithPeeking>(programCounter);
        NEXT_INSTRUCTION();
    }

    JumpIfTopOfStackValueIsTrueWithPeekingOpcodeLbl:
    {
        JumpIfTopOfStackValueIsTrueWithPeeking* code = (JumpIfTopOfStackValueIsTrueWithPeeking *)currentCode;
        ESValue* top = peek<ESValue>(stack, bp);
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        if (top->toBoolean())
            programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
        else
            executeNextCode<JumpIfTopOfStackValueIsTrueWithPeeking>(programCounter);
        NEXT_INSTRUCTION();
    }

    LoopStartOpcodeLbl:
    {
        executeNextCode<LoopStart>(programCounter);
        NEXT_INSTRUCTION();
    }

    InitObjectOpcodeLbl:
    {
        ESValue* value = pop<ESValue>(stack, bp);
        ESValue* key = pop<ESValue>(stack, bp);
        peek<ESValue>(stack, bp)->asESPointer()->asESObject()->set(*key, *value);
        executeNextCode<InitObject>(programCounter);
        NEXT_INSTRUCTION();
    }

    UnaryTypeOfOpcodeLbl:
    {
        ESValue* v = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, typeOfOperation(v));
        executeNextCode<UnaryTypeOf>(programCounter);
        NEXT_INSTRUCTION();
    }

    UnaryDeleteOpcodeLbl:
    {
        UnaryDelete* code = (UnaryDelete*)currentCode;
        if (code->m_isDeleteObjectKey) {
            ESValue* key = pop<ESValue>(stack, bp);
            ESValue* obj = pop<ESValue>(stack, bp);
            bool res = obj->toObject()->deleteProperty(*key);
            push<ESValue>(stack, topOfStack, ESValue(res));
        } else {
            // TODO
        }
        executeNextCode<UnaryDelete>(programCounter);
        NEXT_INSTRUCTION();
    }

    UnaryVoidOpcodeLbl:
    {
        ESValue* res = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, ESValue());
        executeNextCode<UnaryVoid>(programCounter);
        NEXT_INSTRUCTION();
    }

    StringInOpcodeLbl:
    {
        ESValue* obj = pop<ESValue>(stack, bp);
        ESValue* key = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, ESValue(inOperation(obj, key)));
        executeNextCode<StringIn>(programCounter);
        NEXT_INSTRUCTION();
    }

    InstanceOfOpcodeLbl:
    {
        ESValue* rval = pop<ESValue>(stack, bp);
        ESValue* lval = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, ESValue(instanceOfOperation(lval, rval)));
        executeNextCode<StringIn>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetByIdWithoutExceptionOpcodeLbl:
    {
        GetById* code = (GetById*)currentCode;
        try {
#ifndef ENABLE_ESJIT
            push<ESValue>(stack, topOfStack, getByIdOperationWithNoInline(instance, ec, code));
#else
            ESValue* value = getByIdOperationWithNoInline(instance, ec, code);
            push<ESValue>(stack, topOfStack, value);
            code->m_profile.addProfile(*value);
#endif
        } catch(...) {
#ifndef ENABLE_ESJIT
            push<ESValue>(stack, topOfStack, ESValue());
#else
            ESValue value = ESValue();
            push<ESValue>(stack, topOfStack, value);
            code->m_profile.addProfile(value);
#endif
        }
        executeNextCode<GetById>(programCounter);
        NEXT_INSTRUCTION();
    }

    LoadStackPointerOpcodeLbl:
    {
        LoadStackPointer* code = (LoadStackPointer *)currentCode;
        sub<ESValue>(stack, bp, code->m_offsetToBasePointer);
        executeNextCode<LoadStackPointer>(programCounter);
        NEXT_INSTRUCTION();
    }

    CheckStackPointerOpcodeLbl:
    {
        CheckStackPointer* byteCode = (CheckStackPointer *)currentCode;
        if (stack != bp) {
            printf("Stack is not equal to Base Point at the end of statement (%ld)\n", byteCode->m_lineNumber);
            RELEASE_ASSERT_NOT_REACHED();
        }
        executeNextCode<CheckStackPointer>(programCounter);
        NEXT_INSTRUCTION();
    }

    PrintSpAndBpOpcodeLbl:
    {
        printf("SP = %p, BP = %p\n", stack, bp);
        executeNextCode<PrintSpAndBp>(programCounter);
        NEXT_INSTRUCTION();
    }

    CallBoundFunctionOpcodeLbl:
    {
        CallBoundFunction* code = (CallBoundFunction*)currentCode;
        size_t argc = code->m_boundArgumentsCount + instance->currentExecutionContext()->argumentCount();
        ESValue* mergedArguments = (ESValue *)alloca(sizeof(ESValue) * argc);
        memcpy(mergedArguments, code->m_boundArguments, sizeof(ESValue) * code->m_boundArgumentsCount);
        memcpy(mergedArguments + code->m_boundArgumentsCount, instance->currentExecutionContext()->arguments(), sizeof(ESValue) * instance->currentExecutionContext()->argumentCount());
        return ESFunctionObject::call(instance, code->m_boundTargetFunction, code->m_boundThis, mergedArguments, argc, false);
    }

    TryOpcodeLbl:
    {
        Try* code = (Try *)currentCode;
        tryOperation(instance, codeBlock, codeBuffer, ec, programCounter, code);
        programCounter = jumpTo(codeBuffer, code->m_statementEndPosition);
        NEXT_INSTRUCTION();
    }

    TryCatchBodyEndOpcodeLbl:
    {
        ASSERT(bp == stack);
        return ESValue(ESValue::ESEmptyValue);
    }

    ThrowOpcodeLbl:
    {
        ESValue v = *pop<ESValue>(stack, bp);
        throw v;
    }

    FinallyEndOpcodeLbl:
    {
        if (ec->tryOrCatchBodyResult().isEmpty()) {
            executeNextCode<FinallyEnd>(programCounter);
            NEXT_INSTRUCTION();
        } else {
            ASSERT(ec->tryOrCatchBodyResult().asESPointer()->isESControlFlowRecord());
            ESControlFlowRecord* record = ec->tryOrCatchBodyResult().asESPointer()->asESControlFlowRecord();
            int32_t dupCnt = record->value2().asInt32();
            if (dupCnt <= 1) {
                if (record->reason() == ESControlFlowRecord::ControlFlowReason::NeedsReturn) {
                    ESValue ret = record->value();
                    ec->tryOrCatchBodyResult() = ESValue(ESValue::ESEmptyValue);
                    // TODO sp check
                    return ret;
                } else if (record->reason() == ESControlFlowRecord::ControlFlowReason::NeedsThrow) {
                    ESValue val = record->value();
                    ec->tryOrCatchBodyResult() = ESValue(ESValue::ESEmptyValue);
                    throw val;
                } else {
                    ASSERT(record->reason() == ESControlFlowRecord::ControlFlowReason::NeedsJump);
                    programCounter = jumpTo(codeBuffer, (size_t)record->value().asESPointer());
                    ec->tryOrCatchBodyResult() = ESValue(ESValue::ESEmptyValue);
                    NEXT_INSTRUCTION();
                }
            } else {
                dupCnt--;
                record->setValue2(ESValue((int32_t)dupCnt));
                return ESValue(ESValue::ESEmptyValue);
            }

        }
    }

    AllocPhiOpcodeLbl:
    {
        executeNextCode<AllocPhi>(programCounter);
        NEXT_INSTRUCTION();
    }

    StorePhiOpcodeLbl:
    {
        executeNextCode<StorePhi>(programCounter);
        NEXT_INSTRUCTION();
    }

    LoadPhiOpcodeLbl:
    {
        executeNextCode<LoadPhi>(programCounter);
        NEXT_INSTRUCTION();
    }

    EnumerateObjectOpcodeLbl:
    {
        ESObject* obj = pop<ESValue>(stack, bp)->toObject();
        push<ESValue>(stack, topOfStack, ESValue((ESPointer *)executeEnumerateObject(obj)));
        executeNextCode<EnumerateObject>(programCounter);
        NEXT_INSTRUCTION();
    }

    CheckIfKeyIsLastOpcodeLbl:
    {
        EnumerateObjectData* data = (EnumerateObjectData *)peek<ESValue>(stack, bp)->asESPointer();
        push<ESValue>(stack, topOfStack, ESValue(data->m_keys.size() == data->m_idx));
        executeNextCode<CheckIfKeyIsLast>(programCounter);
        NEXT_INSTRUCTION();
    }

    EnumerateObjectKeyOpcodeLbl:
    {
        EnumerateObjectKey* code = (EnumerateObjectKey*)currentCode;
        EnumerateObjectData* data = (EnumerateObjectData *)peek<ESValue>(stack, bp)->asESPointer();
        data->m_idx++;
        push<ESValue>(stack, topOfStack, data->m_keys[data->m_idx - 1]);
#ifdef ENABLE_ESJIT
        code->m_profile.addProfile(data->m_keys[data->m_idx - 1]);
#endif
        executeNextCode<EnumerateObjectKey>(programCounter);
        NEXT_INSTRUCTION();
    }

    CallEvalFunctionOpcodeLbl:
    {
        CallEvalFunction* code = (CallEvalFunction *)currentCode;
        const unsigned& argc = code->m_argmentCount;
        stack = (void *)((size_t)stack - argc * sizeof(ESValue));
        ESValue* arguments = (ESValue *)stack;

        ESValue callee = *ec->resolveBinding(strings->eval);
        if (callee.isESPointer() && (void *)callee.asESPointer() == (void *)globalObject->eval()) {
            ESObject* receiver = instance->globalObject();
            ESValue ret = instance->runOnEvalContext([instance, &arguments, &argc](){
                ESValue ret;
                if (argc)
                    ret = instance->evaluate(const_cast<u16string &>(arguments[0].asESString()->string()), false);
                return ret;
            }, true);
            push<ESValue>(stack, topOfStack, ret);
        } else {
            ESObject* receiver = instance->globalObject();
            push<ESValue>(stack, topOfStack, ESFunctionObject::call(instance, callee, receiver, arguments, argc, false));
        }
#ifdef ENABLE_ESJIT
        code->m_profile.addProfile(*peek<ESValue>(stack, bp));
#endif
        executeNextCode<CallEvalFunction>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetArgumentsObjectOpcodeLbl:
    {
        push<ESValue>(stack, topOfStack, ec->resolveArgumentsObjectBinding());
        executeNextCode<GetArgumentsObject>(programCounter);
        NEXT_INSTRUCTION();
    }

    SetArgumentsObjectOpcodeLbl:
    {
        ESValue* value = peek<ESValue>(stack, bp);
        *ec->resolveArgumentsObjectBinding() = *value;
        executeNextCode<SetArgumentsObject>(programCounter);
        NEXT_INSTRUCTION();
    }

    SetObjectPreComputedCaseSlowModeOpcodeLbl:
    {
        SetObjectPreComputedCaseSlowMode* code = (SetObjectPreComputedCaseSlowMode*)currentCode;
        ESValue value = *pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        ESValue v(code->m_propertyValue);
        setObjectOperationSlowMode(willBeObject, &v, value);
        push<ESValue>(stack, topOfStack, value);
        executeNextCode<SetObjectPreComputedCaseSlowMode>(programCounter);
        NEXT_INSTRUCTION();
    }

    SetObjectSlowModeOpcodeLbl:
    {
        SetObjectSlowMode* code = (SetObjectSlowMode*)currentCode;
        ESValue value = *pop<ESValue>(stack, bp);
        ESValue* property = pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        setObjectOperationSlowMode(willBeObject, property, value);
        push<ESValue>(stack, topOfStack, value);
        executeNextCode<SetObjectSlowMode>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetObjectWithPeekingPreComputedCaseSlowModeOpcodeLbl:
    {
        GetObjectWithPeekingPreComputedCaseSlowMode* code = (GetObjectWithPeekingPreComputedCaseSlowMode*)currentCode;
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        stack = (void *)(((size_t)stack) + sizeof(ESValue) * 1);
        ESValue v(code->m_propertyValue);
        push<ESValue>(stack, topOfStack, getObjectOperationSlowMode(willBeObject, &v, globalObject));
        executeNextCode<GetObjectWithPeekingPreComputedCaseSlowMode>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetObjectPreComputedCaseSlowModeOpcodeLbl:
    {
        GetObjectPreComputedCaseSlowMode* code = (GetObjectPreComputedCaseSlowMode*)currentCode;
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        ESValue v(code->m_propertyValue);
        push<ESValue>(stack, topOfStack, getObjectOperationSlowMode(willBeObject, &v, globalObject));
        executeNextCode<GetObjectPreComputedCaseSlowMode>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetObjectPreComputedCaseAndPushObjectSlowModeOpcodeLbl:
    {
        GetObjectPreComputedCaseAndPushObjectSlowMode* code = (GetObjectPreComputedCaseAndPushObjectSlowMode*)currentCode;
        ESValue willBeObject = *pop<ESValue>(stack, bp);
        ESValue v(code->m_propertyValue);
        push<ESValue>(stack, topOfStack, getObjectOperationSlowMode(&willBeObject, &v, globalObject));
        push<ESValue>(stack, topOfStack, willBeObject);
        executeNextCode<GetObjectPreComputedCaseSlowMode>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetObjectWithPeekingSlowModeOpcodeLbl:
    {
        GetObjectWithPeekingSlowMode* code = (GetObjectWithPeekingSlowMode*)currentCode;
        ESValue* property = pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        stack = (void *)(((size_t)stack) + sizeof(ESValue) * 2);
        push<ESValue>(stack, topOfStack, getObjectOperationSlowMode(willBeObject, property, globalObject));
        executeNextCode<GetObjectWithPeekingSlowMode>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetObjectSlowModeOpcodeLbl:
    {
        GetObjectSlowMode* code = (GetObjectSlowMode*)currentCode;
        ESValue* property = pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, getObjectOperationSlowMode(willBeObject, property, globalObject));
        executeNextCode<GetObjectSlowMode>(programCounter);
        NEXT_INSTRUCTION();
    }

    GetObjectAndPushObjectSlowModeOpcodeLbl:
    {
        GetObjectAndPushObjectSlowMode* code = (GetObjectAndPushObjectSlowMode*)currentCode;
        ESValue* property = pop<ESValue>(stack, bp);
        ESValue willBeObject = *pop<ESValue>(stack, bp);
        push<ESValue>(stack, topOfStack, getObjectOperationSlowMode(&willBeObject, property, globalObject));
        push<ESValue>(stack, topOfStack, &willBeObject);
        executeNextCode<GetObjectSlowMode>(programCounter);
        NEXT_INSTRUCTION();
    }

    ExecuteNativeFunctionOpcodeLbl:
    {
        ExecuteNativeFunction* code = (ExecuteNativeFunction*)currentCode;
        ASSERT(bp == stack);
        return code->m_fn(instance);
    }

    SetObjectPropertySetterOpcodeLbl:
    {
        ESValue* value = pop<ESValue>(stack, bp);
        ESValue* key = pop<ESValue>(stack, bp);
        ESObject* obj = peek<ESValue>(stack, bp)->asESPointer()->asESObject();
        ESString* keyString = key->toString();
        if (obj->hasOwnProperty(keyString)) {
            // TODO check property is accessor property
            // TODO check accessor already exists
            obj->accessorData(keyString)->setJSSetter(value->asESPointer()->asESFunctionObject());
        } else {
            obj->defineAccessorProperty(keyString, new ESPropertyAccessorData(NULL, value->asESPointer()->asESFunctionObject()), true, true, true);
        }
        executeNextCode<SetObjectPropertySetter>(programCounter);
        NEXT_INSTRUCTION();
    }

    SetObjectPropertyGetterOpcodeLbl:
    {
        ESValue* value = pop<ESValue>(stack, bp);
        ESValue* key = pop<ESValue>(stack, bp);
        ESObject* obj = peek<ESValue>(stack, bp)->asESPointer()->asESObject();
        ESString* keyString = key->toString();
        if (obj->hasOwnProperty(keyString)) {
            // TODO check property is accessor property
            // TODO check accessor already exists
            obj->accessorData(keyString)->setJSGetter(value->asESPointer()->asESFunctionObject());
        } else {
            obj->defineAccessorProperty(keyString, new ESPropertyAccessorData(value->asESPointer()->asESFunctionObject(), NULL), true, true, true);
        }
        executeNextCode<SetObjectPropertyGetter>(programCounter);
        NEXT_INSTRUCTION();
    }

    EndOpcodeLbl:
    {
        ASSERT(stack == bp);
        return ESValue();
    }

}

}
