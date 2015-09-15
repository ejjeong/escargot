#ifndef __ByteCode__
#define __ByteCode__

#include "runtime/ESValue.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

class Node;

enum Opcode {
    PushOpcode, //Literal
    PopOpcode, //ExpressionStatement
    JustPopOpcode,
    GetByIdOpcode,
    GetByIndexOpcode,
    GetByIndexWithActivationOpcode,
    ResolveAddressByIdOpcode,
    ResolveAddressByIndexOpcode,
    ResolveAddressByIndexWithActivationOpcode,
    PutOpcode,
    CreateBindingOpcode,
    EqualOpcode,
    NotEqualOpcode,
    StrictEqualOpcode,
    NotStrictEqualOpcode,
    BitwiseAndOpcode,
    BitwiseOrOpcode,
    BitwiseXorOpcode,
    LeftShiftOpcode,
    SignedRightShiftOpcode,
    UnsignedRightShiftOpcode,
    LogicalAndOpcode,
    LogicalOrOpcode,
    LessThanOpcode,
    LessThanOrEqualOpcode,
    GreaterThanOpcode,
    GreaterThanOrEqualOpcode,
    PlusOpcode,
    MinusOpcode,
    MultiplyOpcode,
    DivisionOpcode,
    ModOpcode,
    JumpOpcode,
    CallOpcode,
    EndOpcode,
    EndOfKind
};

class ByteCode;

class ByteCode {
public:
    ByteCode(Opcode code)
    {
        m_opcode = code;
#ifndef NDEBUG
        m_node = nullptr;
#endif
    }
    Opcode m_opcode;
#ifndef NDEBUG
    Node* m_node;
#endif
};

class Push : public ByteCode {
public:
    Push(const ESValue& value)
        : ByteCode(PushOpcode)
    {
        m_value = value;
    }
    ESValue m_value;
};

class Pop : public ByteCode {
public:
    Pop()
        : ByteCode(PopOpcode)
    {

    }
};

class JustPop : public ByteCode {
public:
    JustPop()
        : ByteCode(JustPopOpcode)
    {

    }
};

class GetById : public ByteCode {
public:
    GetById(const InternalAtomicString& name, ESString* esName)
        : ByteCode(GetByIdOpcode)
    {
        m_name = name;
        m_nonAtomicName = esName;
        m_identifierCacheInvalidationCheckCount = std::numeric_limits<unsigned>::max();
    }

    InternalAtomicString m_name;
    ESString* m_nonAtomicName;

    unsigned m_identifierCacheInvalidationCheckCount;
    ESSlotAccessor m_cachedSlot;
};

class GetByIndex : public ByteCode {
public:
    GetByIndex(size_t index)
        : ByteCode(GetByIndexOpcode)
    {
        m_index = index;
    }
    size_t m_index;
};

class GetByIndexWithActivation : public ByteCode {
public:
    GetByIndexWithActivation(size_t fastAccessIndex, size_t fastAccessUpIndex)
        : ByteCode(GetByIndexOpcode)
    {
        m_index = fastAccessIndex;
        m_upIndex = fastAccessUpIndex;
    }
    size_t m_index;
    size_t m_upIndex;
};

class ResolveAddressById : public ByteCode {
public:
    ResolveAddressById(const InternalAtomicString& name, ESString* esName)
        : ByteCode(ResolveAddressByIdOpcode)
    {
        m_name = name;
        m_nonAtomicName = esName;
        m_identifierCacheInvalidationCheckCount = std::numeric_limits<unsigned>::max();
    }

    InternalAtomicString m_name;
    ESString* m_nonAtomicName;

    unsigned m_identifierCacheInvalidationCheckCount;
    ESSlotAccessor m_cachedSlot;
};

class ResolveAddressByIndex : public ByteCode {
public:
    ResolveAddressByIndex(size_t index)
        : ByteCode(ResolveAddressByIndexOpcode)
    {
        m_index = index;
    }
    size_t m_index;
};

