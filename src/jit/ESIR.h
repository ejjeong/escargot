#ifndef ESIR_h
#define ESIR_h

#ifdef ENABLE_ESJIT

#include <iostream>
#include "ESIRType.h"

namespace escargot {
namespace ESJIT {

class NativeGenerator;
class ESBasicBlock;

#define FOR_EACH_ESIR_OP(F) \
    /* Typed constant variables */ \
    F(Constant, ) \
    F(ConstantInt, ) \
    F(ConstantDouble, ) \
    F(ConstantPointer, ) \
    F(ConstantTrue, ) \
    F(ConstantFalse, ) \
    \
    /* Type conversions */ \
    F(ToNumber, ) \
    F(ToString, ) \
    F(ToInt32, ) \
    F(ToBoolean, ) \
    \
    /* From BinaryExpression */ \
    F(StringPlus, ) \
    F(NumberPlus, ) \
    F(GenericPlus, ) \
    F(Increment, ) \
    F(Minus, ) \
    F(Multiply, ) \
    F(Division, ) \
    F(Mod, ) \
    \
    F(BitwiseAnd, ) \
    F(BitwiseOr, ) \
    F(BitwiseXor, ) \
    F(LogicalAnd, ) \
    F(LogicalOr, ) \
    \
    F(Equal, ) \
    F(NotEqual, ) \
    F(StrictEqual, ) \
    F(NotStrictEqual, ) \
    F(GreaterThan, ) \
    F(GreaterThanOrEqual, ) \
    F(LessThan, ) \
    F(LessThanOrEqual, ) \
    \
    F(LeftShift, ) \
    F(UnsignedRightShift, ) \
    F(SignedRightShift, ) \
    \
    F(In, ) \
    F(InstanceOf, ) \
    \
    /* From UnaryExpression */ \
    F(BitwiseNot, ) \
    F(LogicalNot, ) \
    \
    F(UnaryPlus, ) \
    F(UnaryMinus, ) \
    \
    F(Delete, ) \
    F(TypeOf, ) \
    F(Void, ) \
    \
    /* Control Flow */ \
    F(LoopStart, ) \
    F(Jump, ) \
    F(Branch, ) \
    F(CallJS, LoadFromHeap) \
    F(CallEval, LoadFromHeap) \
    F(CallNative, LoadFromHeap) \
    F(CallRuntime, LoadFromHeap) \
    F(Return, ) \
    F(ReturnWithValue, ) \
    F(OSRExit, ) \
    F(Phi, ) \
    \
    /* For-in statement */ \
    F(GetEnumarablePropertyLength, ) \
    F(GetEnumerablePropertyNames, ) \
    \
    /* [Get/Set][Variable|Property] */ \
    F(GetThis, ) \
    F(GetArgument, LoadFromHeap) \
    F(GetVar, LoadFromHeap) \
    F(SetVar, ) \
    F(PutInObject, ) \
    F(GetScoped, LoadFromHeap) \
    F(SetScoped, ) \
    F(GetVarGeneric, LoadFromHeap) \
    F(SetVarGeneric, ) \
    F(GetProperty, LoadFromHeap) \
    F(SetProperty, ) \
    \
    /* TODO: ArrayExpression Throw [Var|Fn][Decl|Expr] */ \

#define FOR_EACH_ESIR_FLAGS(F) \
    F(LoadFromHeap, 0) \

#define DECLARE_ESIR_FLAGS(flag, shift) \
const uint32_t flag = 0x1 << shift;
FOR_EACH_ESIR_FLAGS(DECLARE_ESIR_FLAGS)
#undef DECLARE_ESIR_FLAGS

// FIXME find a better allocator
class ESIR : public gc {
    friend class NativeGenerator;
public:
    typedef enum {
        #define DECLARE_IR(name, unused) name,
        FOR_EACH_ESIR_OP(DECLARE_IR)
        #undef DECLARE_IR
        Last
    } Opcode;
    ESIR(Opcode opcode, int target)
        : m_opcode(opcode), m_targetIndex(target), m_reconstructedType(TypeBottom) { }
    Opcode opcode() { return m_opcode; }
    template<typename T>
    T* as() { return static_cast<T*>(this); }

