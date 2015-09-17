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
    PopExpressionStatementOpcode, //ExpressionStatement
    PopOpcode,

    GetByIdOpcode,
    GetByIndexOpcode,
    GetByIndexWithActivationOpcode,
    ResolveAddressByIdOpcode,
    ResolveAddressByIndexOpcode,
    ResolveAddressByIndexWithActivationOpcode,
    ResolveAddressInObjectOpcode,
    PutOpcode,
    PutReverseStackOpcode,
    CreateBindingOpcode,

    //binary expressions
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
    LessThanOpcode,
    LessThanOrEqualOpcode,
    GreaterThanOpcode,
    GreaterThanOrEqualOpcode,
    PlusOpcode,
    MinusOpcode,
    MultiplyOpcode,
    DivisionOpcode,
    ModOpcode,

    //unary expressions
    BitwiseNotOpcode,
    LogicalNotOpcode,
    UnaryMinusOpcode,
    UnaryPlusOpcode,

    //object, array expressions
    CreateObjectOpcode,
    CreateArrayOpcode,
    SetObjectOpcode,
    GetObjectOpcode,

    //control flow
    JumpOpcode,
    JumpIfTopOfStackValueIsFalseOpcode,
    JumpIfTopOfStackValueIsTrueOpcode,
    JumpIfTopOfStackValueIsFalseWithPeekingOpcode,
    JumpIfTopOfStackValueIsTrueWithPeekingOpcode,
    CallOpcode,
    DuplicateTopOfStackValueOpcode,
    ThrowOpcode,

    EndOpcode,
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
    virtual void dump() = 0;
    virtual ~ByteCode() {

    }
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
#ifndef NDEBUG
    virtual void dump()
    {
        printf("Push <%s>\n", m_value.toString()->utf8Data());
    }
#endif
};

class Pop : public ByteCode {
public:
    Pop()
        : ByteCode(PopOpcode)
    {

    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("Pop <>\n");
    }
#endif
};

class PopExpressionStatement : public ByteCode {
public:
    PopExpressionStatement()
        : ByteCode(PopExpressionStatementOpcode)
    {

    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("PopExpressionStatement <>\n");
    }
#endif
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

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetById <%s>\n",m_nonAtomicName->utf8Data());
    }
#endif
};

class GetByIndex : public ByteCode {
public:
    GetByIndex(size_t index)
        : ByteCode(GetByIndexOpcode)
    {
        m_index = index;
    }
    size_t m_index;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetByIndex <%u>\n", (unsigned)m_index);
    }
#endif
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

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetByIndex <%u, %u>\n", (unsigned)m_index, (unsigned)m_upIndex);
    }
#endif
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

#ifndef NDEBUG
    virtual void dump()
    {
        printf("ResolveAddressById <%s>\n", m_nonAtomicName->utf8Data());
    }
#endif
};

class ResolveAddressByIndex : public ByteCode {
public:
    ResolveAddressByIndex(size_t index)
        : ByteCode(ResolveAddressByIndexOpcode)
    {
        m_index = index;
    }
    size_t m_index;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("ResolveAddressByIndex <%u>\n", (unsigned)m_index);
    }
#endif
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

#ifndef NDEBUG
    virtual void dump()
    {
        printf("ResolveAddressByIndex <%u, %u>\n", (unsigned)m_index, (unsigned)m_upIndex);
    }
#endif
};

class ResolveAddressInObject : public ByteCode {
public:
    ResolveAddressInObject()
        : ByteCode(ResolveAddressInObjectOpcode)
    {
        m_cachedHiddenClass = nullptr;
        m_cachedPropertyValue = nullptr;
        m_cachedIndex = SIZE_MAX;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("ResolveAddressInObject <>\n");
    }
#endif

    ESHiddenClass* m_cachedHiddenClass;
    ESString* m_cachedPropertyValue;
    size_t m_cachedIndex;
};

class Put : public ByteCode {
public:
    Put()
        : ByteCode(PutOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("Put <>\n");
    }
#endif
};

class PutReverseStack : public ByteCode {
public:
    PutReverseStack()
        : ByteCode(PutReverseStackOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("PutReverseStack <>\n");
    }
#endif
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

#ifndef NDEBUG
    virtual void dump()
    {
        printf("CreateBinding <%s>\n",m_nonAtomicName->utf8Data());
    }
#endif
};

class Equal : public ByteCode {
public:
    Equal()
        : ByteCode(EqualOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("Equal <>\n");
    }
#endif
};

class NotEqual : public ByteCode {
public:
    NotEqual()
        : ByteCode(NotEqualOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("NotEqual <>\n");
    }
#endif
};

class StrictEqual : public ByteCode {
public:
    StrictEqual()
        : ByteCode(StrictEqualOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("StrictEqual <>\n");
    }
#endif
};

