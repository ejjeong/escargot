#include "Escargot.h"
#include "bytecode/ByteCode.h"

#include "runtime/Operations.h"

namespace escargot {

ESValue interpret(ESVMInstance* instance, CodeBlock* codeBlock, size_t programCounter)
{
    if(codeBlock == NULL) {
#define REGISTER_TABLE(opcode) \
        instance->opcodeTable()->m_table[opcode##Opcode] = &&opcode##OpcodeLbl;
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
    ESValue lastESObjectMetInMemberExpressionNode = globalObject;
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

    NoOp0OpcodeLbl:
    {
        executeNextCode<NoOp0>(programCounter);
        goto NextInstruction;
    }

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

    GetByIdOpcodeLbl:
    {
        GetById* code = (GetById*)currentCode;
        if (LIKELY(code->m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
            ASSERT(ec->resolveBinding(code->m_name, code->m_nonAtomicName) == code->m_cachedSlot);
            push<ESValue>(stack, bp, code->m_cachedSlot);
#ifdef ENABLE_ESJIT
            code->m_profile.addProfile(*code->m_cachedSlot);
#endif
        } else {
            ESValue* slot = ec->resolveBinding(code->m_name, code->m_nonAtomicName);
            if(LIKELY(slot != NULL)) {
                code->m_cachedSlot = slot;
                code->m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
                push<ESValue>(stack, bp, code->m_cachedSlot);
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
                receiver->set(strings->message, ESString::create(std::move(err_msg)));
                throw ESValue(receiver);
            }
        }
        executeNextCode<GetById>(programCounter);
        goto NextInstruction;
    }

    GetByIdWithoutExceptionOpcodeLbl:
    {
        GetById* code = (GetById*)currentCode;
        if (LIKELY(code->m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
            ASSERT(ec->resolveBinding(code->m_name, code->m_nonAtomicName) == code->m_cachedSlot);
            push<ESValue>(stack, bp, code->m_cachedSlot);
        } else {
            ESValue* slot = ec->resolveBinding(code->m_name, code->m_nonAtomicName);
            if(LIKELY(slot != NULL)) {
                code->m_cachedSlot = slot;
                code->m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
                push<ESValue>(stack, bp, code->m_cachedSlot);
            } else {
                push<ESValue>(stack, bp, ESValue(ESValue::ESEmptyValue));
            }
        }
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

    GetArgumentsObjectOpcodeLbl:
    {
        push<ESValue>(stack, bp, ec->resolveArgumentsObjectBinding());
        executeNextCode<GetArgumentsObject>(programCounter);
        goto NextInstruction;
    }

    PutByIdOpcodeLbl:
    {
        PutById* code = (PutById*)currentCode;
        ESValue* value = peek<ESValue>(stack, bp);

        if (LIKELY(code->m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
            ASSERT(ec->resolveBinding(code->m_name, code->m_nonAtomicName) == code->m_cachedSlot);
            *code->m_cachedSlot = *value;
        } else {
            ExecutionContext* ec = instance->currentExecutionContext();
            ESValue* slot = ec->resolveBinding(code->m_name, code->m_nonAtomicName);

            if(LIKELY(slot != NULL)) {
                code->m_cachedSlot = slot;
                code->m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
                *code->m_cachedSlot = *value;
            } else {
                //CHECKTHIS true, true, false is right?
                if(!ec->isStrictMode()) {
                    instance->invalidateIdentifierCacheCheckCount();
                    globalObject->definePropertyOrThrow(code->m_nonAtomicName, true, true, true, *value);
                } else {
                    //ReferenceError: assignment to undeclared variable d
                    u16string err_msg;
                    err_msg.append(u"assignment to undeclared variable ");
                    err_msg.append(code->m_nonAtomicName->data());
                    throw ESValue(ReferenceError::create(ESString::create(std::move(err_msg))));
                }
            }
        }
        executeNextCode<PutById>(programCounter);
        goto NextInstruction;
    }

    PutByIndexOpcodeLbl:
    {
        PutByIndex* code = (PutByIndex*)currentCode;
        nonActivitionModeLocalValuePointer[code->m_index] = *peek<ESValue>(stack, bp);
        executeNextCode<PutByIndex>(programCounter);
        goto NextInstruction;
    }

    PutByIndexWithActivationOpcodeLbl:
    {
        PutByIndexWithActivation* code = (PutByIndexWithActivation*)currentCode;
        LexicalEnvironment* env = ec->environment();
        for(unsigned i = 0; i < code->m_upIndex; i ++) {
            env = env->outerEnvironment();
        }
        ASSERT(env->record()->isDeclarativeEnvironmentRecord());
        *env->record()->toDeclarativeEnvironmentRecord()->bindingValueForActivationMode(code->m_index) = *peek<ESValue>(stack, bp);
        executeNextCode<PutByIndexWithActivation>(programCounter);
        goto NextInstruction;
    }

    PutInObjectOpcodeLbl:
    {
        PutInObject* code = (PutInObject*)currentCode;
        ESValue value = *pop<ESValue>(stack, bp);
        ESValue* property = pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);

        ESObject* obj;
        if(willBeObject->isObject())
            obj = willBeObject->asESPointer()->asESObject();
        else
            obj = willBeObject->toObject();

        if((obj->isESArrayObject() || obj->isESTypedArrayObject()) && property->isInt32()) {
#ifdef ENABLE_ESJIT
            code->m_esir_type.mergeType(escargot::ESJIT::TypeArrayObject);
#endif
            obj->set(*property, value, true);
            push<ESValue>(stack, bp, value);
            executeNextCode<PutInObject>(programCounter);
            goto NextInstruction;
        }

        ESString* val = property->toString();
        if(obj->hiddenClass() == code->m_cachedHiddenClass && (val == code->m_cachedPropertyValue || *val == *code->m_cachedPropertyValue)) {
            if(code->m_cachedIndex == SIZE_MAX) {
                obj->appendHiddenClassItem(val, value);
                push<ESValue>(stack, bp, value);
                executeNextCode<PutInObject>(programCounter);
                goto NextInstruction;
            } else {
                obj->writeHiddenClass(code->m_cachedIndex, value);
                push<ESValue>(stack, bp, value);
                executeNextCode<PutInObject>(programCounter);
                goto NextInstruction;
            }
        }

        if(obj->isHiddenClassMode()) {
            size_t idx = obj->hiddenClass()->findProperty(val);
            if(idx != SIZE_MAX) {
                code->m_cachedHiddenClass = obj->hiddenClass();
                code->m_cachedPropertyValue = val;
                code->m_cachedIndex = idx;
                obj->writeHiddenClass(code->m_cachedIndex, value);
                push<ESValue>(stack, bp, value);
                executeNextCode<PutInObject>(programCounter);
                goto NextInstruction;
            } else {
                code->m_cachedHiddenClass = obj->hiddenClass();
                code->m_cachedPropertyValue = val;
                code->m_cachedIndex = idx;
                obj->appendHiddenClassItem(val, value);
                push<ESValue>(stack, bp, value);
                executeNextCode<PutInObject>(programCounter);
                goto NextInstruction;
            }
        }

        code->m_cachedHiddenClass = (ESHiddenClass*)SIZE_MAX;
        obj->set(*property, value, true);
        push<ESValue>(stack, bp, value);
        executeNextCode<PutInObject>(programCounter);
        goto NextInstruction;
    }

    PutInObjectPreComputedCaseOpcodeLbl:
    {
        PutInObjectPreComputedCase* code = (PutInObjectPreComputedCase*)currentCode;
        ESValue value = *pop<ESValue>(stack, bp);
        const ESValue& property = code->m_propertyValue;
        ESValue* willBeObject = pop<ESValue>(stack, bp);

        ESObject* obj;
        if(willBeObject->isObject())
            obj = willBeObject->asESPointer()->asESObject();
        else
            obj = willBeObject->toObject();

        if((obj->isESArrayObject() || obj->isESTypedArrayObject()) && property.isInt32()) {
            obj->set(property, value, true);
            push<ESValue>(stack, bp, value);
            executeNextCode<PutInObjectPreComputedCase>(programCounter);
            goto NextInstruction;
        }

        if(obj->hiddenClass() == code->m_cachedHiddenClass) {
            if(code->m_cachedIndex != SIZE_MAX) {
                obj->writeHiddenClass(code->m_cachedIndex, value);
                push<ESValue>(stack, bp, value);
                executeNextCode<PutInObjectPreComputedCase>(programCounter);
                goto NextInstruction;
            } else {
                obj->appendHiddenClassItem(property.toString(), value);
                push<ESValue>(stack, bp, value);
                executeNextCode<PutInObjectPreComputedCase>(programCounter);
                goto NextInstruction;
            }

        }

        ESString* str = property.toString();
        if(obj->isHiddenClassMode()) {
            size_t idx = obj->hiddenClass()->findProperty(str);
            if(idx != SIZE_MAX) {
                code->m_cachedHiddenClass = obj->hiddenClass();
                code->m_cachedIndex = idx;
                obj->writeHiddenClass(code->m_cachedIndex, value);
                push<ESValue>(stack, bp, value);
                executeNextCode<PutInObjectPreComputedCase>(programCounter);
                goto NextInstruction;
            } else {
                code->m_cachedHiddenClass = obj->hiddenClass();
                code->m_cachedIndex = idx;
                obj->appendHiddenClassItem(str, value);
                push<ESValue>(stack, bp, value);
                executeNextCode<PutInObjectPreComputedCase>(programCounter);
                goto NextInstruction;
            }
        }

        code->m_cachedHiddenClass = (ESHiddenClass*)SIZE_MAX;
        obj->set(str, value, true);
        push<ESValue>(stack, bp, value);
        executeNextCode<PutInObjectPreComputedCase>(programCounter);
        goto NextInstruction;
    }

    PutArgumentsObjectOpcodeLbl:
    {
        ESValue* value = peek<ESValue>(stack, bp);
        *ec->resolveArgumentsObjectBinding() = *value;
        executeNextCode<PutArgumentsObject>(programCounter);
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

    StringInOpcodeLbl:
    {
        ESValue* obj = pop<ESValue>(stack, bp);
        ESValue* key = pop<ESValue>(stack, bp);
        push<ESValue>(stack, bp, ESValue(!obj->toObject()->find(*key, true).isEmpty()));

        executeNextCode<StringIn>(programCounter);
        goto NextInstruction;
    }

    InstanceOfOpcodeLbl:
    {
        ESValue* rval = pop<ESValue>(stack, bp);
        ESValue* lval = pop<ESValue>(stack, bp);

        if (rval->isESPointer() && rval->asESPointer()->isESFunctionObject() &&
                lval->isESPointer() && lval->asESPointer()->isESObject()) {
            ESFunctionObject* C = rval->asESPointer()->asESFunctionObject();
            ESValue P = C->protoType();
            ESValue O = lval->asESPointer()->asESObject()->__proto__();
            if (P.isESPointer() && P.asESPointer()->isESObject()) {
                while (!O.isUndefinedOrNull()) {
                    if (P == O) {
                        push<ESValue>(stack, bp, ESValue(true));
                        executeNextCode<StringIn>(programCounter);
                        goto NextInstruction;
                       }
                    O = O.asESPointer()->asESObject()->__proto__();
                  }
            } else {
                throw ReferenceError::create(ESString::create(u""));
              }
         }

        push<ESValue>(stack, bp, ESValue(false));

        executeNextCode<StringIn>(programCounter);
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

    UnaryTypeOfOpcodeLbl:
    {
        ESValue* v = pop<ESValue>(stack, bp);

        if(v->isUndefined() || v->isEmpty())
            push<ESValue>(stack, bp, strings->undefined);
        else if(v->isNull())
            push<ESValue>(stack, bp, strings->object);
        else if(v->isBoolean())
            push<ESValue>(stack, bp, strings->boolean);
        else if(v->isNumber())
            push<ESValue>(stack, bp, strings->number);
        else if(v->isESString())
            push<ESValue>(stack, bp, strings->string);
        else if(v->isESPointer()) {
            ESPointer* p = v->asESPointer();
            if(p->isESFunctionObject()) {
                push<ESValue>(stack, bp, strings->function);
            } else {
                push<ESValue>(stack, bp, strings->object);
            }
        }
        else
            RELEASE_ASSERT_NOT_REACHED();


        executeNextCode<UnaryPlus>(programCounter);
        goto NextInstruction;
    }

    UnaryDeleteOpcodeLbl:
    {
        ESValue* key = pop<ESValue>(stack, bp);
        ESValue* obj = pop<ESValue>(stack, bp);
        bool res = obj->toObject()->deletePropety(*key);
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

    CreateObjectOpcodeLbl:
    {
        CreateObject* code = (CreateObject*)currentCode;
        ESObject* obj = ESObject::create(code->m_keyCount);
        obj->setConstructor(globalObject->object());
        obj->set__proto__(globalObject->objectPrototype());
        push<ESValue>(stack, bp, obj);
        executeNextCode<CreateObject>(programCounter);
        goto NextInstruction;
    }

    CreateArrayOpcodeLbl:
    {
        CreateArray* code = (CreateArray*)currentCode;
        ESArrayObject* arr = ESArrayObject::create(code->m_keyCount, globalObject->arrayPrototype());
        push<ESValue>(stack, bp, arr);
        executeNextCode<CreateArray>(programCounter);
        goto NextInstruction;
    }

    SetObjectOpcodeLbl:
    {
        ESValue* value = pop<ESValue>(stack, bp);
        ESValue* key = pop<ESValue>(stack, bp);
        peek<ESValue>(stack, bp)->asESPointer()->asESObject()->set(*key, *value);
        executeNextCode<SetObject>(programCounter);
        goto NextInstruction;
    }

    SetObjectPropertySetterOpcodeLbl:
    {
        ESValue* value = pop<ESValue>(stack, bp);
        ESValue* key = pop<ESValue>(stack, bp);

        ESObject* obj = peek<ESValue>(stack, bp)->asESPointer()->asESObject();
        if(obj->hasOwnProperty(*key)) {
            //TODO check property is accessor property
            //TODO check accessor already exists
            obj->accessorData(*key)->setJSSetter(value->asESPointer()->asESFunctionObject());
        } else {
            obj->defineAccessorProperty(key->toString(), NULL, value->asESPointer()->asESFunctionObject(), true, true, true);
        }

        executeNextCode<SetObjectPropertySetter>(programCounter);
        goto NextInstruction;
    }

    SetObjectPropertyGetterOpcodeLbl:
    {
        ESValue* value = pop<ESValue>(stack, bp);
        ESValue* key = pop<ESValue>(stack, bp);
        ESObject* obj = peek<ESValue>(stack, bp)->asESPointer()->asESObject();
        if(obj->hasOwnProperty(*key)) {
            //TODO check property is accessor property
            //TODO check accessor already exists
            obj->accessorData(*key)->setJSGetter(value->asESPointer()->asESFunctionObject());
        } else {
            obj->defineAccessorProperty(key->toString(), value->asESPointer()->asESFunctionObject(), NULL, true, true, true);
        }
        executeNextCode<SetObjectPropertyGetter>(programCounter);
        goto NextInstruction;
    }

    GetObjectOpcodeLbl:
    {
        GetObject* code = (GetObject*)currentCode;

        ESValue* property = pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);

        if(UNLIKELY(willBeObject->isESString())) {
            if(property->isInt32()) {
               int prop_val = property->toInt32();
               if(LIKELY(0 <= prop_val && prop_val < willBeObject->asESString()->length())) {
                   char16_t c = willBeObject->asESString()->string().data()[prop_val];
                   if(LIKELY(c < ESCARGOT_ASCII_TABLE_MAX)) {
                       push<ESValue>(stack, bp, strings->asciiTable[c]);
                       executeNextCode<GetObject>(programCounter);
                       goto NextInstruction;
                   } else {
                       push<ESValue>(stack, bp, ESString::create(c));
                       executeNextCode<GetObject>(programCounter);
                       goto NextInstruction;
                   }
               } else {
                   push<ESValue>(stack, bp, ESValue());
                   executeNextCode<GetObject>(programCounter);
                   goto NextInstruction;
               }
            } else {
                ESString* val = property->toString();
                if(*val == *strings->length) {
                    lastESObjectMetInMemberExpressionNode = *willBeObject;
                    push<ESValue>(stack, bp, ESValue(willBeObject->asESString()->length()));
                    executeNextCode<GetObject>(programCounter);
                    goto NextInstruction;
                }
                ESValue ret = globalObject->stringObjectProxy()->find(val, true);
                if(ret != ESValue(ESValue::ESEmptyValue) && ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->codeBlock()->m_isBuiltInFunction) {
                    lastESObjectMetInMemberExpressionNode = *willBeObject;
                    push<ESValue>(stack, bp, ret);
                    executeNextCode<GetObject>(programCounter);
                    goto NextInstruction;
                }
            }
        } else if(UNLIKELY(willBeObject->isNumber())) {
            ESString* val = property->toString();
            ESValue ret = globalObject->numberObjectProxy()->find(val, true);
            if(ret != ESValue(ESValue::ESEmptyValue) && ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->codeBlock()->m_isBuiltInFunction) {
                lastESObjectMetInMemberExpressionNode = *willBeObject;
                push<ESValue>(stack, bp, ret);
                executeNextCode<GetObject>(programCounter);
                goto NextInstruction;
            }
        }

        ESObject* obj;
        if(willBeObject->isObject())
            obj = willBeObject->asESPointer()->asESObject();
        else {
            obj = willBeObject->toObject();
        }

        lastESObjectMetInMemberExpressionNode = obj;


        if((obj->isESArrayObject() || obj->isESTypedArrayObject()) && property->isInt32()) {
            push<ESValue>(stack, bp, obj->get(*property, true));
            executeNextCode<GetObject>(programCounter);
            goto NextInstruction;
        }

        ESString* val = property->toString();
        if(obj->hiddenClass() == code->m_cachedHiddenClass && (val == code->m_cachedPropertyValue || *val == *code->m_cachedPropertyValue)) {
            if(code->m_cachedIndex != SIZE_MAX) {
                push<ESValue>(stack, bp, obj->readHiddenClass(code->m_cachedIndex));
                executeNextCode<GetObject>(programCounter);
                goto NextInstruction;
            } else {
                ESValue v = obj->findOnlyPrototype(val);
                if(v.isEmpty()) {
                    push<ESValue>(stack, bp, ESValue());
                    executeNextCode<GetObject>(programCounter);
                    goto NextInstruction;
                }
                push<ESValue>(stack, bp, v);
                executeNextCode<GetObject>(programCounter);
                goto NextInstruction;
            }
        }

        if(obj->isHiddenClassMode()) {
            size_t idx = obj->hiddenClass()->findProperty(val);
            code->m_cachedHiddenClass = obj->hiddenClass();
            code->m_cachedPropertyValue = val;
            code->m_cachedIndex = idx;
            if(idx != SIZE_MAX) {
                push<ESValue>(stack, bp, obj->readHiddenClass(code->m_cachedIndex));
                executeNextCode<GetObject>(programCounter);
                goto NextInstruction;
            } else {
                ESValue v = obj->findOnlyPrototype(val);
                if(v.isEmpty()) {
                    push<ESValue>(stack, bp, ESValue());
                    executeNextCode<GetObject>(programCounter);
                    goto NextInstruction;
                }
                push<ESValue>(stack, bp, v);
                executeNextCode<GetObject>(programCounter);
                goto NextInstruction;
            }
        } else {
            push<ESValue>(stack, bp, obj->get(val, true));
            executeNextCode<GetObject>(programCounter);
            goto NextInstruction;
        }

        RELEASE_ASSERT_NOT_REACHED();
    }

    GetObjectWithPeekingOpcodeLbl:
    {
        GetObject* code = (GetObject*)currentCode;

        ESValue* property = pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);

        stack = (void *)(((size_t)stack) + sizeof(ESValue) * 2);
#ifndef NDEBUG
        stack = (void *)(((size_t)stack) + sizeof(size_t) * 2);
#endif

        if(UNLIKELY(willBeObject->isESString())) {
            if(property->isInt32()) {
               int prop_val = property->toInt32();
               if(LIKELY(0 <= prop_val && prop_val < willBeObject->asESString()->length())) {
                   char16_t c = willBeObject->asESString()->string().data()[prop_val];
                   if(LIKELY(c < ESCARGOT_ASCII_TABLE_MAX)) {
                       push<ESValue>(stack, bp, strings->asciiTable[c]);
                       executeNextCode<GetObject>(programCounter);
                       goto NextInstruction;
                   } else {
                       push<ESValue>(stack, bp, ESString::create(c));
                       executeNextCode<GetObject>(programCounter);
                       goto NextInstruction;
                   }
               } else {
                   push<ESValue>(stack, bp, ESValue());
                   executeNextCode<GetObject>(programCounter);
                   goto NextInstruction;
               }
               push<ESValue>(stack, bp, willBeObject->asESString()->substring(prop_val, prop_val+1));
               executeNextCode<GetObject>(programCounter);
               goto NextInstruction;
            } else {
                ESString* val = property->toString();
                if(*val == *strings->length) {
                    lastESObjectMetInMemberExpressionNode = *willBeObject;
                    push<ESValue>(stack, bp, ESValue(willBeObject->asESString()->length()));
                    executeNextCode<GetObject>(programCounter);
                    goto NextInstruction;
                }
                ESValue ret = globalObject->stringObjectProxy()->find(val, true);
                if(ret != ESValue(ESValue::ESEmptyValue) && ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->codeBlock()->m_isBuiltInFunction) {
                    lastESObjectMetInMemberExpressionNode = *willBeObject;
                    push<ESValue>(stack, bp, ret);
                    executeNextCode<GetObject>(programCounter);
                    goto NextInstruction;
                }
            }
        } else if(UNLIKELY(willBeObject->isNumber())) {
            ESString* val = property->toString();
            ESValue ret = globalObject->numberObjectProxy()->find(val, true);
            if(ret != ESValue(ESValue::ESEmptyValue) && ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->codeBlock()->m_isBuiltInFunction) {
                lastESObjectMetInMemberExpressionNode = *willBeObject;
                push<ESValue>(stack, bp, ret);
                executeNextCode<GetObject>(programCounter);
                goto NextInstruction;
            }
        }

        ESObject* obj;
        if(willBeObject->isObject())
            obj = willBeObject->asESPointer()->asESObject();
        else
            obj = willBeObject->toObject();

        lastESObjectMetInMemberExpressionNode = obj;


        if((obj->isESArrayObject() || obj->isESTypedArrayObject()) && property->isInt32()) {
            push<ESValue>(stack, bp, obj->get(*property, true));
            executeNextCode<GetObject>(programCounter);
            goto NextInstruction;
        }

        ESString* val = property->toString();
        if(obj->hiddenClass() == code->m_cachedHiddenClass && (val == code->m_cachedPropertyValue || *val == *code->m_cachedPropertyValue)) {
            if(code->m_cachedIndex != SIZE_MAX) {
                push<ESValue>(stack, bp, obj->readHiddenClass(code->m_cachedIndex));
                executeNextCode<GetObject>(programCounter);
                goto NextInstruction;
            } else {
                ESValue v = obj->findOnlyPrototype(val);
                if(v.isEmpty()) {
                    push<ESValue>(stack, bp, ESValue());
                    executeNextCode<GetObject>(programCounter);
                    goto NextInstruction;
                }
                push<ESValue>(stack, bp, v);
                executeNextCode<GetObject>(programCounter);
                goto NextInstruction;
            }
        }

        if(obj->isHiddenClassMode()) {
            size_t idx = obj->hiddenClass()->findProperty(val);
            code->m_cachedHiddenClass = obj->hiddenClass();
            code->m_cachedPropertyValue = val;
            code->m_cachedIndex = idx;
            if(idx != SIZE_MAX) {
                push<ESValue>(stack, bp, obj->readHiddenClass(code->m_cachedIndex));
                executeNextCode<GetObject>(programCounter);
                goto NextInstruction;
            } else {
                ESValue v = obj->findOnlyPrototype(val);
                if(v.isEmpty()) {
                    push<ESValue>(stack, bp, ESValue());
                    executeNextCode<GetObject>(programCounter);
                    goto NextInstruction;
                }
                push<ESValue>(stack, bp, v);
                executeNextCode<GetObject>(programCounter);
                goto NextInstruction;
            }
        } else {
            push<ESValue>(stack, bp, obj->get(val, true));
            executeNextCode<GetObject>(programCounter);
            goto NextInstruction;
        }

        RELEASE_ASSERT_NOT_REACHED();
    }

    GetObjectPreComputedCaseOpcodeLbl:
    {
        GetObjectPreComputedCase* code = (GetObjectPreComputedCase*)currentCode;

        const ESValue& property = code->m_propertyValue;
        ESValue* willBeObject = pop<ESValue>(stack, bp);

        if(UNLIKELY(willBeObject->isESString())) {
            if(property.isInt32()) {
               int prop_val = property.toInt32();
               if(LIKELY(0 <= prop_val && prop_val < willBeObject->asESString()->length())) {
                   char16_t c = willBeObject->asESString()->string().data()[prop_val];
                   if(LIKELY(c < ESCARGOT_ASCII_TABLE_MAX)) {
                       push<ESValue>(stack, bp, strings->asciiTable[c]);
                       executeNextCode<GetObjectPreComputedCase>(programCounter);
                       goto NextInstruction;
                   } else {
                       push<ESValue>(stack, bp, ESString::create(c));
                       executeNextCode<GetObjectPreComputedCase>(programCounter);
                       goto NextInstruction;
                   }
               } else {
                   push<ESValue>(stack, bp, ESValue());
                   executeNextCode<GetObjectPreComputedCase>(programCounter);
                   goto NextInstruction;
               }
            } else {
                ESString* val = property.toString();
                if(*val == *strings->length) {
                    lastESObjectMetInMemberExpressionNode = *willBeObject;
                    push<ESValue>(stack, bp, ESValue(willBeObject->asESString()->length()));
                    executeNextCode<GetObjectPreComputedCase>(programCounter);
                    goto NextInstruction;
                }
                ESValue ret = globalObject->stringObjectProxy()->find(val, true);
                if(ret != ESValue(ESValue::ESEmptyValue) && ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->codeBlock()->m_isBuiltInFunction) {
                    lastESObjectMetInMemberExpressionNode = *willBeObject;
                    push<ESValue>(stack, bp, ret);
                    executeNextCode<GetObjectPreComputedCase>(programCounter);
                    goto NextInstruction;
                }
            }
        } else if(UNLIKELY(willBeObject->isNumber())) {
            ESString* val = property.toString();
            ESValue ret = globalObject->numberObjectProxy()->find(val, true);
            if(ret != ESValue(ESValue::ESEmptyValue) && ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->codeBlock()->m_isBuiltInFunction) {
                lastESObjectMetInMemberExpressionNode = *willBeObject;
                push<ESValue>(stack, bp, ret);
                executeNextCode<GetObjectPreComputedCase>(programCounter);
                goto NextInstruction;
            }
        }

        ESObject* obj;
        if(willBeObject->isObject())
            obj = willBeObject->asESPointer()->asESObject();
        else {
            obj = willBeObject->toObject();
        }

        lastESObjectMetInMemberExpressionNode = obj;


        if((obj->isESArrayObject() || obj->isESTypedArrayObject()) && property.isInt32()) {
            push<ESValue>(stack, bp, obj->get(property, true));
            executeNextCode<GetObjectPreComputedCase>(programCounter);
            goto NextInstruction;
        }

        if(obj->hiddenClass() == code->m_cachedHiddenClass) {
            if(code->m_cachedIndex != SIZE_MAX) {
                push<ESValue>(stack, bp, obj->readHiddenClass(code->m_cachedIndex));
                executeNextCode<GetObjectPreComputedCase>(programCounter);
                goto NextInstruction;
            } else {
                ESValue v = obj->findOnlyPrototype(property);
                if(v.isEmpty()) {
                    push<ESValue>(stack, bp, ESValue());
                    executeNextCode<GetObjectPreComputedCase>(programCounter);
                    goto NextInstruction;
                }
                push<ESValue>(stack, bp, v);
                executeNextCode<GetObjectPreComputedCase>(programCounter);
                goto NextInstruction;
            }
        }

        ESString* val = property.toString();
        if(obj->isHiddenClassMode()) {
            size_t idx = obj->hiddenClass()->findProperty(val);
            code->m_cachedHiddenClass = obj->hiddenClass();
            code->m_cachedIndex = idx;
            if(idx != SIZE_MAX) {
                push<ESValue>(stack, bp, obj->readHiddenClass(code->m_cachedIndex));
                executeNextCode<GetObjectPreComputedCase>(programCounter);
                goto NextInstruction;
            } else {
                ESValue v = obj->findOnlyPrototype(val);
                if(v.isEmpty()) {
                    push<ESValue>(stack, bp, ESValue());
                    executeNextCode<GetObjectPreComputedCase>(programCounter);
                    goto NextInstruction;
                }
                push<ESValue>(stack, bp, v);
                executeNextCode<GetObjectPreComputedCase>(programCounter);
                goto NextInstruction;
            }
        } else {
            push<ESValue>(stack, bp, obj->get(val, true));
            executeNextCode<GetObjectPreComputedCase>(programCounter);
            goto NextInstruction;
        }

        RELEASE_ASSERT_NOT_REACHED();
    }

    GetObjectWithPeekingPreComputedCaseOpcodeLbl:
    {
        GetObjectWithPeekingPreComputedCase* code = (GetObjectWithPeekingPreComputedCase*)currentCode;

        const ESValue& property = code->m_propertyValue;
        ESValue* willBeObject = pop<ESValue>(stack, bp);

        stack = (void *)(((size_t)stack) + sizeof(ESValue) * 1);
#ifndef NDEBUG
        stack = (void *)(((size_t)stack) + sizeof(size_t) * 1);
#endif

        if(UNLIKELY(willBeObject->isESString())) {
            if(property.isInt32()) {
               int prop_val = property.toInt32();
               if(LIKELY(0 <= prop_val && prop_val < willBeObject->asESString()->length())) {
                   char16_t c = willBeObject->asESString()->string().data()[prop_val];
                   if(LIKELY(c < ESCARGOT_ASCII_TABLE_MAX)) {
                       push<ESValue>(stack, bp, strings->asciiTable[c]);
                       executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
                       goto NextInstruction;
                   } else {
                       push<ESValue>(stack, bp, ESString::create(c));
                       executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
                       goto NextInstruction;
                   }
               } else {
                   push<ESValue>(stack, bp, ESValue());
                   executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
                   goto NextInstruction;
               }
               push<ESValue>(stack, bp, willBeObject->asESString()->substring(prop_val, prop_val+1));
               executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
               goto NextInstruction;
            } else {
                ESString* val = property.toString();
                if(*val == *strings->length) {
                    lastESObjectMetInMemberExpressionNode = *willBeObject;
                    push<ESValue>(stack, bp, ESValue(willBeObject->asESString()->length()));
                    executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
                    goto NextInstruction;
                }
                ESValue ret = globalObject->stringObjectProxy()->find(val, true);
                if(ret != ESValue(ESValue::ESEmptyValue) && ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->codeBlock()->m_isBuiltInFunction) {
                    lastESObjectMetInMemberExpressionNode = *willBeObject;
                    push<ESValue>(stack, bp, ret);
                    executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
                    goto NextInstruction;
                }
            }
        } else if(UNLIKELY(willBeObject->isNumber())) {
            ESString* val = property.toString();
            ESValue ret = globalObject->numberObjectProxy()->find(val, true);
            if(ret != ESValue(ESValue::ESEmptyValue) && ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->codeBlock()->m_isBuiltInFunction) {
                lastESObjectMetInMemberExpressionNode = *willBeObject;
                push<ESValue>(stack, bp, ret);
                executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
                goto NextInstruction;
            }
        }

        ESObject* obj;
        if(willBeObject->isObject())
            obj = willBeObject->asESPointer()->asESObject();
        else
            obj = willBeObject->toObject();

        lastESObjectMetInMemberExpressionNode = obj;


        if((obj->isESArrayObject() || obj->isESTypedArrayObject()) && property.isInt32()) {
            push<ESValue>(stack, bp, obj->get(property, true));
            executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
            goto NextInstruction;
        }

        if(obj->hiddenClass() == code->m_cachedHiddenClass) {
            if(code->m_cachedIndex != SIZE_MAX) {
                push<ESValue>(stack, bp, obj->readHiddenClass(code->m_cachedIndex));
                executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
                goto NextInstruction;
            } else {
                ESValue v = obj->findOnlyPrototype(property);
                if(v.isEmpty()) {
                    push<ESValue>(stack, bp, ESValue());
                    executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
                    goto NextInstruction;
                }
                push<ESValue>(stack, bp, v);
                executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
                goto NextInstruction;
            }
        }

        ESString* val = property.toString();

        if(obj->isHiddenClassMode()) {
            size_t idx = obj->hiddenClass()->findProperty(val);
            code->m_cachedHiddenClass = obj->hiddenClass();
            code->m_cachedIndex = idx;
            if(idx != SIZE_MAX) {
                push<ESValue>(stack, bp, obj->readHiddenClass(code->m_cachedIndex));
                executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
                goto NextInstruction;
            } else {
                ESValue v = obj->findOnlyPrototype(val);
                if(v.isEmpty()) {
                    push<ESValue>(stack, bp, ESValue());
                    executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
                    goto NextInstruction;
                }
                push<ESValue>(stack, bp, v);
                executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
                goto NextInstruction;
            }
        } else {
            push<ESValue>(stack, bp, obj->get(val, true));
            executeNextCode<GetObjectWithPeekingPreComputedCase>(programCounter);
            goto NextInstruction;
        }

        RELEASE_ASSERT_NOT_REACHED();
    }

    EnumerateObjectOpcodeLbl:
    {
        ESObject* obj = pop<ESValue>(stack, bp)->toObject();
        EnumerateObjectData* data = new EnumerateObjectData();

        data->m_object = obj;
        data->m_keys.reserve(obj->keyCount());
        obj->enumeration([&data](ESValue key) {
            data->m_keys.push_back(key);
        });

        push<ESValue>(stack, bp, ESValue((ESPointer *)data));
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
            if (data->m_object->hasOwnProperty(data->m_keys[data->m_idx++]))
            {
                push<ESValue>(stack, bp, data->m_keys[data->m_idx - 1]);
                executeNextCode<EnumerateObjectKey>(programCounter);
                goto NextInstruction;
            }
        }
    }

    EnumerateObjectEndOpcodeLbl:
    {
        pop<ESValue>(stack, bp);
        executeNextCode<EnumerateObjectEnd>(programCounter);
        goto NextInstruction;
    }

    CreateFunctionOpcodeLbl:
    {
        CreateFunction* code = (CreateFunction*)currentCode;
        ASSERT(((size_t)code->m_codeBlock % sizeof(size_t)) == 0);
        ESFunctionObject* function = ESFunctionObject::create(ec->environment(), code->m_codeBlock, code->m_nonAtomicName == NULL ? strings->emptyESString : code->m_nonAtomicName);
        function->set__proto__(instance->globalObject()->functionPrototype());
        ESObject* prototype = ESObject::create();
        prototype->setConstructor(function);
        prototype->set__proto__(instance->globalObject()->object()->protoType());
        function->setProtoType(prototype);
        if(code->m_nonAtomicName) { //FD
            function->set(strings->name, code->m_nonAtomicName);
            ec->environment()->record()->setMutableBinding(code->m_name, code->m_nonAtomicName, function, false);
        }
        else //FE
            push<ESValue>(stack, bp, function);
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

        ESValue callee = *ec->resolveBinding(strings->atomicEval, strings->eval);
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

    CallBoundFunctionOpcodeLbl:
    {
        CallBoundFunction* code = (CallBoundFunction*)currentCode;
        size_t argc = code->m_boundArgumentsCount + instance->currentExecutionContext()->argumentCount();
        ESValue* mergedArguments = (ESValue *)alloca(sizeof(ESValue) * argc);
        memcpy(mergedArguments, code->m_boundArguments, sizeof(ESValue) * code->m_boundArgumentsCount);
        memcpy(mergedArguments + code->m_boundArgumentsCount, instance->currentExecutionContext()->arguments(), sizeof(ESValue) * instance->currentExecutionContext()->argumentCount());
        return ESFunctionObject::call(instance, code->m_boundTargetFunction, code->m_boundThis, mergedArguments, argc, false);
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
            receiver = ESRegExpObject::create(strings->emptyESString,ESRegExpObject::Option::None);
        } else if (function == globalObject->boolean()) {
            receiver = ESBooleanObject::create(ESValue(ESValue::ESFalseTag::ESFalse));
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
        receiver->setConstructor(fn);
        if(function->protoType().isObject())
            receiver->set__proto__(function->protoType());
        else
            receiver->set__proto__(ESObject::create());

        ESValue res = ESFunctionObject::call(instance, fn, receiver, arguments, argc, true);
        if (res.isObject()) {
            push<ESValue>(stack, bp, res);
        } else
            push<ESValue>(stack, bp, receiver);
        executeNextCode<NewFunctionCall>(programCounter);
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

    JumpOpcodeLbl:
    {
        Jump* code = (Jump *)currentCode;
        ASSERT(code->m_jumpPosition != SIZE_MAX);

        programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
        goto NextInstruction;
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

    DuplicateTopOfStackValueOpcodeLbl:
    {
        push<ESValue>(stack, bp, peek<ESValue>(stack, bp));
        executeNextCode<DuplicateTopOfStackValue>(programCounter);
        goto NextInstruction;
    }

    LoopStartOpcodeLbl:
    {
        executeNextCode<LoopStart>(programCounter);
        goto NextInstruction;
    }

    TryOpcodeLbl:
    {
        Try* code = (Try *)currentCode;
        LexicalEnvironment* oldEnv = ec->environment();
        ExecutionContext* backupedEC = ec;
        try {
            ESValue ret = interpret(instance, codeBlock, resolveProgramCounter(codeBuffer, programCounter + sizeof(Try)));
            if(ret != ESValue(ESValue::ESEmptyValue)) {
                //TODO add sp check
                return ret;
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
            ESValue ret = interpret(instance, codeBlock, code->m_catchPosition);
            instance->currentExecutionContext()->setEnvironment(oldEnv);
            if(ret != ESValue(ESValue::ESEmptyValue)) {
                //TODO add sp check
                return ret;
            }
        }
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
        ESValue* v = pop<ESValue>(stack, bp);
        throw *v;
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

    EndOpcodeLbl:
    {
        ASSERT(stack == bp);
        return ESValue();
    }

}

ByteCode::ByteCode(Opcode code) {
    m_opcode = (ESVMInstance::currentInstance()->opcodeTable())->m_table[(unsigned)code];
#ifndef NDEBUG
    m_orgOpcode = code;
    m_node = nullptr;
#endif
}

CodeBlock* generateByteCode(Node* node)
{
    CodeBlock* block = CodeBlock::create();

    ByteCodeGenerateContext context;
    //unsigned long start = ESVMInstance::tickCount();
    node->generateStatementByteCode(block, context);
    //unsigned long end = ESVMInstance::tickCount();
    //printf("generate code takes %lfms\n",(end-start)/1000.0);
#ifndef NDEBUG
    if(ESVMInstance::currentInstance()->m_dumpByteCode) {
        char* code = block->m_code.data();
        ByteCode* currentCode = (ByteCode *)(&code[0]);
        if(currentCode->m_orgOpcode != ExecuteNativeFunctionOpcode) {
            dumpBytecode(block);
        }
    }
#endif
    return block;
}

#ifndef NDEBUG

void dumpBytecode(CodeBlock* codeBlock)
{
    printf("dumpBytecode...>>>>>>>>>>>>>>>>>>>>>>\n");
    printf("function %s\n", codeBlock->m_nonAtomicId ? (codeBlock->m_nonAtomicId->utf8Data()):"(anonymous)");
    size_t idx = 0;
#ifdef ENABLE_ESJIT
    size_t bytecodeCounter = 0;
    size_t callInfoIndex = 0;
#endif
    char* code = codeBlock->m_code.data();
    while(idx < codeBlock->m_code.size()) {
        ByteCode* currentCode = (ByteCode *)(&code[idx]);
        if(currentCode->m_node)
            printf("%u\t\t%p\t(nodeinfo %d)\t\t\t",(unsigned)idx, currentCode, (int)currentCode->m_node->sourceLocation().m_lineNumber);
        else
            printf("%u\t\t%p\t(nodeinfo null)\t\t\t",(unsigned)idx, currentCode);

        Opcode opcode = getOpcodeFromAddress(currentCode->m_opcode);

#ifdef ENABLE_ESJIT
        if (opcode == CallFunctionOpcode) {
            int calleeIndex = codeBlock->m_functionCallInfos[callInfoIndex++];
            int receiverIndex = codeBlock->m_functionCallInfos[callInfoIndex++];
            int argumentCount = codeBlock->m_functionCallInfos[callInfoIndex++];
            printf("[%3d,%3d,%3d", calleeIndex, receiverIndex, argumentCount);
            for (int i=0; i<argumentCount; i++)
                printf(",%3d", codeBlock->m_functionCallInfos[callInfoIndex++]);
            printf("] ");
        }
        switch(opcode) {
#define DUMP_BYTE_CODE(code) \
        case code##Opcode:\
        codeBlock->getSSAIndex(bytecodeCounter)->dump(); \
        currentCode->dump(); \
        idx += sizeof (code); \
        bytecodeCounter++; \
        continue;
        FOR_EACH_BYTECODE_OP(DUMP_BYTE_CODE)
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        };
#else // ENABLE_ESJIT
        switch(opcode) {
#define DUMP_BYTE_CODE(code) \
        case code##Opcode:\
        currentCode->dump(); \
        idx += sizeof (code); \
        continue;
        FOR_EACH_BYTECODE_OP(DUMP_BYTE_CODE)
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        };
#endif // ENABLE_ESJIT
    }
    printf("dumpBytecode...<<<<<<<<<<<<<<<<<<<<<<\n");
}

#endif
}