    int targetIndex() { return m_targetIndex; }

    const char* getOpcodeName();
    uint32_t getFlags();
    bool isValueLoadedFromHeap();
#ifndef NDEBUG
    virtual void dump(std::ostream& out);
#endif

protected:
    Opcode m_opcode;
    int m_targetIndex;
    Type m_reconstructedType;
};

#define DECLARE_STATIC_GENERATOR_0(opcode) \
    static opcode##IR* create(unsigned target) { \
        return new opcode##IR(target); \
    }

#define DECLARE_STATIC_GENERATOR_1(opcode, Type1) \
    static opcode##IR* create(unsigned target, Type1 operand1) { \
        return new opcode##IR(target, operand1); \
    }

#define DECLARE_STATIC_GENERATOR_2(opcode, Type1, Type2) \
    static opcode##IR* create(unsigned target, Type1 operand1, Type2 operand2) { \
        return new opcode##IR(target, operand1, operand2); \
    }

#define DECLARE_STATIC_GENERATOR_3(opcode, Type1, Type2, Type3) \
    static opcode##IR* create(unsigned target, Type1 operand1, Type2 operand2, Type3 operand3) { \
        return new opcode##IR(target, operand1, operand2, operand3); \
    }

#define DECLARE_STATIC_GENERATOR_4(opcode, Type1, Type2, Type3, Type4) \
    static opcode##IR* create(unsigned target, Type1 operand1, Type2 operand2, Type3 operand3, Type4 operand4) { \
        return new opcode##IR(target, operand1, operand2, operand3, operand4); \
    }



class ConstantIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(Constant, ESValue)

    static ConstantIR* getMask(Type type)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    static ConstantIR* s_undefined;
    static ConstantIR* s_int32Mask;
    static ConstantIR* s_zero;

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " const " << m_value.toString()->utf8Data();
    }
#endif

private:
    ConstantIR(int target, ESValue value)
        : ESIR(ESIR::Opcode::Constant, target), m_value(value) { }
    ESValue m_value;
};

class ConstantIntIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(ConstantInt, int32_t);

    int32_t value() { return m_value; }

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " const int " << m_value;
    }
#endif

private:
    ConstantIntIR(int target, int32_t value)
        : ESIR(ESIR::Opcode::ConstantInt, target), m_value(value) { }
    int32_t m_value;
};

class ConstantDoubleIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(ConstantDouble, double);

    double value() { return m_value; }

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " const double " << m_value;
    }
#endif

private:
    ConstantDoubleIR(int target, double value)
        : ESIR(ESIR::Opcode::ConstantDouble, target), m_value(value) { }
    double m_value;
};

class ConstantPointerIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(ConstantPointer, void*);

    void* value() { return m_value; }

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " const pointer " << std::hex << bitwise_cast<uint64_t>(m_value) << std::dec;
    }
#endif

private:
    ConstantPointerIR(int target, void* value)
        : ESIR(ESIR::Opcode::ConstantPointer, target), m_value(value) { }
    void* m_value;
};


class ToNumberIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(ToNumber, int);

    int sourceIndex() { return m_sourceIndex; }

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " tmp" << m_sourceIndex;
    }
#endif

private:
    ToNumberIR(int targetIndex, int sourceIndex)
        : ESIR(ESIR::Opcode::ToNumber, targetIndex), m_sourceIndex(sourceIndex) { }
    int m_sourceIndex;
};

class GetArgumentIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(GetArgument, int)

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " arg" << m_argIndex;
    }
#endif

    int argumentIndex() { return m_argIndex; }

