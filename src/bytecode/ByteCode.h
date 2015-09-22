#ifndef __ByteCode__
#define __ByteCode__

#include "runtime/ESValue.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#ifdef ENABLE_ESJIT
#include "jit/ESIRType.h"
#endif

namespace escargot {

class Node;

#define ESCARGOT_INTERPRET_STACK_SIZE 4096
#define FOR_EACH_BYTECODE_OP(F) \
    F(NoOp0) \
    F(Push) \
    F(PopExpressionStatement) \
    F(Pop) \
    F(PushIntoTempStack) \
    F(PopFromTempStack) \
    F(LoadStackPointer) \
    F(CheckStackPointer) \
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
    F(StringIn) \
    F(InstanceOf) \
\
    /*unary expressions*/ \
    F(BitwiseNot) \
    F(LogicalNot) \
    F(UnaryMinus) \
    F(UnaryPlus) \
    F(UnaryTypeOf) \
    F(UnaryDelete) \
    F(UnaryVoid) \
    F(ToNumber) \
\
    /*object, array*/ \
    F(CreateObject) \
    F(CreateArray) \
    F(SetObject) \
    F(SetObjectPropertySetter) \
    F(SetObjectPropertyGetter) \
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
    F(CallBoundFunction) \
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
    F(LoopStart) \
\
    /*try-catch*/ \
    F(Try) \
    F(TryCatchBodyEnd) \
    F(Throw) \
\
    /*etc*/ \
    F(This) \
    F(PrintSpAndBp) \
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
#ifdef ENABLE_ESJIT
    ByteCodeGenerateContext(int currentNodeIndex = 0)
    {
        m_currentNodeIndex = currentNodeIndex;
    }
#else
    ByteCodeGenerateContext()
      : m_offsetToBasePointer(0)
    {
    }
#endif

    ~ByteCodeGenerateContext()
    {
        ASSERT(m_breakStatementPositions.size() == 0);
        ASSERT(m_continueStatementPositions.size() == 0);
        ASSERT(m_labeledBreakStatmentPositions.size() == 0);
        ASSERT(m_labeledContinueStatmentPositions.size() == 0);
    }

    void propagateInformationTo(ByteCodeGenerateContext& ctx)
    {
        ctx.m_breakStatementPositions.insert(ctx.m_breakStatementPositions.end(), m_breakStatementPositions.begin(), m_breakStatementPositions.end());
        ctx.m_continueStatementPositions.insert(ctx.m_continueStatementPositions.end(), m_continueStatementPositions.begin(), m_continueStatementPositions.end());
        ctx.m_labeledBreakStatmentPositions.insert(ctx.m_labeledBreakStatmentPositions.end(), m_labeledBreakStatmentPositions.begin(), m_labeledBreakStatmentPositions.end());
        ctx.m_labeledContinueStatmentPositions.insert(ctx.m_labeledContinueStatmentPositions.end(), m_labeledContinueStatmentPositions.begin(), m_labeledContinueStatmentPositions.end());
        ctx.m_offsetToBasePointer = m_offsetToBasePointer;
        m_breakStatementPositions.clear();
        m_continueStatementPositions.clear();
        m_labeledBreakStatmentPositions.clear();
        m_labeledContinueStatmentPositions.clear();
#ifdef ENABLE_ESJIT
        ctx.m_currentNodeIndex = m_currentNodeIndex;
#endif
    }

    void pushBreakPositions(size_t pos)
    {
        m_breakStatementPositions.push_back(pos);
    }

    void pushLabeledBreakPositions(size_t pos, ESString* lbl)
    {
        m_labeledBreakStatmentPositions.push_back(std::make_pair(lbl, pos));
    }

    void pushContinuePositions(size_t pos)
    {
        m_continueStatementPositions.push_back(pos);
    }

    void pushLabeledContinuePositions(size_t pos, ESString* lbl)
    {
        m_labeledContinueStatmentPositions.push_back(std::make_pair(lbl, pos));
    }

    ALWAYS_INLINE void consumeBreakPositions(CodeBlock* cb, size_t position);
    ALWAYS_INLINE void consumeLabeledBreakPositions(CodeBlock* cb, size_t position, ESString* lbl);
    ALWAYS_INLINE void consumeContinuePositions(CodeBlock* cb, size_t position);
    ALWAYS_INLINE void consumeLabeledContinuePositions(CodeBlock* cb, size_t position, ESString* lbl);

#ifdef ENABLE_ESJIT
    ALWAYS_INLINE unsigned getCurrentNodeIndex()
    {
        return m_currentNodeIndex;
    }