class NotStrictEqual : public ByteCode {
public:
    NotStrictEqual()
        : ByteCode(NotStrictEqualOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("NotStrictEqual <>\n");
    }
#endif
};

class BitwiseAnd : public ByteCode {
public:
    BitwiseAnd()
        : ByteCode(BitwiseAndOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("BitwiseAnd <>\n");
    }
#endif
};

class BitwiseOr : public ByteCode {
public:
    BitwiseOr()
        : ByteCode(BitwiseOrOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("BitwiseOr <>\n");
    }
#endif
};

class BitwiseXor : public ByteCode {
public:
    BitwiseXor()
        : ByteCode(BitwiseXorOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("BitwiseXor <>\n");
    }
#endif
};

class LeftShift : public ByteCode {
public:
    LeftShift()
        : ByteCode(LeftShiftOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("LeftShift <>\n");
    }
#endif
};

class SignedRightShift : public ByteCode {
public:
    SignedRightShift()
        : ByteCode(SignedRightShiftOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("SignedRightShift <>\n");
    }
#endif
};

class UnsignedRightShift : public ByteCode {
public:
    UnsignedRightShift()
        : ByteCode(UnsignedRightShiftOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("UnsignedRightShift <>\n");
    }
#endif
};

class LessThan : public ByteCode {
public:
    LessThan()
        : ByteCode(LessThanOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("LessThan <>\n");
    }
#endif
};

class LessThanOrEqual : public ByteCode {
public:
    LessThanOrEqual()
        : ByteCode(LessThanOrEqualOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("LessThanOrEqual <>\n");
    }
#endif
};

class GreaterThan : public ByteCode {
public:
    GreaterThan()
        : ByteCode(GreaterThanOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GreaterThan <>\n");
    }
#endif
};

class GreaterThanOrEqual : public ByteCode {
public:
    GreaterThanOrEqual()
        : ByteCode(GreaterThanOrEqualOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GreaterThanOrEqual <>\n");
    }
#endif
};

class Plus : public ByteCode {
public:
    Plus()
        : ByteCode(PlusOpcode)
    {

    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("Plus <>\n");
    }
#endif
};

class Minus : public ByteCode {
public:
    Minus()
        : ByteCode(MinusOpcode)
    {

    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("Minus <>\n");
    }
#endif
};

class Multiply : public ByteCode {
public:
    Multiply ()
        : ByteCode(MultiplyOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("Multiply <>\n");
    }
#endif
};

class Division : public ByteCode {
public:
    Division()
        : ByteCode(DivisionOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("Division <>\n");
    }
#endif
};

class Mod : public ByteCode {
public:
    Mod()
        : ByteCode(ModOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("Mod <>\n");
    }
#endif
};

class BitwiseNot : public ByteCode {
public:
    BitwiseNot()
        : ByteCode(BitwiseNotOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("BitwiseNot <>\n");
    }
#endif
};

class LogicalNot : public ByteCode {
public:
    LogicalNot()
        : ByteCode(LogicalNotOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("LogicalNot <>\n");
    }
#endif
};

class UnaryMinus : public ByteCode {
public:
    UnaryMinus()
        : ByteCode(UnaryMinusOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("UnaryMinus <>\n");
    }
#endif
};

class UnaryPlus : public ByteCode {
public:
    UnaryPlus()
        : ByteCode(UnaryPlusOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("UnaryPlus <>\n");
    }
#endif
};

class JumpIfTopOfStackValueIsFalse : public ByteCode {
public:
    JumpIfTopOfStackValueIsFalse(size_t jumpPosition)
        : ByteCode(JumpIfTopOfStackValueIsFalseOpcode)
    {
        m_jumpPosition = jumpPosition;
    }

    size_t m_jumpPosition;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("JumpIfTopOfStackValueIsFalse <>\n");
    }
#endif
};

class JumpIfTopOfStackValueIsTrue : public ByteCode {
public:
    JumpIfTopOfStackValueIsTrue(size_t jumpPosition)
        : ByteCode(JumpIfTopOfStackValueIsTrueOpcode)
    {
        m_jumpPosition = jumpPosition;
    }

    size_t m_jumpPosition;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("JumpIfTopOfStackValueIsTrue <>\n");
    }
#endif
};

class JumpIfTopOfStackValueIsFalseWithPeeking : public ByteCode {
public:
    JumpIfTopOfStackValueIsFalseWithPeeking(size_t jumpPosition)
        : ByteCode(JumpIfTopOfStackValueIsFalseWithPeekingOpcode)
    {
        m_jumpPosition = jumpPosition;
    }

    size_t m_jumpPosition;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("JumpIfTopOfStackValueIsFalseWithPeeking <>\n");
    }
#endif
};