class ResolveAddressByIndexWithActivation : public ByteCode {
public:
    ResolveAddressByIndexWithActivation(size_t fastAccessIndex, size_t fastAccessUpIndex)
        : ByteCode(ResolveAddressByIndexOpcode)
    {
        m_index = fastAccessIndex;
        m_upIndex = fastAccessUpIndex;
    }
    size_t m_index;
    size_t m_upIndex;
};

class Put : public ByteCode {
public:
    Put()
        : ByteCode(PutOpcode)
    {

    }
};

class CreateBinding : public ByteCode {
public:
    CreateBinding(InternalAtomicString name, ESString* nonAtomicName)
        : ByteCode(CreateBindingOpcode)
    {
        m_name = name;
        m_nonAtomicName = nonAtomicName;
    }
    InternalAtomicString m_name;
    ESString* m_nonAtomicName;
};

class Equal : public ByteCode {
public:
    Equal()
        : ByteCode(EqualOpcode)
    {

    }
};

class NotEqual : public ByteCode {
public:
    NotEqual()
        : ByteCode(NotEqualOpcode)
    {

    }
};

class StrictEqual : public ByteCode {
public:
    StrictEqual()
        : ByteCode(StrictEqualOpcode)
    {

    }
};

class NotStrictEqual : public ByteCode {
public:
    NotStrictEqual()
        : ByteCode(NotStrictEqualOpcode)
    {

    }
};

class BitwiseAnd : public ByteCode {
public:
    BitwiseAnd()
        : ByteCode(BitwiseAndOpcode)
    {

    }
};

class BitwiseOr : public ByteCode {
public:
    BitwiseOr()
        : ByteCode(BitwiseOrOpcode)
    {

    }
};

class BitwiseXor : public ByteCode {
public:
    BitwiseXor()
        : ByteCode(BitwiseXorOpcode)
    {

    }
};

class LeftShift : public ByteCode {
public:
    LeftShift()
        : ByteCode(LeftShiftOpcode)
    {

    }
};

class SignedRightShift : public ByteCode {
public:
    SignedRightShift()
        : ByteCode(SignedRightShiftOpcode)
    {

    }
};

class UnsignedRightShift : public ByteCode {
public:
    UnsignedRightShift()
        : ByteCode(UnsignedRightShiftOpcode)
    {

    }
};

class LogicalAnd : public ByteCode {
public:
    LogicalAnd()
        : ByteCode(LogicalAndOpcode)
    {

    }
};

class LogicalOr : public ByteCode {
public:
    LogicalOr()
        : ByteCode(LogicalOrOpcode)
    {

    }
};

class LessThan : public ByteCode {
public:
    LessThan()
        : ByteCode(LessThanOpcode)
    {

    }
};

class LessThanOrEqual : public ByteCode {
public:
    LessThanOrEqual()
        : ByteCode(LessThanOrEqualOpcode)
    {

    }
};

class GreaterThan : public ByteCode {
public:
    GreaterThan()
        : ByteCode(GreaterThanOpcode)
    {

    }
};

class GreaterThanOrEqual : public ByteCode {
public:
    GreaterThanOrEqual()
        : ByteCode(GreaterThanOrEqualOpcode)
    {

    }
};

class Plus : public ByteCode {
public:
    Plus()
        : ByteCode(PlusOpcode)
    {

    }
};

class Minus : public ByteCode {
public:
    Minus()
        : ByteCode(MinusOpcode)
    {

    }
};

class Multiply : public ByteCode {
public:
    Multiply ()
        : ByteCode(MultiplyOpcode)
    {

    }
};

class Division : public ByteCode {
public:
    Division()
        : ByteCode(DivisionOpcode)
    {

    }
};

class Mod : public ByteCode {
public:
    Mod()
        : ByteCode(ModOpcode)
    {

    }
};


class End : public ByteCode {
public:
    End()
        : ByteCode(EndOpcode)
    {

    }
};