private:
    GetArgumentIR(int targetIndex, int argIndex)
        : ESIR(ESIR::Opcode::GetArgument, targetIndex), m_argIndex(argIndex) { }
    int m_argIndex;
};

class GetVarIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(GetVar, int)

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " var" << m_varIndex;
    }
#endif

    int varIndex() { return m_varIndex; }

private:
    GetVarIR(int targetIndex, int varIndex)
        : ESIR(ESIR::Opcode::GetVar, targetIndex), m_varIndex(varIndex) { }
    int m_varIndex;
};

class SetVarIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_2(SetVar, int, int);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " var" << m_localVarIndex << " = tmp" << m_sourceIndex;
    }
#endif

    int localVarIndex() { return m_localVarIndex; }
    int sourceIndex() { return m_sourceIndex; }

private:
    SetVarIR(int targetIndex, int localVarIndex, int sourceIndex)
        : ESIR(ESIR::Opcode::SetVar, targetIndex), m_localVarIndex(localVarIndex), m_sourceIndex(sourceIndex) { }
    int m_localVarIndex;
    int m_sourceIndex;
};

class GetVarGenericIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_2(GetVarGeneric, const InternalAtomicString&, ESString*)


#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);

        out << " " << m_esName->utf8Data();
    }
#endif
    InternalAtomicString* name() { return &m_name; }
    ESString* nonAtomicName() { return m_esName; }

private:
    GetVarGenericIR(int targetIndex, const InternalAtomicString& name, ESString* esName)
        : ESIR(ESIR::Opcode::GetVarGeneric, targetIndex), m_name(name), m_esName(esName) { }
    InternalAtomicString m_name;
    ESString* m_esName;
};

class SetVarGenericIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_3(SetVarGeneric, int, InternalAtomicString*, ESString*);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " " << m_esName->utf8Data() << " = tmp" << m_sourceIndex;
    }
#endif

    int sourceIndex() { return m_sourceIndex; }
    InternalAtomicString* name() { return m_name; }
    ESString* nonAtomicName() { return m_esName; }

private:
    SetVarGenericIR(int targetIndex, int sourceIndex, InternalAtomicString* name, ESString* esName)
        : ESIR(ESIR::Opcode::SetVarGeneric, targetIndex), m_sourceIndex(sourceIndex), m_name(name), m_esName(esName) { }
    int m_sourceIndex;
    InternalAtomicString* m_name;
    ESString* m_esName;
};

class PutInObjectIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_3(PutInObject, ESHiddenClass*, size_t, int);
    DECLARE_STATIC_GENERATOR_3(PutInObject, int, int, int);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " tmp" << m_objectIndex << "[tmp" << m_propertyIndex << "] = " << "tmp" << m_sourceIndex;
    }
#endif

    ESHiddenClass* cachedHiddenClass() { return m_cachedHiddenClass; }
    size_t cachedIndex() { return m_cachedIndex; }
    int objectIndex() { return m_objectIndex; }
    int propertyIndex() { return m_propertyIndex; }
    int sourceIndex() { return m_sourceIndex; }

private:
    PutInObjectIR(int targetIndex, ESHiddenClass* cachedHiddenClass, size_t cachedIndex, int sourceIndex)
        : ESIR(ESIR::Opcode::PutInObject, targetIndex),
          m_cachedHiddenClass(cachedHiddenClass),
          m_cachedIndex(cachedIndex),
          m_sourceIndex(sourceIndex) { }
    PutInObjectIR(int targetIndex, int objectIndex, int propertyIndex, int sourceIndex)
        : ESIR(ESIR::Opcode::PutInObject, targetIndex),
          m_objectIndex(objectIndex),
          m_propertyIndex(propertyIndex),
          m_sourceIndex(sourceIndex) { }
    ESHiddenClass* m_cachedHiddenClass;
    int m_cachedIndex;
    int m_objectIndex;
    int m_propertyIndex;
    int m_sourceIndex;
};

