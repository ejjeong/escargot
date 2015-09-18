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
    ReferenceTopValueWithPeekingOpcode,
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

    //object, array
    CreateObjectOpcode,
    CreateArrayOpcode,
    SetObjectOpcode,
    GetObjectOpcode,

    //function
    CreateFunctionOpcode,
    ExecuteNativeFunctionOpcode,
    PrepareFunctionCallOpcode,
    CallFunctionOpcode,
    NewFunctionCallOpcode,
    ReturnFunctionOpcode,
    ReturnFunctionWithValueOpcode,

    //control flow
    JumpOpcode,
    JumpIfTopOfStackValueIsFalseOpcode,
    JumpIfTopOfStackValueIsTrueOpcode,
    JumpIfTopOfStackValueIsFalseWithPeekingOpcode,
    JumpIfTopOfStackValueIsTrueWithPeekingOpcode,
    DuplicateTopOfStackValueOpcode,

    //try-catch
    TryOpcode,
    TryCatchBodyEndOpcode,
    ThrowOpcode,

    //etc
    ThisOpcode,

    EndOpcode,
};

class ByteCode;
class CodeBlock;

struct ByteCodeGenereateContext {
    ByteCodeGenereateContext()
    {
    }

    void pushBreakPositions(size_t pos)
    {
        m_breakStatementPositions.push_back(pos);
    }

    void pushContinuePositions(size_t pos)
    {
        m_continueStatementPositions.push_back(pos);
    }

    ALWAYS_INLINE void consumeBreakPositions(CodeBlock* cb, size_t position);
    ALWAYS_INLINE void consumeContinuePositions(CodeBlock* cb, size_t position);

    std::vector<size_t> m_breakStatementPositions;
    std::vector<size_t> m_continueStatementPositions;
};

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
        , m_value(value)
    {
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
    ESString* m_name;
    virtual void dump()
    {
        printf("GetByIndex <%s, %u>\n", m_name->utf8Data(),  (unsigned)m_index);
    }
#endif
};

