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
    GetByIdOpcode,
    GetByIndexOpcode,
    GetByIndexWithActivationOpcode,
    ResolveAddressByIdOpcode,
    ResolveAddressByIndexOpcode,
    ResolveAddressByIndexWithActivationOpcode,
    PutOpcode,
    AddOpcode,
    SubOpcode,
    MultiplyOpcode,
    DivideOpcode,
    ModOpcode,
    EqualOpcode,
    NotEqualOpcode,
    StrictEqualOpcode,
    NotStrictEqualOpcode,
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
        case EndOpcode:
        {
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
