#ifndef ESIR_h
#define ESIR_h

#ifdef ENABLE_ESJIT

#include <iostream>
#include "ESIRType.h"

namespace escargot {

class ByteCode;

namespace ESJIT {

class NativeGenerator;
class ESBasicBlock;

#define FOR_EACH_ESIR_OP(F) \
    /* Typed constant variables */ \
    F(Constant, ) \
    F(ConstantInt, ) \
    F(ConstantDouble, ) \
    F(ConstantPointer, ) \
    F(ConstantBoolean, ) \
    F(ConstantString, ) \
    \
    /* Type conversions */ \
    F(ToNumber, ) \
    F(ToString, ) \
    F(ToInt32, ) \
    F(ToBoolean, ) \
    \
    /* From BinaryExpression */ \
    F(Int32Plus, ) \
    F(DoublePlus, ) \
    F(StringPlus, ) \
    F(GenericPlus, ) \
    F(Increment, ) \
    F(Minus, ) \
    F(Int32Multiply, ) \
    F(DoubleMultiply, ) \
    F(GenericMultiply, ) \
    F(DoubleDivision, ) \
    F(GenericDivision, ) \
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
    F(Move, ) \
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
    F(GetObject, LoadFromHeap)\
    F(SetObject, ) \
    F(GetObjectPreComputed, LoadFromHeap) \
    F(GetArrayObject, LoadFromHeap) \
    F(SetArrayObject, ) \
    F(GetArrayObjectPreComputed, LoadFromHeap) \
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

class ConstantBooleanIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(ConstantBoolean, bool);

    double value() { return m_value; }

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " const boolean " << std::boolalpha << m_value;
    }
#endif

private:
    ConstantBooleanIR(int target, bool value)
        : ESIR(ESIR::Opcode::ConstantBoolean, target), m_value(value) { }
    bool m_value;
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

class ConstantStringIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(ConstantString, ESString*);

    ESString* value() { return m_value; }

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " const ESString \"" << m_value->utf8Data() << "\"";
    }
#endif

private:
    ConstantStringIR(int target, ESString* value)
        : ESIR(ESIR::Opcode::ConstantString, target), m_value(value) { }
    ESString* m_value;
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
    DECLARE_STATIC_GENERATOR_3(GetVarGeneric, ByteCode*, const InternalAtomicString&, ESString*)


#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);

        out << " " << m_esName->utf8Data();
    }
#endif
    ByteCode* originalGetByIdByteCode() { return m_originalGetByIdByteCode; }
    InternalAtomicString* name() { return &m_name; }
    ESString* nonAtomicName() { return m_esName; }

private:
    GetVarGenericIR(int targetIndex, ByteCode* originalGetByIdByteCode, const InternalAtomicString& name, ESString* esName)
        : ESIR(ESIR::Opcode::GetVarGeneric, targetIndex), m_originalGetByIdByteCode(originalGetByIdByteCode), m_name(name), m_esName(esName) { }
    ByteCode* m_originalGetByIdByteCode;
    InternalAtomicString m_name;
    ESString* m_esName;
};

class SetVarGenericIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_4(SetVarGeneric, ByteCode*, int, InternalAtomicString*, ESString*);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " " << m_esName->utf8Data() << " = tmp" << m_sourceIndex;
    }
#endif

    ByteCode* originalPutByIdByteCode() { return m_originalPutByIdByteCode; }
    int sourceIndex() { return m_sourceIndex; }
    InternalAtomicString* name() { return m_name; }
    ESString* nonAtomicName() { return m_esName; }

private:
    SetVarGenericIR(int targetIndex, ByteCode* originalPutByIdByteCode, int sourceIndex, InternalAtomicString* name, ESString* esName)
        : ESIR(ESIR::Opcode::SetVarGeneric, targetIndex), m_originalPutByIdByteCode(originalPutByIdByteCode), m_sourceIndex(sourceIndex), m_name(name), m_esName(esName) { }
    ByteCode* m_originalPutByIdByteCode;
    int m_sourceIndex;
    InternalAtomicString* m_name;
    ESString* m_esName;
};

class GetObjectIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_2(GetObject, int, int);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " tmp" << m_objectIndex << "[tmp" << m_propertyIndex << "]";
    }
#endif

    ESHiddenClass* cachedHiddenClass() { return m_cachedHiddenClass; }
    size_t cachedIndex() { return m_cachedIndex; }
    int objectIndex() { return m_objectIndex; }
    int propertyIndex() { return m_propertyIndex; }

private:
    GetObjectIR(int targetIndex, int objectIndex, int propertyIndex)
        : ESIR(ESIR::Opcode::GetObject, targetIndex),
          m_objectIndex(objectIndex),
          m_propertyIndex(propertyIndex){ }
    ESHiddenClass* m_cachedHiddenClass;
    int m_cachedIndex;
    int m_objectIndex;
    int m_propertyIndex;
};

class GetObjectPreComputedIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_3(GetObjectPreComputed, size_t, int, ESValue);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " tmp" << m_objectIndex << "." << m_propertyValue.asESString()->utf8Data();
    }
