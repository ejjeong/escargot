#ifndef ESIR_h
#define ESIR_h

#include <iostream>
#include "ESIRType.h"

namespace escargot {
namespace ESJIT {

class NativeGenerator;
class ESBasicBlock;

#define FOR_EACH_ESIR_OP(F) \
    /* Typed constant variables */ \
    F(Constant) \
    F(ConstantInt) \
    F(ConstantDouble) \
    F(ConstantTrue) \
    F(ConstantFalse) \
    \
    /* Type conversions */ \
    F(ToNumber) \
    F(ToString) \
    F(ToInt32) \
    F(ToBoolean) \
    \
    /* From BinaryExpression */ \
    F(StringPlus) \
    F(NumberPlus) \
    F(GenericPlus) \
    F(Minus) \
    F(Multiply) \
    F(Division) \
    F(Mod) \
    \
    F(BitwiseAnd) \
    F(BitwiseOr) \
    F(BitwiseXor) \
    F(LogicalAnd) \
    F(LogicalOr) \
    \
    F(Equal) \
    F(NotEqual) \
    F(StrictEqual) \
    F(NotStrictEqual) \
    F(GreaterThan) \
    F(GreaterThanOrEqual) \
    F(LessThan) \
    F(LessThanOrEqual) \
    \
    F(LeftShift) \
    F(UnsignedRightShift) \
    F(SignedRightShift) \
    \
    F(In) \
    F(InstanceOf) \
    \
    /* From UnaryExpression */ \
    F(BitwiseNot) \
    F(LogicalNot) \
    \
    F(UnaryPlus) \
    F(UnaryMinus) \
    \
    F(Delete) \
    F(TypeOf) \
    F(Void) \
    \
    /* Control Flow */ \
    F(LoopStart) \
    F(Jump) \
    F(Branch) \
    F(CallJS) \
    F(CallEval) \
    F(CallNative) \
    F(CallRuntime) \
    F(Return) \
    F(ReturnWithValue) \
    F(OSRExit) \
    F(Phi) \
    \
    /* For-in statement */ \
    F(GetEnumarablePropertyLength) \
    F(GetEnumerablePropertyNames) \
    \
    /* [Get/Set][Variable|Property] */ \
    F(GetThis) \
    F(GetArgument) \
    F(GetVar) \
    F(SetVar) \
    F(GetScoped) \
    F(SetScoped) \
    F(GetProperty) \
    F(SetProperty) \
    \
    /* TODO: ArrayExpression Throw [Var|Fn][Decl|Expr] */ \

// FIXME find a better allocator
class ESIR : public gc {
    friend class NativeGenerator;
public:
    typedef enum {
        #define DECLARE_IR(name) name,
        FOR_EACH_ESIR_OP(DECLARE_IR)
        #undef DECLARE_IR
        Last
    } Opcode;
    ESIR(Opcode opcode, int target) : m_opcode(opcode), m_targetIndex(target) { }
    Opcode opcode() { return m_opcode; }
    template<typename T>
    T* as() { return static_cast<T*>(this); }

    const char* getOpcodeName();
#ifndef NDEBUG
    virtual void dump(std::ostream& out);
#endif

protected:
    Opcode m_opcode;
    int m_targetIndex;
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

class GetArgumentIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(GetArgument, size_t)

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
    GetArgumentIR(int target, int argIndex)
        : ESIR(ESIR::Opcode::GetArgument, target), m_argIndex(argIndex) { }
    int m_argIndex;
};

class GetVarIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(GetVar, size_t)

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " var" << m_varIndex;
    }
#endif

private:
    GetVarIR(int targetIndex, int varIndex)
        : ESIR(ESIR::Opcode::GetVar, targetIndex), m_varIndex(varIndex) { }
    int m_varIndex;
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

class LessThanIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(LessThan, int, int)

private:
    LessThanIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::LessThan, targetIndex, leftIndex, rightIndex) { }
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

#if 0
class SetVar : public ESIR {
public:
    static SetVar* create(int varIndex, ESIR* setValue) { return new SetVar(varIndex, setValue); }

private:
    SetVar(int varIndex, ESIR* setValue)
        : ESIR(ESIR::Opcode::SetVar), m_varIndex(varIndex), m_setValue(setValue) { }
    int m_varIndex;
    ESIR* m_setValue;
};

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

class GenericPlusIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_2(GenericPlus, ESIR*, ESIR*);

private:
    GenericPlusIR(ESIR* left, ESIR* right)
        : ESIR(ESIR::Opcode::GenericPlus), m_left(left), m_right(right) { }
    ESIR* m_left;
    ESIR* m_right;
};

#undef DECLARE_STATIC_GENERATOR_2
#endif

}}
#endif