    ALWAYS_INLINE void setCurrentNodeIndex(unsigned index)
    {
        m_currentNodeIndex = index;
    }

    unsigned m_currentNodeIndex;
#endif

    std::vector<size_t> m_breakStatementPositions;
    std::vector<size_t> m_continueStatementPositions;
    std::vector<std::pair<ESString*, size_t> > m_labeledBreakStatmentPositions;
    std::vector<std::pair<ESString*, size_t> > m_labeledContinueStatmentPositions;
    size_t m_offsetToBasePointer;
};

class ByteCode {
public:
    ByteCode(Opcode code, int targetIndex = -1);

    void* m_opcode;
#ifndef NDEBUG
    Opcode m_orgOpcode;
    Node* m_node;
    virtual void dump() {
        ASSERT_NOT_REACHED();
    }
    virtual ~ByteCode() {

    }
#endif
    int m_targetIndex;
};

#ifdef ENABLE_ESJIT
class ProfileData {
public:
    ProfileData() : m_type(ESJIT::TypeBottom), m_value(ESValue()) { }

    void addProfile(ESValue value)
    {
        //mergeType(); // it would be too slow
        m_value = value;
    }
    void updateProfiledType()
    {
        // TODO what happens if this function is called multiple times?
        m_type.mergeType(ESJIT::Type::getType(m_value));
        // TODO if m_type is function, profile function address
        // if m_value is not set to undefined, profiled type will be updated again
        // m_value = ESValue();
    }
    ESJIT::Type& getType() { return m_type; }
protected:
    ESJIT::Type m_type;
    ESValue m_value;
};
#endif

#ifdef NDEBUG
ASSERT_STATIC(sizeof(ByteCode) == sizeof(size_t), "sizeof(ByteCode) should be == sizeof(size_t)");
#endif

class NoOp0 : public ByteCode {
public:
    NoOp0()
        : ByteCode(NoOp0Opcode)
    {
    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("NoOp0 <>\n");
    }
#endif
};

ASSERT_STATIC(sizeof(NoOp0) == sizeof(ByteCode), "sizeof(NoOp0) should be == sizeof(ByteCode)");

class Push : public ByteCode {
public:
    Push(const ESValue& value, int nodeIndex = -1)
        : ByteCode(PushOpcode, nodeIndex)
        , m_value(value)
    {
    }
    ESValue m_value;
#ifndef NDEBUG
    virtual void dump()
    {
        printf("(t%d = ) ", m_targetIndex);
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

class LoadStackPointer : public ByteCode {
public:
    LoadStackPointer(size_t offsetToBasePointer)
        : ByteCode(LoadStackPointerOpcode)
    {
        m_offsetToBasePointer = offsetToBasePointer;
    }

    size_t m_offsetToBasePointer;
#ifndef NDEBUG
    virtual void dump()
    {
        printf("LoadStackPointer <%u>\n", (unsigned)m_offsetToBasePointer);
    }
#endif
};

class CheckStackPointer : public ByteCode {
public:
    CheckStackPointer(size_t lineNumber)
        : ByteCode(CheckStackPointerOpcode)
    {
        m_lineNumber = lineNumber - 1;
    }
    size_t m_lineNumber;
#ifndef NDEBUG
    virtual void dump()
    {
        printf("CheckStackPointer <>\n");
    }
#endif
};

class GetById : public ByteCode {
public:
    GetById(const InternalAtomicString& name, ESString* esName, int targetIndex = -1)
        : ByteCode(GetByIdOpcode, targetIndex)
    {
        m_name = name;
        m_nonAtomicName = esName;
        m_identifierCacheInvalidationCheckCount = std::numeric_limits<unsigned>::max();
        m_cachedSlot = NULL;
    }

    InternalAtomicString m_name;
    ESString* m_nonAtomicName;

    unsigned m_identifierCacheInvalidationCheckCount;
    ESValue* m_cachedSlot;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("(t%d = resolve \'%s\') ", m_targetIndex, m_nonAtomicName->utf8Data());
        printf("GetById <%s>\n", m_nonAtomicName->utf8Data());
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
        m_cachedSlot = NULL;
    }

    InternalAtomicString m_name;
    ESString* m_nonAtomicName;

    unsigned m_identifierCacheInvalidationCheckCount;
    ESValue* m_cachedSlot;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetByIdWithoutException <%s>\n",m_nonAtomicName->utf8Data());
    }
#endif
};

