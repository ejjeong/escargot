#ifndef __ByteCode__
#define __ByteCode__

#include "runtime/ESValue.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#ifdef ENABLE_ESJIT
#include "jit/ESIRType.h"
#endif

#ifdef ENABLE_ESJIT
namespace nanojit {
class CodeAlloc;
class Allocator;
}
#endif

namespace escargot {

class Node;
class ProgramNode;
class ByteCode;
class CodeBlock;

    // /<OpcodeName, PushCount, PopCount, PeekCount, JITSupported, HasProfileData>
#define FOR_EACH_BYTECODE_OP(F) \
    F(Push, 1, 0, 0, 1, 0) \
    F(PopExpressionStatement, 0, 1, 0, 1, 0) \
    F(Pop, 0, 1, 0, 1, 0) \
    F(FakePop, 0, 0, 0, 1, 0) \
    F(PushIntoTempStack, 0, 1, 0, 1, 0) \
    F(PopFromTempStack, 1, 0, 0, 1, 0) \
    F(LoadStackPointer, 0, 0, 0, 0, 0) \
    F(CheckStackPointer, 0, 0, 0, 0, 0) \
    \
    F(GetById, 1, 0, 0, 1, 1) \
    F(SetById, 0, 0, 1, 1, 0) \
    F(GetByIdWithoutException, 1, 0, 0, 1, 1) \
    F(GetByGlobalIndex, 1, 0, 0, 1, 1) \
    F(SetByGlobalIndex, 0, 0, 1, 1, 0) \
    F(GetByIndex, 1, 0, 0, 1, 1) \
    F(SetByIndex, 0, 0, 1, 1, 0) \
    F(GetByIndexInHeap, 1, 0, 0, 1, 1) \
    F(SetByIndexInHeap, 0, 0, 1, 1, 0) \
    F(GetByIndexInUpperContextHeap, 1, 0, 0, 1, 1) \
    F(SetByIndexInUpperContextHeap, 0, 0, 1, 1, 0) \
    F(GetArgumentsObject, 1, 0, 0, 0, 0) \
    F(SetArgumentsObject, 0, 0, 1, 0, 0) \
    F(CreateBinding, 0, 0, 0, 0, 0) \
    \
    /*binary expressions*/\
    F(Equal, 1, 2, 0, 1, 0) \
    F(NotEqual, 1, 2, 0, 1, 0) \
    F(StrictEqual, 1, 2, 0, 1, 0) \
    F(NotStrictEqual, 1, 2, 0, 1, 0) \
    F(BitwiseAnd, 1, 2, 0, 1, 0) \
    F(BitwiseOr, 1, 2, 0, 1, 0) \
    F(BitwiseXor, 1, 2, 0, 1, 0) \
    F(LeftShift, 1, 2, 0, 1, 0) \
    F(SignedRightShift, 1, 2, 0, 1, 0) \
    F(UnsignedRightShift, 1, 2, 0, 1, 0) \
    F(LessThan, 1, 2, 0, 1, 0) \
    F(LessThanOrEqual, 1, 2, 0, 1, 0) \
    F(GreaterThan, 1, 2, 0, 1, 0) \
    F(GreaterThanOrEqual, 1, 2, 0, 1, 0) \
    F(Plus, 1, 2, 0, 1, 0) \
    F(Minus, 1, 2, 0, 1, 0) \
    F(Multiply, 1, 2, 0, 1, 0) \
    F(Division, 1, 2, 0, 1, 0) \
    F(Mod, 1, 2, 0, 1, 0) \
    F(StringIn, 1, 2, 0, 0, 0) \
    F(InstanceOf, 1, 2, 0, 0, 0) \
    \
    /*unary expressions*/\
    F(BitwiseNot, 1, 1, 0, 1, 0) \
    F(LogicalNot, 1, 1, 0, 1, 0) \
    F(UnaryMinus, 1, 1, 0, 1, 0) \
    F(UnaryPlus, 1, 1, 0, 0, 0) \
    F(UnaryTypeOf, 1, 1, 0, 1, 0) \
    F(UnaryDelete, 1, -1, 0, 0, 0) \
    F(UnaryVoid, 1, 1, 0, 0, 0) \
    F(ToNumber, 1, 1, 0, 1, 0) \
    F(Increment, 1, 1, 0, 1, 0) \
    F(Decrement, 1, 1, 0, 1, 0) \
    \
    /*object, array*/\
    F(CreateObject, 1, 0, 0, 1, 0) \
    F(CreateArray, 1, 0, 0, 1, 0) \
    F(InitObject, 0, 2, 1, 1, 0) \
    F(SetObjectPropertySetter, 0, 2, 0, 0, 0) \
    F(SetObjectPropertyGetter, 0, 2, 0, 0, 0) \
    F(GetObject, 1, 2, 0, 1, 1) \
    F(GetObjectAndPushObject, 2, 2, 0, 1, 1) \
    F(GetObjectSlowMode, 1, 2, 0, 0, 0) \
    F(GetObjectAndPushObjectSlowMode, 2, 2, 0, 0, 0) \
    F(GetObjectWithPeeking, 1, 0, 2, 1, 1) \
    F(GetObjectWithPeekingSlowMode, 1, 0, 2, 0, 0) \
    F(GetObjectPreComputedCase, 1, 1, 0, 1, 1) \
    F(GetObjectPreComputedCaseAndPushObject, 2, 1, 0, 1, 1) \
    F(GetObjectPreComputedCaseSlowMode, 1, 1, 0, 0, 0) \
    F(GetObjectPreComputedCaseAndPushObjectSlowMode, 2, 1, 0, 0, 0) \
    F(GetObjectWithPeekingPreComputedCase, 1, 0, 1, 1, 1) \
    F(GetObjectWithPeekingPreComputedCaseSlowMode, 1, 0, 1, 0, 0) \
    F(SetObject, 1, 3, 0, 1, 0) \
    F(SetObjectSlowMode, 1, 3, 0, 0, 0) \
    F(SetObjectPreComputedCase, 1, 2, 0, 1, 0) \
    F(SetObjectPreComputedCaseSlowMode, 1, 2, 0, 0, 0) \
    \
    /*function*/\
    F(CreateFunction, -1, 0, 0, 1, 0) \
    F(ExecuteNativeFunction, 0, 0, 0, 0, 0) \
    F(CallFunction, 1, -1, 0, 1, 1) \
    F(CallFunctionWithReceiver, 1, -1, 0, 1, 1) \
    F(CallEvalFunction, 1, -1, 0, 1, 1) \
    F(CallBoundFunction, 0, 0, 0, 0, 0) \
    F(NewFunctionCall, 1, -1, 0, 1, 1) \
    F(ReturnFunction, 0, 0, 0, 1, 0) \
    F(ReturnFunctionWithValue, 0, 1, 0, 1, 0) \
    \
    /* control flow */\
    F(Jump, 0, 0, 0, 1, 0) \
    F(JumpComplexCase, 0, 0, 0, 0, 0) \
    F(JumpIfTopOfStackValueIsFalse, 0, 1, 0, 1, 0) \
    F(JumpIfTopOfStackValueIsTrue, 0, 1, 0, 1, 0) \
    F(JumpAndPopIfTopOfStackValueIsTrue, 0, 1, 0, 1, 0) \
    F(JumpIfTopOfStackValueIsFalseWithPeeking, 0, 0, 1, 1, 0) \
    F(JumpIfTopOfStackValueIsTrueWithPeeking, 0, 0, 1, 1, 0) \
    F(DuplicateTopOfStackValue, 1, 0, 1, 1, 0) \
    F(LoopStart, 0, 0, 0, 1, 0) \
    \
    /*try-catch*/\
    F(Try, 0, 0, 0, 0, 0) \
    F(TryCatchBodyEnd, 0, 0, 0, 0, 0) \
    F(Throw, 0, 1, 0, 1, 0) \
    F(ThrowStatic, 0, 0, 0, 0, 0) \
    F(FinallyEnd, 0, 0, 0, 0, 0) \
    \
    /*phi*/\
    F(AllocPhi, 0, 0, 0, 1, 0) \
    F(StorePhi, 0, 0, 0, 1, 0) \
    F(LoadPhi, 0, 0, 0, 1, 0) \
    \
    /*etc*/\
    F(This, 1, 0, 0, 1, 1) \
    F(InitRegExpObject, 1, 0, 0, 1, 1) \
    F(EnumerateObject, 1, 1, 0, 1, 0) \
    F(CheckIfKeyIsLast, 1, 0, 1, 1, 0) \
    F(EnumerateObjectKey, 1, 0, 1, 1, 1) \
    F(PrintSpAndBp, 0, 0, 0, 0, 0) \
    \
    F(End, 0, 0, 0, 1, 0)


enum Opcode {
#define DECLARE_BYTECODE(name, pushCount, popCount, peekCount, JITSupported, hasProfileData) name##Opcode,
    FOR_EACH_BYTECODE_OP(DECLARE_BYTECODE)
#undef DECLARE_BYTECODE
    OpcodeKindEnd
} __attribute__((packed));

struct OpcodeTable {
    void* m_table[OpcodeKindEnd];
};

unsigned char popCountFromOpcode(ByteCode* code, Opcode opcode);
unsigned char pushCountFromOpcode(ByteCode* code, Opcode opcode);
unsigned char peekCountFromOpcode(ByteCode* code, Opcode opcode);

#ifndef NDEBUG
inline const char* getByteCodeName(Opcode opcode)
{
    switch (opcode) {
#define RETURN_BYTECODE_NAME(name, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
    case name##Opcode: \
        return #name; 
        FOR_EACH_BYTECODE_OP(RETURN_BYTECODE_NAME)
#undef  RETURN_BYTECODE_NAME
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}
#endif

struct ByteCodeGenerateContext {
    ByteCodeGenerateContext(CodeBlock* codeBlock, bool isGlobalScope)
        : m_baseRegisterCount(0)
        , m_codeBlock(codeBlock)
        , m_isGlobalScope(isGlobalScope)
        , m_isOutermostContext(true)
        , m_offsetToBasePointer(0)
        , m_positionToContinue(0)
        , m_tryStatementScopeCount(0)
#ifdef ENABLE_ESJIT
        , m_phiIndex(0)
#endif
    {
        m_inCallingExpressionScope = false;
        m_isHeadOfMemberExpression = false;
        m_shouldGenereateByteCodeInstantly = true;
#ifndef ENABLE_ESJIT
        (void)codeBlock;
#endif
    }

    ByteCodeGenerateContext(const ByteCodeGenerateContext& contextBefore)
        : m_baseRegisterCount(contextBefore.m_baseRegisterCount)
        , m_codeBlock(contextBefore.m_codeBlock)
        , m_isGlobalScope(contextBefore.m_isGlobalScope)
        , m_isOutermostContext(false)
        , m_shouldGenereateByteCodeInstantly(contextBefore.m_shouldGenereateByteCodeInstantly)
        , m_inCallingExpressionScope(contextBefore.m_inCallingExpressionScope)
        , m_offsetToBasePointer(contextBefore.m_offsetToBasePointer)
        , m_tryStatementScopeCount(contextBefore.m_tryStatementScopeCount)
#ifdef ENABLE_ESJIT
        , m_phiIndex(contextBefore.m_phiIndex)
#endif
    {
        m_isHeadOfMemberExpression = false;
    }


    ALWAYS_INLINE ~ByteCodeGenerateContext();

    void propagateInformationTo(ByteCodeGenerateContext& ctx)
    {
        ctx.m_breakStatementPositions.insert(ctx.m_breakStatementPositions.end(), m_breakStatementPositions.begin(), m_breakStatementPositions.end());
        ctx.m_continueStatementPositions.insert(ctx.m_continueStatementPositions.end(), m_continueStatementPositions.begin(), m_continueStatementPositions.end());
        ctx.m_labeledBreakStatmentPositions.insert(ctx.m_labeledBreakStatmentPositions.end(), m_labeledBreakStatmentPositions.begin(), m_labeledBreakStatmentPositions.end());
        ctx.m_labeledContinueStatmentPositions.insert(ctx.m_labeledContinueStatmentPositions.end(), m_labeledContinueStatmentPositions.begin(), m_labeledContinueStatmentPositions.end());
        ctx.m_complexCaseStatementPositions.insert(m_complexCaseStatementPositions.begin(), m_complexCaseStatementPositions.end());
        ctx.m_offsetToBasePointer = m_offsetToBasePointer;
        ctx.m_positionToContinue = m_positionToContinue;
#ifdef ENABLE_ESJIT
        ctx.m_phiIndex = m_phiIndex;
#endif

        m_breakStatementPositions.clear();
        m_continueStatementPositions.clear();
        m_labeledBreakStatmentPositions.clear();
        m_labeledContinueStatmentPositions.clear();
        m_complexCaseStatementPositions.clear();
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

    void registerJumpPositionsToComplexCase(size_t frontlimit)
    {
        ASSERT(m_tryStatementScopeCount);
        for (unsigned i = 0 ; i < m_breakStatementPositions.size() ; i ++) {
            if (m_breakStatementPositions[i] > (unsigned long) frontlimit) {
                if (m_complexCaseStatementPositions.find(m_breakStatementPositions[i]) == m_complexCaseStatementPositions.end()) {
                    m_complexCaseStatementPositions.insert(std::make_pair(m_breakStatementPositions[i], m_tryStatementScopeCount));
                }
            }
        }

        for (unsigned i = 0 ; i < m_continueStatementPositions.size() ; i ++) {
            if (m_continueStatementPositions[i] > (unsigned long) frontlimit) {
                if (m_complexCaseStatementPositions.find(m_continueStatementPositions[i]) == m_complexCaseStatementPositions.end()) {
                    m_complexCaseStatementPositions.insert(std::make_pair(m_continueStatementPositions[i], m_tryStatementScopeCount));
                }
            }
        }

        for (unsigned i = 0 ; i < m_labeledBreakStatmentPositions.size() ; i ++) {
            if (m_labeledBreakStatmentPositions[i].second > (unsigned long) frontlimit) {
                if (m_complexCaseStatementPositions.find(m_labeledBreakStatmentPositions[i].second) == m_complexCaseStatementPositions.end()) {
                    m_complexCaseStatementPositions.insert(std::make_pair(m_labeledBreakStatmentPositions[i].second, m_tryStatementScopeCount));
                }
            }
        }

        for (unsigned i = 0 ; i < m_labeledContinueStatmentPositions.size() ; i ++) {
            if (m_labeledContinueStatmentPositions[i].second > (unsigned long) frontlimit) {
                if (m_complexCaseStatementPositions.find(m_labeledContinueStatmentPositions[i].second) == m_complexCaseStatementPositions.end()) {
                    m_complexCaseStatementPositions.insert(std::make_pair(m_labeledContinueStatmentPositions[i].second, m_tryStatementScopeCount));
                }
            }
        }

    }

    ALWAYS_INLINE void consumeBreakPositions(CodeBlock* cb, size_t position);
    ALWAYS_INLINE void consumeLabeledBreakPositions(CodeBlock* cb, size_t position, ESString* lbl);
    ALWAYS_INLINE void consumeContinuePositions(CodeBlock* cb, size_t position);
    ALWAYS_INLINE void consumeLabeledContinuePositions(CodeBlock* cb, size_t position, ESString* lbl);
    ALWAYS_INLINE void morphJumpPositionIntoComplexCase(CodeBlock* cb, size_t codePos);

    int m_baseRegisterCount;

    CodeBlock* m_codeBlock;
    bool m_isGlobalScope;
    bool m_isOutermostContext;

    bool m_shouldGenereateByteCodeInstantly;
    bool m_inCallingExpressionScope;
    bool m_isHeadOfMemberExpression;

    std::vector<size_t> m_breakStatementPositions;
    std::vector<size_t> m_continueStatementPositions;
    std::vector<std::pair<ESString*, size_t> > m_labeledBreakStatmentPositions;
    std::vector<std::pair<ESString*, size_t> > m_labeledContinueStatmentPositions;
    // For For In Statement
    size_t m_offsetToBasePointer;
    // For Label Statement
    size_t m_positionToContinue;
    // code position, tryStatement count
    int m_tryStatementScopeCount;
    std::map<size_t, size_t> m_complexCaseStatementPositions;
#ifdef ENABLE_ESJIT
    // For AllocPhi, StorePhi, LoadPhi
    size_t m_phiIndex;
#endif
};

#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
struct ExtraDataGenerateContext {
    inline ExtraDataGenerateContext(CodeBlock* codeBlock);
    inline ~ExtraDataGenerateContext();

#ifdef ENABLE_ESJIT
    void cleanupSSARegisterCount()
    {
        m_currentSSARegisterCount = -1;
    }

    ALWAYS_INLINE int lastUsedSSAIndex()
    {
        return m_currentSSARegisterCount - 1;
    }
#endif

    CodeBlock* m_codeBlock;
    int m_baseRegisterCount;
#ifdef ENABLE_ESJIT
    int m_currentSSARegisterCount;
    std::vector<int> m_ssaComputeStack;
    std::vector<int> m_phiIndexToAllocIndexMapping;
    std::vector<std::pair<int, int> > m_phiIndexToStoreIndexMapping;
#endif
};
#endif

#ifdef ENABLE_ESJIT

class ProfileData {
public:
    ProfileData()
        : m_type(ESJIT::TypeBottom), m_value(ESValue(ESValue::ESEmptyValueTag::ESEmptyValue)) { }

    void addProfile(const ESValue& value)
    {
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

class JITProfileTarget {
public:
    ProfileData m_profile;
};

#else
class JITProfileTarget {
};
#endif
class ByteCode {
public:
    ByteCode(Opcode code);

    void assignOpcodeInAddress();
    void* m_opcodeInAddress;

#ifndef NDEBUG
    Opcode m_orgOpcode;
    Node* m_node;
    virtual void dump()
    {
        ASSERT_NOT_REACHED();
    }
    virtual ~ByteCode() { }
#endif
};

struct __attribute__((__packed__)) ByteCodeExtraData {
    Opcode m_opcode;
#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
    struct DecoupledData {
        DecoupledData()
        {
            m_codePosition = SIZE_MAX;
            m_baseRegisterIndex = 0;
            m_registerIncrementCount = 0;
            m_registerDecrementCount = 0;
#ifdef ENABLE_ESJIT
            m_targetIndex0 = -1;
            m_targetIndex1 = -1;
#endif
        }
        size_t m_codePosition;
        short m_baseRegisterIndex;
        char m_registerIncrementCount; // stack push count
        char m_registerDecrementCount; // stack pop count
#ifdef ENABLE_ESJIT
        int m_targetIndex0;
        int m_targetIndex1;
        std::vector<int> m_sourceIndexes;
#endif
    };
    DecoupledData* m_decoupledData;
#endif

    ByteCodeExtraData()
    {
        m_opcode = (Opcode)0;
#ifndef NDEBUG
        m_decoupledData = new DecoupledData();
#elif defined(ENABLE_ESJIT)
        m_decoupledData = nullptr;
#endif
    }
};

#ifdef NDEBUG
ASSERT_STATIC(sizeof(ByteCode) == sizeof(size_t), "sizeof(ByteCode) should be == sizeof(size_t)");
#endif


struct ESHiddenClassInlineCacheData {
    ESHiddenClassInlineCacheData()
    {
        m_cachedIndex = SIZE_MAX;
    }

    ESHiddenClassChain m_cachedhiddenClassChain;
    size_t m_cachedIndex;
};

struct ESHiddenClassInlineCache {
    std::vector<ESHiddenClassInlineCacheData, gc_allocator<ESHiddenClassInlineCacheData> > m_cache;
    size_t m_executeCount;
    ESHiddenClassInlineCache()
    {
        m_executeCount = 0;
    }
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
        if (m_value.isEmpty())
            printf("Push <Empty>\n");
        else if (m_value.isESString()) {
            ESString* str = m_value.asESString();
            if (str->length() > 30) {
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

class FakePop : public ByteCode {
public:
    FakePop()
        : ByteCode(FakePopOpcode)
    {

    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("FakePop <>\n");
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
    PopFromTempStack(size_t pushPos)
        : ByteCode(PopFromTempStackOpcode)
    {
        m_pushCodePosition = pushPos;
    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("PopFromTempStack <>\n");
    }
#endif

    size_t m_pushCodePosition;
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

class GetById : public ByteCode, public JITProfileTarget {
public:
    GetById(const InternalAtomicString& name, bool onlySearchGlobal)
        : ByteCode(GetByIdOpcode)
        , m_name(name)
        , m_onlySearchGlobal(onlySearchGlobal)
    {
        m_identifierCacheInvalidationCheckCount = std::numeric_limits<unsigned>::max();
        m_cachedSlot = NULL;
    }
    InternalAtomicString m_name;

    unsigned m_identifierCacheInvalidationCheckCount;
    ESValue* m_cachedSlot;
    bool m_onlySearchGlobal;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetById <%s (%d)>\n", m_name.string()->utf8Data(), m_onlySearchGlobal);
    }
#endif
};

class GetByIdWithoutException : public ByteCode, public JITProfileTarget {
public:
    GetByIdWithoutException(const InternalAtomicString& name, bool onlySearchGlobal)
        : ByteCode(GetByIdWithoutExceptionOpcode)
        , m_name(name)
        , m_onlySearchGlobal(onlySearchGlobal)
    {
        m_identifierCacheInvalidationCheckCount = std::numeric_limits<unsigned>::max();
        m_cachedSlot = NULL;
    }

    InternalAtomicString m_name;

    unsigned m_identifierCacheInvalidationCheckCount;
    ESValue* m_cachedSlot;
    bool m_onlySearchGlobal;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetByIdWithoutException <%s (%d)>\n", m_name.string()->utf8Data(), m_onlySearchGlobal);
    }
#endif
};

ASSERT_STATIC(sizeof(GetById) == sizeof(GetByIdWithoutException), "");

class GetByIndex : public ByteCode, public JITProfileTarget {
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

class GetByIndexInHeap : public ByteCode, public JITProfileTarget {
public:
    GetByIndexInHeap(size_t index)
        : ByteCode(GetByIndexInHeapOpcode)
    {
        m_index = index;
    }
    size_t m_index;

#ifndef NDEBUG
    ESString* m_name;
    virtual void dump()
    {
        printf("GetByIndexInHeap <%s, %u>\n", m_name->utf8Data(),  (unsigned)m_index);
    }
#endif
};

class GetByGlobalIndex : public ByteCode, public JITProfileTarget {
public:
    GetByGlobalIndex(size_t index, ESString* name)
        : ByteCode(GetByGlobalIndexOpcode)
    {
        m_index = index;
        m_name = name;
    }
    size_t m_index;
    ESString* m_name;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetByGlobalIndex <%s, %u>\n", m_name->utf8Data(),  (unsigned)m_index);
    }
#endif
};

class GetByIndexInUpperContextHeap : public ByteCode, public JITProfileTarget {
public:
    GetByIndexInUpperContextHeap(size_t fastAccessIndex, size_t fastAccessUpIndex)
        : ByteCode(GetByIndexInUpperContextHeapOpcode)
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
        printf("GetByIndexInUpperContextHeap <%s, %u, %u>\n", m_name->utf8Data(), (unsigned)m_index, (unsigned)m_upIndex);
    }
#endif
};

class GetArgumentsObject : public ByteCode {
public:
    GetArgumentsObject()
        : ByteCode(GetArgumentsObjectOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetArgumentsObject <>\n");
    }
#endif
};

class SetById : public ByteCode {
public:
    SetById(const InternalAtomicString& name, bool onlySearchGlobal)
        : ByteCode(SetByIdOpcode)
        , m_name(name)
        , m_onlySearchGlobal(onlySearchGlobal)
    {
        m_identifierCacheInvalidationCheckCount = std::numeric_limits<unsigned>::max();
        m_cachedSlot = NULL;
    }

    InternalAtomicString m_name;

    unsigned m_identifierCacheInvalidationCheckCount;
    ESValue* m_cachedSlot;
    bool m_onlySearchGlobal;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("SetById <%s (%d)>\n", m_name.string()->utf8Data(), m_onlySearchGlobal);
    }
#endif
};

class SetByIndex : public ByteCode {
public:
    SetByIndex(size_t index, Opcode code = SetByIndexOpcode)
        : ByteCode(code)
    {
        m_index = index;
    }
    size_t m_index;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("SetByIndex <%u>\n", (unsigned)m_index);
    }
#endif
};

class SetByIndexInHeap : public ByteCode {
public:
    SetByIndexInHeap(size_t index, Opcode code = SetByIndexInHeapOpcode)
        : ByteCode(code)
    {
        m_index = index;
    }
    size_t m_index;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("SetByIndexInHeap <%u>\n", (unsigned)m_index);
    }
#endif
};


class SetByGlobalIndex : public ByteCode {
public:
    SetByGlobalIndex(size_t index, ESString* name)
        : ByteCode(SetByGlobalIndexOpcode)
    {
        m_index = index;
        m_name = name;
    }
    size_t m_index;
    ESString* m_name;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("SetByGlobalIndex <%u>\n", (unsigned)m_index);
    }
#endif
};

class SetByIndexInUpperContextHeap : public ByteCode {
public:
    SetByIndexInUpperContextHeap(size_t fastAccessIndex, size_t fastAccessUpIndex, Opcode code = SetByIndexInUpperContextHeapOpcode)
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
        printf("SetByIndexInUpperContextHeap <%u, %u>\n", (unsigned)m_index, (unsigned)m_upIndex);
    }
#endif
};

class SetArgumentsObject : public ByteCode {
public:
    SetArgumentsObject()
        : ByteCode(SetArgumentsObjectOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("SetArgumentsObject <>\n");
    }
#endif
};

class CreateBinding : public ByteCode {
public:
    CreateBinding(InternalAtomicString name)
        : ByteCode(CreateBindingOpcode)
        , m_name(name)
    {
    }
    InternalAtomicString m_name;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("CreateBinding <%s>\n", m_name.string()->utf8Data());
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
    Multiply()
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
private:
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
    UnaryDelete(bool isDeleteObjectKey, ESString* name = nullptr)
        : ByteCode(UnaryDeleteOpcode)
    {
        m_isDeleteObjectKey = isDeleteObjectKey;
        m_name = name;
    }

    bool m_isDeleteObjectKey;
    ESString* m_name;

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
    ToNumber()
        : ByteCode(ToNumberOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("ToNumber <>\n");
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
        printf("JumpIfTopOfStackValueIsFalse <%u>\n", (unsigned)m_jumpPosition);
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
        printf("JumpIfTopOfStackValueIsTrue <%u>\n", (unsigned)m_jumpPosition);
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
        printf("JumpAndPopIfTopOfStackValueIsTrue <%u>\n", (unsigned)m_jumpPosition);
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
        printf("JumpIfTopOfStackValueIsFalseWithPeeking <%u>\n", (unsigned)m_jumpPosition);
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
        printf("JumpIfTopOfStackValueIsTrueWithPeeking <%u>\n", (unsigned)m_jumpPosition);
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
        printf("CreateObject <%u>\n", (unsigned)m_keyCount);
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
        printf("CreateArray <%u>\n", (unsigned)m_keyCount);
    }
#endif
};

class InitObject : public ByteCode {
public:
    InitObject()
        : ByteCode(InitObjectOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("InitObject <>\n");
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

class GetObject : public ByteCode, public JITProfileTarget {
public:
    GetObject(Opcode code = GetObjectOpcode)
        : ByteCode(code)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetObject <>\n");
    }
#endif
};

class GetObjectAndPushObject : public ByteCode, public JITProfileTarget {
public:
    GetObjectAndPushObject(Opcode code = GetObjectAndPushObjectOpcode)
        : ByteCode(code)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetObjectAndPushObject <>\n");
    }
#endif
};

ASSERT_STATIC(sizeof(GetObject) == sizeof(GetObjectAndPushObject), "");

class GetObjectSlowMode : public GetObject {
public:
    GetObjectSlowMode()
        : GetObject(GetObjectSlowModeOpcode)
    {
    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetObjectSlowMode <>\n");
    }
#endif
};

ASSERT_STATIC(sizeof(GetObject) == sizeof(GetObjectSlowMode), "");

class GetObjectAndPushObjectSlowMode : public GetObjectAndPushObject {
public:
    GetObjectAndPushObjectSlowMode()
        : GetObjectAndPushObject(GetObjectAndPushObjectSlowModeOpcode)
    {
    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetObjectAndPushObjectSlowMode <>\n");
    }
#endif
};

ASSERT_STATIC(sizeof(GetObjectAndPushObject) == sizeof(GetObjectAndPushObjectSlowMode), "");

class GetObjectWithPeeking : public ByteCode, public JITProfileTarget {
public:
    GetObjectWithPeeking(Opcode code = GetObjectWithPeekingOpcode)
        : ByteCode(code)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetObjectWithPeeking <>\n");
    }
#endif
};

class GetObjectWithPeekingSlowMode : public GetObjectWithPeeking {
public:
    GetObjectWithPeekingSlowMode()
        : GetObjectWithPeeking(GetObjectWithPeekingSlowModeOpcode)
    {
    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetObjectWithPeekingSlowMode <>\n");
    }
#endif
};

ASSERT_STATIC(sizeof(GetObjectWithPeeking) == sizeof(GetObjectWithPeekingSlowMode), "");

class GetObjectPreComputedCase : public ByteCode, public JITProfileTarget {
public:
    GetObjectPreComputedCase(const ESValue& v, Opcode code = GetObjectPreComputedCaseOpcode)
        : ByteCode(code)
    {
        m_propertyValue = v.toString();
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetObjectPreComputedCase <%s>\n", m_propertyValue->utf8Data());
    }
#endif
    ESString* m_propertyValue;
    ESHiddenClassInlineCache m_inlineCache;
};

class GetObjectPreComputedCaseAndPushObject : public ByteCode, public JITProfileTarget {
public:
    GetObjectPreComputedCaseAndPushObject(const ESValue& v, Opcode code = GetObjectPreComputedCaseAndPushObjectOpcode)
        : ByteCode(code)
    {
        m_propertyValue = v.toString();
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetObjectPreComputedCaseAndPushObject <%s>\n", m_propertyValue->utf8Data());
    }
#endif
    ESString* m_propertyValue;
    ESHiddenClassInlineCache m_inlineCache;
};

ASSERT_STATIC(sizeof(GetObjectPreComputedCase) == sizeof(GetObjectPreComputedCaseAndPushObject), "");

class GetObjectPreComputedCaseSlowMode : public GetObjectPreComputedCase {
public:
    GetObjectPreComputedCaseSlowMode(const ESValue& v)
        : GetObjectPreComputedCase(v, GetObjectPreComputedCaseSlowModeOpcode)
    {
    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetObjectPreComputedCaseSlowMode <>\n");
    }
#endif
};

class GetObjectPreComputedCaseAndPushObjectSlowMode : public GetObjectPreComputedCaseAndPushObject {
public:
    GetObjectPreComputedCaseAndPushObjectSlowMode(const ESValue& v)
        : GetObjectPreComputedCaseAndPushObject(v, GetObjectPreComputedCaseAndPushObjectSlowModeOpcode)
    {
    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetObjectPreComputedCaseAndPushObjectSlowMode <>\n");
    }
#endif
};

ASSERT_STATIC(sizeof(GetObjectPreComputedCaseAndPushObject) == sizeof(GetObjectPreComputedCaseAndPushObjectSlowMode), "");

class GetObjectWithPeekingPreComputedCase : public ByteCode, public JITProfileTarget  {
public:
    GetObjectWithPeekingPreComputedCase(const ESValue& v, Opcode code = GetObjectWithPeekingPreComputedCaseOpcode)
        : ByteCode(code)
    {
        m_propertyValue = v.toString();
    }

