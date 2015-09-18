#include "Escargot.h"
#include "bytecode/ByteCode.h"

namespace escargot {

ESValue interpret(ESVMInstance* instance, CodeBlock* codeBlock, size_t programCounter)
{
    //GC_gcollect();
    //dumpBytecode(codeBlock);
    //void* stack = instance->m_stack;
    //size_t& sp = instance->m_sp;
    char stackBuf[1024*4];
    void* stack = stackBuf;
    unsigned sp  = 0;
    unsigned bp = 0;
    char* code = codeBlock->m_code.data();
    ExecutionContext* ec = instance->currentExecutionContext();
    GlobalObject* globalObject = instance->globalObject();
    ESObject* lastESObjectMetInMemberExpressionNode = globalObject;
    ESValue* lastExpressionStatementValue = &instance->m_lastExpressionStatementValue;
    ESValue* nonActivitionModeLocalValuePointer = ec->cachedDeclarativeEnvironmentRecordESValue();
    while(1) {
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
        switch(currentCode->m_opcode) {
        case PushOpcode:
        {
            Push* pushCode = (Push*)currentCode;
            push<ESValue>(stack, sp, pushCode->m_value);
            excuteNextCode<Push>(programCounter);
            break;
        }

        case PopOpcode:
        {
            Pop* popCode = (Pop*)currentCode;
            pop<ESValue>(stack, sp);
            excuteNextCode<Pop>(programCounter);
            break;
        }

        case PopExpressionStatementOpcode:
        {
            ESValue* t = pop<ESValue>(stack, sp);
            *lastExpressionStatementValue = *t;
            excuteNextCode<PopExpressionStatement>(programCounter);
            break;
        }

        case GetByIdOpcode:
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
            excuteNextCode<GetById>(programCounter);
            break;
        }

        case GetByIndexOpcode:
        {
            GetByIndex* code = (GetByIndex*)currentCode;
            ASSERT(code->m_index < ec->environment()->record()->toDeclarativeEnvironmentRecord()->innerIdentifiers()->size());
            push<ESValue>(stack, sp, nonActivitionModeLocalValuePointer[code->m_index]);
            excuteNextCode<GetByIndex>(programCounter);
            break;
        }

        case GetByIndexWithActivationOpcode:
        {
            GetByIndexWithActivation* code = (GetByIndexWithActivation*)currentCode;
            LexicalEnvironment* env = ec->environment();
            for(unsigned i = 0; i < code->m_upIndex; i ++) {
                env = env->outerEnvironment();
            }
            ASSERT(env->record()->isDeclarativeEnvironmentRecord());
            push<ESValue>(stack, sp, *env->record()->toDeclarativeEnvironmentRecord()->bindingValueForActivationMode(code->m_index));
            excuteNextCode<GetByIndexWithActivation>(programCounter);
            break;
        }

        case ResolveAddressByIdOpcode:
        {
            ResolveAddressById* code = (ResolveAddressById*)currentCode;

            if (LIKELY(code->m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
                push<ESSlotAccessor>(stack, sp, code->m_cachedSlot);
            } else {
                ExecutionContext* ec = instance->currentExecutionContext();
                ESSlotAccessor slot = ec->resolveBinding(code->m_name, code->m_nonAtomicName);

                if(LIKELY(slot.hasData())) {
                    code->m_cachedSlot = slot;
                    code->m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
                    push<ESSlotAccessor>(stack, sp, slot);
                } else {
                    //CHECKTHIS true, true, false is right?
                    instance->invalidateIdentifierCacheCheckCount();
                    push<ESSlotAccessor>(stack, sp, globalObject->definePropertyOrThrow(code->m_nonAtomicName, true, true, true));
                }
            }

            excuteNextCode<ResolveAddressById>(programCounter);
            break;
        }

        case ResolveAddressByIndexOpcode:
        {
            ResolveAddressByIndex* code = (ResolveAddressByIndex*)currentCode;
            push<ESSlotAccessor>(stack, sp, ESSlotAccessor(&nonActivitionModeLocalValuePointer[code->m_index]));
            excuteNextCode<ResolveAddressByIndex>(programCounter);
            break;
        }

        case ResolveAddressByIndexWithActivationOpcode:
        {
            ResolveAddressByIndexWithActivation* code = (ResolveAddressByIndexWithActivation*)currentCode;
            LexicalEnvironment* env = ec->environment();
            for(unsigned i = 0; i < code->m_upIndex; i ++) {
                env = env->outerEnvironment();
            }
            ASSERT(env->record()->isDeclarativeEnvironmentRecord());
            push<ESSlotAccessor>(stack, sp, ESSlotAccessor(env->record()->toDeclarativeEnvironmentRecord()->bindingValueForActivationMode(code->m_index)));
            excuteNextCode<ResolveAddressByIndexWithActivation>(programCounter);
            break;
        }

        case ResolveAddressInObjectOpcode:
        {
            ResolveAddressInObject* code = (ResolveAddressInObject*)currentCode;
            ESValue* property = pop<ESValue>(stack, sp);
            ESValue* willBeObject = pop<ESValue>(stack, sp);

            ESObject* obj;
            if(willBeObject->isObject())
                obj = willBeObject->asESPointer()->asESObject();
            else
                obj = willBeObject->toObject();

            lastESObjectMetInMemberExpressionNode = obj;

            if(obj->isHiddenClassMode() && !obj->isESArrayObject()) {
                ESString* val = property->toString();
                if(code->m_cachedHiddenClass == obj->hiddenClass() && (val == code->m_cachedPropertyValue || *val == *code->m_cachedPropertyValue)) {
                    push<ESSlotAccessor>(stack, sp, ESSlotAccessor(obj, val, code->m_cachedIndex));
                    excuteNextCode<ResolveAddressInObject>(programCounter);
                } else {
                    size_t idx = obj->hiddenClass()->findProperty(val);
                    if(idx != SIZE_MAX) {
                        code->m_cachedHiddenClass = obj->hiddenClass();
                        code->m_cachedPropertyValue = val;
                        code->m_cachedIndex = idx;
                        push<ESSlotAccessor>(stack, sp, ESSlotAccessor(obj, val, code->m_cachedIndex));
                        excuteNextCode<ResolveAddressInObject>(programCounter);
                        break;
                    } else {
                        code->m_cachedHiddenClass = nullptr;
                        push<ESSlotAccessor>(stack, sp, ESSlotAccessor(obj, *property));
                        excuteNextCode<ResolveAddressInObject>(programCounter);
                        break;
                    }
                }
            } else {
                push<ESSlotAccessor>(stack, sp, ESSlotAccessor(obj, *property));
                excuteNextCode<ResolveAddressInObject>(programCounter);
                break;
            }
            break;
        }

        case ReferenceTopValueWithPeekingOpcode:
        {
            push<ESValue>(stack, sp, peek<ESSlotAccessor>(stack, sp)->value());
            excuteNextCode<ReferenceTopValueWithPeeking>(programCounter);
            break;
        }

        case PutOpcode:
        {
            ESValue* value = pop<ESValue>(stack, sp);
            ESSlotAccessor* slot = pop<ESSlotAccessor>(stack, sp);
            slot->setValue(*value);
            push<ESValue>(stack, sp, *value);
            excuteNextCode<Put>(programCounter);
            break;
        }

        case PutReverseStackOpcode:
        {
            ESSlotAccessor* slot = pop<ESSlotAccessor>(stack, sp);
            ESValue* value = pop<ESValue>(stack, sp);
            slot->setValue(*value);
            push<ESValue>(stack, sp, *value);
            excuteNextCode<PutReverseStack>(programCounter);
            break;
        }

        case CreateBindingOpcode:
        {
            CreateBinding* code = (CreateBinding*)currentCode;
            ec->environment()->record()->createMutableBindingForAST(code->m_name,
                    code->m_nonAtomicName, false);
            excuteNextCode<CreateBinding>(programCounter);
            break;
        }

        case EqualOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            push<ESValue>(stack, sp, ESValue(left->abstractEqualsTo(*right)));
            excuteNextCode<Equal>(programCounter);
            break;
        }

        case NotEqualOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            push<ESValue>(stack, sp, ESValue(!left->abstractEqualsTo(*right)));
            excuteNextCode<NotEqual>(programCounter);
            break;
        }

        case StrictEqualOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            push<ESValue>(stack, sp, ESValue(left->equalsTo(*right)));
            excuteNextCode<StrictEqual>(programCounter);
            break;
        }

