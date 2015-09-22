#include "Escargot.h"
#include "bytecode/ByteCode.h"

namespace escargot {

ALWAYS_INLINE ESValue plusOperation(const ESValue& left, const ESValue& right);
ALWAYS_INLINE ESValue minusOperation(const ESValue& left, const ESValue& right);

ESValue interpret(ESVMInstance* instance, CodeBlock* codeBlock, size_t programCounter)
{
    if(codeBlock == NULL) {
#define REGISTER_TABLE(opcode) \
        instance->opcodeTable()->m_table[opcode##Opcode] = &&opcode##OpcodeLbl;
        FOR_EACH_BYTECODE_OP(REGISTER_TABLE);
        return ESValue();
    }
#ifndef NDEBUG
    if(instance->m_dumpByteCode) {
        char* code = codeBlock->m_code.data();
        ByteCode* currentCode = (ByteCode *)(&code[0]);
        if(currentCode->m_orgOpcode != ExecuteNativeFunctionOpcode) {
            dumpBytecode(codeBlock);
        }
    }
#endif
    char stackBuf[1024];
    void* stack = stackBuf;
    void* bp = stack;
    char tmpStackBuf[1024];
    void* tmpStack = tmpStackBuf;
    void* tmpBp = tmpStack;
    char* codeBuffer = codeBlock->m_code.data();
    ExecutionContext* ec = instance->currentExecutionContext();
    GlobalObject* globalObject = instance->globalObject();
    ESObject* lastESObjectMetInMemberExpressionNode = globalObject;
    ESValue* lastExpressionStatementValue = &instance->m_lastExpressionStatementValue;
    ESValue* nonActivitionModeLocalValuePointer = ec->cachedDeclarativeEnvironmentRecordESValue();
    ASSERT(((size_t)stack % sizeof(size_t)) == 0);
    ASSERT(((size_t)tmpStack % sizeof(size_t)) == 0);
    //resolve programCounter into address
    programCounter = (size_t)(&codeBuffer[programCounter]);
    NextInstruction:
    ByteCode* currentCode = (ByteCode *)programCounter;
    ASSERT(((size_t)currentCode % sizeof(size_t)) == 0);
    /*
    {
        size_t tt = (size_t)currentCode;
        ASSERT(tt % sizeof(size_t) == 0);
        if(currentCode->m_node)
            printf("execute %p %u \t(nodeinfo %d)\t",currentCode, (unsigned)(programCounter-(size_t)codeBuffer), (int)currentCode->m_node->sourceLocation().m_lineNumber);
        else
            printf("execute %p %u \t(nodeinfo null)\t",currentCode, (unsigned)(programCounter-(size_t)codeBuffer));
        currentCode->dump();
    }*/

#ifndef NDEBUG
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

    PopExpressionStatementOpcodeLbl:
    {
        ESValue* t = pop<ESValue>(stack, bp);
        *lastExpressionStatementValue = *t;
        executeNextCode<PopExpressionStatement>(programCounter);
        goto NextInstruction;
    }

    PushIntoTempStackOpcodeLbl:
    {
        push<ESValue>(tmpStack, tmpBp, *pop<ESValue>(stack, bp));
        executeNextCode<PushIntoTempStack>(programCounter);
        goto NextInstruction;
    }

    PopFromTempStackOpcodeLbl:
    {
        push<ESValue>(stack, bp, *pop<ESValue>(tmpStack, tmpBp));
        executeNextCode<PopFromTempStack>(programCounter);
        goto NextInstruction;
    }

    GetByIdOpcodeLbl:
    {
        GetById* code = (GetById*)currentCode;
        if (LIKELY(code->m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
            ASSERT(ec->resolveBinding(code->m_name, code->m_nonAtomicName).dataAddress() == code->m_cachedSlot.dataAddress());
            push<ESValue>(stack, bp, code->m_cachedSlot.readDataProperty());
        } else {
            ESSlotAccessor slot = ec->resolveBinding(code->m_name, code->m_nonAtomicName);
            if(LIKELY(slot.hasData())) {
                code->m_cachedSlot = ESSlotAccessor(slot);
                code->m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
                push<ESValue>(stack, bp, code->m_cachedSlot.readDataProperty());
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
            ASSERT(ec->resolveBinding(code->m_name, code->m_nonAtomicName).dataAddress() == code->m_cachedSlot.dataAddress());
            push<ESValue>(stack, bp, code->m_cachedSlot.readDataProperty());
        } else {
            ESSlotAccessor slot = ec->resolveBinding(code->m_name, code->m_nonAtomicName);
            if(LIKELY(slot.hasData())) {
                code->m_cachedSlot = ESSlotAccessor(slot);
                code->m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
                push<ESValue>(stack, bp, code->m_cachedSlot.readDataProperty());
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
        push<ESValue>(stack, bp, nonActivitionModeLocalValuePointer[code->m_index]);
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
        push<ESValue>(stack, bp, *env->record()->toDeclarativeEnvironmentRecord()->bindingValueForActivationMode(code->m_index));
        executeNextCode<GetByIndexWithActivation>(programCounter);
        goto NextInstruction;
    }

    PutByIdOpcodeLbl:
    {
        PutById* code = (PutById*)currentCode;
        ESValue* value = peek<ESValue>(stack, bp);

        if (LIKELY(code->m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
            code->m_cachedSlot.setValue(*value);
        } else {
            ExecutionContext* ec = instance->currentExecutionContext();
            ESSlotAccessor slot = ec->resolveBinding(code->m_name, code->m_nonAtomicName);

            if(LIKELY(slot.hasData())) {
                code->m_cachedSlot = slot;
                code->m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
                code->m_cachedSlot.setValue(*value);
            } else {
                //CHECKTHIS true, true, false is right?
                instance->invalidateIdentifierCacheCheckCount();
                globalObject->definePropertyOrThrow(code->m_nonAtomicName, true, true, true).setValue(*value);
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
        ESValue* value = pop<ESValue>(stack, bp);
        ESValue* property = pop<ESValue>(stack, bp);
        ESValue* willBeObject = pop<ESValue>(stack, bp);

        ESObject* obj;
        if(willBeObject->isObject())
            obj = willBeObject->asESPointer()->asESObject();
        else
            obj = willBeObject->toObject();

        if(obj->isHiddenClassMode() && !obj->isESArrayObject()) {
            ESString* val = property->toString();
            if(code->m_cachedHiddenClass == obj->hiddenClass() && (val == code->m_cachedPropertyValue || *val == *code->m_cachedPropertyValue)) {
                obj->writeHiddenClass(code->m_cachedIndex, *value);
                push<ESValue>(stack, bp, *value);
                executeNextCode<PutInObject>(programCounter);
                goto NextInstruction;
            } else {
                size_t idx = obj->hiddenClass()->findProperty(val);
                if(idx != SIZE_MAX) {
                    code->m_cachedHiddenClass = obj->hiddenClass();
                    code->m_cachedPropertyValue = val;
                    code->m_cachedIndex = idx;
                    obj->writeHiddenClass(code->m_cachedIndex, *value);
                    push<ESValue>(stack, bp, *value);
                    executeNextCode<PutInObject>(programCounter);
                    goto NextInstruction;
                } else {
                    code->m_cachedHiddenClass = nullptr;
                    obj->set(*property, *value, true);
                    push<ESValue>(stack, bp, *value);
                    executeNextCode<PutInObject>(programCounter);
                    goto NextInstruction;
                }
            }
        } else {
            obj->set(*property, *value, true);
            push<ESValue>(stack, bp, *value);
            executeNextCode<PutInObject>(programCounter);
            goto NextInstruction;
        }
        RELEASE_ASSERT_NOT_REACHED();
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
        int32_t lnum = left->toInt32();
        int32_t rnum = right->toInt32();
        lnum = ((unsigned int)lnum) >> (((unsigned int)rnum) & 0x1F);
        push<ESValue>(stack, bp, ESValue(lnum));
        executeNextCode<UnsignedRightShift>(programCounter);
        goto NextInstruction;
    }

    LessThanOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        ESValue lval = left->toPrimitive();
        ESValue rval = right->toPrimitive();

        // TODO http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
        // string, NaN, zero, infinity, ...
        bool b;
        if(lval.isInt32() && rval.isInt32()) {
            b = lval.asInt32() < rval.asInt32();
        } else if (lval.isESString() || rval.isESString()) {
            b = lval.toString()->string() < rval.toString()->string();
        } else {
            b = lval.toNumber() < rval.toNumber();
        }
        push<ESValue>(stack, bp, ESValue(b));
        executeNextCode<LessThan>(programCounter);
        goto NextInstruction;
    }

    LessThanOrEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        ESValue lval = left->toPrimitive();
        ESValue rval = right->toPrimitive();

        // TODO http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
        // string, NaN, zero, infinity, ...
        bool b;
        if(lval.isInt32() && rval.isInt32()) {
         b = lval.asInt32() <= rval.asInt32();
        } else if (lval.isESString() || rval.isESString()) {
         b = lval.toString()->string() <= rval.toString()->string();
        } else {
         b = lval.toNumber() <= rval.toNumber();
        }
        push<ESValue>(stack, bp, ESValue(b));
        executeNextCode<LessThanOrEqual>(programCounter);
        goto NextInstruction;
    }

    GreaterThanOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        ESValue lval = left->toPrimitive();
        ESValue rval = right->toPrimitive();

        // TODO http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
        // string, NaN, zero, infinity, ...
        bool b;
        if(lval.isInt32() && rval.isInt32()) {
            b = lval.asInt32() > rval.asInt32();
        } else if (lval.isESString() || rval.isESString()) {
            b = lval.toString()->string() > rval.toString()->string();
        } else {
            b = lval.toNumber() > rval.toNumber();
        }
        push<ESValue>(stack, bp, ESValue(b));
        executeNextCode<GreaterThan>(programCounter);
        goto NextInstruction;
    }

    GreaterThanOrEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, bp);
        ESValue* left = pop<ESValue>(stack, bp);
        ESValue lval = left->toPrimitive();
        ESValue rval = right->toPrimitive();

        // TODO http://www.ecma-international.org/ecma-262/5.1/#sec-11.8.5
        // string, NaN, zero, infinity, ...
        bool b;
        if(lval.isInt32() && rval.isInt32()) {
         b = lval.asInt32() >= rval.asInt32();
        } else if (lval.isESString() || rval.isESString()) {
         b = lval.toString()->string() >= rval.toString()->string();
        } else {
         b = lval.toNumber() >= rval.toNumber();
        }
        push<ESValue>(stack, bp, ESValue(b));
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
        push<ESValue>(stack, bp, minusOperation(*pop<ESValue>(stack, bp), *pop<ESValue>(stack, bp)));
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
        ESValue ret(ESValue::ESForceUninitialized);
        if (left->isInt32() && right->isInt32()) {
            ret = ESValue(left->asInt32() % right->asInt32());
        } else {
            double lvalue = left->toNumber();
            double rvalue = right->toNumber();
            // http://www.ecma-international.org/ecma-262/5.1/#sec-11.5.3
            if (std::isnan(lvalue) || std::isnan(rvalue))
                ret = ESValue(std::numeric_limits<double>::quiet_NaN());
            else if (lvalue == std::numeric_limits<double>::infinity() || lvalue == -std::numeric_limits<double>::infinity() || rvalue == 0 || rvalue == -0.0) {
                ret = ESValue(std::numeric_limits<double>::quiet_NaN());
            } else {
                bool isNeg = lvalue < 0;
                bool lisZero = lvalue == 0 || lvalue == -0.0;
                bool risZero = rvalue == 0 || rvalue == -0.0;
                if (!lisZero && (rvalue == std::numeric_limits<double>::infinity() || rvalue == -std::numeric_limits<double>::infinity()))
                    ret = ESValue(lvalue);
                else if (lisZero && !risZero)
                    ret = ESValue(lvalue);
                else {
                    int d = lvalue / rvalue;
                    ret = ESValue(lvalue - (d * rvalue));
                }
            }
        }
        push<ESValue>(stack, bp, ret);
        executeNextCode<Mod>(programCounter);
        goto NextInstruction;
    }

    IncrementOpcodeLbl:
    {
        push<ESValue>(stack, bp, plusOperation(*pop<ESValue>(stack, bp), ESValue(1)));
        executeNextCode<Division>(programCounter);
        goto NextInstruction;
    }

    DecrementOpcodeLbl:
    {
        push<ESValue>(stack, bp, minusOperation(*pop<ESValue>(stack, bp), ESValue(1)));
        executeNextCode<Division>(programCounter);
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

    UnaryTypeOfOpcodeLbl:
    {
        ESValue* v = pop<ESValue>(stack, bp);

        if(v->isUndefined() || v->isEmpty())
            push<ESValue>(stack, bp, strings->undefined);
        else if(v->isNull())
            push<ESValue>(stack, bp, strings->null);
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
        obj->toObject()->deletePropety(*key);
        push<ESValue>(stack, bp, ESValue(true));

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
               push<ESValue>(stack, bp, willBeObject->asESString()->substring(prop_val, prop_val+1));
               executeNextCode<GetObject>(programCounter);
               goto NextInstruction;
            } else {
                globalObject->stringObjectProxy()->setString(willBeObject->asESString());
                ESValue ret = globalObject->stringObjectProxy()->find(*property, true);
                if(!ret.isEmpty()) {
                    if(ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->codeBlock()->m_isBuiltInFunction) {
                        lastESObjectMetInMemberExpressionNode = (globalObject->stringObjectProxy());
                        push<ESValue>(stack, bp, ret);
                        executeNextCode<GetObject>(programCounter);
                        goto NextInstruction;
                    }
                }
            }
        } else if(UNLIKELY(willBeObject->isNumber())) {
            globalObject->numberObjectProxy()->setNumberData(willBeObject->asNumber());
            ESValue ret = globalObject->numberObjectProxy()->find(*property, true);
            if(!ret.isEmpty()) {
                if(ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->codeBlock()->m_isBuiltInFunction) {
                    lastESObjectMetInMemberExpressionNode = (globalObject->numberObjectProxy());
                    push<ESValue>(stack, bp, ret);
                    executeNextCode<GetObject>(programCounter);
                    goto NextInstruction;
                }
            }
        }

        ESObject* obj;
        if(willBeObject->isObject())
            obj = willBeObject->asESPointer()->asESObject();
        else
            obj = willBeObject->toObject();

        lastESObjectMetInMemberExpressionNode = obj;

        if(obj->isHiddenClassMode() && !obj->isESArrayObject()) {
            ESString* val = property->toString();
            if(code->m_cachedHiddenClass == obj->hiddenClass() && (val == code->m_cachedPropertyValue || *val == *code->m_cachedPropertyValue)) {
                push<ESValue>(stack, bp, obj->readHiddenClass(code->m_cachedIndex));
                executeNextCode<GetObject>(programCounter);
                goto NextInstruction;
            } else {
                size_t idx = obj->hiddenClass()->findProperty(val);
                if(idx != SIZE_MAX) {
                    code->m_cachedHiddenClass = obj->hiddenClass();
                    code->m_cachedPropertyValue = val;
                    code->m_cachedIndex = idx;
                    push<ESValue>(stack, bp, obj->readHiddenClass(idx));
                    executeNextCode<GetObject>(programCounter);
                    goto NextInstruction;
                } else {
                    code->m_cachedHiddenClass = nullptr;
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
        } else {
            push<ESValue>(stack, bp, obj->get(*property, true));
            executeNextCode<GetObject>(programCounter);
            goto NextInstruction;
        }
        RELEASE_ASSERT_NOT_REACHED();
        goto NextInstruction;
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
                       executeNextCode<GetObjectWithPeeking>(programCounter);
                       goto NextInstruction;
                   } else {
                       push<ESValue>(stack, bp, ESString::create(c));
                       executeNextCode<GetObjectWithPeeking>(programCounter);
                       goto NextInstruction;
                   }
               } else {
                   push<ESValue>(stack, bp, ESValue());
                   executeNextCode<GetObjectWithPeeking>(programCounter);
                   goto NextInstruction;
               }
               push<ESValue>(stack, bp, willBeObject->asESString()->substring(prop_val, prop_val+1));
               executeNextCode<GetObjectWithPeeking>(programCounter);
               goto NextInstruction;
            } else {
                globalObject->stringObjectProxy()->setString(willBeObject->asESString());
                ESValue ret = globalObject->stringObjectProxy()->find(*property, true);
                if(!ret.isEmpty()) {
                    if(ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->codeBlock()->m_isBuiltInFunction) {
                        lastESObjectMetInMemberExpressionNode = (globalObject->stringObjectProxy());
                        push<ESValue>(stack, bp, ret);
                        executeNextCode<GetObjectWithPeeking>(programCounter);
                        goto NextInstruction;
                    }
                }
            }
        } else if(UNLIKELY(willBeObject->isNumber())) {
            globalObject->numberObjectProxy()->setNumberData(willBeObject->asNumber());
            ESValue ret = globalObject->numberObjectProxy()->find(*property, true);
            if(!ret.isEmpty()) {
                if(ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->codeBlock()->m_isBuiltInFunction) {
                    lastESObjectMetInMemberExpressionNode = (globalObject->numberObjectProxy());
                    push<ESValue>(stack, bp, ret);
                    executeNextCode<GetObjectWithPeeking>(programCounter);
                    goto NextInstruction;
                }
            }
        }

        ESObject* obj;
        if(willBeObject->isObject())
            obj = willBeObject->asESPointer()->asESObject();
        else
            obj = willBeObject->toObject();

        lastESObjectMetInMemberExpressionNode = obj;

        if(obj->isHiddenClassMode() && !obj->isESArrayObject()) {
            ESString* val = property->toString();
            if(code->m_cachedHiddenClass == obj->hiddenClass() && (val == code->m_cachedPropertyValue || *val == *code->m_cachedPropertyValue)) {
                push<ESValue>(stack, bp, obj->readHiddenClass(code->m_cachedIndex));
                executeNextCode<GetObjectWithPeeking>(programCounter);
                goto NextInstruction;
            } else {
                size_t idx = obj->hiddenClass()->findProperty(val);
                if(idx != SIZE_MAX) {
                    code->m_cachedHiddenClass = obj->hiddenClass();
                    code->m_cachedPropertyValue = val;
                    code->m_cachedIndex = idx;
                    push<ESValue>(stack, bp, obj->readHiddenClass(idx));
                    executeNextCode<GetObjectWithPeeking>(programCounter);
                    goto NextInstruction;
                } else {
                    code->m_cachedHiddenClass = nullptr;
                    ESValue v = obj->findOnlyPrototype(val);
                    if(v.isEmpty()) {
                        push<ESValue>(stack, bp, ESValue());
                        executeNextCode<GetObjectWithPeeking>(programCounter);
                        goto NextInstruction;
                    }
                    push<ESValue>(stack, bp, v);
                    executeNextCode<GetObjectWithPeeking>(programCounter);
                    goto NextInstruction;
                }
            }
        } else {
            push<ESValue>(stack, bp, obj->get(*property, true));
            executeNextCode<GetObjectWithPeeking>(programCounter);
            goto NextInstruction;
        }
        RELEASE_ASSERT_NOT_REACHED();
        goto NextInstruction;
    }

    EnumerateObjectOpcodeLbl:
    {
        ESObject* obj = pop<ESValue>(stack, bp)->toObject();
        EnumerateObjectData* data = new EnumerateObjectData();

        data->m_object = obj;
        data->m_keys.reserve(obj->keyCount());
        obj->enumeration([&data](ESValue key, ESValue value) {
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
        lastESObjectMetInMemberExpressionNode = globalObject;
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
        ESObject* receiver = pop<ESValue>(stack, bp)->asESPointer()->asESObject();
        push<ESValue>(stack, bp, ESFunctionObject::call(instance, *pop<ESValue>(stack, bp), receiver, arguments, argc, false));
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

        ESValue callee = ec->resolveBinding(strings->atomicEval, strings->eval).value();
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
        if(*top == ESValue(ESValue::ESFalse) || !top->toBoolean())
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
        if(*top == ESValue(ESValue::ESTrue) || top->toBoolean())
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
        if(*top == ESValue(ESValue::ESTrue) || top->toBoolean()) {
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
        if(*top == ESValue(ESValue::ESFalse) || !top->toBoolean())
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
        if(*top == ESValue(ESValue::ESTrue) || top->toBoolean())
            programCounter = jumpTo(codeBuffer, code->m_jumpPosition);
        else
            executeNextCode<JumpIfTopOfStackValueIsTrueWithPeeking>(programCounter);
        goto NextInstruction;
    }

    DuplicateTopOfStackValueOpcodeLbl:
    {
        push<ESValue>(stack, bp, *peek<ESValue>(stack, bp));
        executeNextCode<DuplicateTopOfStackValue>(programCounter);
        goto NextInstruction;
    }

    TryOpcodeLbl:
    {
        Try* code = (Try *)currentCode;
        LexicalEnvironment* oldEnv = ec->environment();
        ExecutionContext* backupedEC = ec;
        try {
            interpret(instance, codeBlock, resolveProgramCounter(codeBuffer, programCounter + sizeof(Try)));
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
            //TODO process return value in catch-body
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
        push<ESValue>(stack, bp, ESValue(ec->resolveThisBinding()));
        executeNextCode<This>(programCounter);
        goto NextInstruction;
    }

    EndOpcodeLbl:
    {
        ASSERT(stack == bp);
        return ESValue();
    }

}

ALWAYS_INLINE ESValue plusOperation(const ESValue& left, const ESValue& right)
{
    ESValue lval = left.toPrimitive();
    ESValue rval = right.toPrimitive();
    // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.1

    ESValue ret(ESValue::ESForceUninitialized);
    if(lval.isInt32() && rval.isInt32()) {
        int a = lval.asInt32(), b = rval.asInt32();
        if (UNLIKELY(a > 0 && b > std::numeric_limits<int32_t>::max() - a)) {
            //overflow
            ret = ESValue((double)lval.asInt32() + (double)rval.asInt32());
        } else if (UNLIKELY(a < 0 && b < std::numeric_limits<int32_t>::min() - a)) {
            //underflow
            ret = ESValue((double)lval.asInt32() + (double)rval.asInt32());
        } else {
            ret = ESValue(lval.asInt32() + rval.asInt32());
        }
    } else if (lval.isESString() || rval.isESString()) {
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
        int a = left.asInt32(), b = right.asInt32();
        if (UNLIKELY((a > 0 && b < 0 && b < a - std::numeric_limits<int32_t>::max()))) {
            //overflow
            ret = ESValue((double)left.asInt32() - (double)right.asInt32());
        } else if (UNLIKELY(a < 0 && b > 0 && b > a - std::numeric_limits<int32_t>::min())) {
            //underflow
            ret = ESValue((double)left.asInt32() - (double)right.asInt32());
        } else {
            ret = ESValue(left.asInt32() - right.asInt32());
        }
    }
    else
        ret = ESValue(left.toNumber() - right.toNumber());
    return ret;
}

ByteCode::ByteCode(Opcode code) {
    m_opcode = (ESVMInstance::currentInstance()->opcodeTable())->m_table[(unsigned)code];
#ifndef NDEBUG
    m_orgOpcode = code;
    m_node = nullptr;
#endif
}

#ifndef NDEBUG

ALWAYS_INLINE void dumpBytecode(CodeBlock* codeBlock)
{
    printf("dumpBytecode...>>>>>>>>>>>>>>>>>>>>>>\n");
    size_t idx = 0;
    char* code = codeBlock->m_code.data();
    while(idx < codeBlock->m_code.size()) {
        ByteCode* currentCode = (ByteCode *)(&code[idx]);
        printf("%u\t\t%p\t",(unsigned)idx, currentCode);

        Opcode opcode = Opcode::OpcodeKindEnd;
        for(int i = 0; i < Opcode::OpcodeKindEnd; i ++) {
            if((ESVMInstance::currentInstance()->opcodeTable())->m_table[i] == currentCode->m_opcode) {
                opcode = (Opcode)i;
                break;
            }
        }

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
    }
    printf("dumpBytecode...<<<<<<<<<<<<<<<<<<<<<<\n");
}

#endif

}