    ESString* m_propertyValue;
    ESHiddenClassInlineCache m_inlineCache;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetObjectWithPeekingPreComputedCase <>\n");
    }
#endif
};

class GetObjectWithPeekingPreComputedCaseSlowMode : public GetObjectWithPeekingPreComputedCase {
public:
    GetObjectWithPeekingPreComputedCaseSlowMode(const ESValue& v)
        : GetObjectWithPeekingPreComputedCase(v, GetObjectWithPeekingPreComputedCaseSlowModeOpcode)
    {
    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("GetObjectWithPeekingPreComputedCaseSlowMode <>\n");
    }
#endif
};

ASSERT_STATIC(sizeof(GetObjectWithPeekingPreComputedCase) == sizeof(GetObjectWithPeekingPreComputedCaseSlowMode), "");

class SetObject : public ByteCode {
public:
    SetObject(Opcode code = SetObjectOpcode)
        : ByteCode(code)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("SetObject <>\n");
    }
#endif
};

class SetObjectSlowMode : public SetObject {
public:
    SetObjectSlowMode()
        : SetObject(SetObjectSlowModeOpcode)
    {
    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("SetObjectSlowMode <>\n");
    }
#endif
};

ASSERT_STATIC(sizeof(SetObject) == sizeof(SetObjectSlowMode), "");