class JumpIfTopOfStackValueIsTrueWithPeeking : public ByteCode {
public:
    JumpIfTopOfStackValueIsTrueWithPeeking(size_t jumpPosition)
        : ByteCode(JumpIfTopOfStackValueIsTrueWithPeekingOpcode)
    {
        m_jumpPosition = jumpPosition;
    }

    size_t m_jumpPosition;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("JumpIfTopOfStackValueIsTrueWithPeeking <>\n");
    }
#endif
};

class CreateObject : public ByteCode {
public:
    CreateObject(size_t keyCount)
        : ByteCode(CreateObjectOpcode)
    {
        m_keyCount = keyCount;
    }

    size_t m_keyCount;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("CreateObject <%u>\n",(unsigned)m_keyCount);
    }
#endif
};

class CreateArray : public ByteCode {
public:
    CreateArray(size_t keyCount)
        : ByteCode(CreateArrayOpcode)
    {
        m_keyCount = keyCount;
    }

    size_t m_keyCount;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("CreateArray <%u>\n",(unsigned)m_keyCount);
    }
#endif
};

class SetObject : public ByteCode {
public:
    SetObject()
        : ByteCode(SetObjectOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("SetObject <>\n");
    }
#endif
};

class GetObject : public ByteCode {
public:
    GetObject()
        : ByteCode(GetObjectOpcode)
    {
        m_cachedHiddenClass = nullptr;
        m_cachedPropertyValue = nullptr;
        m_cachedIndex = SIZE_MAX;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetObject <>\n");
    }
#endif

    ESHiddenClass* m_cachedHiddenClass;
    ESString* m_cachedPropertyValue;
    size_t m_cachedIndex;
};

class Jump : public ByteCode {
public:
    Jump(size_t jumpPosition)
        : ByteCode(JumpOpcode)
    {
        m_jumpPosition = jumpPosition;
    }

    size_t m_jumpPosition;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("Jump <>\n");
    }
#endif
};

class DuplicateTopOfStackValue : public ByteCode {
public:
    DuplicateTopOfStackValue()
        : ByteCode(DuplicateTopOfStackValueOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("DuplicateTopOfStackValue <>\n");
    }
#endif
};

class Throw : public ByteCode {
public:
    Throw()
        : ByteCode(ThrowOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("Throw <>\n");
    }
#endif
};

class End : public ByteCode {
public:
    End()
        : ByteCode(EndOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("End <>\n");
    }
#endif
};

class CodeBlock : public gc_cleanup {
public:
    template <typename CodeType>
    void pushCode(const CodeType& type, Node* node);
    template <typename CodeType>
    CodeType* peekCode(size_t position)
    {
        char* pos = m_code.data();
        pos = &pos[position];
        return (CodeType *)pos;
    }

    template <typename CodeType>
    size_t lastCodePosition()
    {
        return m_code.size() - sizeof(CodeType);
    }

    size_t currentCodeSize()
    {
        return m_code.size();
    }
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

template <typename Type>
ALWAYS_INLINE Type* peek(void* stk, size_t sp)
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
    GlobalObject* globalObject = instance->globalObject();
    ESObject* lastESObjectMetInMemberExpressionNode = globalObject;
    while(1) {
        ByteCode* currentCode = (ByteCode *)(&code[programCounter]);
        currentCode->dump();
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
            instance->m_lastExpressionStatementValue = *t;
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
                    push<ESSlotAccessor>(stack, sp, globalObject->definePropertyOrThrow(code->m_nonAtomicName, true, true, true));
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

        case ResolveAddressInObjectOpcode:
        {
            ResolveAddressInObject* code = (ResolveAddressInObject*)currentCode;
            ESValue* property = pop<ESValue>(stack, sp);
            ESValue* willBeObject = pop<ESValue>(stack, sp);

            ESObject* obj = willBeObject->toObject();

            ExecutionContext* ec = instance->currentExecutionContext();
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
                        if(ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->isBuiltInFunction()) {
                            lastESObjectMetInMemberExpressionNode = (instance->globalObject()->stringObjectProxy());
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
                    if(ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->isBuiltInFunction()) {
                        lastESObjectMetInMemberExpressionNode = (instance->globalObject()->numberObjectProxy());
                        push<ESValue>(stack, sp, ret);
                        excuteNextCode<GetObject>(programCounter);
                        break;
                    }
                }
            }

            ESObject* obj = willBeObject->toObject();

            ExecutionContext* ec = instance->currentExecutionContext();
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
        }

        case JumpOpcode:
        {
            Jump* code = (Jump *)currentCode;
            programCounter = code->m_jumpPosition;
            break;
        }

        case JumpIfTopOfStackValueIsFalseOpcode:
        {
            JumpIfTopOfStackValueIsFalse* code = (JumpIfTopOfStackValueIsFalse *)currentCode;
            ESValue* top = pop<ESValue>(stack, sp);
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

        case ThrowOpcode:
        {
            ESValue* v = pop<ESValue>(stack, sp);
            throw *v;
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