class BinaryExpressionIR : public ESIR {
public:
#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " tmp" << m_leftIndex << " (op) tmp" << m_rightIndex;
    }
#endif

    int leftIndex() { return m_leftIndex; }
    int rightIndex() { return m_rightIndex; }

protected:
    BinaryExpressionIR(ESIR::Opcode opcode, int targetIndex, int leftIndex, int rightIndex)
        : ESIR(opcode, targetIndex), m_leftIndex(leftIndex), m_rightIndex(rightIndex) { }
    int m_leftIndex;
    int m_rightIndex;
};

class GenericPlusIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(GenericPlus, int, int);

private:
    GenericPlusIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::GenericPlus, targetIndex, leftIndex, rightIndex) { }
};

class MinusIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(Minus, int, int);

private:
    MinusIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::Minus, targetIndex, leftIndex, rightIndex) { }
};

class BitwiseAndIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(BitwiseAnd, int, int);

private:
    BitwiseAndIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::BitwiseAnd, targetIndex, leftIndex, rightIndex) { }
};

class EqualIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(Equal, int, int);

private:
    EqualIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::Equal, targetIndex, leftIndex, rightIndex) { }
};

class GreaterThanIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(GreaterThan, int, int);

private:
    GreaterThanIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::GreaterThan, targetIndex, leftIndex, rightIndex) { }
};

class GreaterThanOrEqualIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(GreaterThanOrEqual, int, int);

private:
    GreaterThanOrEqualIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::GreaterThanOrEqual, targetIndex, leftIndex, rightIndex) { }
};

class LessThanIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(LessThan, int, int);

private:
    LessThanIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::LessThan, targetIndex, leftIndex, rightIndex) { }
};

class LessThanOrEqualIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(LessThanOrEqual, int, int);

private:
    LessThanOrEqualIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::LessThanOrEqual, targetIndex, leftIndex, rightIndex) { }
};

class LeftShiftIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(LeftShift, int, int);

private:
    LeftShiftIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::LeftShift, targetIndex, leftIndex, rightIndex) { }
};

class SignedRightShiftIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(SignedRightShift, int, int);

private:
    SignedRightShiftIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::SignedRightShift, targetIndex, leftIndex, rightIndex) { }
};

class UnaryExpressionIR : public ESIR {
public:
#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " tmp" << m_sourceIndex;
    }
#endif

    int sourceIndex() { return m_sourceIndex; }

protected:
    UnaryExpressionIR(ESIR::Opcode opcode, int targetIndex, int sourceIndex)
        : ESIR(opcode, targetIndex), m_sourceIndex(sourceIndex) { }
    int m_sourceIndex;
};

class IncrementIR : public UnaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_1(Increment, int);

private:
    IncrementIR(int targetIndex, int sourceIndex)
        : UnaryExpressionIR(ESIR::Opcode::Increment, targetIndex, sourceIndex) { }
};

class ReturnWithValueIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(ReturnWithValue, int)

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << ": ";
        ESIR::dump(out);
        out << " tmp" << m_returnIndex;
    }
#endif

    int returnIndex() { return m_returnIndex; }

private:
    ReturnWithValueIR(int targetIndex, int returnIndex)
        : ESIR(ESIR::Opcode::ReturnWithValue, targetIndex), m_returnIndex(returnIndex) { }
    int m_returnIndex;
};

class JumpIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(Jump, ESBasicBlock*);

#ifndef NDEBUG
    virtual void dump(std::ostream& out);
#endif
    ESBasicBlock* targetBlock() { return m_targetBlock; }

private:
    JumpIR(int targetIndex, ESBasicBlock* targetBlock)
        : ESIR(ESIR::Opcode::Jump, targetIndex), m_targetBlock(targetBlock) { }
    ESBasicBlock* m_targetBlock;
};

class BranchIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_3(Branch, int, ESBasicBlock*, ESBasicBlock*);