class SetObjectPreComputedCase : public ByteCode {
public:
    SetObjectPreComputedCase(const ESValue& v, Opcode code = SetObjectPreComputedCaseOpcode)
        : ByteCode(code)
    {
        m_propertyValue = v.toString();
        m_cachedIndex = SIZE_MAX;
        m_hiddenClassWillBe = NULL;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("SetObjectPreComputedCase <>\n");
    }
#endif
    ESHiddenClassChain m_cachedhiddenClassChain;
    ESString* m_propertyValue;
    size_t m_cachedIndex;
    ESHiddenClass* m_hiddenClassWillBe;
};

class SetObjectPreComputedCaseSlowMode : public SetObjectPreComputedCase {
public:
    SetObjectPreComputedCaseSlowMode(const ESValue& v)
        : SetObjectPreComputedCase(v, SetObjectPreComputedCaseSlowModeOpcode)
    {
    }
#ifndef NDEBUG
    virtual void dump()
    {
        printf("SetObjectPreComputedCaseSlowMode <>\n");
    }
#endif
};

ASSERT_STATIC(sizeof(SetObjectPreComputedCase) == sizeof(SetObjectPreComputedCaseSlowMode), "");

struct EnumerateObjectData : public gc {
    EnumerateObjectData()
    {
        m_idx = 0;
    }