class GetByIndex : public ByteCode {
public:
    GetByIndex(size_t index, int targetIndex = -1)
        : ByteCode(GetByIndexOpcode, targetIndex)
    {
        m_index = index;
    }
    size_t m_index;

#ifndef NDEBUG
    ESString* m_name;
    virtual void dump()
    {
        printf("(t%d = id%u) ", m_targetIndex, (unsigned)m_index);
        printf("GetByIndex <%s, %u>\n", m_name->utf8Data(),  (unsigned)m_index);
    }
#endif
#ifdef ENABLE_ESJIT
    ProfileData m_profile;
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
    PutById(const InternalAtomicString& name, ESString* esName, int targetIndex = -1, Opcode code = PutByIdOpcode)
        : ByteCode(code, targetIndex)
    {
        m_name = name;
        m_nonAtomicName = esName;
        m_identifierCacheInvalidationCheckCount = std::numeric_limits<unsigned>::max();
        m_cachedSlot = NULL;
    }

    InternalAtomicString m_name;
    ESString* m_nonAtomicName;

    unsigned m_identifierCacheInvalidationCheckCount;
    ESValue* m_cachedSlot;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("(t%d = %s) ", m_targetIndex, m_nonAtomicName->utf8Data());
        printf("PutById <%s>\n", m_nonAtomicName->utf8Data());
    }
#endif
};

class PutByIndex : public ByteCode {
public:
    PutByIndex(size_t index, int targetIndex = -1, int srcIndex = -1, Opcode code = PutByIndexOpcode)
        : ByteCode(code, targetIndex)
    {
        m_index = index;
        m_srcIndex = srcIndex;
    }
    size_t m_index;
    int m_srcIndex;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("(t%d: &id%u = t%d) ", m_targetIndex, (unsigned)m_index, m_srcIndex);
        printf("PutByIndex <%u>\n", (unsigned)m_index);
    }
#endif
};

class PutByIndexWithActivation : public ByteCode {
public:
    PutByIndexWithActivation(size_t fastAccessIndex, size_t fastAccessUpIndex, Opcode code = PutByIndexWithActivationOpcode)
        : ByteCode(code)
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
    PutInObject(Opcode code = PutInObjectOpcode, int targetIndex = -1, int objectIndex = -1, int propertyIndex = -1)
        : ByteCode(code, targetIndex)
    {
        m_cachedHiddenClass = (ESHiddenClass*)SIZE_MAX;
        m_cachedPropertyValue = nullptr;
        m_cachedIndex = SIZE_MAX;
        m_objectIndex = objectIndex;
        m_propertyIndex = propertyIndex;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("(t%d = t%d[t%d]) ", m_targetIndex, m_objectIndex, m_propertyIndex);
        printf("PutInObject <>\n");
    }
#endif

    ESHiddenClass* m_cachedHiddenClass;
    ESString* m_cachedPropertyValue;
    size_t m_cachedIndex;
    int m_objectIndex;
    int m_propertyIndex;
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
    BitwiseAnd(int targetIndex = -1, int leftIndex = -1, int rightIndex = -1)
        : ByteCode(BitwiseAndOpcode, targetIndex)
    {
        m_leftIndex = leftIndex;
        m_rightIndex = rightIndex;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("(t%d = t%d&t%d) ", m_targetIndex, m_leftIndex, m_rightIndex);
        printf("BitwiseAnd <>\n");
    }
#endif
    int m_leftIndex;
    int m_rightIndex;
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
    LeftShift(int targetIndex = -1, int leftIndex = -1, int rightIndex = -1)
        : ByteCode(LeftShiftOpcode, targetIndex)
    {
        m_leftIndex = leftIndex;
        m_rightIndex = rightIndex;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("(t%d = t%d<<t%d) ", m_targetIndex, m_leftIndex, m_rightIndex);
        printf("LeftShift <>\n");
    }
#endif
    int m_leftIndex;
    int m_rightIndex;
};