class GetByIndexWithActivation : public ByteCode {
public:
    GetByIndexWithActivation(size_t fastAccessIndex, size_t fastAccessUpIndex)
        : ByteCode(GetByIndexWithActivationOpcode)
    {
        m_index = fastAccessIndex;
        m_upIndex = fastAccessUpIndex;
    }
    size_t m_index;
    size_t m_upIndex;

#ifndef NDEBUG
    ESString* m_name;
    virtual void dump()
    {
        printf("GetByIndexWithActivation <%s, %u, %u>\n", m_name->utf8Data(), (unsigned)m_index, (unsigned)m_upIndex);
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
        : ByteCode(ResolveAddressByIndexWithActivationOpcode)
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

class ReferenceTopValueWithPeeking : public ByteCode {
public:
    ReferenceTopValueWithPeeking()
        : ByteCode(ReferenceTopValueWithPeekingOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("ReferenceTopValueWithPeeking <>\n");
    }
#endif
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
        printf("JumpIfTopOfStackValueIsFalse <%u>\n",(unsigned)m_jumpPosition);
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
        printf("JumpIfTopOfStackValueIsTrue <%u>\n",(unsigned)m_jumpPosition);
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
        printf("JumpIfTopOfStackValueIsFalseWithPeeking <%u>\n",(unsigned)m_jumpPosition);
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
        printf("JumpIfTopOfStackValueIsTrueWithPeeking <%u>\n",(unsigned)m_jumpPosition);
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

class CreateFunction : public ByteCode {
public:
    CreateFunction(InternalAtomicString name, ESString* nonAtomicName, CodeBlock* codeBlock)
        : ByteCode(CreateFunctionOpcode)
    {
        m_name = name;
        m_nonAtomicName = nonAtomicName;
        m_codeBlock = codeBlock;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("CreateFunction <>\n");
    }
#endif
    InternalAtomicString m_name;
    ESString* m_nonAtomicName;
    CodeBlock* m_codeBlock;
};

class ExecuteNativeFunction : public ByteCode {
public:
    ExecuteNativeFunction(const NativeFunctionType& fn)
        : ByteCode(ExecuteNativeFunctionOpcode)
    {
        m_fn = fn;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("ExecuteNativeFunction <>\n");
    }
#endif

    NativeFunctionType m_fn;
};

class PrepareFunctionCall : public ByteCode {
public:
    PrepareFunctionCall()
        : ByteCode(PrepareFunctionCallOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("PrepareFunctionCall <>\n");
    }
#endif

};

class CallFunction : public ByteCode {
public:
    CallFunction()
        : ByteCode(CallFunctionOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("CallFunction <>\n");
    }
#endif

};

class NewFunctionCall : public ByteCode {
public:
    NewFunctionCall()
        : ByteCode(NewFunctionCallOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("NewFunctionCall <>\n");
    }
#endif

};

class ReturnFunction : public ByteCode {
public:
    ReturnFunction()
        : ByteCode(ReturnFunctionOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("ReturnFunction <>\n");
    }
#endif

};

class ReturnFunctionWithValue : public ByteCode {
public:
    ReturnFunctionWithValue()
        : ByteCode(ReturnFunctionWithValueOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("ReturnFunctionWithValue <>\n");
    }
#endif

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
        printf("Jump <%u>\n", (unsigned)m_jumpPosition);
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

class Try : public ByteCode {
public:
    Try()
        : ByteCode(TryOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("Try <>\n");
    }
#endif

    size_t m_catchPosition;
    size_t m_statementEndPosition;
    InternalAtomicString m_name;
    ESString* m_nonAtomicName;
};

class TryCatchBodyEnd : public ByteCode {
public:
    TryCatchBodyEnd()
        : ByteCode(TryCatchBodyEndOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("TryBodyEnd <>\n");
    }
#endif
};

class This : public ByteCode {
public:
    This()
        : ByteCode(ThisOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("This <>\n");
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

class CodeBlock : public gc {
public:
    CodeBlock()
    {
        m_needsActivation = false;
        m_needsArgumentsObject = false;
        m_isBuiltInFunction = false;
    }
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

    InternalAtomicStringVector m_params; //params: [ Pattern ];
    ESStringVector m_nonAtomicParams;
    InternalAtomicStringVector m_innerIdentifiers;
    bool m_needsActivation;
    bool m_needsArgumentsObject;
    bool m_isBuiltInFunction;
};

template <typename Type>
ALWAYS_INLINE void push(void* stk, unsigned& sp, const Type& ptr)
{
    memcpy(((char *)stk) + sp, &ptr, sizeof (Type));
    sp += sizeof (Type);

#ifndef NDEBUG
    size_t siz = sizeof (Type);
    memcpy(((char *)stk) + sp, &siz, sizeof (size_t));
    sp += sizeof (size_t);

    if(sp > 1024*4) {
        puts("stackoverflow!!!");
        ASSERT_NOT_REACHED();
    }
    ASSERT(sp % sizeof(size_t) == 0);
#endif
}

template <typename Type>
ALWAYS_INLINE Type* pop(void* stk, unsigned& sp)
{
#ifndef NDEBUG
    if(sp < sizeof (Type) + sizeof (size_t)) {
        ASSERT_NOT_REACHED();
    }
    sp -= sizeof (size_t);
    size_t* siz = (size_t *)(&(((char *)stk)[sp]));
    ASSERT(*siz == sizeof (Type));
#endif
    sp -= sizeof (Type);
    ASSERT(sp % sizeof(size_t) == 0);
    return (Type *)(&(((char *)stk)[sp]));
}

template <typename Type>
ALWAYS_INLINE Type* peek(void* stk, size_t sp)
{
#ifndef NDEBUG
    if(sp < sizeof (Type) + sizeof (size_t)) {
        ASSERT_NOT_REACHED();
    }
    sp -= sizeof (size_t);
    size_t* siz = (size_t *)(&(((char *)stk)[sp]));
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

#ifndef NDEBUG

ALWAYS_INLINE void dumpBytecode(CodeBlock* codeBlock)
{
    printf("dumpBytecode...>>>>>>>>>>>>>>>>>>>>>>\n");
    size_t idx = 0;
    char* code = codeBlock->m_code.data();
    while(idx < codeBlock->m_code.size()) {
        ByteCode* currentCode = (ByteCode *)(&code[idx]);
        printf("%u\t\t%p\t",(unsigned)idx, currentCode);
        switch(currentCode->m_opcode) {
#define DUMP_BYTE_CODE(code) \
        case code##Opcode:\
        currentCode->dump(); \
        idx += sizeof (code); \
        continue;
        DUMP_BYTE_CODE(Push);
        DUMP_BYTE_CODE(PopExpressionStatement);
        DUMP_BYTE_CODE(Pop);
        DUMP_BYTE_CODE(GetById);
        DUMP_BYTE_CODE(GetByIndex);
        DUMP_BYTE_CODE(GetByIndexWithActivation);
        DUMP_BYTE_CODE(ResolveAddressById);
        DUMP_BYTE_CODE(ResolveAddressByIndex);
        DUMP_BYTE_CODE(ResolveAddressByIndexWithActivation);
        DUMP_BYTE_CODE(ResolveAddressInObject);
        DUMP_BYTE_CODE(ReferenceTopValueWithPeeking);
        DUMP_BYTE_CODE(Put);
        DUMP_BYTE_CODE(PutReverseStack);
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
        DUMP_BYTE_CODE(BitwiseNot);
        DUMP_BYTE_CODE(LogicalNot);
        DUMP_BYTE_CODE(UnaryMinus);
        DUMP_BYTE_CODE(UnaryPlus);
        DUMP_BYTE_CODE(CreateObject);
        DUMP_BYTE_CODE(CreateArray);
        DUMP_BYTE_CODE(SetObject);
        DUMP_BYTE_CODE(GetObject);
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
            printf("please add %d\n",(int)currentCode->m_opcode);
            RELEASE_ASSERT_NOT_REACHED();
            break;
        };
    }
    printf("dumpBytecode...<<<<<<<<<<<<<<<<<<<<<<\n");
}

#endif

ESValue interpret(ESVMInstance* instance, CodeBlock* codeBlock, size_t programCounter = 0);

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

ALWAYS_INLINE void ByteCodeGenereateContext::consumeBreakPositions(CodeBlock* cb, size_t position)
{
    for(size_t i = 0; i < m_breakStatementPositions.size(); i ++) {
        Jump* shouldBeJump = cb->peekCode<Jump>(m_breakStatementPositions[i]);
        ASSERT(shouldBeJump->m_opcode == JumpOpcode);
        shouldBeJump->m_jumpPosition = position;
    }
    m_breakStatementPositions.clear();
}

ALWAYS_INLINE void ByteCodeGenereateContext::consumeContinuePositions(CodeBlock* cb, size_t position)
{
    for(size_t i = 0; i < m_continueStatementPositions.size(); i ++) {
        Jump* shouldBeJump = cb->peekCode<Jump>(m_continueStatementPositions[i]);
        ASSERT(shouldBeJump->m_opcode == JumpOpcode);
        shouldBeJump->m_jumpPosition = position;
    }
    m_continueStatementPositions.clear();
}

}

#endif
