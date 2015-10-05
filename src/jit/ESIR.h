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
    F(GetScoped, LoadFromHeap) \
    F(SetScoped, ) \
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
        out << " const " << m_value;
    }
#endif

private:
    ConstantIntIR(int target, int32_t value)
        : ESIR(ESIR::Opcode::ConstantInt, target), m_value(value) { }
    int32_t m_value;
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

class BitwiseAndIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(BitwiseAnd, int, int);

private:
    BitwiseAndIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::BitwiseAnd, targetIndex, leftIndex, rightIndex) { }
};

class LessThanIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(LessThan, int, int);

private:
    LessThanIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::LessThan, targetIndex, leftIndex, rightIndex) { }
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