    ESHiddenClassChainStd m_hiddenClassChain;
    ESObject* m_object;
    unsigned m_idx;
    std::vector<ESValue, gc_allocator<ESValue> > m_keys;
};

class CreateFunction : public ByteCode {
public:
    CreateFunction(InternalAtomicString name, ESString* nonAtomicName, CodeBlock* codeBlock, bool isDecl, size_t idIndex, bool isIdIndexOnHeapStorage)
        : ByteCode(CreateFunctionOpcode)
        , m_name(name)
    {
        m_nonAtomicName = nonAtomicName;
        m_codeBlock = codeBlock;
        m_isDeclaration = isDecl;
        m_idIndex = idIndex;
        m_isIdIndexOnHeapStorage = isIdIndexOnHeapStorage;
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
    bool m_isDeclaration;
    bool m_isIdIndexOnHeapStorage;
    size_t m_idIndex;
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

class CallFunction : public ByteCode, public JITProfileTarget  {
public:
    CallFunction(unsigned argumentCount)
        : ByteCode(CallFunctionOpcode)
    {
        m_argmentCount = argumentCount;
    }

    unsigned m_argmentCount;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("CallFunction <>\n");
    }
#endif

};

class CallFunctionWithReceiver : public ByteCode, public JITProfileTarget  {
public:
    CallFunctionWithReceiver(unsigned argumentCount)
        : ByteCode(CallFunctionWithReceiverOpcode)
    {
        m_argmentCount = argumentCount;
    }

