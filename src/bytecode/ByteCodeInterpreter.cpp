#include "Escargot.h"
#include "bytecode/ByteCode.h"

#include "ByteCodeOperations.h"

namespace escargot {

ESValue interpret(ESVMInstance* instance, CodeBlock* codeBlock, size_t programCounter)
{
    if(codeBlock == NULL) {
#define REGISTER_TABLE(opcode) \
        instance->opcodeTable()->m_table[opcode##Opcode] = &&opcode##OpcodeLbl; \
        instance->opcodeTable()->m_reverseTable[&&opcode##OpcodeLbl] = opcode##Opcode;
        FOR_EACH_BYTECODE_OP(REGISTER_TABLE);
        return ESValue();
    }

    char stackBuf[ESCARGOT_INTERPRET_STACK_SIZE];
    void* stack = stackBuf;
    void* bp = stack;
    char tmpStackBuf[ESCARGOT_INTERPRET_STACK_SIZE];
    void* tmpStack = tmpStackBuf;
    void* tmpBp = tmpStack;
    char* codeBuffer = codeBlock->m_code.data();
#ifdef ENABLE_ESJIT
    size_t numParams = codeBlock->m_params.size();
#endif
    ExecutionContext* ec = instance->currentExecutionContext();
    GlobalObject* globalObject = instance->globalObject();
    ESValue lastESObjectMetInMemberExpressionNode = ESValue();
    ESValue* lastExpressionStatementValue = &instance->m_lastExpressionStatementValue;
    ESValue* nonActivitionModeLocalValuePointer = ec->cachedDeclarativeEnvironmentRecordESValue();
    ESValue thisValue(ESValue::ESEmptyValue);
    ASSERT(((size_t)stack % sizeof(size_t)) == 0);
    ASSERT(((size_t)tmpStack % sizeof(size_t)) == 0);
    //resolve programCounter into address
    programCounter = (size_t)(&codeBuffer[programCounter]);
    NextInstruction:
    ByteCode* currentCode = (ByteCode *)programCounter;
    ASSERT(((size_t)currentCode % sizeof(size_t)) == 0);

#ifndef NDEBUG
    if(instance->m_dumpExecuteByteCode) {
        size_t tt = (size_t)currentCode;
        ASSERT(tt % sizeof(size_t) == 0);
        if(currentCode->m_node)
            printf("execute %p %u \t(nodeinfo %d)\t",currentCode, (unsigned)(programCounter-(size_t)codeBuffer), (int)currentCode->m_node->sourceLocation().m_lineNumber);
        else
            printf("execute %p %u \t(nodeinfo null)\t",currentCode, (unsigned)(programCounter-(size_t)codeBuffer));
        currentCode->dump();
    }

    if (currentCode->m_orgOpcode < 0 || currentCode->m_orgOpcode > OpcodeKindEnd) {
        printf("Error: unknown opcode\n");
        RELEASE_ASSERT_NOT_REACHED();
    } else {
#endif
    goto *currentCode->m_opcode;
#ifndef NDEBUG
    }
#endif

    PushOpcodeLbl:
    {
        Push* pushCode = (Push*)currentCode;
        push<ESValue>(stack, bp, pushCode->m_value);
        executeNextCode<Push>(programCounter);
        goto NextInstruction;
    }

    PopOpcodeLbl:
    {
        Pop* popCode = (Pop*)currentCode;
        pop<ESValue>(stack, bp);
        executeNextCode<Pop>(programCounter);
        goto NextInstruction;
    }

    DuplicateTopOfStackValueOpcodeLbl:
    {
        push<ESValue>(stack, bp, peek<ESValue>(stack, bp));
        executeNextCode<DuplicateTopOfStackValue>(programCounter);
        goto NextInstruction;
    }

    PopExpressionStatementOpcodeLbl:
    {
        ESValue* t = pop<ESValue>(stack, bp);
        *lastExpressionStatementValue = *t;
        executeNextCode<PopExpressionStatement>(programCounter);
        goto NextInstruction;
    }

    PushIntoTempStackOpcodeLbl:
    {
        push<ESValue>(tmpStack, tmpBp, pop<ESValue>(stack, bp));
        executeNextCode<PushIntoTempStack>(programCounter);
        goto NextInstruction;
    }

    PopFromTempStackOpcodeLbl:
    {
        push<ESValue>(stack, bp, pop<ESValue>(tmpStack, tmpBp));
        executeNextCode<PopFromTempStack>(programCounter);
        goto NextInstruction;
    }

    GetByIdOpcodeLbl:
    {
        GetById* code = (GetById*)currentCode;
        push<ESValue>(stack, bp, getByIdOperation(instance, ec, code));
        executeNextCode<GetById>(programCounter);
        goto NextInstruction;
    }

    GetByIndexOpcodeLbl:
    {
        GetByIndex* code = (GetByIndex*)currentCode;
        ASSERT(code->m_index < ec->environment()->record()->toDeclarativeEnvironmentRecord()->innerIdentifiers()->size());
        push<ESValue>(stack, bp, &nonActivitionModeLocalValuePointer[code->m_index]);
#ifdef ENABLE_ESJIT
        code->m_profile.addProfile(nonActivitionModeLocalValuePointer[code->m_index]);
#endif
        executeNextCode<GetByIndex>(programCounter);
        goto NextInstruction;
    }

    GetByIndexWithActivationOpcodeLbl:
    {
        GetByIndexWithActivation* code = (GetByIndexWithActivation*)currentCode;
        LexicalEnvironment* env = ec->environment();
        for(unsigned i = 0; i < code->m_upIndex; i ++) {
            env = env->outerEnvironment();
        }
        ASSERT(env->record()->isDeclarativeEnvironmentRecord());
        push<ESValue>(stack, bp, env->record()->toDeclarativeEnvironmentRecord()->bindingValueForActivationMode(code->m_index));
        executeNextCode<GetByIndexWithActivation>(programCounter);
        goto NextInstruction;
    }

    SetByIdOpcodeLbl:
    {
        SetById* code = (SetById*)currentCode;
        ESValue* value = peek<ESValue>(stack, bp);

        if (LIKELY(code->m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
            ASSERT(ec->resolveBinding(code->m_name, code->m_nonAtomicName) == code->m_cachedSlot);
            *code->m_cachedSlot = *value;
        } else {
            ExecutionContext* ec = instance->currentExecutionContext();
            //TODO
            //Object.defineProperty(this,"asdf",{value:1}) //this == global
            //asdf = 2
            ESValue* slot = ec->resolveBinding(code->m_name, code->m_nonAtomicName);

            if(LIKELY(slot != NULL)) {
                code->m_cachedSlot = slot;
                code->m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
                *code->m_cachedSlot = *value;
            } else {
                if(!ec->isStrictMode()) {
                    globalObject->defineDataProperty(code->m_nonAtomicName, true, true, true, *value);
                } else {
                    u16string err_msg;
                    err_msg.append(u"assignment to undeclared variable ");
                    err_msg.append(code->m_nonAtomicName->data());
                    throw ESValue(ReferenceError::create(ESString::create(std::move(err_msg))));
                }
            }
        }
        executeNextCode<SetById>(programCounter);
        goto NextInstruction;
    }

    SetByIndexOpcodeLbl:
    {
        SetByIndex* code = (SetByIndex*)currentCode;
        nonActivitionModeLocalValuePointer[code->m_index] = *peek<ESValue>(stack, bp);
        executeNextCode<SetByIndex>(programCounter);
        goto NextInstruction;
    }

    SetByIndexWithActivationOpcodeLbl:
    {
        SetByIndexWithActivation* code = (SetByIndexWithActivation*)currentCode;
        LexicalEnvironment* env = ec->environment();
        for(unsigned i = 0; i < code->m_upIndex; i ++) {
            env = env->outerEnvironment();
        }
        ASSERT(env->record()->isDeclarativeEnvironmentRecord());
        *env->record()->toDeclarativeEnvironmentRecord()->bindingValueForActivationMode(code->m_index) = *peek<ESValue>(stack, bp);
        executeNextCode<SetByIndexWithActivation>(programCounter);
        goto NextInstruction;
    }

    CreateBindingOpcodeLbl:
    {
        CreateBinding* code = (CreateBinding*)currentCode;
        ec->environment()->record()->createMutableBindingForAST(code->m_name,
                code->m_nonAtomicName, false);
        executeNextCode<CreateBinding>(programCounter);
        goto NextInstruction;
    }

    EqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);

        push<ESValue>(stack, bp, ESValue(left->abstractEqualsTo(*right)));
        executeNextCode<Equal>(programCounter);
        goto NextInstruction;
    }

    NotEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, ESValue(!left->abstractEqualsTo(*right)));
        executeNextCode<NotEqual>(programCounter);
        goto NextInstruction;
    }

    StrictEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, ESValue(left->equalsTo(*right)));
        executeNextCode<StrictEqual>(programCounter);
        goto NextInstruction;
    }

    NotStrictEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, ESValue(!left->equalsTo(*right)));
        executeNextCode<NotStrictEqual>(programCounter);
        goto NextInstruction;
    }

    BitwiseAndOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, ESValue(left->toInt32() & right->toInt32()));
        executeNextCode<BitwiseAnd>(programCounter);
        goto NextInstruction;
    }

    BitwiseOrOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, ESValue(left->toInt32() | right->toInt32()));
        executeNextCode<BitwiseOr>(programCounter);
        goto NextInstruction;
    }

    BitwiseXorOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, ESValue(left->toInt32() ^ right->toInt32()));
        executeNextCode<BitwiseXor>(programCounter);
        goto NextInstruction;
    }

    LeftShiftOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        int32_t lnum = left->toInt32();
        int32_t rnum = right->toInt32();
        lnum <<= ((unsigned int)rnum) & 0x1F;
        push<ESValue>(stack, bp, ESValue(lnum));
        executeNextCode<LeftShift>(programCounter);
        goto NextInstruction;
    }

    SignedRightShiftOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        int32_t lnum = left->toInt32();
        int32_t rnum = right->toInt32();
        lnum >>= ((unsigned int)rnum) & 0x1F;
        push<ESValue>(stack, bp, ESValue(lnum));
        executeNextCode<SignedRightShift>(programCounter);
        goto NextInstruction;
    }

    UnsignedRightShiftOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        uint32_t lnum = left->toUint32();
        uint32_t rnum = right->toUint32();
        lnum = (lnum) >> ((rnum) & 0x1F);
        push<ESValue>(stack, bp, ESValue(lnum));
        executeNextCode<UnsignedRightShift>(programCounter);
        goto NextInstruction;
    }

    LessThanOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        ESValue r = abstractRelationalComparison(*left, *right, true);
        if(r.isUndefined())
            push<ESValue>(stack, bp, ESValue(false));
        else
            push<ESValue>(stack, bp, r);
        executeNextCode<LessThan>(programCounter);
        goto NextInstruction;
    }

    LessThanOrEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        ESValue r = abstractRelationalComparison(*right, *left, false);
        if(r == ESValue(true) || r.isUndefined())
            push<ESValue>(stack, bp, ESValue(false));
        else
            push<ESValue>(stack, bp, ESValue(true));
        executeNextCode<LessThanOrEqual>(programCounter);
        goto NextInstruction;
    }

    GreaterThanOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);

        ESValue r = abstractRelationalComparison(*right, *left, false);
        if(r.isUndefined())
            push<ESValue>(stack, bp, ESValue(false));
        else
            push<ESValue>(stack, bp, r);
        executeNextCode<GreaterThan>(programCounter);
        goto NextInstruction;
    }

    GreaterThanOrEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);

        ESValue r = abstractRelationalComparison(*left, *right, true);
        if(r == ESValue(true) || r.isUndefined())
            push<ESValue>(stack, bp, ESValue(false));
        else
            push<ESValue>(stack, bp, ESValue(true));
        executeNextCode<GreaterThanOrEqual>(programCounter);
        goto NextInstruction;
    }

    PlusOpcodeLbl:
    {
        push<ESValue>(stack, bp, plusOperation(*pop<ESValue>(stack, bp), *pop<ESValue>(stack, bp)));
        executeNextCode<Plus>(programCounter);
        goto NextInstruction;
    }

    MinusOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, minusOperation(*left, *right));
        executeNextCode<Minus>(programCounter);
        goto NextInstruction;
    }

    MultiplyOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);

        push<ESValue>(stack, bp, ESValue(left->toNumber() * right->toNumber()));
        executeNextCode<Multiply>(programCounter);
        goto NextInstruction;
    }

    DivisionOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, ESValue(left->toNumber() / right->toNumber()));
        executeNextCode<Division>(programCounter);
        goto NextInstruction;
    }

    ModOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, modOperation(*left, *right));
        executeNextCode<Mod>(programCounter);
        goto NextInstruction;
    }

    IncrementOpcodeLbl:
    {
        ESValue* src = pop<ESValue>(stack, bp);

        ESValue ret(ESValue::ESForceUninitialized);
        if(src->isInt32()) {
            int32_t a = src->asInt32();
            if(a == std::numeric_limits<int32_t>::max())
                ret = ESValue(ESValue::EncodeAsDouble, ((double)a) + 1);
            else
                ret = ESValue(a + 1);
        } else {
            ret = ESValue(ESValue::EncodeAsDouble, src->asNumber() + 1);
        }
        push<ESValue>(stack, bp, ret);
        executeNextCode<Increment>(programCounter);
        goto NextInstruction;
    }

    DecrementOpcodeLbl:
    {
        ESValue* src = pop<ESValue>(stack, bp);
        ESValue ret(ESValue::ESForceUninitialized);
        if(src->isInt32()) {
            int32_t a = src->asInt32();
            if(a == std::numeric_limits<int32_t>::min())
                ret = ESValue(ESValue::EncodeAsDouble, ((double)a) - 1);
            else
                ret = ESValue(a - 1);
        } else {
            ret = ESValue(ESValue::EncodeAsDouble, src->asNumber() - 1);
        }
        push<ESValue>(stack, bp, ret);
        executeNextCode<Decrement>(programCounter);
        goto NextInstruction;
    }

    BitwiseNotOpcodeLbl:
    {
        push<ESValue>(stack, bp, ESValue(~pop<ESValue>(stack, bp)->toInt32()));
        executeNextCode<BitwiseNot>(programCounter);
        goto NextInstruction;
    }

    LogicalNotOpcodeLbl:
    {
        push<ESValue>(stack, bp, ESValue(!pop<ESValue>(stack, bp)->toBoolean()));
        executeNextCode<LogicalNot>(programCounter);
        goto NextInstruction;
    }

    UnaryMinusOpcodeLbl:
    {
        push<ESValue>(stack, bp, ESValue(-pop<ESValue>(stack, bp)->toNumber()));
        executeNextCode<UnaryMinus>(programCounter);
        goto NextInstruction;
    }

    UnaryPlusOpcodeLbl:
    {
        push<ESValue>(stack, bp, ESValue(pop<ESValue>(stack, bp)->toNumber()));
        executeNextCode<UnaryPlus>(programCounter);
        goto NextInstruction;
    }

    ToNumberOpcodeLbl:
    {
        ESValue* v = peek<ESValue>(stack, bp);
        if(!v->isNumber()) {
            v = pop<ESValue>(stack, bp);
            push<ESValue>(stack, bp, ESValue(v->toNumber()));
        }

        executeNextCode<ToNumber>(programCounter);
        goto NextInstruction;
    }

    ThisOpcodeLbl:
    {
        if(UNLIKELY(thisValue.isEmpty())) {
            thisValue = ec->resolveThisBinding();
        }
        push<ESValue>(stack, bp, thisValue);
        executeNextCode<This>(programCounter);
        goto NextInstruction;
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
        ESObject* obj = ESObject::create(code->m_keyCount);
        push<ESValue>(stack, bp, obj);
        executeNextCode<CreateObject>(programCounter);
        goto NextInstruction;
    }

    CreateArrayOpcodeLbl:
    {
        CreateArray* code = (CreateArray*)currentCode;
        ESArrayObject* arr = ESArrayObject::create(code->m_keyCount);
        push<ESValue>(stack, bp, arr);
        executeNextCode<CreateArray>(programCounter);
        goto NextInstruction;
    }



    GetObjectOpcodeLbl:
    {
        GetObject* code = (GetObject*)currentCode;

        ESValue* property = pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, getObjectOperation(willBeObject, property, &lastESObjectMetInMemberExpressionNode, globalObject));
        executeNextCode<GetObject>(programCounter);
        goto NextInstruction;
    }

    GetObjectPreComputedCaseOpcodeLbl:
    {
        GetObjectPreComputedCase* code = (GetObjectPreComputedCase*)currentCode;
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, getObjectPreComputedCaseOperation(willBeObject, code->m_propertyValue, &lastESObjectMetInMemberExpressionNode, globalObject,
                &code->m_cachedhiddenClassChain, &code->m_cachedIndex));
        executeNextCode<GetObjectPreComputedCase>(programCounter);
        goto NextInstruction;
    }

    GetObjectWithPeekingOpcodeLbl:
    {
        GetObjectWithPeeking* code = (GetObjectWithPeeking*)currentCode;

        ESValue* property = pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);

        stack = (void *)(((size_t)stack) + sizeof(ESValue) * 2);