#ifndef NDEBUG
    virtual void dump(std::ostream& out);
#endif

    int operandIndex() { return m_operandIndex; }
    ESBasicBlock* trueBlock() { return m_trueBlock; }
    ESBasicBlock* falseBlock() { return m_falseBlock; }

private:
    BranchIR(int targetIndex, int operandIndex, ESBasicBlock* trueBlock, ESBasicBlock* falseBlock)
        : ESIR(ESIR::Opcode::Branch, targetIndex), m_operandIndex(operandIndex)
          , m_trueBlock(trueBlock), m_falseBlock(falseBlock) { }
    int m_operandIndex;
    ESBasicBlock* m_trueBlock;
    ESBasicBlock* m_falseBlock;
};

class CallJSIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_4(CallJS, int, int, int, int*);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " callee tmp" << m_calleeIndex;
        out << " receiver tmp" << m_receiverIndex;
        out << " argumentCount " << m_argumentIndexes.size() << " :";
        for (size_t i=0; i<m_argumentIndexes.size(); i++)
            out << ", tmp" << m_argumentIndexes[i];
    }
#endif
    int calleeIndex() { return m_calleeIndex; }
    int receiverIndex() { return m_receiverIndex; }
    int argumentCount() { return m_argumentIndexes.size(); }
    int argumentIndex(size_t idx) { return m_argumentIndexes[idx]; }

private:
    CallJSIR(int targetIndex, int calleeIndex, int receiverIndex, int argumentCount, int* argumentIndexes)
        : ESIR(ESIR::Opcode::CallJS, targetIndex), m_calleeIndex(calleeIndex), m_receiverIndex(receiverIndex), m_argumentIndexes(argumentCount)
    {
        for (int i=0; i<argumentCount; i++)
            m_argumentIndexes[i] = argumentIndexes[i];
    }
    int m_calleeIndex;
    int m_receiverIndex;
    std::vector<int, gc_allocator<int> > m_argumentIndexes;
};

class ReturnIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_0(Return);

private:
    ReturnIR(int targetIndex)
        : ESIR(ESIR::Opcode::Return, targetIndex) { }
};

class PhiIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_0(Phi)

    void addIndex(int argumentIndex) { m_argumentIndexes.push_back(argumentIndex); }

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        for (size_t i=0; i<m_argumentIndexes.size(); i++)
            out << "var " << m_argumentIndexes[i] << ", ";
    }
#endif

private:
    PhiIR(int targetIndex)
        : ESIR(ESIR::Opcode::Phi, targetIndex) { }
    std::vector<int, gc_allocator<int> > m_argumentIndexes;
};

#if 0
class BitwiseAndIR : public ESIR {
public:
    static BitwiseAndIR* create(ESIR* left, ESIR* right) { return new BitwiseAndIR(left, right); }

private:
    BitwiseAndIR(ESIR* left, ESIR* right)
        : ESIR(ESIR::Opcode::BitwiseAnd), m_left(left), m_right(right) { }
    ESIR* m_left;
    ESIR* m_right;
};

class EqualIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_2(Equal, ESIR*, ESIR*);

private:
    EqualIR(ESIR* left, ESIR* right)
        : ESIR(ESIR::Opcode::Equal), m_left(left), m_right(right) { }
    ESIR* m_left;
    ESIR* m_right;
};

class ReturnIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(Return, ESIR*);

private:
    ReturnIR(ESIR* returnValue)
        : ESIR(ESIR::Opcode::Return), m_returnValue(returnValue) { }
    ESIR* m_returnValue;
};

class OSRExitIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(OSRExit, ESIR*);

private:
    OSRExitIR(ESIR* exitNumber)
        : ESIR(ESIR::Opcode::OSRExit), m_exitNumber(exitNumber) { }
    ESIR* m_exitNumber;
};

#endif

}}
#endif
#endif