    unsigned m_argmentCount;

#ifndef NDEBUG
    virtual void dump()
    {
        printf("CallFunctionWithReceiver <>\n");
    }
#endif

};

class CallEvalFunction : public ByteCode, public JITProfileTarget  {
public:
    CallEvalFunction(unsigned argumentCount)
        : ByteCode(CallEvalFunctionOpcode)
    {
        m_argmentCount = argumentCount;
    }

    unsigned m_argmentCount;

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

class NewFunctionCall : public ByteCode, public JITProfileTarget  {
public:
    NewFunctionCall(unsigned argumentCount)
        : ByteCode(NewFunctionCallOpcode)
    {
        m_argmentCount = argumentCount;
    }

    unsigned m_argmentCount;
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

class JumpComplexCase : public ByteCode {
public:
    JumpComplexCase(Jump* jmp, size_t tryDupCount)
        : ByteCode(JumpComplexCaseOpcode)
    {
        m_controlFlowRecord = ESControlFlowRecord::create(ESControlFlowRecord::ControlFlowReason::NeedsJump,
            (ESPointer *)jmp->m_jumpPosition, ESValue((int32_t)tryDupCount));
#ifndef NDEBUG
        m_node = jmp->m_node;
#endif
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("JumpComplexCase <%zd, %zd>\n", (size_t)m_controlFlowRecord->value().asESPointer(), (size_t)m_controlFlowRecord->value2().asInt32());
    }
#endif

