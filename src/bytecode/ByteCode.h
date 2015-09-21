#ifndef __ByteCode__
#define __ByteCode__

#include "runtime/ESValue.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

class Node;

#define FOR_EACH_BYTECODE_OP(F) \
    F(Push) \
    F(PopExpressionStatement) \
    F(Pop) \
    F(PushIntoTempStack) \
    F(PopFromTempStack) \
\
    F(GetById) \
    F(GetByIdWithoutException) \
    F(GetByIndex) \
    F(GetByIndexWithActivation) \
    F(PutById) \
    F(PutByIndex) \
    F(PutByIndexWithActivation) \
    F(PutInObject) \
    F(CreateBinding) \
\
    /*binary expressions*/ \
    F(Equal) \
    F(NotEqual) \
    F(StrictEqual) \
    F(NotStrictEqual) \
    F(BitwiseAnd) \
    F(BitwiseOr) \
    F(BitwiseXor) \
    F(LeftShift) \
    F(SignedRightShift) \
    F(UnsignedRightShift) \
    F(LessThan) \
    F(LessThanOrEqual) \
    F(GreaterThan) \
    F(GreaterThanOrEqual) \
    F(Plus) \
    F(Minus) \
    F(Multiply) \
    F(Division) \
    F(Mod) \
    F(Increment) \
    F(Decrement) \
\
    /*unary expressions*/ \
    F(BitwiseNot) \
    F(LogicalNot) \
    F(UnaryMinus) \
    F(UnaryPlus) \
    F(UnaryTypeOf) \
\
    /*//object, array*/ \
    F(CreateObject) \
    F(CreateArray) \
    F(SetObject) \
    F(GetObject) \
    F(GetObjectWithPeeking) \
    F(EnumerateObject) \
    F(EnumerateObjectKey) \
    F(EnumerateObjectEnd) \
\
    /*function*/\
    F(CreateFunction) \
    F(ExecuteNativeFunction) \
    F(PrepareFunctionCall) \
    F(PushFunctionCallReceiver) \
    F(CallFunction) \
    F(CallEvalFunction) \
    F(NewFunctionCall) \
    F(ReturnFunction) \
    F(ReturnFunctionWithValue) \
\
    /* control flow */ \
    F(Jump) \
    F(JumpIfTopOfStackValueIsFalse) \
    F(JumpIfTopOfStackValueIsTrue) \
    F(JumpAndPopIfTopOfStackValueIsTrue) \
    F(JumpIfTopOfStackValueIsFalseWithPeeking) \
    F(JumpIfTopOfStackValueIsTrueWithPeeking) \
    F(DuplicateTopOfStackValue) \
\
    /*try-catch*/ \
    F(Try) \
    F(TryCatchBodyEnd) \
    F(Throw) \
\
    /*etc*/ \
    F(This) \
\
    F(End)


enum Opcode {
#define DECLARE_BYTECODE(name) name##Opcode,
    FOR_EACH_BYTECODE_OP(DECLARE_BYTECODE)
    OpcodeKindEnd
};

struct OpcodeTable {
    void* m_table[OpcodeKindEnd];
};

class ByteCode;
class CodeBlock;

struct ByteCodeGenerateContext {
    ByteCodeGenerateContext()
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
    ByteCode(Opcode code);

    void* m_opcode;
#ifndef NDEBUG
    Opcode m_orgOpcode;
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
        if(m_value.isESString()) {
            ESString* str = m_value.asESString();
            if(str->length() > 30) {
                printf("Push <%s>\n", str->substring(0, 30)->utf8Data());
            } else
                printf("Push <%s>\n", m_value.toString()->utf8Data());
        } else
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

class PushIntoTempStack : public ByteCode {
public:
    PushIntoTempStack()
        : ByteCode(PushIntoTempStackOpcode)
    {

    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("PushIntoTempStack <>\n");
    }
#endif
};

class PopFromTempStack : public ByteCode {
public:
    PopFromTempStack()
        : ByteCode(PopFromTempStackOpcode)
    {

    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("PopFromTempStack <>\n");
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

class GetByIdWithoutException : public ByteCode {
public:
    GetByIdWithoutException(const InternalAtomicString& name, ESString* esName)
        : ByteCode(GetByIdWithoutExceptionOpcode)
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
        printf("GetByIdWithoutException <%s>\n",m_nonAtomicName->utf8Data());
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

class PutById : public ByteCode {
public:
    PutById(const InternalAtomicString& name, ESString* esName)
        : ByteCode(PutByIdOpcode)
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
        printf("PutById <%s>\n", m_nonAtomicName->utf8Data());
    }
#endif
};

class PutByIndex : public ByteCode {
public:
    PutByIndex(size_t index)
        : ByteCode(PutByIndexOpcode)
    {
        m_index = index;
    }
    size_t m_index;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("PutByIndex <%u>\n", (unsigned)m_index);
    }
#endif
};

class PutByIndexWithActivation : public ByteCode {
public:
    PutByIndexWithActivation(size_t fastAccessIndex, size_t fastAccessUpIndex)
        : ByteCode(PutByIndexWithActivationOpcode)
    {
        m_index = fastAccessIndex;
        m_upIndex = fastAccessUpIndex;
    }
    size_t m_index;
    size_t m_upIndex;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("PutByIndexWithActivation <%u, %u>\n", (unsigned)m_index, (unsigned)m_upIndex);
    }
#endif
};

class PutInObject : public ByteCode {
public:
    PutInObject()
        : ByteCode(PutInObjectOpcode)
    {
        m_cachedHiddenClass = nullptr;
        m_cachedPropertyValue = nullptr;
        m_cachedIndex = SIZE_MAX;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("PutInObject <>\n");
    }
#endif