class SignedRightShift : public ByteCode {
public:
    SignedRightShift(int targetIndex = -1, int leftIndex = -1, int rightIndex = -1)
        : ByteCode(SignedRightShiftOpcode, targetIndex)
    {
        m_leftIndex = leftIndex;
        m_rightIndex = rightIndex;
    }


#ifndef NDEBUG
    virtual void dump()
    {
        printf("(t%d = t%d>>t%d) ", m_targetIndex, m_leftIndex, m_rightIndex);
        printf("SignedRightShift <>\n");
    }
#endif
    int m_leftIndex;
    int m_rightIndex;
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
    LessThan(int targetIndex = -1, int leftIndex = -1, int rightIndex = -1)
        : ByteCode(LessThanOpcode, targetIndex)
    {
        m_leftIndex = leftIndex;
        m_rightIndex = rightIndex;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("(t%d = ? t%d<t%d) ", m_targetIndex, m_leftIndex, m_rightIndex);
        printf("LessThan <>\n");
    }
#endif
    int m_leftIndex;
    int m_rightIndex;
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
    Plus(int targetIndex = -1, int leftIndex = -1, int rightIndex = -1)
        : ByteCode(PlusOpcode, targetIndex)
    {
        m_leftIndex = leftIndex;
        m_rightIndex = rightIndex;
    }
    unsigned m_leftIndex;
    unsigned m_rightIndex;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("(t%d = t%d + t%d) ", m_targetIndex, m_leftIndex, m_rightIndex);
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
    Increment(int targetIndex = -1, size_t sourceIndex = -1)
        : ByteCode(IncrementOpcode, targetIndex), m_sourceIndex(sourceIndex)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("(t%d = t%d + 1)", m_targetIndex, m_sourceIndex);
        printf("Increment <>\n");
    }
#endif
private:
    int m_sourceIndex;
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

class StringIn : public ByteCode {
public:
    StringIn()
        : ByteCode(StringInOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("StringIn <>\n");
    }
#endif
};

class InstanceOf : public ByteCode {
public:
    InstanceOf()
        : ByteCode(InstanceOfOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("InstanceOf <>\n");
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

class UnaryDelete : public ByteCode {
public:
    UnaryDelete()
        : ByteCode(UnaryDeleteOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("UnaryDelete <>\n");
    }
#endif
};

class UnaryVoid : public ByteCode {
public:
    UnaryVoid()
        : ByteCode(UnaryVoidOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("UnaryVoid <>\n");
    }
#endif
};

class ToNumber : public ByteCode {
public:
    ToNumber(int targetIndex = -1, size_t sourceIndex = -1)
        : ByteCode(ToNumberOpcode, targetIndex), m_sourceIndex(sourceIndex)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("(t%d = toNumber(t%d)) ", m_targetIndex, m_sourceIndex);
        printf("ToNumber <>\n");
    }
#endif
    int m_sourceIndex;
};

class JumpIfTopOfStackValueIsFalse : public ByteCode {
public:
    JumpIfTopOfStackValueIsFalse(size_t jumpPosition, int targetIndex = -1, int conditionIndex = -1)
        : ByteCode(JumpIfTopOfStackValueIsFalseOpcode, targetIndex), m_conditionIndex(conditionIndex)
    {
        m_jumpPosition = jumpPosition;
    }

    size_t m_jumpPosition;
    int m_conditionIndex;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("(if (!t%d) goto %lu) ", m_conditionIndex, m_jumpPosition);
        printf("JumpIfTopOfStackValueIsFalse <%u>\n",(unsigned)m_jumpPosition);
    }
#endif
};

class JumpIfTopOfStackValueIsTrue : public ByteCode {
public:
    JumpIfTopOfStackValueIsTrue(size_t jumpPosition, int nodeIndex = -1)
        : ByteCode(JumpIfTopOfStackValueIsTrueOpcode, nodeIndex)
    {
        m_jumpPosition = jumpPosition;
    }

    size_t m_jumpPosition;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("(if (t%d) goto %lu) ", m_targetIndex, m_jumpPosition);
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

class SetObjectPropertySetter : public ByteCode {
public:
    SetObjectPropertySetter()
        : ByteCode(SetObjectPropertySetterOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("SetObjectPropertyGetter <>\n");
    }
#endif
};

class SetObjectPropertyGetter : public ByteCode {
public:
    SetObjectPropertyGetter()
        : ByteCode(SetObjectPropertyGetterOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("SetObjectPropertyGetter <>\n");
    }
#endif
};

class GetObject : public ByteCode {
public:
    GetObject(int targetIndex = -1, int objectIndex = -1, int propertyIndex = -1)
        : ByteCode(GetObjectOpcode, targetIndex)
    {
        m_cachedHiddenClass = (ESHiddenClass*)SIZE_MAX;
        m_cachedPropertyValue = nullptr;
        m_cachedIndex = SIZE_MAX;
        m_objectIndex = objectIndex;
        m_propertyIndex = propertyIndex;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("(t%d = t%d[t%d]) ", m_targetIndex, m_objectIndex, m_propertyIndex);
        printf("GetObject <>\n");
    }
#endif