    ESControlFlowRecord* m_controlFlowRecord;
};

ASSERT_STATIC(sizeof(Jump) == sizeof(JumpComplexCase), "sizeof(Jump) == sizeof(JumpComplexCase)");

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
    size_t m_tryDupCount;
    size_t m_catchPosition;
    size_t m_statementEndPosition;
    InternalAtomicString m_name;
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


class AllocPhi : public ByteCode {
public:
    AllocPhi(size_t phiIndex)
        : ByteCode(AllocPhiOpcode)
    {
        m_phiIndex = phiIndex;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("AllocPhi <%zu>\n", m_phiIndex);
    }
#endif

    size_t m_phiIndex;
};

class StorePhi : public ByteCode {
public:
    StorePhi(size_t phiIndex, bool consumeSource, bool isConsequent)
        : ByteCode(StorePhiOpcode)
    {
        m_phiIndex = phiIndex;
        m_consumeSource = consumeSource;
        m_isConsequent = isConsequent;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("StorePhi <%zu>\n", m_phiIndex);
    }
#endif

    size_t m_phiIndex;
    bool m_consumeSource;
    bool m_isConsequent;
};

class LoadPhi : public ByteCode {
public:
    LoadPhi(size_t phiIndex)
        : ByteCode(LoadPhiOpcode)
    {
        m_phiIndex = phiIndex;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("LoadPhi <%zu>\n", m_phiIndex);
    }
#endif

    size_t m_phiIndex;
};

class This : public ByteCode, public JITProfileTarget {
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

class InitRegExpObject : public ByteCode, public JITProfileTarget {
public:
    InitRegExpObject(ESString* body, ESRegExpObject::Option flag)
        : ByteCode(InitRegExpObjectOpcode)
    {
        m_body = body;
        m_flag = flag;
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("InitRegExpObject <%s>\n", m_body->utf8Data());
    }
#endif

    ESString* m_body;
    escargot::ESRegExpObject::Option m_flag;
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

class CheckIfKeyIsLast : public ByteCode {
public:
    CheckIfKeyIsLast()
        : ByteCode(CheckIfKeyIsLastOpcode)
    {
    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("CheckIfKeyIsLast <>\n");
    }
#endif

};

class EnumerateObjectKey : public ByteCode, public JITProfileTarget {
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

class ThrowStatic : public ByteCode {
public:
    ThrowStatic(ESErrorObject::Code code, ESString* msg)
        : ByteCode(ThrowStaticOpcode)
        , m_code(code), m_msg(msg)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("ThrowStatic <%s, %d>\n", m_msg->utf8Data(), m_code);
    }
#endif

    ESErrorObject::Code m_code;
    ESString* m_msg;
};

class FinallyEnd : public ByteCode {
public:
    FinallyEnd()
        : ByteCode(FinallyEndOpcode)
    {

    }

#ifndef NDEBUG
    virtual void dump()
    {
        printf("FinallyEnd <>\n");
    }
#endif
    size_t m_tryDupCount;
    bool m_finalizerExists;
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
public:
    enum ExecutableType { GlobalCode, FunctionCode, EvalCode };
    static CodeBlock* create(ExecutableType type, size_t roughCodeBlockSizeInWordSize = 0, bool isBuiltInFunction = false)
    {
        return new CodeBlock(type, roughCodeBlockSizeInWordSize, isBuiltInFunction);
    }

private:
    CodeBlock(ExecutableType type, size_t roughCodeBlockSizeInWordSize, bool isBuiltInFunction);
public:
    void finalize();

    template <typename CodeType>
    void pushCode(const CodeType& type, ByteCodeGenerateContext& context, Node* node);
    inline void pushCode(const ExecuteNativeFunction& code);
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

#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
    Opcode lastCode()
    {
        return m_extraData[m_extraData.size() - 1].m_opcode;
    }
#endif

    template <typename CodeType>
    void popLastCode()
    {
        m_code.resize(m_code.size() - sizeof(CodeType));
#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
        m_extraData.erase(m_extraData.end() - 1);
#endif
    }

    size_t currentCodeSize()
    {
        return m_code.size();
    }

    bool shouldUseStrictMode()
    {
        return m_isStrict;
    }

    std::vector<char, gc_malloc_allocator<char> > m_code;
#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
    std::vector<ByteCodeExtraData> m_extraData;
#endif

    ExecutableType m_type;
    Node* m_ast;

    size_t m_stackAllocatedIdentifiersCount;
    InternalAtomicStringVector m_heapAllocatedIdentifiers;
    FunctionParametersInfoVector m_paramsInformation;

    unsigned m_requiredStackSizeInESValueSize;
    unsigned m_argumentCount;

    bool m_hasCode;
    bool m_isStrict;
    bool m_isFunctionExpression;
    bool m_needsActivation;
    bool m_needsHeapAllocatedExecutionContext;
    bool m_needsComplexParameterCopy; // parameters are captured
    bool m_needsToPrepareGenerateArgumentsObject;
    bool m_isBuiltInFunction;
    bool m_isCached;