        case NotStrictEqualOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            push<ESValue>(stack, sp, ESValue(!left->equalsTo(*right)));
            excuteNextCode<NotStrictEqual>(programCounter);
            break;
        }

        case BitwiseAndOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            push<ESValue>(stack, sp, ESValue(left->toInt32() & right->toInt32()));
            excuteNextCode<BitwiseAnd>(programCounter);
            break;
        }

        case BitwiseOrOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            push<ESValue>(stack, sp, ESValue(left->toInt32() | right->toInt32()));
            excuteNextCode<BitwiseOr>(programCounter);
            break;
        }

        case BitwiseXorOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            push<ESValue>(stack, sp, ESValue(left->toInt32() ^ right->toInt32()));
            excuteNextCode<BitwiseXor>(programCounter);
            break;
        }

        case LeftShiftOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            int32_t lnum = left->toInt32();
            int32_t rnum = right->toInt32();
            lnum <<= ((unsigned int)rnum) & 0x1F;
            push<ESValue>(stack, sp, ESValue(lnum));
            excuteNextCode<LeftShift>(programCounter);
            break;
        }

        case SignedRightShiftOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            int32_t lnum = left->toInt32();
            int32_t rnum = right->toInt32();
            lnum >>= ((unsigned int)rnum) & 0x1F;
            push<ESValue>(stack, sp, ESValue(lnum));
            excuteNextCode<SignedRightShift>(programCounter);
            break;
        }

        case UnsignedRightShiftOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            int32_t lnum = left->toInt32();
            int32_t rnum = right->toInt32();
            lnum = ((unsigned int)lnum) >> (((unsigned int)rnum) & 0x1F);
            push<ESValue>(stack, sp, ESValue(lnum));
            excuteNextCode<UnsignedRightShift>(programCounter);
            break;
        }

        case LessThanOpcode:
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
            excuteNextCode<LessThan>(programCounter);
            break;
        }

        case LessThanOrEqualOpcode:
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
            excuteNextCode<LessThanOrEqual>(programCounter);
            break;
        }

        case GreaterThanOpcode:
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
            excuteNextCode<GreaterThan>(programCounter);
            break;
        }

        case GreaterThanOrEqualOpcode:
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
            excuteNextCode<GreaterThanOrEqual>(programCounter);
            break;
        }

        case PlusOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            ESValue lval = left->toPrimitive();
            ESValue rval = right->toPrimitive();
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
            push<ESValue>(stack, sp, ret);
            excuteNextCode<Plus>(programCounter);
            break;
        }

        case MinusOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            // http://www.ecma-international.org/ecma-262/5.1/#sec-11.6.2
            ESValue ret(ESValue::ESForceUninitialized);
            if (left->isInt32() && right->isInt32()) {
                int a = left->asInt32(), b = right->asInt32();
                if (UNLIKELY((a > 0 && b < 0 && b < a - std::numeric_limits<int32_t>::max()))) {
                    //overflow
                    ret = ESValue((double)left->asInt32() - (double)right->asInt32());
                } else if (UNLIKELY(a < 0 && b > 0 && b > a - std::numeric_limits<int32_t>::min())) {
                    //underflow
                    ret = ESValue((double)left->asInt32() - (double)right->asInt32());
                } else {
                    ret = ESValue(left->asInt32() - right->asInt32());
                }
            }
            else
                ret = ESValue(left->toNumber() - right->toNumber());
            push<ESValue>(stack, sp, ret);
            excuteNextCode<Minus>(programCounter);
            break;
        }

        case MultiplyOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            push<ESValue>(stack, sp, ESValue(left->toNumber() * right->toNumber()));
            excuteNextCode<Multiply>(programCounter);
            break;
        }

        case DivisionOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            push<ESValue>(stack, sp, ESValue(left->toNumber() / right->toNumber()));
            excuteNextCode<Division>(programCounter);
            break;
        }

        case ModOpcode:
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
            excuteNextCode<Mod>(programCounter);
            break;
        }

        case BitwiseNotOpcode:
        {
            push<ESValue>(stack, sp, ESValue(~pop<ESValue>(stack, sp)->toInt32()));
            excuteNextCode<BitwiseNot>(programCounter);
            break;
        }

        case LogicalNotOpcode:
        {
            push<ESValue>(stack, sp, ESValue(!pop<ESValue>(stack, sp)->toBoolean()));
            excuteNextCode<LogicalNot>(programCounter);
            break;
        }

        case UnaryMinusOpcode:
        {
            push<ESValue>(stack, sp, ESValue(-pop<ESValue>(stack, sp)->toNumber()));
            excuteNextCode<UnaryMinus>(programCounter);
            break;
        }

        case UnaryPlusOpcode:
        {
            push<ESValue>(stack, sp, ESValue(pop<ESValue>(stack, sp)->toNumber()));
            excuteNextCode<UnaryPlus>(programCounter);
            break;
        }

        case CreateObjectOpcode:
        {
            CreateObject* code = (CreateObject*)currentCode;
            ESObject* obj = ESObject::create(code->m_keyCount);
            obj->setConstructor(globalObject->object());
            obj->set__proto__(globalObject->objectPrototype());
            push<ESValue>(stack, sp, obj);
            excuteNextCode<CreateObject>(programCounter);
            break;
        }

        case CreateArrayOpcode:
        {
            CreateArray* code = (CreateArray*)currentCode;
            ESArrayObject* arr = ESArrayObject::create(code->m_keyCount, globalObject->arrayPrototype());
            push<ESValue>(stack, sp, arr);
            excuteNextCode<CreateArray>(programCounter);
            break;
        }

        case SetObjectOpcode:
        {
            SetObject* code = (SetObject*)currentCode;
            ESValue* value = pop<ESValue>(stack, sp);
            ESValue* key = pop<ESValue>(stack, sp);
            peek<ESValue>(stack, sp)->asESPointer()->asESObject()->set(*key, *value);
            excuteNextCode<SetObject>(programCounter);
            break;
        }

        case GetObjectOpcode:
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
                           excuteNextCode<GetObject>(programCounter);
                           break;
                       } else {
                           push<ESValue>(stack, sp, ESString::create(c));
                           excuteNextCode<GetObject>(programCounter);
                           break;
                       }
                   } else {
                       push<ESValue>(stack, sp, ESValue());
                       excuteNextCode<GetObject>(programCounter);
                       break;
                   }
                   push<ESValue>(stack, sp, willBeObject->asESString()->substring(prop_val, prop_val+1));
                   excuteNextCode<GetObject>(programCounter);
                   break;
                } else {
                    globalObject->stringObjectProxy()->setString(willBeObject->asESString());
                    ESValue ret = globalObject->stringObjectProxy()->find(*property, true);
                    if(!ret.isEmpty()) {
                        if(ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->codeBlock()->m_isBuiltInFunction) {
                            lastESObjectMetInMemberExpressionNode = (globalObject->stringObjectProxy());
                            push<ESValue>(stack, sp, ret);
                            excuteNextCode<GetObject>(programCounter);
                            break;
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
                        excuteNextCode<GetObject>(programCounter);
                        break;
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
                    excuteNextCode<GetObject>(programCounter);
                    break;
                } else {
                    size_t idx = obj->hiddenClass()->findProperty(val);
                    if(idx != SIZE_MAX) {
                        code->m_cachedHiddenClass = obj->hiddenClass();
                        code->m_cachedPropertyValue = val;
                        code->m_cachedIndex = idx;
                        push<ESValue>(stack, sp, obj->readHiddenClass(idx));
                        excuteNextCode<GetObject>(programCounter);
                        break;
                    } else {
                        code->m_cachedHiddenClass = nullptr;
                        ESValue v = obj->findOnlyPrototype(val);
                        if(v.isEmpty()) {
                            push<ESValue>(stack, sp, ESValue());
                            excuteNextCode<GetObject>(programCounter);
                            break;
                        }
                        push<ESValue>(stack, sp, v);
                        excuteNextCode<GetObject>(programCounter);
                        break;
                    }
                }
            } else {
                push<ESValue>(stack, sp, obj->get(*property, true));
                excuteNextCode<GetObject>(programCounter);
                break;
            }
            RELEASE_ASSERT_NOT_REACHED();
            break;
        }

        case CreateFunctionOpcode:
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
            excuteNextCode<CreateFunction>(programCounter);
            break;
        }

        case ExecuteNativeFunctionOpcode:
        {
            ExecuteNativeFunction* code = (ExecuteNativeFunction*)currentCode;
            ASSERT(bp == sp);
            return code->m_fn(instance);
        }
        case PrepareFunctionCallOpcode:
        {
            lastESObjectMetInMemberExpressionNode = globalObject;
            excuteNextCode<PrepareFunctionCall>(programCounter);
            break;
        }

        case CallFunctionOpcode:
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
            excuteNextCode<CallFunction>(programCounter);
            break;
        }

        case NewFunctionCallOpcode:
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
            excuteNextCode<NewFunctionCall>(programCounter);
            break;
        }

        case ReturnFunctionOpcode:
        {
            ASSERT(bp == sp);
            return ESValue();
        }

        case ReturnFunctionWithValueOpcode:
        {
            ESValue* ret = pop<ESValue>(stack, sp);
            ASSERT(bp == sp);
            return *ret;
        }

        case JumpOpcode:
        {
            Jump* code = (Jump *)currentCode;
            ASSERT(code->m_jumpPosition != SIZE_MAX);
            programCounter = code->m_jumpPosition;
            break;
        }

        case JumpIfTopOfStackValueIsFalseOpcode:
        {
            JumpIfTopOfStackValueIsFalse* code = (JumpIfTopOfStackValueIsFalse *)currentCode;
            ESValue* top = pop<ESValue>(stack, sp);
            ASSERT(code->m_jumpPosition != SIZE_MAX);
            if(!top->toBoolean())
                programCounter = code->m_jumpPosition;
            else
                excuteNextCode<JumpIfTopOfStackValueIsFalse>(programCounter);
            break;
        }

        case JumpIfTopOfStackValueIsTrueOpcode:
        {
            JumpIfTopOfStackValueIsTrue* code = (JumpIfTopOfStackValueIsTrue *)currentCode;
            ESValue* top = pop<ESValue>(stack, sp);
            ASSERT(code->m_jumpPosition != SIZE_MAX);
            if(top->toBoolean())
                programCounter = code->m_jumpPosition;
            else
                excuteNextCode<JumpIfTopOfStackValueIsTrue>(programCounter);
            break;
        }

        case JumpIfTopOfStackValueIsFalseWithPeekingOpcode:
        {
            JumpIfTopOfStackValueIsFalseWithPeeking* code = (JumpIfTopOfStackValueIsFalseWithPeeking *)currentCode;
            ESValue* top = peek<ESValue>(stack, sp);
            ASSERT(code->m_jumpPosition != SIZE_MAX);
            if(!top->toBoolean())
                programCounter = code->m_jumpPosition;
            else
                excuteNextCode<JumpIfTopOfStackValueIsFalseWithPeeking>(programCounter);
            break;
        }

        case JumpIfTopOfStackValueIsTrueWithPeekingOpcode:
        {
            JumpIfTopOfStackValueIsTrueWithPeeking* code = (JumpIfTopOfStackValueIsTrueWithPeeking *)currentCode;
            ESValue* top = peek<ESValue>(stack, sp);
            ASSERT(code->m_jumpPosition != SIZE_MAX);
            if(top->toBoolean())
                programCounter = code->m_jumpPosition;
            else
                excuteNextCode<JumpIfTopOfStackValueIsTrueWithPeeking>(programCounter);
            break;
        }

        case DuplicateTopOfStackValueOpcode:
        {
            push<ESValue>(stack, sp, *peek<ESValue>(stack, sp));
            excuteNextCode<DuplicateTopOfStackValue>(programCounter);
            break;
        }

        case TryOpcode:
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
            break;
        }

        case TryCatchBodyEndOpcode:
        {
            ASSERT(bp == sp);
            return ESValue();
        }

        case ThrowOpcode:
        {
            ESValue* v = pop<ESValue>(stack, sp);
            throw *v;
            break;
        }

        case ThisOpcode:
        {
            push<ESValue>(stack, sp, ESValue(ec->resolveThisBinding()));
            excuteNextCode<This>(programCounter);
            break;
        }

        case EndOpcode:
        {
            ASSERT(sp == 0);
            return ESValue();
        }
        default:
            printf("%d\n", currentCode->m_opcode);
            fflush(stdout);
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
}

}
