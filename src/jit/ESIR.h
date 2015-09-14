#ifndef ESIR_h
#define ESIR_h

#include <iostream>

namespace escargot {
namespace ESJIT {

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
    F(OSRExit) \
    F(Phi) \
    \
    /* For-in statement */ \
    F(GetEnumarablePropertyLength) \
    F(GetEnumerablePropertyNames) \
    \
    /* [Get/Set][Variable|Property] */ \
    F(GetThis) \
    F(GetVar) \
    F(SetVar) \
    F(GetScoped) \
    F(SetScoped) \
    F(GetProperty) \
    F(SetProperty) \
    \
    /* TODO: ArrayExpression Throw [Var|Fn][Decl|Expr] */ \

#define FOR_EACH_ESIR_TYPE(F) \
    F(Int32) \
    F(Boolean) \
    F(Number) \
    F(Null) \
    F(Undefined) \
    F(Empty) \
    F(Deleted) \
    F(String) \
    F(RopeString) \
    F(Object) \
    F(FunctionObject) \
    F(ArrayObject) \
    F(StringObject) \
    F(ErrorObject) \
    F(DateObject) \
    F(NumberObject) \
    F(BooleanObject) \
    F(Top) \

typedef enum {
    #define DECLARE_TYPE(name) name,
    FOR_EACH_ESIR_TYPE(DECLARE_TYPE)
    #undef DECLARE_TYPE
    Last
} Type;

class ESIR : public gc {
public:
    typedef enum {
        #define DECLARE_IR(name) name,
        FOR_EACH_ESIR_OP(DECLARE_IR)
        #undef DECLARE_IR
        Last
    } Opcode;
    ESIR(Opcode opcode, unsigned target) : m_opcode(opcode), m_target(target) { }
    Opcode opcode() { return m_opcode; }
    template<typename T>
    T* as() { return static_cast<T*>(this); }

#ifndef NDEBUG
    void dump(std::ostream& out);
#endif

private:
    Opcode m_opcode;
    unsigned m_target;
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


class ConstantIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(Constant, ESValue)

    static ConstantIR* getMask(Type type)
    {
        if (type == Type::Int32)
            return s_int32Mask;
        else
            RELEASE_ASSERT_NOT_REACHED();
    }

    static ConstantIR* s_undefined;
    static ConstantIR* s_int32Mask;
    static ConstantIR* s_zero;

private:
    ConstantIR(int target, ESValue value)
        : ESIR(ESIR::Opcode::Constant, target), m_value(value) { }
    ESValue m_value;
};

#if 0
class GetVarIR : public ESIR {
public:
    static GetVarIR* create(int varIndex) { return new GetVarIR(varIndex); }

private:
    GetVarIR(int target, int varIndex)
        : ESIR(ESIR::Opcode::GetVar, target), m_varIndex(varIndex) { }
    int m_varIndex;
};

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

class BranchIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_0(Branch);

private:
    BranchIR() : ESIR(ESIR::Opcode::Branch) { }
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