    ESHiddenClass* m_cachedHiddenClass;
    ESString* m_cachedPropertyValue;
    size_t m_cachedIndex;
    int m_objectIndex;
    int m_propertyIndex;
};

class GetObjectWithPeeking : public ByteCode {
public:
    GetObjectWithPeeking()
        : ByteCode(GetObjectWithPeekingOpcode)
    {
        m_cachedHiddenClass = (ESHiddenClass*)SIZE_MAX;
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

class CallBoundFunction : public ByteCode {
public:
    CallBoundFunction()
        : ByteCode(CallBoundFunctionOpcode)
    {
    }

    ESFunctionObject* m_boundTargetFunction;
    ESValue m_boundThis;
    ESValue* m_boundArguments;
    size_t m_boundArgumentsCount;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("CallBoundFunction <>\n");
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
    ReturnFunctionWithValue(int returnIndex = -1)
        : ByteCode(ReturnFunctionWithValueOpcode)
    {
        m_returnIndex = returnIndex;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("(return t%d) ", m_returnIndex);
        printf("ReturnFunctionWithValue <>\n");
    }
#endif
    int m_returnIndex;
};

class Jump : public ByteCode {
public:
    Jump(size_t jumpPosition, int targetIndex = -1)
        : ByteCode(JumpOpcode, targetIndex)
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

class LoopStart : public ByteCode {
public:
    LoopStart()
        : ByteCode(LoopStartOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("LoopStart <>\n");
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

class PrintSpAndBp : public ByteCode {
public:
    PrintSpAndBp()
        : ByteCode(PrintSpAndBpOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("PrintSpAndBp <>\n");
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
        m_isStrict = false;
#ifdef ENABLE_ESJIT
        m_executeCount = 0;
#endif
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

    bool shouldUseStrictMode()
    {
        return m_isStrict || m_isBuiltInFunction;
    }

    InternalAtomicStringVector m_params; //params: [ Pattern ];
    ESStringVector m_nonAtomicParams;
    InternalAtomicStringVector m_innerIdentifiers;
    bool m_needsActivation;
    bool m_needsArgumentsObject;
    bool m_isBuiltInFunction;
    bool m_isStrict;

#ifdef ENABLE_ESJIT
public:
    void ensureHeapProfileDataSlotSize(size_t size);
    void ensureArgumentProfileDataSlotSize(size_t size);
    void writeHeapProfileData(unsigned index, ESValue& value);
    void writeArgumentProfileData(unsigned index, ESValue& value);
    ProfileData* getHeapProfileData(unsigned index) { return &m_heapProfileDatas[index]; }
    ProfileData* getArgumentProfileData(unsigned index) { return &m_argumentProfileDatas[index]; }

    // TODO remove size_t (which stands for bytecode index, used only in assert)
    std::vector<ProfileData, gc_allocator<ProfileData> > m_heapProfileDatas;
    std::vector<ProfileData, gc_allocator<ProfileData> > m_argumentProfileDatas;
    typedef ESValue (*JITFunction)(ESVMInstance*);
    JITFunction m_cachedJITFunction;
    bool m_dontJIT;
    size_t m_tempRegisterSize;
    size_t m_executionCount;
    size_t m_executeCount;
#endif
};

template <typename Type>
ALWAYS_INLINE void push(void*& stk, void* bp, const Type& ptr)
{
    //memcpy(((char *)stk), &ptr, sizeof (Type));
    *((Type *)stk) = ptr;
    stk = (void *)(((size_t)stk) + sizeof(Type));

#ifndef NDEBUG
    size_t siz = sizeof (Type);
    memcpy(((char *)stk), &siz, sizeof (size_t));
    stk = (void *)(((size_t)stk) + sizeof(size_t));

    if(((size_t)stk) - ((size_t)bp) > ESCARGOT_INTERPRET_STACK_SIZE) {
        puts("stackoverflow!!!");
        ASSERT_NOT_REACHED();
    }
    ASSERT(((size_t)stk) % sizeof(size_t) == 0);
#endif
}

template <typename Type>
ALWAYS_INLINE void push(void*& stk, void* bp, Type* ptr)
{
    //memcpy(((char *)stk), &ptr, sizeof (Type));
    *((Type *)stk) = *ptr;
    stk = (void *)(((size_t)stk) + sizeof(Type));

#ifndef NDEBUG
    size_t siz = sizeof (Type);
    memcpy(((char *)stk), &siz, sizeof (size_t));
    stk = (void *)(((size_t)stk) + sizeof(size_t));

    if(((size_t)stk) - ((size_t)bp) > ESCARGOT_INTERPRET_STACK_SIZE) {
        puts("stackoverflow!!!");
        ASSERT_NOT_REACHED();
    }
    ASSERT(((size_t)stk) % sizeof(size_t) == 0);
#endif
}

template <typename Type>
ALWAYS_INLINE Type* pop(void*& stk, void* bp)
{
    if(((size_t)stk) - sizeof (Type) < ((size_t)bp)) {
        ASSERT_NOT_REACHED();
    }
    stk = (void *)(((size_t)stk) - sizeof(Type));
    size_t* siz = (size_t *)stk;
    ASSERT(*siz == sizeof (Type));
#ifndef NDEBUG
    stk = (void *)(((size_t)stk) - sizeof(size_t));
    ASSERT(((size_t)stk) % sizeof(size_t) == 0);
#endif
    return (Type *)stk;
}

template <typename Type>
ALWAYS_INLINE Type* peek(void* stk, void* bp)
{
    void* address = stk;
#ifndef NDEBUG
    address = (void *)(((size_t)address) - sizeof(size_t));
    size_t* siz = (size_t *)address;
    ASSERT(*siz == sizeof (Type));
#endif
    address = (void *)(((size_t)address) - sizeof(Type));
    return (Type *)address;
}

template <typename Type>
ALWAYS_INLINE void sub(void*& stk, void* bp, size_t offsetToBasePointer)
{

#ifndef NDEBUG
    if(((size_t)stk) - offsetToBasePointer * (sizeof(Type) + sizeof(size_t)) < ((size_t)bp)) {
              ASSERT_NOT_REACHED();
    }
    stk = (void *)(((size_t)stk) - offsetToBasePointer * (sizeof(Type) + sizeof(size_t)));
#else
    if(((size_t)stk) - offsetToBasePointer * sizeof(Type) < ((size_t)bp)) {
            ASSERT_NOT_REACHED();
    }
    stk = (void *)(((size_t)stk) - offsetToBasePointer * sizeof(Type));
#endif

}

template <typename CodeType>
ALWAYS_INLINE void executeNextCode(size_t& programCounter)
{
    programCounter += sizeof (CodeType);
}

ALWAYS_INLINE size_t jumpTo(char* codeBuffer, const size_t& jumpPosition)
{
    return (size_t)&codeBuffer[jumpPosition];
}

ALWAYS_INLINE size_t resolveProgramCounter(char* codeBuffer, const size_t& programCounter)
{
    return programCounter - (size_t)codeBuffer;
}

#ifndef NDEBUG
void dumpBytecode(CodeBlock* codeBlock);
#endif


void initOpcodeTable(OpcodeTable& table);
ESValue interpret(ESVMInstance* instance, CodeBlock* codeBlock, size_t programCounter = 0);
CodeBlock* generateByteCode(Node* node);
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

ALWAYS_INLINE void ByteCodeGenerateContext::consumeLabeledContinuePositions(CodeBlock* cb, size_t position, ESString* lbl)
{
    for(size_t i = 0; i < m_labeledContinueStatmentPositions.size(); i ++) {
        if(*m_labeledContinueStatmentPositions[i].first == *lbl) {
            Jump* shouldBeJump = cb->peekCode<Jump>(m_labeledContinueStatmentPositions[i].second);
            ASSERT(shouldBeJump->m_orgOpcode == JumpOpcode);
            shouldBeJump->m_jumpPosition = position;
            m_labeledContinueStatmentPositions.erase(m_labeledContinueStatmentPositions.begin() + i);
            i = -1;
        }
    }
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

ALWAYS_INLINE void ByteCodeGenerateContext::consumeLabeledBreakPositions(CodeBlock* cb, size_t position, ESString* lbl)
{
    for(size_t i = 0; i < m_labeledBreakStatmentPositions.size(); i ++) {
        if(*m_labeledBreakStatmentPositions[i].first == *lbl) {
            Jump* shouldBeJump = cb->peekCode<Jump>(m_labeledBreakStatmentPositions[i].second);
            ASSERT(shouldBeJump->m_orgOpcode == JumpOpcode);
            shouldBeJump->m_jumpPosition = position;
            m_labeledBreakStatmentPositions.erase(m_labeledBreakStatmentPositions.begin() + i);
            i = -1;
        }
    }
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