    ESHiddenClass* m_cachedHiddenClass;
    ESString* m_cachedPropertyValue;
    size_t m_cachedIndex;
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

class Increment : public ByteCode {
public:
    Increment()
        : ByteCode(IncrementOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("Increment <>\n");
    }
#endif
};

class Decrement : public ByteCode {
public:
    Decrement()
        : ByteCode(DecrementOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("Decrement <>\n");
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

class UnaryTypeOf : public ByteCode {
public:
    UnaryTypeOf()
        : ByteCode(UnaryTypeOfOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("UnaryTypeOf <>\n");
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

class JumpAndPopIfTopOfStackValueIsTrue : public ByteCode {
public:
    JumpAndPopIfTopOfStackValueIsTrue(size_t jumpPosition)
        : ByteCode(JumpAndPopIfTopOfStackValueIsTrueOpcode)
    {
        m_jumpPosition = jumpPosition;
    }

    size_t m_jumpPosition;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("JumpAndPopIfTopOfStackValueIsTrue <%u>\n",(unsigned)m_jumpPosition);
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

class GetObjectWithPeeking : public ByteCode {
public:
    GetObjectWithPeeking()
        : ByteCode(GetObjectWithPeekingOpcode)
    {
        m_cachedHiddenClass = nullptr;
        m_cachedPropertyValue = nullptr;
        m_cachedIndex = SIZE_MAX;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetObjectWithPeeking <>\n");
    }
#endif

    ESHiddenClass* m_cachedHiddenClass;
    ESString* m_cachedPropertyValue;
    size_t m_cachedIndex;
};

struct EnumerateObjectData : public gc {
    EnumerateObjectData()
    {
        m_idx = 0;
    }

    ESObject* m_object;
    unsigned m_idx;
    std::vector<ESValue, gc_allocator<ESValue> > m_keys;
};

class EnumerateObject : public ByteCode {
public:
    EnumerateObject()
        : ByteCode(EnumerateObjectOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("EnumerateObject <>\n");
    }
#endif

};

class EnumerateObjectKey : public ByteCode {
public:
    EnumerateObjectKey()
        : ByteCode(EnumerateObjectKeyOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("EnumerateObjectKey <>\n");
    }
#endif
    size_t m_forInEnd;

};

class EnumerateObjectEnd : public ByteCode {
public:
    EnumerateObjectEnd()
        : ByteCode(EnumerateObjectEndOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("EnumerateObjectEnd <>\n");
    }
#endif


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

class PushFunctionCallReceiver : public ByteCode {
public:
    PushFunctionCallReceiver()
        : ByteCode(PushFunctionCallReceiverOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("PushFunctionCallReceiver <>\n");
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

class CallEvalFunction : public ByteCode {
public:
    CallEvalFunction()
        : ByteCode(CallEvalFunctionOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("CallEvalFunction <>\n");
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
    CodeBlock()
    {
        m_needsActivation = false;
        m_needsArgumentsObject = false;
        m_isBuiltInFunction = false;
    }
public:
    static CodeBlock* create()
    {
        return new(GC) CodeBlock();
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
    std::vector<char, gc_malloc_allocator<char> > m_code;

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

    if(sp > 1024) {
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
ALWAYS_INLINE void executeNextCode(size_t& programCounter)
{
    programCounter += sizeof (CodeType);
}

#ifndef NDEBUG
ALWAYS_INLINE void dumpBytecode(CodeBlock* codeBlock);
#endif


void initOpcodeTable(OpcodeTable& table);
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

ALWAYS_INLINE void ByteCodeGenerateContext::consumeBreakPositions(CodeBlock* cb, size_t position)
{
    for(size_t i = 0; i < m_breakStatementPositions.size(); i ++) {
        Jump* shouldBeJump = cb->peekCode<Jump>(m_breakStatementPositions[i]);
        ASSERT(shouldBeJump->m_orgOpcode == JumpOpcode);
        shouldBeJump->m_jumpPosition = position;
    }
    m_breakStatementPositions.clear();
}

ALWAYS_INLINE void ByteCodeGenerateContext::consumeContinuePositions(CodeBlock* cb, size_t position)
{
    for(size_t i = 0; i < m_continueStatementPositions.size(); i ++) {
        Jump* shouldBeJump = cb->peekCode<Jump>(m_continueStatementPositions[i]);
        ASSERT(shouldBeJump->m_orgOpcode == JumpOpcode);
        shouldBeJump->m_jumpPosition = position;
    }
    m_continueStatementPositions.clear();
}

}

#endif
