#include "Escargot.h"
#include "bytecode/ByteCode.h"

#include "ByteCodeOperations.h"

namespace escargot {


NEVER_INLINE void registerOpcode(ESVMInstance* instance, Opcode opcode, const char* opcodeName, void* opcodeAddress)
{
    static std::unordered_set<void *> labelAddressChecker;

    if (labelAddressChecker.find(opcodeAddress) != labelAddressChecker.end()) {
        ESCARGOT_LOG_ERROR("%s\n", opcodeName);
        RELEASE_ASSERT_NOT_REACHED();
    }

    labelAddressChecker.insert(opcodeAddress);
    instance->opcodeTable()->m_table[opcode] = opcodeAddress;
    instance->opcodeResverseTable().insert(std::make_pair(opcodeAddress, opcode));

    if (opcode == EndOpcode) {
        labelAddressChecker.clear();
    }
}

#ifdef ENABLE_ESJIT
ESValue interpret(ESVMInstance* instance, CodeBlock* codeBlock, size_t programCounter, unsigned maxStackPos)
#else
ESValue interpret(ESVMInstance* instance, CodeBlock* codeBlock, size_t programCounter, ESValue* stackStorage, ESValueVector* heapStorage)
#endif
{
    if (UNLIKELY(codeBlock == NULL)) {
        goto RegisterOpcodes;
    }
    {
#define NEXT_INSTRUCTION() \
            goto NextInstruction

        ExecutionContext* ec = instance->currentExecutionContext();
        unsigned stackSiz = codeBlock->m_requiredStackSizeInESValueSize * sizeof(ESValue);
#ifdef ENABLE_ESJIT
        char* stackBuf;
        void* bp;
        void* stack;
        if (maxStackPos == 0) {
            stackBuf = (char *)alloca(stackSiz);
            bp = stackBuf;
            stack = bp;
        } else {
            size_t offset = maxStackPos*sizeof(ESValue);
            stackBuf = ec->getBp();
            bp = stackBuf;
            stack = (void*)(((size_t)bp) + offset);
        }
#ifndef NDEBUG
        bp = stackBuf;
        void* topOfStack = stackBuf + stackSiz;
#endif
#else
        char* stackBuf[stackSiz];
        // stackBuf = (char *)alloca(stackSiz);
        void* stack = stackBuf;
#ifndef NDEBUG
        void* bp;
        bp = stackBuf;
        void* topOfStack = stackBuf + stackSiz;
#endif
#endif


#ifndef ANDROID
        char tmpStackBuf[32];
        void* tmpStack = tmpStackBuf;
#else
        void* tmpStack = alloca(32);
#endif
        void* tmpBp = tmpStack;
#ifndef NDEBUG
        void* tmpTopOfStack = (void *)((size_t)tmpStack + 32);
#endif
        char* codeBuffer = codeBlock->m_code.data();
        GlobalObject* globalObject = instance->globalObject();
        ESValue* lastExpressionStatementValue = &instance->m_lastExpressionStatementValue;
        ESValue* stackAllocatedLocalValuePointer = stackStorage;
        ESValueVector* heapAllocatedHeapValue = heapStorage;
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
                printf("execute %p %u \t(nodeinfo %d)\t", currentCode, (unsigned)(programCounter-(size_t)codeBuffer), (int)currentCode->m_node->sourceLocation().m_lineNumber);
            else
                printf("execute %p %u \t(nodeinfo null)\t", currentCode, (unsigned)(programCounter-(size_t)codeBuffer));
            currentCode->dump();
            fflush(stdout);
        }

        if (currentCode->m_orgOpcode > OpcodeKindEnd) {
            printf("Error: unknown opcode\n");
            RELEASE_ASSERT_NOT_REACHED();
        } else {


        }
#endif

        goto *(currentCode->m_opcodeInAddress);

        PushOpcodeLbl:
        {
            Push* pushCode = (Push*)currentCode;
            PUSH(stack, topOfStack, pushCode->m_value);
            executeNextCode<Push>(programCounter);
            NEXT_INSTRUCTION();
        }

        PopOpcodeLbl:
        {
            POP(stack, bp);
            executeNextCode<Pop>(programCounter);
            NEXT_INSTRUCTION();
        }

        DuplicateTopOfStackValueOpcodeLbl:
        {
            PUSH(stack, topOfStack, PEEK(stack, bp));
            executeNextCode<DuplicateTopOfStackValue>(programCounter);
            NEXT_INSTRUCTION();
        }

        PopExpressionStatementOpcodeLbl:
        {
            ESValue* t = POP(stack, bp);
            *lastExpressionStatementValue = *t;
            executeNextCode<PopExpressionStatement>(programCounter);
            NEXT_INSTRUCTION();
        }

        PushIntoTempStackOpcodeLbl:
        {
#ifdef NDEBUG
            push<ESValue>(tmpStack, pop<ESValue>(stack));
#else
            push<ESValue>(tmpStack, tmpTopOfStack, pop<ESValue>(stack, bp));
#endif
            executeNextCode<PushIntoTempStack>(programCounter);
            NEXT_INSTRUCTION();
        }

        PopFromTempStackOpcodeLbl:
        {
#ifdef NDEBUG
            push<ESValue>(stack, pop<ESValue>(tmpStack));
#else
            push<ESValue>(stack, topOfStack, pop<ESValue>(tmpStack, tmpBp));
#endif
            executeNextCode<PopFromTempStack>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetByIdOpcodeLbl:
        {
            GetById* code = (GetById*)currentCode;
            ESValue* value = getByIdOperation(instance, ec, code);
            PUSH(stack, topOfStack, value);
#ifdef ENABLE_ESJIT
            code->m_profile.addProfile(*value);
#endif
            executeNextCode<GetById>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetByIndexOpcodeLbl:
        {
            GetByIndex* code = (GetByIndex*)currentCode;
#ifndef ENABLE_ESJIT
            PUSH(stack, topOfStack, &stackAllocatedLocalValuePointer[code->m_index]);
#else
            ESValue v = stackAllocatedLocalValuePointer[code->m_index];
            PUSH(stack, topOfStack, v);
            code->m_profile.addProfile(v);
#endif
            executeNextCode<GetByIndex>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetByIndexInHeapOpcodeLbl:
        {
            GetByIndexInHeap* code = (GetByIndexInHeap*)currentCode;
#ifndef ENABLE_ESJIT
            PUSH(stack, topOfStack, (*heapAllocatedHeapValue)[code->m_index]);
#else
            ESValue v = (*heapAllocatedHeapValue)[code->m_index];
            PUSH(stack, topOfStack, v);
            code->m_profile.addProfile(v);
#endif
            executeNextCode<GetByIndexInHeap>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetByIndexInUpperContextHeapOpcodeLbl:
        {
            GetByIndexInUpperContextHeap* code = (GetByIndexInUpperContextHeap*)currentCode;
            LexicalEnvironment* env = ec->environment();
            for (unsigned i = 0; i < code->m_upIndex; i ++) {
                env = env->outerEnvironment();
            }
            ASSERT(env->record()->isDeclarativeEnvironmentRecord());

#ifndef ENABLE_ESJIT
            PUSH(stack, topOfStack, *env->record()->toDeclarativeEnvironmentRecord()->bindingValueForHeapAllocatedData(code->m_index));
#else
            ESValue v = *env->record()->toDeclarativeEnvironmentRecord()->bindingValueForHeapAllocatedData(code->m_index);
            PUSH(stack, topOfStack, v);
            code->m_profile.addProfile(v);
#endif
            executeNextCode<GetByIndexInUpperContextHeap>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetByGlobalIndexOpcodeLbl:
        {
            GetByGlobalIndex* code = (GetByGlobalIndex*)currentCode;
#ifndef ENABLE_ESJIT
            PUSH(stack, topOfStack, getByGlobalIndexOperation(globalObject, code));
#else
            ESValue value = getByGlobalIndexOperation(globalObject, code);
            PUSH(stack, topOfStack, value);
            code->m_profile.addProfile(value);
#endif
            executeNextCode<GetByGlobalIndex>(programCounter);
            NEXT_INSTRUCTION();
        }

        SetByIdOpcodeLbl:
        {
            SetById* code = (SetById*)currentCode;
            ESValue* value = PEEK(stack, bp);

            if (LIKELY(code->m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
                ASSERT(ec->resolveBinding(code->m_name) == code->m_cachedSlot);
                *code->m_cachedSlot = *value;
            } else {
                setByIdSlowCase(instance, globalObject, code, value);
            }
            executeNextCode<SetById>(programCounter);
            NEXT_INSTRUCTION();
        }

        SetByIndexOpcodeLbl:
        {
            SetByIndex* code = (SetByIndex*)currentCode;
            stackAllocatedLocalValuePointer[code->m_index] = *PEEK(stack, bp);
            executeNextCode<SetByIndex>(programCounter);
            NEXT_INSTRUCTION();
        }

        SetByIndexInHeapOpcodeLbl:
        {
            SetByIndexInHeap* code = (SetByIndexInHeap*)currentCode;
            (*heapAllocatedHeapValue)[code->m_index] = *PEEK(stack, bp);
            executeNextCode<SetByIndexInHeap>(programCounter);
            NEXT_INSTRUCTION();
        }

        SetByIndexInUpperContextHeapOpcodeLbl:
        {
            SetByIndexInUpperContextHeap* code = (SetByIndexInUpperContextHeap*)currentCode;
            LexicalEnvironment* env = ec->environment();
            for (unsigned i = 0; i < code->m_upIndex; i ++) {
                env = env->outerEnvironment();
            }
            ASSERT(env->record()->isDeclarativeEnvironmentRecord());
            *env->record()->toDeclarativeEnvironmentRecord()->bindingValueForHeapAllocatedData(code->m_index) = *PEEK(stack, bp);
            executeNextCode<SetByIndexInUpperContextHeap>(programCounter);
            NEXT_INSTRUCTION();
        }

        SetByGlobalIndexOpcodeLbl:
        {
            SetByGlobalIndex* code = (SetByGlobalIndex*)currentCode;
            setByGlobalIndexOperation(globalObject, code, *PEEK(stack, bp));
            executeNextCode<SetByGlobalIndex>(programCounter);
            NEXT_INSTRUCTION();
        }

        CreateBindingOpcodeLbl:
        {
            CreateBinding* code = (CreateBinding*)currentCode;
            ec->environment()->record()->createMutableBindingForAST(code->m_name, false);
            ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
            executeNextCode<CreateBinding>(programCounter);
            NEXT_INSTRUCTION();
        }

        EqualOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            PUSH(stack, topOfStack, ESValue(left->abstractEqualsTo(*right)));
            executeNextCode<Equal>(programCounter);
            NEXT_INSTRUCTION();
        }

        NotEqualOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            PUSH(stack, topOfStack, ESValue(!left->abstractEqualsTo(*right)));
            executeNextCode<NotEqual>(programCounter);
            NEXT_INSTRUCTION();
        }

        StrictEqualOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            PUSH(stack, topOfStack, ESValue(left->equalsTo(*right)));
            executeNextCode<StrictEqual>(programCounter);
            NEXT_INSTRUCTION();
        }

        NotStrictEqualOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            PUSH(stack, topOfStack, ESValue(!left->equalsTo(*right)));
            executeNextCode<NotStrictEqual>(programCounter);
            NEXT_INSTRUCTION();
        }

        BitwiseAndOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            PUSH(stack, topOfStack, ESValue(left->toInt32() & right->toInt32()));
            executeNextCode<BitwiseAnd>(programCounter);
            NEXT_INSTRUCTION();
        }

        BitwiseOrOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            PUSH(stack, topOfStack, ESValue(left->toInt32() | right->toInt32()));
            executeNextCode<BitwiseOr>(programCounter);
            NEXT_INSTRUCTION();
        }

        BitwiseXorOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            PUSH(stack, topOfStack, ESValue(left->toInt32() ^ right->toInt32()));
            executeNextCode<BitwiseXor>(programCounter);
            NEXT_INSTRUCTION();
        }

        LeftShiftOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            int32_t lnum = left->toInt32();
            int32_t rnum = right->toInt32();
            lnum <<= ((unsigned int)rnum) & 0x1F;
            PUSH(stack, topOfStack, ESValue(lnum));
            executeNextCode<LeftShift>(programCounter);
            NEXT_INSTRUCTION();
        }

        SignedRightShiftOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            int32_t lnum = left->toInt32();
            int32_t rnum = right->toInt32();
            lnum >>= ((unsigned int)rnum) & 0x1F;
            PUSH(stack, topOfStack, ESValue(lnum));
            executeNextCode<SignedRightShift>(programCounter);
            NEXT_INSTRUCTION();
        }

        UnsignedRightShiftOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            uint32_t lnum = left->toUint32();
            uint32_t rnum = right->toUint32();
            lnum = (lnum) >> ((rnum) & 0x1F);
            PUSH(stack, topOfStack, ESValue(lnum));
            executeNextCode<UnsignedRightShift>(programCounter);
            NEXT_INSTRUCTION();
        }

        LessThanOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            PUSH(stack, topOfStack, ESValue(abstractRelationalComparison(left, right, true)));
            executeNextCode<LessThan>(programCounter);
            NEXT_INSTRUCTION();
        }

        LessThanOrEqualOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            PUSH(stack, topOfStack, ESValue(abstractRelationalComparisonOrEqual(left, right, true)));
            executeNextCode<LessThanOrEqual>(programCounter);
            NEXT_INSTRUCTION();
        }

        GreaterThanOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            PUSH(stack, topOfStack, ESValue(abstractRelationalComparison(right, left, false)));
            executeNextCode<GreaterThan>(programCounter);
            NEXT_INSTRUCTION();
        }

        GreaterThanOrEqualOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            PUSH(stack, topOfStack, ESValue(abstractRelationalComparisonOrEqual(right, left, false)));
            executeNextCode<GreaterThanOrEqual>(programCounter);
            NEXT_INSTRUCTION();
        }

        PlusOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            PUSH(stack, topOfStack, plusOperation(left, right));
            executeNextCode<Plus>(programCounter);
            NEXT_INSTRUCTION();
        }

        MinusOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            PUSH(stack, topOfStack, minusOperation(left, right));
            executeNextCode<Minus>(programCounter);
            NEXT_INSTRUCTION();
        }

        MultiplyOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);

            if (left->isInt32() && right->isInt32()) {
                int32_t a = left->asInt32();
                int32_t b = right->asInt32();
                int32_t c = right->asInt32();
                bool result = ArithmeticOperations<int32_t, int32_t, int32_t>::multiply(a, b, c);
                if (LIKELY(result)) {
                    PUSH(stack, topOfStack, ESValue(c));
                } else {
                    PUSH(stack, topOfStack, ESValue(left->toNumber() * right->toNumber()));
                }
            } else {
                PUSH(stack, topOfStack, ESValue(left->toNumber() * right->toNumber()));
            }
            executeNextCode<Multiply>(programCounter);
            NEXT_INSTRUCTION();
        }

        DivisionOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            PUSH(stack, topOfStack, ESValue(left->toNumber() / right->toNumber()));
            executeNextCode<Division>(programCounter);
            NEXT_INSTRUCTION();
        }

        ModOpcodeLbl:
        {
            ESValue* right = POP(stack, bp);
            ESValue* left = POP(stack, bp);
            PUSH(stack, topOfStack, modOperation(left, right));
            executeNextCode<Mod>(programCounter);
            NEXT_INSTRUCTION();
        }

        IncrementOpcodeLbl:
        {
            ESValue* src = POP(stack, bp);
            ASSERT(src->isNumber());
            ESValue ret(ESValue::ESForceUninitialized);
            if (LIKELY(src->isInt32())) {
                int32_t a = src->asInt32();
                if (UNLIKELY(a == std::numeric_limits<int32_t>::max()))
                    ret = ESValue(ESValue::EncodeAsDouble, ((double)a) + 1);
                else
                    ret = ESValue(a + 1);
            } else {
                ret = ESValue(src->asDouble() + 1);
            }
            PUSH(stack, topOfStack, ret);
            executeNextCode<Increment>(programCounter);
            NEXT_INSTRUCTION();
        }

        DecrementOpcodeLbl:
        {
            ESValue* src = POP(stack, bp);
            ASSERT(src->isNumber());
            ESValue ret(ESValue::ESForceUninitialized);
            if (LIKELY(src->isInt32())) {
                int32_t a = src->asInt32();
                if (UNLIKELY(a == std::numeric_limits<int32_t>::min()))
                    ret = ESValue(ESValue::EncodeAsDouble, ((double)a) - 1);
                else
                    ret = ESValue(a - 1);
            } else {
                ret = ESValue(src->asDouble() - 1);
            }
            PUSH(stack, topOfStack, ret);
            executeNextCode<Decrement>(programCounter);
            NEXT_INSTRUCTION();
        }

        BitwiseNotOpcodeLbl:
        {
            PUSH(stack, topOfStack, ESValue(~POP(stack, bp)->toInt32()));
            executeNextCode<BitwiseNot>(programCounter);
            NEXT_INSTRUCTION();
        }

        LogicalNotOpcodeLbl:
        {
            PUSH(stack, topOfStack, ESValue(!POP(stack, bp)->toBoolean()));
            executeNextCode<LogicalNot>(programCounter);
            NEXT_INSTRUCTION();
        }

        UnaryMinusOpcodeLbl:
        {
            PUSH(stack, topOfStack, ESValue(-POP(stack, bp)->toNumber()));
            executeNextCode<UnaryMinus>(programCounter);
            NEXT_INSTRUCTION();
        }

        UnaryPlusOpcodeLbl:
        {
            PUSH(stack, topOfStack, ESValue(POP(stack, bp)->toNumber()));
            executeNextCode<UnaryPlus>(programCounter);
            NEXT_INSTRUCTION();
        }

        ToNumberOpcodeLbl:
        {
            ESValue* v = PEEK(stack, bp);
            if (!v->isNumber()) {
                v = POP(stack, bp);
                PUSH(stack, topOfStack, ESValue(v->toNumber()));
            }
            executeNextCode<ToNumber>(programCounter);
            NEXT_INSTRUCTION();
        }

        ReturnFunctionOpcodeLbl:
        {
            ASSERT(bp == stack);
            return ESValue();
        }

        ReturnFunctionWithValueOpcodeLbl:
        {
            ESValue* ret = POP(stack, bp);
            return *ret;
        }


        ThisOpcodeLbl:
        {
#ifdef ENABLE_ESJIT
            This* code = (This*)currentCode;
            code->m_profile.addProfile(ec->resolveThisBinding());
#endif
            PUSH(stack, topOfStack, ec->resolveThisBinding());
            executeNextCode<This>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetObjectOpcodeLbl:
        {
            ESValue* property = POP(stack, bp);
            ESValue* willBeObject = POP(stack, bp);
#ifndef ENABLE_ESJIT
            PUSH(stack, topOfStack, getObjectOperation(willBeObject, property, globalObject));
#else
            ESValue value = getObjectOperation(willBeObject, property, globalObject);
            PUSH(stack, topOfStack, value);
            GetObject* code = (GetObject*)currentCode;
            code->m_profile.addProfile(value);
#endif
            executeNextCode<GetObject>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetObjectAndPushObjectOpcodeLbl:
        {
            ESValue* property = POP(stack, bp);
            ESValue willBeObject = *POP(stack, bp);
#ifndef ENABLE_ESJIT
            PUSH(stack, topOfStack, getObjectOperation(&willBeObject, property, globalObject));
            PUSH(stack, topOfStack, willBeObject);
#else
            ESValue value = getObjectOperation(&willBeObject, property, globalObject);
            PUSH(stack, topOfStack, value);
            PUSH(stack, topOfStack, willBeObject);
            GetObject* code = (GetObject*)currentCode;
            code->m_profile.addProfile(value);
#endif
            executeNextCode<GetObjectAndPushObject>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetObjectPreComputedCaseOpcodeLbl:
        {
            GetObjectPreComputedCase* code = (GetObjectPreComputedCase*)currentCode;
            ESValue* willBeObject = POP(stack, bp);
#ifndef ENABLE_ESJIT
            PUSH(stack, topOfStack, getObjectPreComputedCaseOperation(willBeObject, code->m_propertyValue, globalObject,
                &code->m_inlineCache));
#else
            ESValue value = getObjectPreComputedCaseOperation(willBeObject, code->m_propertyValue, globalObject,
                &code->m_inlineCache);
            PUSH(stack, topOfStack, value);
            code->m_profile.addProfile(value);
#endif
            executeNextCode<GetObjectPreComputedCase>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetObjectPreComputedCaseAndPushObjectOpcodeLbl:
        {
            GetObjectPreComputedCaseAndPushObject* code = (GetObjectPreComputedCaseAndPushObject*)currentCode;
            ESValue willBeObject = *POP(stack, bp);
#ifndef ENABLE_ESJIT
            PUSH(stack, topOfStack, getObjectPreComputedCaseOperation(&willBeObject, code->m_propertyValue, globalObject,
                &code->m_inlineCache));
            PUSH(stack, topOfStack, willBeObject);
#else
            ESValue value = getObjectPreComputedCaseOperation(&willBeObject, code->m_propertyValue, globalObject,
                &code->m_inlineCache);
            PUSH(stack, topOfStack, value);
            PUSH(stack, topOfStack, willBeObject);
            code->m_profile.addProfile(value);
#endif
            executeNextCode<GetObjectPreComputedCaseAndPushObject>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetObjectWithPeekingOpcodeLbl:
        {
            ESValue* property = (ESValue *)((size_t)stack - sizeof(ESValue));
            ESValue* willBeObject = (ESValue *)((size_t)stack - sizeof(ESValue) * 2);
#ifndef ENABLE_ESJIT
            PUSH(stack, topOfStack, getObjectOperation(willBeObject, property, globalObject));
#else
            ESValue value = getObjectOperation(willBeObject, property, globalObject);
            PUSH(stack, topOfStack, value);
            GetObjectWithPeeking* code = (GetObjectWithPeeking*)currentCode;
            code->m_profile.addProfile(value);
#endif
            executeNextCode<GetObjectWithPeeking>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetObjectWithPeekingPreComputedCaseOpcodeLbl:
        {
            GetObjectWithPeekingPreComputedCase* code = (GetObjectWithPeekingPreComputedCase*)currentCode;
            ESValue* willBeObject = PEEK(stack, bp);
#ifndef ENABLE_ESJIT
            PUSH(stack, topOfStack, getObjectPreComputedCaseOperationWithNeverInline(willBeObject, code->m_propertyValue, globalObject,
                &code->m_inlineCache));
#else
            ESValue value = getObjectPreComputedCaseOperationWithNeverInline(willBeObject, code->m_propertyValue, globalObject,
                &code->m_inlineCache);
            PUSH(stack, topOfStack, value);
            code->m_profile.addProfile(value);
#endif
            executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
            NEXT_INSTRUCTION();
        }

        SetObjectOpcodeLbl:
        {
            const ESValue& value = *POP(stack, bp);
            ESValue* property = POP(stack, bp);
            ESValue* willBeObject = POP(stack, bp);
            setObjectOperation(willBeObject, property, value);
            PUSH(stack, topOfStack, value);
            executeNextCode<SetObject>(programCounter);
            NEXT_INSTRUCTION();
        }

        SetObjectPreComputedCaseOpcodeLbl:
        {
            SetObjectPreComputedCase* code = (SetObjectPreComputedCase*)currentCode;
            const ESValue& value = *POP(stack, bp);
            ESValue* willBeObject = POP(stack, bp);
            setObjectPreComputedCaseOperation(willBeObject, code->m_propertyValue, value, &code->m_cachedhiddenClassChain
                , &code->m_cachedIndex, &code->m_hiddenClassWillBe);
            PUSH(stack, topOfStack, value);
            executeNextCode<SetObjectPreComputedCase>(programCounter);
            NEXT_INSTRUCTION();
        }

        CallFunctionOpcodeLbl:
        {
            CallFunction* code = (CallFunction*)currentCode;
            const unsigned& argc = code->m_argmentCount;
            stack = (void *)((size_t)stack - argc * sizeof(ESValue));
            ESValue* arguments = (ESValue *)stack;
#ifndef ENABLE_ESJIT
            PUSH(stack, topOfStack, ESFunctionObject::call(instance, *POP(stack, bp), ESValue(), arguments, argc, false));
#else
            ESValue result = ESFunctionObject::call(instance, *POP(stack, bp), ESValue(), arguments, argc, false);
            code->m_profile.addProfile(result);
            PUSH(stack, topOfStack, result);
#endif
            executeNextCode<CallFunction>(programCounter);
            NEXT_INSTRUCTION();
        }

        CallFunctionWithReceiverOpcodeLbl:
        {
            CallFunctionWithReceiver* code = (CallFunctionWithReceiver*)currentCode;
            const unsigned& argc = code->m_argmentCount;
            stack = (void *)((size_t)stack - argc * sizeof(ESValue));
            ESValue* arguments = (ESValue *)stack;
            ESValue* receiver = POP(stack, bp);
#ifndef ENABLE_ESJIT
            PUSH(stack, topOfStack, ESFunctionObject::call(instance, *POP(stack, bp), *receiver, arguments, argc, false));
#else
            ESValue result = ESFunctionObject::call(instance, *POP(stack, bp), *receiver, arguments, argc, false);
            code->m_profile.addProfile(result);
            PUSH(stack, topOfStack, result);
#endif
            executeNextCode<CallFunctionWithReceiver>(programCounter);
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
            ESValue* top = POP(stack, bp);
            ASSERT(code->m_jumpPosition != SIZE_MAX);
            if (!top->toBoolean()) {
                programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
            } else {
                executeNextCode<JumpIfTopOfStackValueIsFalse>(programCounter);
            }
            NEXT_INSTRUCTION();
        }

        JumpIfTopOfStackValueIsTrueOpcodeLbl:
        {
            JumpIfTopOfStackValueIsTrue* code = (JumpIfTopOfStackValueIsTrue *)currentCode;
            ESValue* top = POP(stack, bp);
            ASSERT(code->m_jumpPosition != SIZE_MAX);
            if (top->toBoolean()) {
                programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
            } else {
                executeNextCode<JumpIfTopOfStackValueIsTrue>(programCounter);
            }
            NEXT_INSTRUCTION();
        }

        JumpAndPopIfTopOfStackValueIsTrueOpcodeLbl:
        {
            JumpAndPopIfTopOfStackValueIsTrue* code = (JumpAndPopIfTopOfStackValueIsTrue *)currentCode;
            ESValue* top = POP(stack, bp);
            ASSERT(code->m_jumpPosition != SIZE_MAX);
            if (top->toBoolean()) {
                programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
                POP(stack, bp);
            } else {
                executeNextCode<JumpAndPopIfTopOfStackValueIsTrue>(programCounter);
            }
            NEXT_INSTRUCTION();
        }

        JumpIfTopOfStackValueIsFalseWithPeekingOpcodeLbl:
        {
            JumpIfTopOfStackValueIsFalseWithPeeking* code = (JumpIfTopOfStackValueIsFalseWithPeeking *)currentCode;
            ESValue* top = PEEK(stack, bp);
            ASSERT(code->m_jumpPosition != SIZE_MAX);
            if (!top->toBoolean()) {
                programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
            } else {
                executeNextCode<JumpIfTopOfStackValueIsFalseWithPeeking>(programCounter);
            }
            NEXT_INSTRUCTION();
        }

        JumpIfTopOfStackValueIsTrueWithPeekingOpcodeLbl:
        {
            JumpIfTopOfStackValueIsTrueWithPeeking* code = (JumpIfTopOfStackValueIsTrueWithPeeking *)currentCode;
            ESValue* top = PEEK(stack, bp);
            ASSERT(code->m_jumpPosition != SIZE_MAX);
            if (top->toBoolean()) {
                programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
            } else {
                executeNextCode<JumpIfTopOfStackValueIsTrueWithPeeking>(programCounter);
            }
            NEXT_INSTRUCTION();
        }

        NewFunctionCallOpcodeLbl:
        {
            NewFunctionCall* code = (NewFunctionCall*)currentCode;
            const unsigned& argc = code->m_argmentCount;
            stack = (void *)((size_t)stack - argc * sizeof(ESValue));
            ESValue* arguments = (ESValue *)stack;
            ESValue fn = *POP(stack, bp);
#ifndef ENABLE_ESJIT
            PUSH(stack, topOfStack, newOperation(instance, globalObject, fn, arguments, argc));
#else
            ESValue result = newOperation(instance, globalObject, fn, arguments, argc);
            code->m_profile.addProfile(result);
            PUSH(stack, topOfStack, result);
#endif
            executeNextCode<NewFunctionCall>(programCounter);
            NEXT_INSTRUCTION();
        }

        CreateObjectOpcodeLbl:
        {
            CreateObject* code = (CreateObject*)currentCode;
            ESObject* obj = ESObject::create(code->m_keyCount + 1);
            PUSH(stack, topOfStack, obj);
            executeNextCode<CreateObject>(programCounter);
            NEXT_INSTRUCTION();
        }

        CreateArrayOpcodeLbl:
        {
            CreateArray* code = (CreateArray*)currentCode;
            ESArrayObject* arr = ESArrayObject::create(code->m_keyCount);
            PUSH(stack, topOfStack, arr);
            executeNextCode<CreateArray>(programCounter);
            NEXT_INSTRUCTION();
        }

        CreateFunctionOpcodeLbl:
        {
            CreateFunction* code = (CreateFunction*)currentCode;
            ASSERT(((size_t)code->m_codeBlock % sizeof(size_t)) == 0);
            ESFunctionObject* function = ESFunctionObject::create(ec->environment(), code->m_codeBlock, code->m_nonAtomicName == NULL ? strings->emptyString.string() : code->m_nonAtomicName, code->m_codeBlock->m_argumentCount);
            if (code->m_isDeclaration) { // FD
                initializeFunctionDeclaration(code, ec, function);
            } else { // FE
                function->set(strings->name.string(), code->m_nonAtomicName);
                PUSH(stack, topOfStack, function);
            }
            ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
            executeNextCode<CreateFunction>(programCounter);
            NEXT_INSTRUCTION();
        }

        InitObjectOpcodeLbl:
        {
            ESValue* value = POP(stack, bp);
            ESValue* key = POP(stack, bp);
            PEEK(stack, bp)->asESPointer()->asESObject()->defineDataProperty(*key, true, true, true, *value);
            executeNextCode<InitObject>(programCounter);
            NEXT_INSTRUCTION();
        }

        UnaryTypeOfOpcodeLbl:
        {
            ESValue* v = POP(stack, bp);
            PUSH(stack, topOfStack, typeOfOperation(v));
            executeNextCode<UnaryTypeOf>(programCounter);
            NEXT_INSTRUCTION();
        }

        UnaryDeleteOpcodeLbl:
        {
            UnaryDelete* code = (UnaryDelete*)currentCode;
            if (code->m_isDeleteObjectKey) {
                ESValue* key = POP(stack, bp);
                ESValue* obj = POP(stack, bp);
                bool res = obj->toObject()->deleteProperty(*key);
                if (!res)
                    throwObjectWriteError("Unable to delete property");
                PUSH(stack, topOfStack, ESValue(res));
            } else {
                PUSH(stack, topOfStack, ESValue(deleteBindingOperation(code, ec, globalObject)));
            }
            executeNextCode<UnaryDelete>(programCounter);
            NEXT_INSTRUCTION();
        }

        UnaryVoidOpcodeLbl:
        {
            POP(stack, bp);
            PUSH(stack, topOfStack, ESValue());
            executeNextCode<UnaryVoid>(programCounter);
            NEXT_INSTRUCTION();
        }

        StringInOpcodeLbl:
        {
            ESValue* obj = POP(stack, bp);
            ESValue* key = POP(stack, bp);
            PUSH(stack, topOfStack, ESValue(inOperation(obj, key)));
            executeNextCode<StringIn>(programCounter);
            NEXT_INSTRUCTION();
        }

        InstanceOfOpcodeLbl:
        {
            ESValue* rval = POP(stack, bp);
            ESValue* lval = POP(stack, bp);
            PUSH(stack, topOfStack, ESValue(instanceOfOperation(lval, rval)));
            executeNextCode<StringIn>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetByIdWithoutExceptionOpcodeLbl:
        {
            GetById* code = (GetById*)currentCode;
#ifndef ENABLE_ESJIT
            PUSH(stack, topOfStack, getByIdOperationWithNoInline(instance, ec, code));
#else
            ESValue value = getByIdOperationWithNoInline(instance, ec, code);
            PUSH(stack, topOfStack, value);
            code->m_profile.addProfile(value);
#endif
            executeNextCode<GetById>(programCounter);
            NEXT_INSTRUCTION();
        }

        LoadStackPointerOpcodeLbl:
        {
            LoadStackPointer* code = (LoadStackPointer *)currentCode;
            SUB_STACK(stack, bp, code->m_offsetToBasePointer);
            executeNextCode<LoadStackPointer>(programCounter);
            NEXT_INSTRUCTION();
        }

        CallBoundFunctionOpcodeLbl:
        {
            CallBoundFunction* code = (CallBoundFunction*)currentCode;
            size_t argc = code->m_boundArgumentsCount + instance->currentExecutionContext()->argumentCount();
            ESValue* mergedArguments = (ESValue *)alloca(sizeof(ESValue) * argc);
            if (code->m_boundArgumentsCount)
                memcpy(mergedArguments, code->m_boundArguments, sizeof(ESValue) * code->m_boundArgumentsCount);
            if (instance->currentExecutionContext()->argumentCount())
                memcpy(mergedArguments + code->m_boundArgumentsCount, instance->currentExecutionContext()->arguments(), sizeof(ESValue) * instance->currentExecutionContext()->argumentCount());
            return ESFunctionObject::call(instance, code->m_boundTargetFunction, code->m_boundThis, mergedArguments, argc, false);
        }

        TryOpcodeLbl:
        {
            Try* code = (Try *)currentCode;
            tryOperation(instance, codeBlock, codeBuffer, ec, programCounter, code, stackStorage, heapStorage);
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
            ESValue v = *POP(stack, bp);
            instance->throwError(v);
        }

        ThrowStaticOpcodeLbl:
        {
            ThrowStatic* code = (ThrowStatic*) currentCode;
            switch (code->m_code) {
            case ESErrorObject::Code::ReferenceError:
                instance->throwError(ReferenceError::create(code->m_msg));
            default:
                instance->throwError(ESErrorObject::create());
            }
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
                        instance->throwError(val);
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
            ESObject* obj = POP(stack, bp)->toObject();
            PUSH(stack, topOfStack, ESValue((ESPointer *)executeEnumerateObject(obj)));
            executeNextCode<EnumerateObject>(programCounter);
            NEXT_INSTRUCTION();
        }

        CheckIfKeyIsLastOpcodeLbl:
        {
            EnumerateObjectData* data = (EnumerateObjectData *)PEEK(stack, bp)->asESPointer();
            if (data->m_object->isESArrayObject() && data->m_object->asESArrayObject()->isFastmode()) {
                while (LIKELY(data->m_idx < data->m_keys.size())) {
                    uint32_t idx = data->m_keys[data->m_idx].toIndex();
                    if (idx == ESValue::ESInvalidIndexValue || idx >= data->m_object->asESArrayObject()->length())
                        break;

                    ESValue e = data->m_object->asESArrayObject()->data()[idx];
                    if (UNLIKELY(e.isEmpty())) {
                        data->m_idx++;
                        idx = data->m_keys[data->m_idx].toIndex();
                    } else
                        break;
                }
            }

            if (data->m_object->hiddenClass() != data->m_hiddenClass) {
                POP(stack, bp);
                EnumerateObjectData* newData = executeEnumerateObject(data->m_object);
                std::vector<ESValue, gc_allocator<ESValue> > differenceKeys;
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
                data = newData;
                data->m_keys = differenceKeys;
                PUSH(stack, topOfStack, ESValue((ESPointer *)data));
            }

            PUSH(stack, topOfStack, ESValue(data->m_keys.size() == data->m_idx));
            executeNextCode<CheckIfKeyIsLast>(programCounter);
            NEXT_INSTRUCTION();
        }

        EnumerateObjectKeyOpcodeLbl:
        {
            EnumerateObjectData* data = (EnumerateObjectData *)PEEK(stack, bp)->asESPointer();
            data->m_idx++;
            PUSH(stack, topOfStack, data->m_keys[data->m_idx - 1]);
#ifdef ENABLE_ESJIT
            EnumerateObjectKey* code = (EnumerateObjectKey*)currentCode;
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
                ESValue ret;
                if (argc > 0) {
                    if (arguments[0].isESString())
                        ret = instance->evaluateEval((arguments[0].asESString()), true);
                    else
                        ret = arguments[0];
                }
                PUSH(stack, topOfStack, ret);
            } else {
                ESObject* receiver = instance->globalObject();
                PUSH(stack, topOfStack, ESFunctionObject::call(instance, callee, receiver, arguments, argc, false));
            }
#ifdef ENABLE_ESJIT
            code->m_profile.addProfile(*PEEK(stack, bp));
#endif
            executeNextCode<CallEvalFunction>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetArgumentsObjectOpcodeLbl:
        {
            ESValue* argumentsBinding = ec->resolveArgumentsObjectBinding();
            if (argumentsBinding)
                PUSH(stack, topOfStack, argumentsBinding);
            else
                PUSH(stack, topOfStack, ESValue());
            executeNextCode<GetArgumentsObject>(programCounter);
            NEXT_INSTRUCTION();
        }

        SetArgumentsObjectOpcodeLbl:
        {
            ESValue* value = PEEK(stack, bp);
            *ec->resolveArgumentsObjectBinding() = *value;
            executeNextCode<SetArgumentsObject>(programCounter);
            NEXT_INSTRUCTION();
        }

        SetObjectPreComputedCaseSlowModeOpcodeLbl:
        {
            SetObjectPreComputedCaseSlowMode* code = (SetObjectPreComputedCaseSlowMode*)currentCode;
            const ESValue& value = *POP(stack, bp);
            ESValue* willBeObject = POP(stack, bp);
            ESValue v(code->m_propertyValue);
            setObjectOperationSlowMode(willBeObject, &v, value);
            PUSH(stack, topOfStack, value);
            executeNextCode<SetObjectPreComputedCaseSlowMode>(programCounter);
            NEXT_INSTRUCTION();
        }

        SetObjectSlowModeOpcodeLbl:
        {
            const ESValue& value = *POP(stack, bp);
            ESValue* property = POP(stack, bp);
            ESValue* willBeObject = POP(stack, bp);
            setObjectOperationSlowMode(willBeObject, property, value);
            PUSH(stack, topOfStack, value);
            executeNextCode<SetObjectSlowMode>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetObjectWithPeekingPreComputedCaseSlowModeOpcodeLbl:
        {
            GetObjectWithPeekingPreComputedCaseSlowMode* code = (GetObjectWithPeekingPreComputedCaseSlowMode*)currentCode;
            ESValue* willBeObject = POP(stack, bp);
            stack = (void *)(((size_t)stack) + sizeof(ESValue) * 1);
            ESValue v(code->m_propertyValue);
            PUSH(stack, topOfStack, getObjectOperationSlowMode(willBeObject, &v, globalObject));
            executeNextCode<GetObjectWithPeekingPreComputedCaseSlowMode>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetObjectPreComputedCaseSlowModeOpcodeLbl:
        {
            GetObjectPreComputedCaseSlowMode* code = (GetObjectPreComputedCaseSlowMode*)currentCode;
            ESValue* willBeObject = POP(stack, bp);
            ESValue v(code->m_propertyValue);
            PUSH(stack, topOfStack, getObjectOperationSlowMode(willBeObject, &v, globalObject));
            executeNextCode<GetObjectPreComputedCaseSlowMode>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetObjectPreComputedCaseAndPushObjectSlowModeOpcodeLbl:
        {
            GetObjectPreComputedCaseAndPushObjectSlowMode* code = (GetObjectPreComputedCaseAndPushObjectSlowMode*)currentCode;
            ESValue willBeObject = *POP(stack, bp);
            ESValue v(code->m_propertyValue);
            PUSH(stack, topOfStack, getObjectOperationSlowMode(&willBeObject, &v, globalObject));
            PUSH(stack, topOfStack, willBeObject);
            executeNextCode<GetObjectPreComputedCaseSlowMode>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetObjectWithPeekingSlowModeOpcodeLbl:
        {
            ESValue* property = POP(stack, bp);
            ESValue* willBeObject = POP(stack, bp);
            stack = (void *)(((size_t)stack) + sizeof(ESValue) * 2);
            PUSH(stack, topOfStack, getObjectOperationSlowMode(willBeObject, property, globalObject));
            executeNextCode<GetObjectWithPeekingSlowMode>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetObjectSlowModeOpcodeLbl:
        {
            ESValue* property = POP(stack, bp);
            ESValue* willBeObject = POP(stack, bp);
            PUSH(stack, topOfStack, getObjectOperationSlowMode(willBeObject, property, globalObject));
            executeNextCode<GetObjectSlowMode>(programCounter);
            NEXT_INSTRUCTION();
        }

        GetObjectAndPushObjectSlowModeOpcodeLbl:
        {
            ESValue* property = POP(stack, bp);
            ESValue willBeObject = *POP(stack, bp);
            PUSH(stack, topOfStack, getObjectOperationSlowMode(&willBeObject, property, globalObject));
            PUSH(stack, topOfStack, &willBeObject);
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
            ESValue* value = POP(stack, bp);
            ESValue* key = POP(stack, bp);
            ESObject* obj = PEEK(stack, bp)->asESPointer()->asESObject();
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
            ESValue* value = POP(stack, bp);
            ESValue* key = POP(stack, bp);
            ESObject* obj = PEEK(stack, bp)->asESPointer()->asESObject();
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

        LoopStartOpcodeLbl:
        {
            executeNextCode<LoopStart>(programCounter);
            NEXT_INSTRUCTION();
        }

        FakePopOpcodeLbl:
        {
            executeNextCode<FakePop>(programCounter);
            NEXT_INSTRUCTION();
        }

        EndOpcodeLbl:
        {
            ASSERT(stack == bp);
            return ESValue(ESValue::ESDeletedValue);
        }


        CheckStackPointerOpcodeLbl:
        {
            CheckStackPointer* byteCode = (CheckStackPointer *)currentCode;
#ifndef NDEBUG
            if (stack != bp) {
                printf("Stack is not equal to Base Point at the end of statement (%zd)\n", byteCode->m_lineNumber);
                RELEASE_ASSERT_NOT_REACHED();
            }
#endif
            executeNextCode<CheckStackPointer>(programCounter);
            NEXT_INSTRUCTION();
        }

        PrintSpAndBpOpcodeLbl:
        {
#ifndef NDEBUG
            printf("SP = %p, BP = %p\n", stack, bp);
#endif
            executeNextCode<PrintSpAndBp>(programCounter);
            NEXT_INSTRUCTION();
        }

    }

    RegisterOpcodes:
    {
        std::unordered_set<void *> labelAddressChecker;
#define REGISTER_TABLE(opcode, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
        registerOpcode(instance, opcode##Opcode, #opcode, &&opcode##OpcodeLbl);

        FOR_EACH_BYTECODE_OP(REGISTER_TABLE);
        return ESValue();
    }
}

}