class CodeBlock : public gc_cleanup {
public:
    template <typename CodeType>
    void pushCode(const CodeType& type, Node* node);
    std::vector<char, gc_allocator<char> > m_code;
};

template <typename Type>
ALWAYS_INLINE void push(void* stk, size_t& sp, const Type& ptr)
{
    memcpy(((char *)stk) + sp, &ptr, sizeof (Type));
    sp += sizeof (Type);

#ifndef NDEBUG
    int siz = sizeof (Type);
    memcpy(((char *)stk) + sp, &siz, sizeof (int));
    sp += sizeof (int);

    if(sp > 1024*1024*4) {
        ASSERT_NOT_REACHED();
    }
#endif
}

template <typename Type>
ALWAYS_INLINE Type* pop(void* stk, size_t& sp)
{
#ifndef NDEBUG
    if(sp < sizeof (Type) + sizeof (int)) {
        ASSERT_NOT_REACHED();
    }
    sp -= sizeof (int);
    int* siz = (int *)(&(((char *)stk)[sp]));
    ASSERT(*siz == sizeof (Type));
#endif
    sp -= sizeof (Type);
    return (Type *)(&(((char *)stk)[sp]));
}

template <typename CodeType>
ALWAYS_INLINE void excuteNextCode(size_t& programCounter)
{
    programCounter += sizeof (CodeType);
}

ALWAYS_INLINE void interpret(ESVMInstance* instance, CodeBlock* codeBlock)
{
    size_t programCounter = 0;
    void* stack = instance->m_stack;
    size_t& sp = instance->m_sp;
    char* code = codeBlock->m_code.data();
    ExecutionContext* ec = instance->currentExecutionContext();
    while(1) {
        ByteCode* currentCode = (ByteCode *)(&code[programCounter]);
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
            ESValue* t = pop<ESValue>(stack, sp);
            instance->m_lastExpressionStatementValue = *t;
            excuteNextCode<Pop>(programCounter);
            break;
        }

        case JustPopOpcode:
        {
            pop<ESValue>(stack, sp);
            excuteNextCode<JustPop>(programCounter);
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
            push<ESValue>(stack, sp, ec->cachedDeclarativeEnvironmentRecordESValue()[code->m_index]);
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
                    push<ESSlotAccessor>(stack, sp, instance->globalObject()->definePropertyOrThrow(code->m_nonAtomicName, true, true, true));
                }
            }

            excuteNextCode<ResolveAddressById>(programCounter);
            break;
        }

        case ResolveAddressByIndexOpcode:
        {
            ResolveAddressByIndex* code = (ResolveAddressByIndex*)currentCode;
            push<ESSlotAccessor>(stack, sp, ESSlotAccessor(&instance->currentExecutionContext()->cachedDeclarativeEnvironmentRecordESValue()[code->m_index]));
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

        case PutOpcode:
        {
            Put* code = (Put*)currentCode;
            ESValue* value = pop<ESValue>(stack, sp);
            ESSlotAccessor* slot = pop<ESSlotAccessor>(stack, sp);
            slot->setValue(*value);
            push<ESValue>(stack, sp, *value);
            excuteNextCode<Put>(programCounter);
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

        case LogicalAndOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            //TODO
            excuteNextCode<LogicalAnd>(programCounter);
            break;
        }

        case LogicalOrOpcode:
        {
            ESValue* right = pop<ESValue>(stack, sp);
            ESValue* left = pop<ESValue>(stack, sp);
            //TODO
            excuteNextCode<LogicalOr>(programCounter);
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

        case EndOpcode:
        {
            ASSERT(sp == 0);
            return ;
        }
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
}

}

#include "ast/Node.h"
namespace escargot {

template <typename CodeType>
void CodeBlock::pushCode(const CodeType& type, Node* node)
{
#ifndef NDEBUG
    {
        CodeType& t = const_cast<CodeType &>(type);
        t.m_node = node;
    }
#endif
    char* first = (char *)&type;
    m_code.insert(m_code.end(), first, first + sizeof(CodeType));
}
}

#endif