#ifndef NDEBUG
        stack = (void *)(((size_t)stack) + sizeof(size_t) * 2);
#endif
        push<ESValue>(stack, bp, getObjectOperation(willBeObject, property, &lastESObjectMetInMemberExpressionNode, globalObject));
        executeNextCode<GetObjectWithPeeking>(programCounter);
        goto NextInstruction;
    }

    GetObjectWithPeekingPreComputedCaseOpcodeLbl:
    {
        GetObjectWithPeekingPreComputedCase* code = (GetObjectWithPeekingPreComputedCase*)currentCode;

        ESValue* willBeObject = pop<ESValue>(stack, bp);

        stack = (void *)(((size_t)stack) + sizeof(ESValue) * 1);
#ifndef NDEBUG
        stack = (void *)(((size_t)stack) + sizeof(size_t) * 1);
#endif

        push<ESValue>(stack, bp, getObjectPreComputedCaseOperationWithNeverInline(willBeObject, code->m_propertyValue, &lastESObjectMetInMemberExpressionNode, globalObject,
                        &code->m_cachedhiddenClassChain, &code->m_cachedIndex));
        executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
        goto NextInstruction;
    }

    SetObjectOpcodeLbl:
    {
        SetObject* code = (SetObject*)currentCode;
        ESValue value = *pop<ESValue>(stack, bp);
        ESValue* property = pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        setObjectOperation(willBeObject, property, value);
        push<ESValue>(stack, bp, value);
        executeNextCode<SetObject>(programCounter);
        goto NextInstruction;
    }

    SetObjectPreComputedCaseOpcodeLbl:
    {
        SetObjectPreComputedCase* code = (SetObjectPreComputedCase*)currentCode;
        ESValue value = *pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        setObjectPreComputedCaseOperation(willBeObject, code->m_propertyValue, value, &code->m_cachedhiddenClassChain
                , &code->m_cachedIndex, &code->m_hiddenClassWillBe);
        push<ESValue>(stack, bp, value);
        executeNextCode<SetObjectPreComputedCase>(programCounter);
        goto NextInstruction;
    }

    CreateFunctionOpcodeLbl:
    {
        CreateFunction* code = (CreateFunction*)currentCode;
        ASSERT(((size_t)code->m_codeBlock % sizeof(size_t)) == 0);
        ESFunctionObject* function = ESFunctionObject::create(ec->environment(), code->m_codeBlock, code->m_nonAtomicName == NULL ? strings->emptyString.string() : code->m_nonAtomicName, code->m_codeBlock->m_params.size());
        if(code->m_isDeclaration) { //FD
            function->set(strings->name.string(), code->m_nonAtomicName);
            ec->environment()->record()->setMutableBinding(code->m_name, code->m_nonAtomicName, function, false);
        }
        else {//FE
            function->set(strings->name.string(), code->m_nonAtomicName);
            push<ESValue>(stack, bp, function);
        }
        executeNextCode<CreateFunction>(programCounter);
        goto NextInstruction;
    }

    ExecuteNativeFunctionOpcodeLbl:
    {
        ExecuteNativeFunction* code = (ExecuteNativeFunction*)currentCode;
        ASSERT(bp == stack);
        return code->m_fn(instance);
    }

    PrepareFunctionCallOpcodeLbl:
    {
        lastESObjectMetInMemberExpressionNode = ESValue();
        executeNextCode<PrepareFunctionCall>(programCounter);
        goto NextInstruction;
    }

    PushFunctionCallReceiverOpcodeLbl:
    {
        push<ESValue>(stack, bp, lastESObjectMetInMemberExpressionNode);
        executeNextCode<PushFunctionCallReceiver>(programCounter);
        goto NextInstruction;
    }

    CallFunctionOpcodeLbl:
    {
        size_t argc = (size_t)pop<ESValue>(stack, bp)->asInt32();
#ifdef NDEBUG
        stack = (void *)((size_t)stack - argc * sizeof(ESValue));
#else
        stack = (void *)((size_t)stack - argc * sizeof(ESValue));
        stack = (void *)((size_t)stack - argc * sizeof(size_t));

        {
            ESValue* arguments = (ESValue *)stack;
            for(size_t i = 0; i < argc ; i ++) {
                arguments[i] = *((ESValue *)&(((char *)stack)[i*(sizeof(ESValue)+sizeof(size_t))]));
            }
        }
#endif
        ESValue* arguments = (ESValue *)stack;
        ESValue* receiver = pop<ESValue>(stack, bp);
        ESValue result = ESFunctionObject::call(instance, *pop<ESValue>(stack, bp), *receiver, arguments, argc, false);
#ifdef ENABLE_ESJIT
        CallFunction* code = (CallFunction*)currentCode;
        code->m_profile.addProfile(result);
#endif
        push<ESValue>(stack, bp, result);
        executeNextCode<CallFunction>(programCounter);
        goto NextInstruction;
    }

    NewFunctionCallOpcodeLbl:
    {
        size_t argc = (size_t)pop<ESValue>(stack, bp)->asInt32();
#ifdef NDEBUG
        stack = (void *)((size_t)stack - argc * sizeof(ESValue));
#else
        stack = (void *)((size_t)stack - argc * sizeof(ESValue));
        stack = (void *)((size_t)stack - argc * sizeof(size_t));

        {
            ESValue* arguments = (ESValue *)stack;
            for(size_t i = 0; i < argc ; i ++) {
                arguments[i] = *((ESValue *)&(((char *)stack)[i*(sizeof(ESValue)+sizeof(size_t))]));
            }
        }
#endif
        ESValue* arguments = (ESValue *)stack;
        ESValue fn = *pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, newOperation(instance, globalObject, fn, arguments, argc));
        executeNextCode<NewFunctionCall>(programCounter);
        goto NextInstruction;
    }

    JumpOpcodeLbl:
    {
        Jump* code = (Jump *)currentCode;
        ASSERT(code->m_jumpPosition != SIZE_MAX);

        programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
        goto NextInstruction;
    }

    JumpComplexCaseOpcodeLbl:
    {
        JumpComplexCase* code = (JumpComplexCase*)currentCode;
        ec->tryOrCatchBodyResult() = code->m_controlFlowRecord->clone();
        //TODO add check stack pointer;
        return ESValue(ESValue::ESEmptyValue);
    }

    JumpIfTopOfStackValueIsFalseOpcodeLbl:
    {
        JumpIfTopOfStackValueIsFalse* code = (JumpIfTopOfStackValueIsFalse *)currentCode;
        ESValue* top = pop<ESValue>(stack, bp);
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        if(!top->toBoolean())
            programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
        else
            executeNextCode<JumpIfTopOfStackValueIsFalse>(programCounter);
        goto NextInstruction;
    }

    JumpIfTopOfStackValueIsTrueOpcodeLbl:
    {
        JumpIfTopOfStackValueIsTrue* code = (JumpIfTopOfStackValueIsTrue *)currentCode;
        ESValue* top = pop<ESValue>(stack, bp);
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        if(top->toBoolean())
            programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
        else
            executeNextCode<JumpIfTopOfStackValueIsTrue>(programCounter);
        goto NextInstruction;
    }

    JumpAndPopIfTopOfStackValueIsTrueOpcodeLbl:
    {
        JumpAndPopIfTopOfStackValueIsTrue* code = (JumpAndPopIfTopOfStackValueIsTrue *)currentCode;
        ESValue* top = pop<ESValue>(stack, bp);
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        if(top->toBoolean()) {
            programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
            pop<ESValue>(stack, bp);
        }
        else
            executeNextCode<JumpAndPopIfTopOfStackValueIsTrue>(programCounter);
        goto NextInstruction;
    }

    JumpIfTopOfStackValueIsFalseWithPeekingOpcodeLbl:
    {
        JumpIfTopOfStackValueIsFalseWithPeeking* code = (JumpIfTopOfStackValueIsFalseWithPeeking *)currentCode;
        ESValue* top = peek<ESValue>(stack, bp);
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        if(!top->toBoolean())
            programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
        else
            executeNextCode<JumpIfTopOfStackValueIsFalseWithPeeking>(programCounter);
        goto NextInstruction;
    }

    JumpIfTopOfStackValueIsTrueWithPeekingOpcodeLbl:
    {
        JumpIfTopOfStackValueIsTrueWithPeeking* code = (JumpIfTopOfStackValueIsTrueWithPeeking *)currentCode;
        ESValue* top = peek<ESValue>(stack, bp);
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        if(top->toBoolean())
            programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
        else
            executeNextCode<JumpIfTopOfStackValueIsTrueWithPeeking>(programCounter);
        goto NextInstruction;
    }

    InitObjectOpcodeLbl:
    {
        ESValue* value = pop<ESValue>(stack, bp);
        ESValue* key = pop<ESValue>(stack, bp);
        peek<ESValue>(stack, bp)->asESPointer()->asESObject()->set(*key, *value);
        executeNextCode<InitObject>(programCounter);
        goto NextInstruction;
    }

    SetObjectPropertySetterOpcodeLbl:
    {
        ESValue* value = pop<ESValue>(stack, bp);
        ESValue* key = pop<ESValue>(stack, bp);

        ESObject* obj = peek<ESValue>(stack, bp)->asESPointer()->asESObject();
        RELEASE_ASSERT_NOT_REACHED();
        /*
        if(obj->hasOwnProperty(*key)) {
            //TODO check property is accessor property
            //TODO check accessor already exists
            obj->accessorData(*key)->setJSSetter(value->asESPointer()->asESFunctionObject());
        } else {
            obj->defineAccessorProperty(key->toString(), NULL, value->asESPointer()->asESFunctionObject(), true, true, true);
        }
*/
        executeNextCode<SetObjectPropertySetter>(programCounter);
        goto NextInstruction;
    }

    SetObjectPropertyGetterOpcodeLbl:
    {
        ESValue* value = pop<ESValue>(stack, bp);
        ESValue* key = pop<ESValue>(stack, bp);
        ESObject* obj = peek<ESValue>(stack, bp)->asESPointer()->asESObject();

        RELEASE_ASSERT_NOT_REACHED();
        /*
        if(obj->hasOwnProperty(*key)) {
            //TODO check property is accessor property
            //TODO check accessor already exists
            obj->accessorData(*key)->setJSGetter(value->asESPointer()->asESFunctionObject());
        } else {
            obj->defineAccessorProperty(key->toString(), value->asESPointer()->asESFunctionObject(), NULL, true, true, true);
        }*/
        executeNextCode<SetObjectPropertyGetter>(programCounter);
        goto NextInstruction;
    }

    UnaryTypeOfOpcodeLbl:
    {
        ESValue* v = pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, typeOfOperation(v));
        executeNextCode<UnaryTypeOf>(programCounter);
        goto NextInstruction;
    }

    UnaryDeleteOpcodeLbl:
    {
        ESValue* key = pop<ESValue>(stack, bp);
        ESValue* obj = pop<ESValue>(stack, bp);
        bool res = obj->toObject()->deleteProperty(*key);
        push<ESValue>(stack, bp, ESValue(res));

        executeNextCode<UnaryDelete>(programCounter);
        goto NextInstruction;
    }

    UnaryVoidOpcodeLbl:
    {
        ESValue* res = pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, ESValue());

        executeNextCode<UnaryDelete>(programCounter);
        goto NextInstruction;
    }

    StringInOpcodeLbl:
    {
        ESValue* obj = pop<ESValue>(stack, bp);
        ESValue* key = pop<ESValue>(stack, bp);

        push<ESValue>(stack, bp, ESValue(inOperation(obj, key)));
        executeNextCode<StringIn>(programCounter);
        goto NextInstruction;
    }

    InstanceOfOpcodeLbl:
    {
        ESValue* rval = pop<ESValue>(stack, bp);
        ESValue* lval = pop<ESValue>(stack, bp);

        push<ESValue>(stack, bp, ESValue(instanceOfOperation(lval, rval)));

        executeNextCode<StringIn>(programCounter);
        goto NextInstruction;
    }

    GetByIdWithoutExceptionOpcodeLbl:
    {
        GetById* code = (GetById*)currentCode;
        try {
            push<ESValue>(stack, bp, getByIdOperationWithNoInline(instance, ec, code));
        } catch(...) {
            push<ESValue>(stack, bp, ESValue(ESValue::ESEmptyValue));
        }
        executeNextCode<GetById>(programCounter);
        goto NextInstruction;
    }

    LoadStackPointerOpcodeLbl:
    {
        LoadStackPointer* code = (LoadStackPointer *)currentCode;
        sub<ESValue>(stack, bp, code->m_offsetToBasePointer);
        executeNextCode<LoadStackPointer>(programCounter);
        goto NextInstruction;
    }

    CheckStackPointerOpcodeLbl:
    {
        CheckStackPointer* byteCode = (CheckStackPointer *)currentCode;
        if (stack != bp) {
            printf("Stack is not equal to Base Point at the end of statement (%ld)\n", byteCode->m_lineNumber);
            RELEASE_ASSERT_NOT_REACHED();
         }

        executeNextCode<CheckStackPointer>(programCounter);
        goto NextInstruction;
    }

    PrintSpAndBpOpcodeLbl:
    {
        printf("SP = %p, BP = %p\n", stack, bp);

        executeNextCode<PrintSpAndBp>(programCounter);
        goto NextInstruction;
    }

    LoopStartOpcodeLbl:
    {
        executeNextCode<LoopStart>(programCounter);
        goto NextInstruction;
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
        goto NextInstruction;
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
        if(ec->tryOrCatchBodyResult().isEmpty()) {
            executeNextCode<FinallyEnd>(programCounter);
            goto NextInstruction;
        } else {
            ASSERT(ec->tryOrCatchBodyResult().asESPointer()->isESControlFlowRecord());
            ESControlFlowRecord* record = ec->tryOrCatchBodyResult().asESPointer()->asESControlFlowRecord();
            int32_t dupCnt = record->value2().asInt32();
            if(dupCnt <= 1) {
                if(record->reason() == ESControlFlowRecord::ControlFlowReason::NeedsReturn) {
                    ESValue ret = record->value();
                    ec->tryOrCatchBodyResult() = ESValue(ESValue::ESEmptyValue);
                    //TODO sp check
                    return ret;
                } else if(record->reason() == ESControlFlowRecord::ControlFlowReason::NeedsThrow) {
                    ESValue val = record->value();
                    ec->tryOrCatchBodyResult() = ESValue(ESValue::ESEmptyValue);
                    throw val;
                } else {
                    ASSERT(record->reason() == ESControlFlowRecord::ControlFlowReason::NeedsJump);
                    programCounter = jumpTo(codeBuffer, (size_t)record->value().asESPointer());
                    ec->tryOrCatchBodyResult() = ESValue(ESValue::ESEmptyValue);
                    goto NextInstruction;
                }

            } else {
                dupCnt--;
                record->setValue2(ESValue((int32_t)dupCnt));
                return ESValue(ESValue::ESEmptyValue);
            }

        }
    }

    EnumerateObjectOpcodeLbl:
    {
        ESObject* obj = pop<ESValue>(stack, bp)->toObject();
        push<ESValue>(stack, bp, ESValue((ESPointer *)executeEnumerateObject(obj)));
        executeNextCode<EnumerateObject>(programCounter);
        goto NextInstruction;
    }

    EnumerateObjectKeyOpcodeLbl:
    {
        EnumerateObjectKey* code = (EnumerateObjectKey*)currentCode;
        EnumerateObjectData* data = (EnumerateObjectData *)peek<ESValue>(stack, bp)->asESPointer();

        while(1) {
            if(data->m_keys.size() == data->m_idx) {
                programCounter = jumpTo(codeBuffer, code->m_forInEnd);
                goto NextInstruction;
            }

            data->m_idx++;
            push<ESValue>(stack, bp, data->m_keys[data->m_idx - 1]);
            executeNextCode<EnumerateObjectKey>(programCounter);
            goto NextInstruction;
        }
    }

    EnumerateObjectEndOpcodeLbl:
    {
        pop<ESValue>(stack, bp);
        executeNextCode<EnumerateObjectEnd>(programCounter);
        goto NextInstruction;
    }

    CallEvalFunctionOpcodeLbl:
    {
        size_t argc = (size_t)pop<ESValue>(stack, bp)->asInt32();
#ifdef NDEBUG
        stack = (void *)((size_t)stack - argc * sizeof(ESValue));
#else
        stack = (void *)((size_t)stack - argc * sizeof(ESValue));
        stack = (void *)((size_t)stack - argc * sizeof(size_t));

        {
            ESValue* arguments = (ESValue *)stack;
            for(size_t i = 0; i < argc ; i ++) {
                arguments[i] = *((ESValue *)&(((char *)stack)[i*(sizeof(ESValue)+sizeof(size_t))]));
            }
        }
#endif
        ESValue* arguments = (ESValue *)stack;

        ESValue callee = *ec->resolveBinding(strings->eval, strings->eval.string());
        if(callee.isESPointer() && (void *)callee.asESPointer() == (void *)globalObject->eval()) {
            ESObject* receiver = instance->globalObject();
            ESValue ret = instance->runOnEvalContext([instance, &arguments, &argc](){
                ESValue ret;
                if(argc)
                    ret = instance->evaluate(const_cast<u16string &>(arguments[0].asESString()->string()));
                return ret;
            }, true);
            push<ESValue>(stack, bp, ret);
        } else {
            ESObject* receiver = instance->globalObject();
            push<ESValue>(stack, bp, ESFunctionObject::call(instance, callee, receiver, arguments, argc, false));
        }

        executeNextCode<CallEvalFunction>(programCounter);
        goto NextInstruction;
    }

    GetArgumentsObjectOpcodeLbl:
    {
        push<ESValue>(stack, bp, ec->resolveArgumentsObjectBinding());
        executeNextCode<GetArgumentsObject>(programCounter);
        goto NextInstruction;
    }

    SetArgumentsObjectOpcodeLbl:
    {
        ESValue* value = peek<ESValue>(stack, bp);
        *ec->resolveArgumentsObjectBinding() = *value;
        executeNextCode<SetArgumentsObject>(programCounter);
        goto NextInstruction;
    }

    SetObjectPreComputedCaseSlowModeOpcodeLbl:
    {
        SetObjectPreComputedCaseSlowMode* code = (SetObjectPreComputedCaseSlowMode*)currentCode;
        ESValue value = *pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        ESValue v(code->m_propertyValue);
        setObjectOperationSlowMode(willBeObject, &v, value);
        push<ESValue>(stack, bp, value);
        executeNextCode<SetObjectPreComputedCaseSlowMode>(programCounter);
        goto NextInstruction;
    }

    SetObjectSlowModeOpcodeLbl:
    {
        SetObjectSlowMode* code = (SetObjectSlowMode*)currentCode;
        ESValue value = *pop<ESValue>(stack, bp);
        ESValue* property = pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        setObjectOperationSlowMode(willBeObject, property, value);
        push<ESValue>(stack, bp, value);
        executeNextCode<SetObjectSlowMode>(programCounter);
        goto NextInstruction;
    }

    GetObjectWithPeekingPreComputedCaseSlowModeOpcodeLbl:
    {
        GetObjectWithPeekingPreComputedCaseSlowMode* code = (GetObjectWithPeekingPreComputedCaseSlowMode*)currentCode;

        ESValue* willBeObject = pop<ESValue>(stack, bp);

        stack = (void *)(((size_t)stack) + sizeof(ESValue) * 1);
#ifndef NDEBUG
        stack = (void *)(((size_t)stack) + sizeof(size_t) * 1);
#endif

        ESValue v(code->m_propertyValue);
        push<ESValue>(stack, bp, getObjectOperationSlowMode(willBeObject, &v, &lastESObjectMetInMemberExpressionNode, globalObject));
        executeNextCode<GetObjectWithPeekingPreComputedCaseSlowMode>(programCounter);
        goto NextInstruction;
    }

    GetObjectPreComputedCaseSlowModeOpcodeLbl:
    {
        GetObjectPreComputedCaseSlowMode* code = (GetObjectPreComputedCaseSlowMode*)currentCode;
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        ESValue v(code->m_propertyValue);
        push<ESValue>(stack, bp, getObjectOperationSlowMode(willBeObject, &v, &lastESObjectMetInMemberExpressionNode, globalObject));
        executeNextCode<GetObjectPreComputedCaseSlowMode>(programCounter);
        goto NextInstruction;
    }

    GetObjectWithPeekingSlowModeOpcodeLbl:
    {
        GetObjectWithPeekingSlowMode* code = (GetObjectWithPeekingSlowMode*)currentCode;

        ESValue* property = pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);

        stack = (void *)(((size_t)stack) + sizeof(ESValue) * 2);
#ifndef NDEBUG
        stack = (void *)(((size_t)stack) + sizeof(size_t) * 2);
#endif
        push<ESValue>(stack, bp, getObjectOperationSlowMode(willBeObject, property, &lastESObjectMetInMemberExpressionNode, globalObject));
        executeNextCode<GetObjectWithPeekingSlowMode>(programCounter);
        goto NextInstruction;
    }

    GetObjectSlowModeOpcodeLbl:
    {
        GetObjectSlowMode* code = (GetObjectSlowMode*)currentCode;

        ESValue* property = pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, getObjectOperationSlowMode(willBeObject, property, &lastESObjectMetInMemberExpressionNode, globalObject));
        executeNextCode<GetObjectSlowMode>(programCounter);
        goto NextInstruction;
    }

    EndOpcodeLbl:
    {
        ASSERT(stack == bp);
        return ESValue();
    }

}

}