    bool m_isFunctionExpressionNameHeapAllocated;
    size_t m_functionExpressionNameIndex;
#ifndef NDEBUG
    InternalAtomicString m_id;
    ESString* m_nonAtomicId;
#endif

#ifdef ENABLE_ESJIT
    void removeJITInfo();
    void removeJITCode();
    nanojit::CodeAlloc* codeAlloc();
    nanojit::Allocator* nanoJITDataAllocator();
    JITFunction m_cachedJITFunction;
    bool m_dontJIT;
    size_t m_recursionDepth;
    std::vector<unsigned> m_byteCodeIndexesHaveToProfile;
    std::vector<unsigned> m_byteCodePositionsHaveToProfile;
    size_t m_tempRegisterSize;
    size_t m_phiSize;
    size_t m_executeCount;
    size_t m_osrExitCount;
    size_t m_jitThreshold;
    nanojit::CodeAlloc* m_codeAlloc;
    nanojit::Allocator* m_nanoJITDataAllocator;
#endif

#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
    void fillExtraData();
#endif
private:
#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
    void pushCodeFillExtraData(ByteCode* code, ByteCodeExtraData* data, ByteCodeGenerateContext& context, size_t codePosition, size_t bytecodeCount);
#else
    void pushCodeFillExtraData(ByteCode* code, ByteCodeExtraData* data, ByteCodeGenerateContext& context, size_t codePosition);
#endif
#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
    void fillExtraData(ByteCode* code, ByteCodeExtraData* data, ExtraDataGenerateContext& context, size_t codePosition, size_t bytecodeCount);
#endif
};

#ifdef NDEBUG
template <typename Type>
ALWAYS_INLINE void push(void*& stk, const Type& ptr)
{
    *((Type *)stk) = ptr;
    stk = (void *)(((size_t)stk) + sizeof(Type));
}

template <typename Type>
ALWAYS_INLINE void push(void*& stk, Type* ptr)
{
    *((Type *)stk) = *ptr;
    stk = (void *)(((size_t)stk) + sizeof(Type));
}

template <typename Type>
ALWAYS_INLINE Type* pop(void*& stk)
{
    stk = (void *)(((size_t)stk) - sizeof(Type));
    return (Type *)stk;
}

template <typename Type>
ALWAYS_INLINE Type* peek(void* stk)
{
    void* address = stk;
    address = (void *)(((size_t)address) - sizeof(Type));
    return (Type *)address;
}

template <typename Type>
ALWAYS_INLINE void sub(void*& stk, size_t offsetToBasePointer)
{
    stk = (void *)(((size_t)stk) - offsetToBasePointer * sizeof(Type));
}

#define PUSH(stk, topOfStack, ptr) push<ESValue>(stk, ptr)
#define POP(stk, bp) pop<ESValue>(stk)
#define PEEK(stk, bp) peek<ESValue>(stk)
#define SUB_STACK(stk, bp, offsetToBasePointer) sub<ESValue>(stk, offsetToBasePointer)

#else
template <typename Type>
ALWAYS_INLINE void push(void*& stk, void* topOfStack, const Type& ptr)
{
    *((Type *)stk) = ptr;
    stk = (void *)(((size_t)stk) + sizeof(Type));

#ifndef NDEBUG
    if (stk > topOfStack) {
        puts("stackoverflow!!!");
        ASSERT_NOT_REACHED();
    }
    ASSERT(((size_t)stk) % sizeof(size_t) == 0);
#endif
}

template <typename Type>
ALWAYS_INLINE void push(void*& stk, void* topOfStack, Type* ptr)
{
    *((Type *)stk) = *ptr;
    stk = (void *)(((size_t)stk) + sizeof(Type));

#ifndef NDEBUG
    if (stk > topOfStack) {
        puts("stackoverflow!!!");
        ASSERT_NOT_REACHED();
    }
    ASSERT(((size_t)stk) % sizeof(size_t) == 0);
#endif
}

template <typename Type>
ALWAYS_INLINE Type* pop(void*& stk, void* bp)
{
#ifndef NDEBUG
    if (((size_t)stk) - sizeof(Type) < ((size_t)bp)) {
        ASSERT_NOT_REACHED();
    }
#endif
    stk = (void *)(((size_t)stk) - sizeof(Type));
    return (Type *)stk;
}

template <typename Type>
ALWAYS_INLINE Type* peek(void* stk, void* bp)
{
    void* address = stk;
    address = (void *)(((size_t)address) - sizeof(Type));
    return (Type *)address;
}

template <typename Type>
ALWAYS_INLINE void sub(void*& stk, void* bp, size_t offsetToBasePointer)
{
    if (((size_t)stk) - offsetToBasePointer * sizeof(Type) < ((size_t)bp)) {
        ASSERT_NOT_REACHED();
    }
    stk = (void *)(((size_t)stk) - offsetToBasePointer * sizeof(Type));
}

#define PUSH(stk, topOfStack, ptr) push<ESValue>(stk, topOfStack, ptr)
#define POP(stk, bp) pop<ESValue>(stk, bp)
#define PEEK(stk, bp) peek<ESValue>(stk, bp)
#define SUB_STACK(stk, bp, offsetToBasePointer) sub<ESValue>(stk, bp, offsetToBasePointer)

#endif




template <typename CodeType>
ALWAYS_INLINE void executeNextCode(size_t& programCounter)
{
    programCounter += sizeof(CodeType);
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
void dumpUnsupported(CodeBlock* codeBlock);
#endif

#ifdef ENABLE_ESJIT
ESValue interpret(ESVMInstance* instance, CodeBlock* codeBlock, size_t programCounter = 0, unsigned maxStackPos = 0);
#else
ESValue interpret(ESVMInstance* instance, CodeBlock* codeBlock, size_t programCounter = 0, ESValue* stackStorage = NULL, ESValueVector* heapStorage = NULL);
#endif
CodeBlock* generateByteCode(ProgramNode* node, CodeBlock::ExecutableType type, bool shouldGenereateBytecodeInstantly = true);
inline void iterateByteCode(CodeBlock* codeBlock, std::function<void(CodeBlock* block, unsigned idx, ByteCode* code, Opcode opcode)> fn);

}

#include "ast/Node.h"
namespace escargot {

template <typename CodeType>
void CodeBlock::pushCode(const CodeType& code, ByteCodeGenerateContext& context, Node* node)
{
#ifndef NDEBUG
    {
        CodeType& t = const_cast<CodeType &>(code);
        t.m_node = node;
    }
#endif

    // record extra Info
    ByteCodeExtraData extraData;
    extraData.m_opcode = (Opcode)(size_t)code.m_opcodeInAddress;
#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
    pushCodeFillExtraData((ByteCode *)&code, &extraData, context, m_code.size(), m_extraData.size());
#else
    pushCodeFillExtraData((ByteCode *)&code, &extraData, context, m_code.size());
#endif

    const_cast<CodeType &>(code).assignOpcodeInAddress();

    char* first = (char *)&code;
    m_code.insert(m_code.end(), first, first + sizeof(CodeType));

#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
    m_extraData.push_back(extraData);
#endif
    m_requiredStackSizeInESValueSize = std::max(m_requiredStackSizeInESValueSize, (unsigned)context.m_baseRegisterCount);
}

inline void CodeBlock::pushCode(const ExecuteNativeFunction& code)
{
    const_cast<ExecuteNativeFunction &>(code).assignOpcodeInAddress();

    char* first = (char *)&code;
    m_code.insert(m_code.end(), first, first + sizeof(ExecuteNativeFunction));
}

ALWAYS_INLINE ByteCodeGenerateContext::~ByteCodeGenerateContext()
{
    ASSERT(m_breakStatementPositions.size() == 0);
    ASSERT(m_continueStatementPositions.size() == 0);
    ASSERT(m_labeledBreakStatmentPositions.size() == 0);
    ASSERT(m_labeledContinueStatmentPositions.size() == 0);
    ASSERT(m_complexCaseStatementPositions.size() == 0);
#ifdef ENABLE_ESJIT
    m_codeBlock->m_phiSize = m_phiIndex;
    if (m_isOutermostContext && m_codeBlock->m_dontJIT)
        m_codeBlock->removeJITInfo();
#endif
#ifndef NDEBUG
    if (m_isOutermostContext) {
        m_codeBlock->fillExtraData();
        if (ESVMInstance::currentInstance()->m_dumpByteCode) {
            char* code = m_codeBlock->m_code.data();
            ByteCode* currentCode = (ByteCode *)(&code[0]);
            if (currentCode->m_orgOpcode != ExecuteNativeFunctionOpcode) {
                dumpBytecode(m_codeBlock);
            }
        }
    }
#endif
}

#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
inline ExtraDataGenerateContext::ExtraDataGenerateContext(CodeBlock* codeBlock)
    : m_codeBlock(codeBlock)
    , m_baseRegisterCount(0)
#ifdef ENABLE_ESJIT
    , m_currentSSARegisterCount(0)
    , m_phiIndexToAllocIndexMapping(codeBlock->m_phiSize)
    , m_phiIndexToStoreIndexMapping(codeBlock->m_phiSize)
#endif
{
}

inline ExtraDataGenerateContext::~ExtraDataGenerateContext()
{
#ifdef ENABLE_ESJIT
    m_codeBlock->m_tempRegisterSize = m_currentSSARegisterCount;
#endif
}
#endif

ALWAYS_INLINE void ByteCodeGenerateContext::consumeLabeledContinuePositions(CodeBlock* cb, size_t position, ESString* lbl)
{
    for (size_t i = 0; i < m_labeledContinueStatmentPositions.size(); i ++) {
        if (*m_labeledContinueStatmentPositions[i].first == *lbl) {
            Jump* shouldBeJump = cb->peekCode<Jump>(m_labeledContinueStatmentPositions[i].second);
            ASSERT(shouldBeJump->m_orgOpcode == JumpOpcode);
            shouldBeJump->m_jumpPosition = position;
            morphJumpPositionIntoComplexCase(cb, m_labeledContinueStatmentPositions[i].second);
            m_labeledContinueStatmentPositions.erase(m_labeledContinueStatmentPositions.begin() + i);
            i = -1;
        }
    }
}

ALWAYS_INLINE void ByteCodeGenerateContext::consumeBreakPositions(CodeBlock* cb, size_t position)
{
    for (size_t i = 0; i < m_breakStatementPositions.size(); i ++) {
        Jump* shouldBeJump = cb->peekCode<Jump>(m_breakStatementPositions[i]);
        ASSERT(shouldBeJump->m_orgOpcode == JumpOpcode);
        shouldBeJump->m_jumpPosition = position;

        morphJumpPositionIntoComplexCase(cb, m_breakStatementPositions[i]);
    }
    m_breakStatementPositions.clear();
}

ALWAYS_INLINE void ByteCodeGenerateContext::consumeLabeledBreakPositions(CodeBlock* cb, size_t position, ESString* lbl)
{
    for (size_t i = 0; i < m_labeledBreakStatmentPositions.size(); i ++) {
        if (*m_labeledBreakStatmentPositions[i].first == *lbl) {
            Jump* shouldBeJump = cb->peekCode<Jump>(m_labeledBreakStatmentPositions[i].second);
            ASSERT(shouldBeJump->m_orgOpcode == JumpOpcode);
            shouldBeJump->m_jumpPosition = position;
            morphJumpPositionIntoComplexCase(cb, m_labeledBreakStatmentPositions[i].second);
            m_labeledBreakStatmentPositions.erase(m_labeledBreakStatmentPositions.begin() + i);
            i = -1;
        }
    }
}

ALWAYS_INLINE void ByteCodeGenerateContext::consumeContinuePositions(CodeBlock* cb, size_t position)
{
    for (size_t i = 0; i < m_continueStatementPositions.size(); i ++) {
        Jump* shouldBeJump = cb->peekCode<Jump>(m_continueStatementPositions[i]);
        ASSERT(shouldBeJump->m_orgOpcode == JumpOpcode);
        shouldBeJump->m_jumpPosition = position;

        morphJumpPositionIntoComplexCase(cb, m_continueStatementPositions[i]);
    }
    m_continueStatementPositions.clear();
}

ALWAYS_INLINE void ByteCodeGenerateContext::morphJumpPositionIntoComplexCase(CodeBlock* cb, size_t codePos)
{
    auto iter = m_complexCaseStatementPositions.find(codePos);
    if (iter != m_complexCaseStatementPositions.end()) {
        JumpComplexCase j(cb->peekCode<Jump>(codePos), iter->second);
        j.assignOpcodeInAddress();
        memcpy(cb->m_code.data() + codePos, &j, sizeof(JumpComplexCase));
#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
        iterateByteCode(cb, [codePos](CodeBlock* block, unsigned idx, ByteCode* code, Opcode opcode) {
            if (codePos == (size_t)((char*)code - block->m_code.data())) {
                block->m_extraData[idx].m_opcode = JumpComplexCaseOpcode;
            }
        });
#endif
        m_complexCaseStatementPositions.erase(iter);
    }
}

inline void iterateByteCode(CodeBlock* codeBlock, std::function<void(CodeBlock* block, unsigned idx, ByteCode* code, Opcode opcode)> fn)
{
    char* ptr = codeBlock->m_code.data();
    unsigned idx = 0;
    char* end = &codeBlock->m_code.data()[codeBlock->m_code.size()];
    while (ptr < end) {
#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
        Opcode code = codeBlock->m_extraData[idx].m_opcode;
#else
        Opcode code = (escargot::Opcode)ESVMInstance::currentInstance()->opcodeResverseTable()[((ByteCode *)ptr)->m_opcodeInAddress];
#endif
        fn(codeBlock, idx, (ByteCode *)ptr, code);
        idx++;
        switch (code) {
#define ADD_BYTECODE_SIZE(name, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
        case name##Opcode: \
            ptr += sizeof(name); \
            break;
            FOR_EACH_BYTECODE_OP(ADD_BYTECODE_SIZE)
        default:
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
}

}

#endif
