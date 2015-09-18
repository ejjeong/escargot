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
        REGISTER_TABLE(Push);
        REGISTER_TABLE(PopExpressionStatement);
        REGISTER_TABLE(Pop);
        REGISTER_TABLE(PushIntoTempStack);
        REGISTER_TABLE(PopFromTempStack);
        REGISTER_TABLE(GetById);
        REGISTER_TABLE(GetByIndex);
        REGISTER_TABLE(GetByIndexWithActivation);
        REGISTER_TABLE(PutById);
        REGISTER_TABLE(PutByIndex);
        REGISTER_TABLE(PutByIndexWithActivation);
        REGISTER_TABLE(PutInObject);
        REGISTER_TABLE(CreateBinding);
        REGISTER_TABLE(Equal);
        REGISTER_TABLE(NotEqual);
        REGISTER_TABLE(StrictEqual);
        REGISTER_TABLE(NotStrictEqual);
        REGISTER_TABLE(BitwiseAnd);
        REGISTER_TABLE(BitwiseOr);
        REGISTER_TABLE(BitwiseXor);
        REGISTER_TABLE(LeftShift);
        REGISTER_TABLE(SignedRightShift);
        REGISTER_TABLE(UnsignedRightShift);
        REGISTER_TABLE(LessThan);
        REGISTER_TABLE(LessThanOrEqual);
        REGISTER_TABLE(GreaterThan);
        REGISTER_TABLE(GreaterThanOrEqual);
        REGISTER_TABLE(Plus);
        REGISTER_TABLE(Minus);
        REGISTER_TABLE(Multiply);
        REGISTER_TABLE(Division);
        REGISTER_TABLE(Mod);
        REGISTER_TABLE(Increment);
        REGISTER_TABLE(Decrement);
        REGISTER_TABLE(BitwiseNot);
        REGISTER_TABLE(LogicalNot);
        REGISTER_TABLE(UnaryMinus);
        REGISTER_TABLE(UnaryPlus);
        REGISTER_TABLE(CreateObject);
        REGISTER_TABLE(CreateArray);
        REGISTER_TABLE(SetObject);
        REGISTER_TABLE(GetObject);
        REGISTER_TABLE(GetObjectWithPeeking);
        REGISTER_TABLE(CreateFunction);
        REGISTER_TABLE(ExecuteNativeFunction);
        REGISTER_TABLE(PrepareFunctionCall);
        REGISTER_TABLE(CallFunction);
        REGISTER_TABLE(NewFunctionCall);
        REGISTER_TABLE(ReturnFunction);
        REGISTER_TABLE(ReturnFunctionWithValue);
        REGISTER_TABLE(Jump);
        REGISTER_TABLE(JumpIfTopOfStackValueIsFalse);
        REGISTER_TABLE(JumpIfTopOfStackValueIsTrue);
        REGISTER_TABLE(JumpIfTopOfStackValueIsFalseWithPeeking);
        REGISTER_TABLE(JumpIfTopOfStackValueIsTrueWithPeeking);
        REGISTER_TABLE(DuplicateTopOfStackValue);
        REGISTER_TABLE(Try);
        REGISTER_TABLE(TryCatchBodyEnd);
        REGISTER_TABLE(Throw);
        REGISTER_TABLE(This);
        REGISTER_TABLE(End);
        return ESValue();
    }
    //GC_gcollect();
    //dumpBytecode(codeBlock);
    //void* stack = instance->m_stack;
    //size_t& sp = instance->m_sp;
    char stackBuf[1024];
    void* stack = stackBuf;
    unsigned sp  = 0;
    unsigned bp = 0;
    char tmpStackBuf[1024];
    void* tmpStack = tmpStackBuf;
    unsigned tmpSp  = 0;
    char* code = codeBlock->m_code.data();
    ExecutionContext* ec = instance->currentExecutionContext();
    GlobalObject* globalObject = instance->globalObject();
    ESObject* lastESObjectMetInMemberExpressionNode = globalObject;
    ESValue* lastExpressionStatementValue = &instance->m_lastExpressionStatementValue;
    ESValue* nonActivitionModeLocalValuePointer = ec->cachedDeclarativeEnvironmentRecordESValue();
    NextInstruction:
    ByteCode* currentCode = (ByteCode *)(&code[programCounter]);
    /*
    {
        size_t tt = (size_t)currentCode;
        ASSERT(tt % sizeof(size_t) == 0);
        if(currentCode->m_node)
            printf("execute %p %u \t(nodeinfo %d)\t",currentCode, (unsigned)programCounter, (int)currentCode->m_node->sourceLocation().m_lineNumber);
        else
            printf("execute %p %u \t(nodeinfo null)\t",currentCode, (unsigned)programCounter);
        currentCode->dump();
    }
    */
    goto *currentCode->m_opcode;

    PushOpcodeLbl:
    {
        Push* pushCode = (Push*)currentCode;
        push<ESValue>(stack, sp, pushCode->m_value);
        executeNextCode<Push>(programCounter);
        goto NextInstruction;
    }

    PopOpcodeLbl:
    {
        Pop* popCode = (Pop*)currentCode;
        pop<ESValue>(stack, sp);
        executeNextCode<Pop>(programCounter);
        goto NextInstruction;
    }

    PopExpressionStatementOpcodeLbl:
    {
        ESValue* t = pop<ESValue>(stack, sp);
        *lastExpressionStatementValue = *t;
        executeNextCode<PopExpressionStatement>(programCounter);
        goto NextInstruction;
    }

    PushIntoTempStackOpcodeLbl:
    {
        push<ESValue>(tmpStack, tmpSp, *pop<ESValue>(stack, sp));
        executeNextCode<PushIntoTempStack>(programCounter);
        goto NextInstruction;
    }

    PopFromTempStackOpcodeLbl:
    {
        push<ESValue>(stack, sp, *pop<ESValue>(tmpStack, tmpSp));
        executeNextCode<PopFromTempStack>(programCounter);
        goto NextInstruction;
    }

    GetByIdOpcodeLbl:
    {
        GetById* code = (GetById*)currentCode;
        if (LIKELY(code->m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
            push<ESValue>(stack, sp, code->m_cachedSlot.readDataProperty());
        } else {
            ESSlotAccessor slot = ec->resolveBinding(code->m_name, code->m_nonAtomicName);
            if(LIKELY(slot.hasData())) {
                code->m_cachedSlot = ESSlotAccessor(slot);
                code->m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
                push<ESValue>(stack, sp, code->m_cachedSlot.readDataProperty());
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

    GetByIndexOpcodeLbl:
    {
        GetByIndex* code = (GetByIndex*)currentCode;
        ASSERT(code->m_index < ec->environment()->record()->toDeclarativeEnvironmentRecord()->innerIdentifiers()->size());
        push<ESValue>(stack, sp, nonActivitionModeLocalValuePointer[code->m_index]);
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
        push<ESValue>(stack, sp, *env->record()->toDeclarativeEnvironmentRecord()->bindingValueForActivationMode(code->m_index));
        executeNextCode<GetByIndexWithActivation>(programCounter);
        goto NextInstruction;
    }

    PutByIdOpcodeLbl:
    {
        PutById* code = (PutById*)currentCode;
        ESValue* value = peek<ESValue>(stack, sp);

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
        nonActivitionModeLocalValuePointer[code->m_index] = *peek<ESValue>(stack, sp);
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
        *env->record()->toDeclarativeEnvironmentRecord()->bindingValueForActivationMode(code->m_index) = *peek<ESValue>(stack, sp);
        executeNextCode<PutByIndexWithActivation>(programCounter);
        goto NextInstruction;
    }

    PutInObjectOpcodeLbl:
    {
        PutInObject* code = (PutInObject*)currentCode;
        ESValue* value = pop<ESValue>(stack, sp);
        ESValue* property = pop<ESValue>(stack, sp);
        ESValue* willBeObject = pop<ESValue>(stack, sp);

        ESObject* obj;
        if(willBeObject->isObject())
            obj = willBeObject->asESPointer()->asESObject();
        else
            obj = willBeObject->toObject();

        if(obj->isHiddenClassMode() && !obj->isESArrayObject()) {
            ESString* val = property->toString();
            if(code->m_cachedHiddenClass == obj->hiddenClass() && (val == code->m_cachedPropertyValue || *val == *code->m_cachedPropertyValue)) {
                obj->writeHiddenClass(code->m_cachedIndex, *value);
                push<ESValue>(stack, sp, *value);
                executeNextCode<PutInObject>(programCounter);
                goto NextInstruction;
            } else {
                size_t idx = obj->hiddenClass()->findProperty(val);
                if(idx != SIZE_MAX) {
                    code->m_cachedHiddenClass = obj->hiddenClass();
                    code->m_cachedPropertyValue = val;
                    code->m_cachedIndex = idx;
                    obj->writeHiddenClass(code->m_cachedIndex, *value);
                    push<ESValue>(stack, sp, *value);
                    executeNextCode<PutInObject>(programCounter);
                    goto NextInstruction;
                } else {
                    code->m_cachedHiddenClass = nullptr;
                    obj->set(*property, *value, true);
                    push<ESValue>(stack, sp, *value);
                    executeNextCode<PutInObject>(programCounter);
                    goto NextInstruction;
                }
            }
        } else {
            obj->set(*property, *value, true);
            push<ESValue>(stack, sp, *value);
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
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
        push<ESValue>(stack, sp, ESValue(left->abstractEqualsTo(*right)));
        executeNextCode<Equal>(programCounter);
        goto NextInstruction;
    }

    NotEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
        push<ESValue>(stack, sp, ESValue(!left->abstractEqualsTo(*right)));
        executeNextCode<NotEqual>(programCounter);
        goto NextInstruction;
    }

    StrictEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
        push<ESValue>(stack, sp, ESValue(left->equalsTo(*right)));
        executeNextCode<StrictEqual>(programCounter);
        goto NextInstruction;
    }

    NotStrictEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
        push<ESValue>(stack, sp, ESValue(!left->equalsTo(*right)));
        executeNextCode<NotStrictEqual>(programCounter);
        goto NextInstruction;
    }

    BitwiseAndOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
        push<ESValue>(stack, sp, ESValue(left->toInt32() & right->toInt32()));
        executeNextCode<BitwiseAnd>(programCounter);
        goto NextInstruction;
    }

    BitwiseOrOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
        push<ESValue>(stack, sp, ESValue(left->toInt32() | right->toInt32()));
        executeNextCode<BitwiseOr>(programCounter);
        goto NextInstruction;
    }

    BitwiseXorOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
        push<ESValue>(stack, sp, ESValue(left->toInt32() ^ right->toInt32()));
        executeNextCode<BitwiseXor>(programCounter);
        goto NextInstruction;
    }

    LeftShiftOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
        int32_t lnum = left->toInt32();
        int32_t rnum = right->toInt32();
        lnum <<= ((unsigned int)rnum) & 0x1F;
        push<ESValue>(stack, sp, ESValue(lnum));
        executeNextCode<LeftShift>(programCounter);
        goto NextInstruction;
    }

    SignedRightShiftOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
        int32_t lnum = left->toInt32();
        int32_t rnum = right->toInt32();
        lnum >>= ((unsigned int)rnum) & 0x1F;
        push<ESValue>(stack, sp, ESValue(lnum));
        executeNextCode<SignedRightShift>(programCounter);
        goto NextInstruction;
    }

    UnsignedRightShiftOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
        int32_t lnum = left->toInt32();
        int32_t rnum = right->toInt32();
        lnum = ((unsigned int)lnum) >> (((unsigned int)rnum) & 0x1F);
        push<ESValue>(stack, sp, ESValue(lnum));
        executeNextCode<UnsignedRightShift>(programCounter);
        goto NextInstruction;
    }

    LessThanOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
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
        push<ESValue>(stack, sp, ESValue(b));
        executeNextCode<LessThan>(programCounter);
        goto NextInstruction;
    }

    LessThanOrEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
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
        push<ESValue>(stack, sp, ESValue(b));
        executeNextCode<LessThanOrEqual>(programCounter);
        goto NextInstruction;
    }

    GreaterThanOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
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
        push<ESValue>(stack, sp, ESValue(b));
        executeNextCode<GreaterThan>(programCounter);
        goto NextInstruction;
    }

    GreaterThanOrEqualOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
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
        push<ESValue>(stack, sp, ESValue(b));
        executeNextCode<GreaterThanOrEqual>(programCounter);
        goto NextInstruction;
    }

    PlusOpcodeLbl:
    {
        push<ESValue>(stack, sp, plusOperation(*pop<ESValue>(stack, sp), *pop<ESValue>(stack, sp)));
        executeNextCode<Plus>(programCounter);
        goto NextInstruction;
    }

    MinusOpcodeLbl:
    {
        push<ESValue>(stack, sp, minusOperation(*pop<ESValue>(stack, sp), *pop<ESValue>(stack, sp)));
        executeNextCode<Minus>(programCounter);
        goto NextInstruction;
    }

    MultiplyOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
        push<ESValue>(stack, sp, ESValue(left->toNumber() * right->toNumber()));
        executeNextCode<Multiply>(programCounter);
        goto NextInstruction;
    }

    DivisionOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
        push<ESValue>(stack, sp, ESValue(left->toNumber() / right->toNumber()));
        executeNextCode<Division>(programCounter);
        goto NextInstruction;
    }

    ModOpcodeLbl:
    {
        ESValue* right = pop<ESValue>(stack, sp);
        ESValue* left = pop<ESValue>(stack, sp);
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
        push<ESValue>(stack, sp, ret);
        executeNextCode<Mod>(programCounter);
        goto NextInstruction;
    }

    IncrementOpcodeLbl:
    {
        push<ESValue>(stack, sp, plusOperation(*pop<ESValue>(stack, sp), ESValue(1)));
        executeNextCode<Division>(programCounter);
        goto NextInstruction;
    }

    DecrementOpcodeLbl:
    {
        push<ESValue>(stack, sp, minusOperation(*pop<ESValue>(stack, sp), ESValue(1)));
        executeNextCode<Division>(programCounter);
        goto NextInstruction;
    }

    BitwiseNotOpcodeLbl:
    {
        push<ESValue>(stack, sp, ESValue(~pop<ESValue>(stack, sp)->toInt32()));
        executeNextCode<BitwiseNot>(programCounter);
        goto NextInstruction;
    }

    LogicalNotOpcodeLbl:
    {
        push<ESValue>(stack, sp, ESValue(!pop<ESValue>(stack, sp)->toBoolean()));
        executeNextCode<LogicalNot>(programCounter);
        goto NextInstruction;
    }

    UnaryMinusOpcodeLbl:
    {
        push<ESValue>(stack, sp, ESValue(-pop<ESValue>(stack, sp)->toNumber()));
        executeNextCode<UnaryMinus>(programCounter);
        goto NextInstruction;
    }

    UnaryPlusOpcodeLbl:
    {
        push<ESValue>(stack, sp, ESValue(pop<ESValue>(stack, sp)->toNumber()));
        executeNextCode<UnaryPlus>(programCounter);
        goto NextInstruction;
    }

    CreateObjectOpcodeLbl:
    {
        CreateObject* code = (CreateObject*)currentCode;
        ESObject* obj = ESObject::create(code->m_keyCount);
        obj->setConstructor(globalObject->object());
        obj->set__proto__(globalObject->objectPrototype());
        push<ESValue>(stack, sp, obj);
        executeNextCode<CreateObject>(programCounter);
        goto NextInstruction;
    }

    CreateArrayOpcodeLbl:
    {
        CreateArray* code = (CreateArray*)currentCode;
        ESArrayObject* arr = ESArrayObject::create(code->m_keyCount, globalObject->arrayPrototype());
        push<ESValue>(stack, sp, arr);
        executeNextCode<CreateArray>(programCounter);
        goto NextInstruction;
    }

    SetObjectOpcodeLbl:
    {
        SetObject* code = (SetObject*)currentCode;
        ESValue* value = pop<ESValue>(stack, sp);
        ESValue* key = pop<ESValue>(stack, sp);
        peek<ESValue>(stack, sp)->asESPointer()->asESObject()->set(*key, *value);
        executeNextCode<SetObject>(programCounter);
        goto NextInstruction;
    }

    GetObjectOpcodeLbl:
    {
        GetObject* code = (GetObject*)currentCode;

        ESValue* property = pop<ESValue>(stack, sp);
        ESValue* willBeObject = pop<ESValue>(stack, sp);

        if(UNLIKELY(willBeObject->isESString())) {
            if(property->isInt32()) {
               int prop_val = property->toInt32();
               if(LIKELY(0 <= prop_val && prop_val < willBeObject->asESString()->length())) {
                   char16_t c = willBeObject->asESString()->string().data()[prop_val];
                   if(LIKELY(c < ESCARGOT_ASCII_TABLE_MAX)) {
                       push<ESValue>(stack, sp, strings->asciiTable[c]);
                       executeNextCode<GetObject>(programCounter);
                       goto NextInstruction;
                   } else {
                       push<ESValue>(stack, sp, ESString::create(c));
                       executeNextCode<GetObject>(programCounter);
                       goto NextInstruction;
                   }
               } else {
                   push<ESValue>(stack, sp, ESValue());
                   executeNextCode<GetObject>(programCounter);
                   goto NextInstruction;
               }
               push<ESValue>(stack, sp, willBeObject->asESString()->substring(prop_val, prop_val+1));
               executeNextCode<GetObject>(programCounter);
               goto NextInstruction;
            } else {
                globalObject->stringObjectProxy()->setString(willBeObject->asESString());
                ESValue ret = globalObject->stringObjectProxy()->find(*property, true);
                if(!ret.isEmpty()) {
                    if(ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->codeBlock()->m_isBuiltInFunction) {
                        lastESObjectMetInMemberExpressionNode = (globalObject->stringObjectProxy());
                        push<ESValue>(stack, sp, ret);
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
                    push<ESValue>(stack, sp, ret);
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
                push<ESValue>(stack, sp, obj->readHiddenClass(code->m_cachedIndex));
                executeNextCode<GetObject>(programCounter);
                goto NextInstruction;
            } else {
                size_t idx = obj->hiddenClass()->findProperty(val);
                if(idx != SIZE_MAX) {
                    code->m_cachedHiddenClass = obj->hiddenClass();
                    code->m_cachedPropertyValue = val;
                    code->m_cachedIndex = idx;
                    push<ESValue>(stack, sp, obj->readHiddenClass(idx));
                    executeNextCode<GetObject>(programCounter);
                    goto NextInstruction;
                } else {
                    code->m_cachedHiddenClass = nullptr;
                    ESValue v = obj->findOnlyPrototype(val);
                    if(v.isEmpty()) {
                        push<ESValue>(stack, sp, ESValue());
                        executeNextCode<GetObject>(programCounter);
                        goto NextInstruction;
                    }
                    push<ESValue>(stack, sp, v);
                    executeNextCode<GetObject>(programCounter);
                    goto NextInstruction;
                }
            }
        } else {
            push<ESValue>(stack, sp, obj->get(*property, true));
            executeNextCode<GetObject>(programCounter);
            goto NextInstruction;
        }
        RELEASE_ASSERT_NOT_REACHED();
        goto NextInstruction;
    }

    GetObjectWithPeekingOpcodeLbl:
    {
        GetObject* code = (GetObject*)currentCode;

        ESValue* property = pop<ESValue>(stack, sp);
        ESValue* willBeObject = pop<ESValue>(stack, sp);

        sp += sizeof(ESValue) * 2;
#ifndef NDEBUG
        sp += sizeof(size_t) * 2;
#endif

        if(UNLIKELY(willBeObject->isESString())) {
            if(property->isInt32()) {
               int prop_val = property->toInt32();
               if(LIKELY(0 <= prop_val && prop_val < willBeObject->asESString()->length())) {
                   char16_t c = willBeObject->asESString()->string().data()[prop_val];
                   if(LIKELY(c < ESCARGOT_ASCII_TABLE_MAX)) {
                       push<ESValue>(stack, sp, strings->asciiTable[c]);
                       executeNextCode<GetObjectWithPeeking>(programCounter);
                       goto NextInstruction;
                   } else {
                       push<ESValue>(stack, sp, ESString::create(c));
                       executeNextCode<GetObjectWithPeeking>(programCounter);
                       goto NextInstruction;
                   }
               } else {
                   push<ESValue>(stack, sp, ESValue());
                   executeNextCode<GetObjectWithPeeking>(programCounter);
                   goto NextInstruction;
               }
               push<ESValue>(stack, sp, willBeObject->asESString()->substring(prop_val, prop_val+1));
               executeNextCode<GetObjectWithPeeking>(programCounter);
               goto NextInstruction;
            } else {
                globalObject->stringObjectProxy()->setString(willBeObject->asESString());
                ESValue ret = globalObject->stringObjectProxy()->find(*property, true);
                if(!ret.isEmpty()) {
                    if(ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->codeBlock()->m_isBuiltInFunction) {
                        lastESObjectMetInMemberExpressionNode = (globalObject->stringObjectProxy());
                        push<ESValue>(stack, sp, ret);
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
                    push<ESValue>(stack, sp, ret);
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
                push<ESValue>(stack, sp, obj->readHiddenClass(code->m_cachedIndex));
                executeNextCode<GetObjectWithPeeking>(programCounter);
                goto NextInstruction;
            } else {
                size_t idx = obj->hiddenClass()->findProperty(val);
                if(idx != SIZE_MAX) {
                    code->m_cachedHiddenClass = obj->hiddenClass();
                    code->m_cachedPropertyValue = val;
                    code->m_cachedIndex = idx;
                    push<ESValue>(stack, sp, obj->readHiddenClass(idx));
                    executeNextCode<GetObjectWithPeeking>(programCounter);
                    goto NextInstruction;
                } else {
                    code->m_cachedHiddenClass = nullptr;
                    ESValue v = obj->findOnlyPrototype(val);
                    if(v.isEmpty()) {
                        push<ESValue>(stack, sp, ESValue());
                        executeNextCode<GetObjectWithPeeking>(programCounter);
                        goto NextInstruction;
                    }
                    push<ESValue>(stack, sp, v);
                    executeNextCode<GetObjectWithPeeking>(programCounter);
                    goto NextInstruction;
                }
            }
        } else {
            push<ESValue>(stack, sp, obj->get(*property, true));
            executeNextCode<GetObjectWithPeeking>(programCounter);
            goto NextInstruction;
        }
        RELEASE_ASSERT_NOT_REACHED();
        goto NextInstruction;
    }

    CreateFunctionOpcodeLbl:
    {
        CreateFunction* code = (CreateFunction*)currentCode;
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
            push<ESValue>(stack, sp, function);
        executeNextCode<CreateFunction>(programCounter);
        goto NextInstruction;
    }

    ExecuteNativeFunctionOpcodeLbl:
    {
        ExecuteNativeFunction* code = (ExecuteNativeFunction*)currentCode;
        ASSERT(bp == sp);
        return code->m_fn(instance);
    }
    PrepareFunctionCallOpcodeLbl:
    {
        lastESObjectMetInMemberExpressionNode = globalObject;
        executeNextCode<PrepareFunctionCall>(programCounter);
        goto NextInstruction;
    }

    CallFunctionOpcodeLbl:
    {
        size_t argc = (size_t)pop<ESValue>(stack, sp)->asInt32();
#ifdef NDEBUG
        sp -= argc * sizeof(ESValue);
#else
        sp -= argc * sizeof(ESValue);
        sp -= argc * sizeof(size_t);
        {
            ESValue* arguments = (ESValue *)&(((char *)stack)[sp]);
            for(size_t i = 0; i < argc ; i ++) {
                arguments[i] = *((ESValue *)&(((char *)stack)[sp + i*(sizeof(ESValue)+sizeof(size_t))]));
            }
        }
#endif
        ESValue* arguments = (ESValue *)&(((char *)stack)[sp]);
        push<ESValue>(stack, sp, ESFunctionObject::call(instance, *pop<ESValue>(stack, sp), lastESObjectMetInMemberExpressionNode, arguments, argc, false));
        executeNextCode<CallFunction>(programCounter);
        goto NextInstruction;
    }

    NewFunctionCallOpcodeLbl:
    {
        size_t argc = (size_t)pop<ESValue>(stack, sp)->asInt32();
#ifdef NDEBUG
        sp -= argc * sizeof(ESValue);
#else
        sp -= argc * sizeof(ESValue);
        sp -= argc * sizeof(size_t);
        {
            ESValue* arguments = (ESValue *)&(((char *)stack)[sp]);
            for(size_t i = 0; i < argc ; i ++) {
                arguments[i] = *((ESValue *)&(((char *)stack)[sp + i*(sizeof(ESValue)+sizeof(size_t))]));
            }
        }
#endif
        ESValue* arguments = (ESValue *)&(((char *)stack)[sp]);
        ESValue fn = *pop<ESValue>(stack, sp);
        if(!fn.isESPointer() || !fn.asESPointer()->isESFunctionObject())
            throw ESValue(TypeError::create(ESString::create(u"constructor is not an function object")));
        ESFunctionObject* function = fn.asESPointer()->asESFunctionObject();
        ESObject* receiver;
        if (function == instance->globalObject()->date()) {
            receiver = ESDateObject::create();
        } else if (function == instance->globalObject()->array()) {
            receiver = ESArrayObject::create(0);
        } else if (function == instance->globalObject()->string()) {
            receiver = ESStringObject::create();
        } else if (function == instance->globalObject()->regexp()) {
            receiver = ESRegExpObject::create(strings->emptyESString,ESRegExpObject::Option::None);
        } else if (function == instance->globalObject()->boolean()) {
            receiver = ESBooleanObject::create(ESValue(ESValue::ESFalseTag::ESFalse));
        } else if (function == instance->globalObject()->error()) {
            receiver = ESErrorObject::create();
        } else if (function == instance->globalObject()->referenceError()) {
            receiver = ReferenceError::create();
        } else if (function == instance->globalObject()->typeError()) {
            receiver = TypeError::create();
        } else if (function == instance->globalObject()->syntaxError()) {
            receiver = SyntaxError::create();
        } else if (function == instance->globalObject()->rangeError()) {
            receiver = RangeError::create();
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
            push<ESValue>(stack, sp, res);
        } else
            push<ESValue>(stack, sp, receiver);
        executeNextCode<NewFunctionCall>(programCounter);
        goto NextInstruction;
    }

    ReturnFunctionOpcodeLbl:
    {
        ASSERT(bp == sp);
        return ESValue();
    }

    ReturnFunctionWithValueOpcodeLbl:
    {
        ESValue* ret = pop<ESValue>(stack, sp);
        ASSERT(bp == sp);
        return *ret;
    }

    JumpOpcodeLbl:
    {
        Jump* code = (Jump *)currentCode;
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        programCounter = code->m_jumpPosition;
        goto NextInstruction;
    }

    JumpIfTopOfStackValueIsFalseOpcodeLbl:
    {
        JumpIfTopOfStackValueIsFalse* code = (JumpIfTopOfStackValueIsFalse *)currentCode;
        ESValue* top = pop<ESValue>(stack, sp);
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        if(!top->toBoolean())
            programCounter = code->m_jumpPosition;
        else
            executeNextCode<JumpIfTopOfStackValueIsFalse>(programCounter);
        goto NextInstruction;
    }

    JumpIfTopOfStackValueIsTrueOpcodeLbl:
    {
        JumpIfTopOfStackValueIsTrue* code = (JumpIfTopOfStackValueIsTrue *)currentCode;
        ESValue* top = pop<ESValue>(stack, sp);
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        if(top->toBoolean())
            programCounter = code->m_jumpPosition;
        else
            executeNextCode<JumpIfTopOfStackValueIsTrue>(programCounter);
        goto NextInstruction;
    }

    JumpIfTopOfStackValueIsFalseWithPeekingOpcodeLbl:
    {
        JumpIfTopOfStackValueIsFalseWithPeeking* code = (JumpIfTopOfStackValueIsFalseWithPeeking *)currentCode;
        ESValue* top = peek<ESValue>(stack, sp);
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        if(!top->toBoolean())
            programCounter = code->m_jumpPosition;
        else
            executeNextCode<JumpIfTopOfStackValueIsFalseWithPeeking>(programCounter);
        goto NextInstruction;
    }

    JumpIfTopOfStackValueIsTrueWithPeekingOpcodeLbl:
    {
        JumpIfTopOfStackValueIsTrueWithPeeking* code = (JumpIfTopOfStackValueIsTrueWithPeeking *)currentCode;
        ESValue* top = peek<ESValue>(stack, sp);
        ASSERT(code->m_jumpPosition != SIZE_MAX);
        if(top->toBoolean())
            programCounter = code->m_jumpPosition;
        else
            executeNextCode<JumpIfTopOfStackValueIsTrueWithPeeking>(programCounter);
        goto NextInstruction;
    }

    DuplicateTopOfStackValueOpcodeLbl:
    {
        push<ESValue>(stack, sp, *peek<ESValue>(stack, sp));
        executeNextCode<DuplicateTopOfStackValue>(programCounter);
        goto NextInstruction;
    }

    TryOpcodeLbl:
    {
        Try* code = (Try *)currentCode;
        LexicalEnvironment* oldEnv = ec->environment();
        ExecutionContext* backupedEC = ec;
        try {
            interpret(instance, codeBlock, programCounter + sizeof(Try));
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
            interpret(instance, codeBlock, code->m_catchPosition);
            instance->currentExecutionContext()->setEnvironment(oldEnv);
        }
        programCounter = code->m_statementEndPosition;
        goto NextInstruction;
    }

    TryCatchBodyEndOpcodeLbl:
    {
        ASSERT(bp == sp);
        return ESValue();
    }

    ThrowOpcodeLbl:
    {
        ESValue* v = pop<ESValue>(stack, sp);
        throw *v;
        goto NextInstruction;
    }

    ThisOpcodeLbl:
    {
        push<ESValue>(stack, sp, ESValue(ec->resolveThisBinding()));
        executeNextCode<This>(programCounter);
        goto NextInstruction;
    }

    EndOpcodeLbl:
    {
        ASSERT(sp == 0);
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
        DUMP_BYTE_CODE(Push);
        DUMP_BYTE_CODE(PopExpressionStatement);
        DUMP_BYTE_CODE(Pop);
        DUMP_BYTE_CODE(PushIntoTempStack);
        DUMP_BYTE_CODE(PopFromTempStack);
        DUMP_BYTE_CODE(GetById);
        DUMP_BYTE_CODE(GetByIndex);
        DUMP_BYTE_CODE(GetByIndexWithActivation);
        DUMP_BYTE_CODE(PutById);
        DUMP_BYTE_CODE(PutByIndex);
        DUMP_BYTE_CODE(PutByIndexWithActivation);
        DUMP_BYTE_CODE(PutInObject);
        DUMP_BYTE_CODE(CreateBinding);
        DUMP_BYTE_CODE(Equal);
        DUMP_BYTE_CODE(NotEqual);
        DUMP_BYTE_CODE(StrictEqual);
        DUMP_BYTE_CODE(NotStrictEqual);
        DUMP_BYTE_CODE(BitwiseAnd);
        DUMP_BYTE_CODE(BitwiseOr);
        DUMP_BYTE_CODE(BitwiseXor);
        DUMP_BYTE_CODE(LeftShift);
        DUMP_BYTE_CODE(SignedRightShift);
        DUMP_BYTE_CODE(UnsignedRightShift);
        DUMP_BYTE_CODE(LessThan);
        DUMP_BYTE_CODE(LessThanOrEqual);
        DUMP_BYTE_CODE(GreaterThan);
        DUMP_BYTE_CODE(GreaterThanOrEqual);
        DUMP_BYTE_CODE(Plus);
        DUMP_BYTE_CODE(Minus);
        DUMP_BYTE_CODE(Multiply);
        DUMP_BYTE_CODE(Division);
        DUMP_BYTE_CODE(Mod);
        DUMP_BYTE_CODE(Increment);
        DUMP_BYTE_CODE(Decrement);
        DUMP_BYTE_CODE(BitwiseNot);
        DUMP_BYTE_CODE(LogicalNot);
        DUMP_BYTE_CODE(UnaryMinus);
        DUMP_BYTE_CODE(UnaryPlus);
        DUMP_BYTE_CODE(CreateObject);
        DUMP_BYTE_CODE(CreateArray);
        DUMP_BYTE_CODE(SetObject);
        DUMP_BYTE_CODE(GetObject);
        DUMP_BYTE_CODE(GetObjectWithPeeking);
        DUMP_BYTE_CODE(CreateFunction);
        DUMP_BYTE_CODE(ExecuteNativeFunction);
        DUMP_BYTE_CODE(PrepareFunctionCall);
        DUMP_BYTE_CODE(CallFunction);
        DUMP_BYTE_CODE(NewFunctionCall);
        DUMP_BYTE_CODE(ReturnFunction);
        DUMP_BYTE_CODE(ReturnFunctionWithValue);
        DUMP_BYTE_CODE(Jump);
        DUMP_BYTE_CODE(JumpIfTopOfStackValueIsFalse);
        DUMP_BYTE_CODE(JumpIfTopOfStackValueIsTrue);
        DUMP_BYTE_CODE(JumpIfTopOfStackValueIsFalseWithPeeking);
        DUMP_BYTE_CODE(JumpIfTopOfStackValueIsTrueWithPeeking);
        DUMP_BYTE_CODE(DuplicateTopOfStackValue);
        DUMP_BYTE_CODE(Try);
        DUMP_BYTE_CODE(TryCatchBodyEnd);
        DUMP_BYTE_CODE(Throw);
        DUMP_BYTE_CODE(This);
        DUMP_BYTE_CODE(End);
        default:
            RELEASE_ASSERT_NOT_REACHED();
            break;
        };
    }
    printf("dumpBytecode...<<<<<<<<<<<<<<<<<<<<<<\n");
}

#endif

}