#endif

    size_t cachedIndex() { return m_cachedIndex; }
    int objectIndex() { return m_objectIndex; }
    ESValue propertyValue() { return m_propertyValue; }

private:
    GetObjectPreComputedIR(int targetIndex, size_t cachedIndex, int objectIndex, ESValue propertyValue)
        : ESIR(ESIR::Opcode::GetObjectPreComputed, targetIndex),
          m_cachedIndex(cachedIndex),
          m_objectIndex(objectIndex),
          m_propertyValue(propertyValue) { }

    int m_cachedIndex;
    int m_objectIndex;
    ESValue m_propertyValue;
};

class GetArrayObjectIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_2(GetArrayObject, int, int);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " tmp" << m_objectIndex << "[tmp" << m_propertyIndex << "]";
    }
#endif

    int objectIndex() { return m_objectIndex; }
    int propertyIndex() { return m_propertyIndex; }

private:
    GetArrayObjectIR(int targetIndex, int objectIndex, int propertyIndex)
        : ESIR(ESIR::Opcode::GetArrayObject, targetIndex),
          m_objectIndex(objectIndex),
          m_propertyIndex(propertyIndex){ }
    int m_objectIndex;
    int m_propertyIndex;
};

class SetArrayObjectIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_3(SetArrayObject, int, int, int);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " tmp" << m_objectIndex << "[tmp" << m_propertyIndex << "] = " << m_sourceIndex;
    }
#endif

    int objectIndex() { return m_objectIndex; }
    int propertyIndex() { return m_propertyIndex; }
    int sourceIndex() { return m_sourceIndex; }

private:
    SetArrayObjectIR(int targetIndex, int objectIndex, int propertyIndex, int sourceIndex)
        : ESIR(ESIR::Opcode::SetArrayObject, targetIndex),
          m_objectIndex(objectIndex),
          m_propertyIndex(propertyIndex),
          m_sourceIndex(sourceIndex) { }
    int m_objectIndex;
    int m_propertyIndex;
    int m_sourceIndex;
};

class GetArrayObjectPreComputedIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_2(GetArrayObjectPreComputed, int, int);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " tmp" << m_objectIndex << "[" << m_computedIndex << "]";
    }
#endif

    int objectIndex() { return m_objectIndex; }
    int computedIndex() { return m_computedIndex; }

private:
    GetArrayObjectPreComputedIR(int targetIndex, int objectIndex, int computedIndex)
        : ESIR(ESIR::Opcode::GetArrayObjectPreComputed, targetIndex),
          m_objectIndex(objectIndex),
          m_computedIndex(computedIndex){ }
    int m_objectIndex;
    int m_computedIndex;
};

class SetObjectIR : public ESIR {
public:
    //DECLARE_STATIC_GENERATOR_3(SetObject, ESHiddenClass*, size_t, int);
    DECLARE_STATIC_GENERATOR_3(SetObject, int, int, int);

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
    SetObjectIR(int targetIndex, int objectIndex, int propertyIndex, int sourceIndex)
        : ESIR(ESIR::Opcode::SetObject, targetIndex),
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

class Int32PlusIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(Int32Plus, int, int);

private:
    Int32PlusIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::Int32Plus, targetIndex, leftIndex, rightIndex) { }
};

class DoublePlusIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(DoublePlus, int, int);

private:
    DoublePlusIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::DoublePlus, targetIndex, leftIndex, rightIndex) { }
};

class StringPlusIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(StringPlus, int, int);

private:
    StringPlusIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::StringPlus, targetIndex, leftIndex, rightIndex) { }
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

class Int32MultiplyIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(Int32Multiply, int, int);

private:
    Int32MultiplyIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::Int32Multiply, targetIndex, leftIndex, rightIndex) { }
};

class DoubleMultiplyIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(DoubleMultiply, int, int);

private:
    DoubleMultiplyIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::DoubleMultiply, targetIndex, leftIndex, rightIndex) { }
};

class GenericMultiplyIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(GenericMultiply, int, int);

private:
    GenericMultiplyIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::GenericMultiply, targetIndex, leftIndex, rightIndex) { }
};

class DoubleDivisionIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(DoubleDivision, int, int);

private:
    DoubleDivisionIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::DoubleDivision, targetIndex, leftIndex, rightIndex) { }
};

class GenericDivisionIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(GenericDivision, int, int);

private:
    GenericDivisionIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::GenericDivision, targetIndex, leftIndex, rightIndex) { }
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

class UnaryMinusIR : public UnaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_1(UnaryMinus, int);

private:
    UnaryMinusIR(int targetIndex, int sourceIndex)
        : UnaryExpressionIR(ESIR::Opcode::UnaryMinus, targetIndex, sourceIndex) { }
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

class MoveIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(Move, int);

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
    MoveIR(int targetIndex, int sourceIndex)
        : ESIR(ESIR::Opcode::Move, targetIndex), m_sourceIndex(sourceIndex) { }

    int m_sourceIndex;
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
