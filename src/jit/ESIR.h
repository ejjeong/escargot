#ifndef ESIR_h
#define ESIR_h

#ifdef ENABLE_ESJIT

#include "ESIRType.h"

#include "nanojit.h"

namespace nanojit {
class LIns;
}

namespace escargot {
class ByteCode;

namespace ESJIT {

class NativeGenerator;
class ESBasicBlock;

#define FOR_EACH_ESIR_OP(F) \
    /* Typed constant variables */ \
    F(ConstantESValue, ) \
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
    F(GenericPlus, ReturnsESValue) \
    F(Increment, ) \
    F(Decrement, ) \
    F(Minus, ) \
    F(Int32Multiply, ) \
    F(DoubleMultiply, ) \
    F(GenericMultiply, ) \
    F(DoubleDivision, ) \
    F(GenericDivision, ) \
    F(Int32Mod, ) \
    F(DoubleMod, ) \
    F(GenericMod, ) \
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
    F(CreateFunction, ReturnsESValue) \
    F(CallJS, ReturnsESValue) \
    F(CallNewJS, ReturnsESValue) \
    F(CallEval, ReturnsESValue) \
    F(CallNative, ReturnsESValue) \
    F(CallRuntime, ReturnsESValue) \
    F(Return, ) \
    F(ReturnWithValue, ) \
    F(OSRExit, ) \
    F(Move, ) \
    F(Phi, ) \
    F(AllocPhi, ) \
    F(StorePhi, ) \
    F(LoadPhi, ) \
    F(CreateObject, ReturnsESValue) \
    F(CreateArray, ReturnsESValue) \
    F(InitObject, ) \
    F(InitArrayObject, ) \
    \
    /* For-in statement */ \
    F(GetEnumerablObject, ) \
    F(CheckIfKeyIsLast, ) \
    F(Enumerate, ReturnsESValue) \
    \
    /* [Get/Set][Variable|Property] */ \
    F(GetThis, ReturnsESValue) \
    F(GetArgument, ReturnsESValue) \
    F(GetVar, ReturnsESValue) \
    F(SetVar, ) \
    F(GetObject, ReturnsESValue)\
    F(GetObjectPreComputed, ReturnsESValue) \
    F(SetObject, ) \
    F(SetObjectPreComputed, ) \
    F(GetArrayObject, ReturnsESValue) \
    F(GetStringByIndex, ReturnsESValue) \
    F(SetArrayObject, ) \
    F(GetScoped, ReturnsESValue) \
    F(SetScoped, ) \
    F(GetVarGeneric, ReturnsESValue) \
    F(SetVarGeneric, ) \
    F(GetVarGenericWithoutException, ReturnsESValue) \
    F(GetGlobalVarGeneric, ReturnsESValue) \
    F(SetGlobalVarGeneric, ) \
    F(GetProperty, ReturnsESValue) \
    F(SetProperty, ) \
    \
    F(Throw, ) \
    \
    /* TODO: ArrayExpression Throw [Var|Fn][Decl|Expr] */ \

#define FOR_EACH_ESIR_FLAGS(F) \
    F(ReturnsESValue, 0) \

#define DECLARE_ESIR_FLAGS(flag, shift) \
const uint32_t flag = 0x1 << shift;
FOR_EACH_ESIR_FLAGS(DECLARE_ESIR_FLAGS)
#undef DECLARE_ESIR_FLAGS

class ESIR;

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
    bool returnsESValue();
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



class ConstantESValueIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(ConstantESValue, double)

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " constant " << ESValue::fromRawDouble(m_value).toString()->utf8Data();
    }
#endif
    double value() { return m_value; }

private:
    ConstantESValueIR(int target, double value)
        : ESIR(ESIR::Opcode::ConstantESValue, target), m_value(value) { }
    double m_value;
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
class JumpIR;
class GetEnumerablObjectIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(GetEnumerablObject, int);

    int sourceIndex() { return m_sourceIndex; }
    void setJumpIR(JumpIR* jumpIR)
    {
        m_jumpIR = jumpIR;
    }
    JumpIR* getJumpIR()
    {
        return m_jumpIR;
    }
#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
    }
#endif

private:
    GetEnumerablObjectIR(int targetIndex, int sourceIndex)
        : ESIR(ESIR::Opcode::GetEnumerablObject, targetIndex), m_sourceIndex(sourceIndex), m_jumpIR(nullptr) { }
    int m_sourceIndex;
    JumpIR* m_jumpIR;
};

class CheckIfKeyIsLastIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(CheckIfKeyIsLast, int);

    int sourceIndex() { return m_sourceIndex; }
#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
    }
#endif

private:
    CheckIfKeyIsLastIR(int targetIndex, int sourceIndex)
        : ESIR(ESIR::Opcode::CheckIfKeyIsLast, targetIndex), m_sourceIndex(sourceIndex) { }
    int m_sourceIndex;
};

class EnumerateIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(Enumerate, int);

    int sourceIndex() { return m_sourceIndex; }
#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
    }
#endif

private:
    EnumerateIR(int targetIndex, int sourceIndex)
        : ESIR(ESIR::Opcode::Enumerate, targetIndex), m_sourceIndex(sourceIndex) { }
    int m_sourceIndex;
};

class GetThisIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_0(GetThis)

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
    }
#endif

private:
    GetThisIR(int targetIndex)
        : ESIR(ESIR::Opcode::GetThis, targetIndex) { }
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
    DECLARE_STATIC_GENERATOR_3(GetVar, int, int, bool)

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " var" << m_varIndex << " up " << m_varUpIndex;
    }
#endif

    int varIndex() { return m_varIndex; }
    int varUpIndex() { return m_varUpIndex; }
    bool needsActivation() { return m_needsActivation; }

private:
    GetVarIR(int targetIndex, int varIndex, int varUpIndex, bool needsActivation)
        : ESIR(ESIR::Opcode::GetVar, targetIndex), m_varIndex(varIndex) , m_varUpIndex(varUpIndex), m_needsActivation(needsActivation) { }
    int m_varIndex;
    int m_varUpIndex;
    bool m_needsActivation;
};

class SetVarIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_4(SetVar, int, int, int, bool);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " var" << m_localVarIndex << " (up) " << m_upVarIndex << " = tmp" << m_sourceIndex;
    }
#endif

    int localVarIndex() { return m_localVarIndex; }
    int upVarIndex() { return m_upVarIndex; }
    int sourceIndex() { return m_sourceIndex; }
    bool needsActivation() { return m_needsActivation; }

private:
    SetVarIR(int targetIndex, int localVarIndex, int upVarIndex, int sourceIndex, bool needsActivation)
        : ESIR(ESIR::Opcode::SetVar, targetIndex), m_localVarIndex(localVarIndex), m_upVarIndex(upVarIndex), m_sourceIndex(sourceIndex), m_needsActivation(needsActivation) { }
    int m_localVarIndex;
    int m_upVarIndex;
    int m_sourceIndex;
    bool m_needsActivation;
};

class GetVarGenericWithoutExceptionIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(GetVarGenericWithoutException, ByteCode*)

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);

        out << " " << ((GetById*) m_originalGetByIdByteCode)->m_name.string()->utf8Data();
    }
#endif
    ByteCode* originalGetByIdByteCode() { return m_originalGetByIdByteCode; }

private:
    GetVarGenericWithoutExceptionIR(int targetIndex, ByteCode* originalGetByIdByteCode)
        : ESIR(ESIR::Opcode::GetVarGenericWithoutException, targetIndex), m_originalGetByIdByteCode(originalGetByIdByteCode) { }
    ByteCode* m_originalGetByIdByteCode;
};

class GetVarGenericIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(GetVarGeneric, ByteCode*)


#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);

        out << " " << ((GetById*) m_originalGetByIdByteCode)->m_name.string()->utf8Data();
    }
#endif
    ByteCode* originalGetByIdByteCode() { return m_originalGetByIdByteCode; }

private:
    GetVarGenericIR(int targetIndex, ByteCode* originalGetByIdByteCode)
        : ESIR(ESIR::Opcode::GetVarGeneric, targetIndex), m_originalGetByIdByteCode(originalGetByIdByteCode) { }
    ByteCode* m_originalGetByIdByteCode;
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

class GetGlobalVarGenericIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_2(GetGlobalVarGeneric, GetByGlobalIndex*, ESString*)


#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);

        out << " " << m_esName->utf8Data();
    }
#endif
    GetByGlobalIndex* byteCode() { return m_byteCode; }
    ESString* nonAtomicName() { return m_esName; }

private:
    GetGlobalVarGenericIR(int targetIndex, GetByGlobalIndex* v, ESString* esName)
        : ESIR(ESIR::Opcode::GetGlobalVarGeneric, targetIndex), m_byteCode(v), m_esName(esName) { }
    GetByGlobalIndex* m_byteCode;
    ESString* m_esName;
};

class SetGlobalVarGenericIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_3(SetGlobalVarGeneric, SetByGlobalIndex*, int, ESString*);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " " << m_esName->utf8Data() << " = tmp" << m_sourceIndex;
    }
#endif

    SetByGlobalIndex* byteCode() { return m_byteCode; }
    int sourceIndex() { return m_sourceIndex; }
    ESString* nonAtomicName() { return m_esName; }

private:
    SetGlobalVarGenericIR(int targetIndex, SetByGlobalIndex* v, int sourceIndex, ESString* esName)
        : ESIR(ESIR::Opcode::SetGlobalVarGeneric, targetIndex), m_byteCode(v), m_sourceIndex(sourceIndex), m_esName(esName) { }
    SetByGlobalIndex* m_byteCode;
    int m_sourceIndex;
    ESString* m_esName;
};

class GetObjectIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_4(GetObject, int, int, int, escargot::GetObject *);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " tmp" << m_objectIndex << "[tmp" << m_propertyIndex << "]";
    }
#endif

    int objectIndex() { return m_objectIndex; }
    int receiverIndex() { return m_objectIndex; }
    int propertyIndex() { return m_propertyIndex; }

private:
    GetObjectIR(int targetIndex, int targetIndex1, int objectIndex, int propertyIndex, escargot::GetObject* b)
        : ESIR(ESIR::Opcode::GetObject, targetIndex),
          m_objectIndex(objectIndex),
          m_receiverIndex(targetIndex1),
          m_propertyIndex(propertyIndex),
          m_byteCode(b) { }

    int m_objectIndex;
    int m_receiverIndex;
    int m_propertyIndex;
    escargot::GetObject* m_byteCode;
};

class GetObjectPreComputedIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_3(GetObjectPreComputed, int, int, GetObjectPreComputedCase *);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " tmp" << m_objectIndex << "." << m_byteCode->m_propertyValue->utf8Data();
    }
#endif

    int targetIndex() { return m_targetIndex; }
    int objectIndex() { return m_objectIndex; }
    GetObjectPreComputedCase* byteCode() { return m_byteCode; }

private:
    GetObjectPreComputedIR(int targetIndex, int targetIndex1, int objectIndex, GetObjectPreComputedCase* b)
        : ESIR(ESIR::Opcode::GetObjectPreComputed, targetIndex),
          m_objectIndex(objectIndex),
          m_receiverIndex(targetIndex1),
          m_byteCode(b) { }

    int m_objectIndex;
    int m_receiverIndex;
    GetObjectPreComputedCase* m_byteCode;
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

class GetStringByIndexIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_2(GetStringByIndex, int, int);

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
    GetStringByIndexIR(int targetIndex, int objectIndex, int propertyIndex)
        : ESIR(ESIR::Opcode::GetStringByIndex, targetIndex),
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

class SetObjectIR : public ESIR {
public:
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
    int objectIndex() { return m_objectIndex; }
    int propertyIndex() { return m_propertyIndex; }
    int sourceIndex() { return m_sourceIndex; }

private:
    SetObjectIR(int targetIndex, int objectIndex, int propertyIndex, int sourceIndex)
        : ESIR(ESIR::Opcode::SetObject, targetIndex),
          m_objectIndex(objectIndex),
          m_propertyIndex(propertyIndex),
          m_sourceIndex(sourceIndex) {
    }
    ESHiddenClass* m_cachedHiddenClass;
    int m_objectIndex;
    int m_propertyIndex;
    int m_sourceIndex;
};

class SetObjectPreComputedIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_3(SetObjectPreComputed, int, int, SetObjectPreComputedCase *);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " tmp" << m_objectIndex << "["<< m_byteCode->m_propertyValue->utf8Data() << "] = " << "tmp" << m_sourceIndex;
    }
#endif

    int objectIndex() { return m_objectIndex; }
    int sourceIndex() { return m_sourceIndex; }
    SetObjectPreComputedCase * byteCode() { return m_byteCode; }

private:
    SetObjectPreComputedIR(int targetIndex, int objectIndex, int sourceIndex, SetObjectPreComputedCase* byteCode)
        : ESIR(ESIR::Opcode::SetObjectPreComputed, targetIndex),
          m_objectIndex(objectIndex),
          m_sourceIndex(sourceIndex),
          m_byteCode(byteCode) { }
    int m_objectIndex;
    int m_sourceIndex;
    SetObjectPreComputedCase* m_byteCode;
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

class Int32ModIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(Int32Mod, int, int);

private:
    Int32ModIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::Int32Mod, targetIndex, leftIndex, rightIndex) { }
};

class DoubleModIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(DoubleMod, int, int);

private:
    DoubleModIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::DoubleMod, targetIndex, leftIndex, rightIndex) { }
};

class GenericModIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(GenericMod, int, int);

private:
    GenericModIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::GenericMod, targetIndex, leftIndex, rightIndex) { }
};

class BitwiseAndIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(BitwiseAnd, int, int);

private:
    BitwiseAndIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::BitwiseAnd, targetIndex, leftIndex, rightIndex) { }
};

class BitwiseOrIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(BitwiseOr, int, int);

private:
    BitwiseOrIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::BitwiseOr, targetIndex, leftIndex, rightIndex) { }
};

class BitwiseXorIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(BitwiseXor, int, int);

private:
    BitwiseXorIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::BitwiseXor, targetIndex, leftIndex, rightIndex) { }
};

class EqualIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(Equal, int, int);

private:
    EqualIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::Equal, targetIndex, leftIndex, rightIndex) { }
};

class NotEqualIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(NotEqual, int, int);

private:
    NotEqualIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::NotEqual, targetIndex, leftIndex, rightIndex) { }
};

class StrictEqualIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(StrictEqual, int, int);

private:
    StrictEqualIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::StrictEqual, targetIndex, leftIndex, rightIndex) { }
};

class NotStrictEqualIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(NotStrictEqual, int, int);

private:
    NotStrictEqualIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::NotStrictEqual, targetIndex, leftIndex, rightIndex) { }
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

class UnsignedRightShiftIR : public BinaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_2(UnsignedRightShift, int, int);

private:
    UnsignedRightShiftIR(int targetIndex, int leftIndex, int rightIndex)
        : BinaryExpressionIR(ESIR::Opcode::UnsignedRightShift, targetIndex, leftIndex, rightIndex) { }
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

class DecrementIR : public UnaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_1(Decrement, int);

private:
    DecrementIR(int targetIndex, int sourceIndex)
        : UnaryExpressionIR(ESIR::Opcode::Decrement, targetIndex, sourceIndex) { }
};

class BitwiseNotIR : public UnaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_1(BitwiseNot, int);

private:
    BitwiseNotIR(int targetIndex, int sourceIndex)
        : UnaryExpressionIR(ESIR::Opcode::BitwiseNot, targetIndex, sourceIndex) { }
};

class LogicalNotIR : public UnaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_1(LogicalNot, int);

private:
    LogicalNotIR(int targetIndex, int sourceIndex)
        : UnaryExpressionIR(ESIR::Opcode::LogicalNot, targetIndex, sourceIndex) { }
};

class UnaryMinusIR : public UnaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_1(UnaryMinus, int);

private:
    UnaryMinusIR(int targetIndex, int sourceIndex)
        : UnaryExpressionIR(ESIR::Opcode::UnaryMinus, targetIndex, sourceIndex) { }
};

class TypeOfIR : public UnaryExpressionIR {
public:
    DECLARE_STATIC_GENERATOR_1(TypeOf, int);

private:
    TypeOfIR(int targetIndex, int sourceIndex)
        : UnaryExpressionIR(ESIR::Opcode::TypeOf, targetIndex, sourceIndex) { }
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

class ThrowIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(Throw, int);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << ": ";
        ESIR::dump(out);
        out << " tmp" << m_sourceIndex;
    }
#endif
    int sourceIndex() { return m_sourceIndex; }

private:
    ThrowIR(int targetIndex, int sourceIndex)
        : ESIR(ESIR::Opcode::Throw, targetIndex), m_sourceIndex(sourceIndex) { }
    int m_sourceIndex;
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

class CreateFunctionIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(CreateFunction, ByteCode*);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " " << ((escargot::CreateFunction*) m_originalByteCode)->m_name.string()->utf8Data();
    }
#endif
    ByteCode* originalByteCode() { return m_originalByteCode; }

private:
    CreateFunctionIR(int targetIndex, ByteCode* originalByteCode)
        : ESIR(ESIR::Opcode::CreateFunction, targetIndex), m_originalByteCode(originalByteCode) { }
    ByteCode* m_originalByteCode;
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

protected:
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

class CallNewJSIR : public CallJSIR {
public:
    DECLARE_STATIC_GENERATOR_4(CallNewJS, int, int, int, int*);

protected:
    CallNewJSIR(int targetIndex, int calleeIndex, int receiverIndex, int argumentCount, int* argumentIndexes)
        : CallJSIR(targetIndex, calleeIndex, receiverIndex, argumentCount, argumentIndexes) { m_opcode = ESIR::Opcode::CallNewJS; }
};

class CallEvalIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_2(CallEval, int, int*);
#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " argumentCount " << m_argumentIndexes.size() << " :";
        for (size_t i=0; i<m_argumentIndexes.size(); i++)
            out << ", tmp" << m_argumentIndexes[i];
    }
#endif
    int argumentIndex(size_t idx) { return m_argumentIndexes[idx]; }
    int argumentCount() { return m_argumentIndexes.size(); }

protected:
    CallEvalIR(int targetIndex, int argumentCount, int* argumentIndexes)
        : ESIR(ESIR::Opcode::CallEval, targetIndex), m_argumentIndexes(argumentCount)
    {
        for (int i=0; i<argumentCount; i++)
            m_argumentIndexes[i] = argumentIndexes[i];
    }
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

class AllocPhiIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_0(AllocPhi);

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
    }
#endif

private:
    AllocPhiIR(int targetIndex)
        : ESIR(ESIR::Opcode::AllocPhi, targetIndex) { }
};

class StorePhiIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_2(StorePhi, int, int);

    int sourceIndex() { return m_sourceIndex; }
    int allocPhiIndex() { return m_allocPhiIndex; }

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << "tmp" << m_sourceIndex << " to tmp" << m_allocPhiIndex;
    }
#endif

private:
    StorePhiIR(int targetIndex, int sourceIndex, int allocPhiIndex)
        : ESIR(ESIR::Opcode::StorePhi, targetIndex),
          m_sourceIndex(sourceIndex),
          m_allocPhiIndex(allocPhiIndex) { }

    int m_sourceIndex;
    int m_allocPhiIndex;
};

class LoadPhiIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_3(LoadPhi, int, int, int)

    int allocPhiIndex() { return m_allocPhiIndex; }
    int srcIndex0() { return m_srcIndex0; }
    int srcIndex1() { return m_srcIndex1; }

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << "tmp" << m_allocPhiIndex;
    }
#endif

private:
    LoadPhiIR(int targetIndex, int allocPhiIndex, int srcIndex0, int srcIndex1)
        : ESIR(ESIR::Opcode::LoadPhi, targetIndex), m_allocPhiIndex(allocPhiIndex), m_srcIndex0(srcIndex0), m_srcIndex1(srcIndex1) { }
    int m_allocPhiIndex;
    int m_srcIndex0;
    int m_srcIndex1;
};

class CreateObjectIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(CreateObject, int)

    int keyCount() { return m_keyCount; }

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << "(" << m_keyCount << ")";
    }
#endif

private:
    CreateObjectIR(int targetIndex, int keyCount)
        : ESIR(ESIR::Opcode::CreateObject, targetIndex), m_keyCount(keyCount) { }
    int m_keyCount;
};

class CreateArrayIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_1(CreateArray, int)

    int keyCount() { return m_keyCount; }

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << "(" << m_keyCount << ")";
    }
#endif

private:
    CreateArrayIR(int targetIndex, int keyCount)
        : ESIR(ESIR::Opcode::CreateArray, targetIndex), m_keyCount(keyCount) { }
    int m_keyCount;
};

class InitObjectIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_3(InitObject, int, int, int)

    int objectIndex() { return m_objectIndex; }
    int keyIndex() { return m_keyIndex; }
    int sourceIndex() { return m_sourceIndex; }

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " tmp" << m_objectIndex << "[tmp" << m_keyIndex << "] = tmp" << m_sourceIndex;
    }
#endif

private:
    InitObjectIR(int targetIndex, int objectIndex, int keyIndex, int sourceIndex)
        : ESIR(ESIR::Opcode::InitObject, targetIndex), m_objectIndex(objectIndex), m_keyIndex(keyIndex), m_sourceIndex(sourceIndex) { }
    int m_objectIndex;
    int m_keyIndex;
    int m_sourceIndex;
};

class InitArrayObjectIR : public ESIR {
public:
    DECLARE_STATIC_GENERATOR_3(InitArrayObject, int, int, int)

    int objectIndex() { return m_objectIndex; }
    int keyIndex() { return m_keyIndex; }
    int sourceIndex() { return m_sourceIndex; }

#ifndef NDEBUG
    virtual void dump(std::ostream& out)
    {
        out << "tmp" << m_targetIndex << ": ";
        ESIR::dump(out);
        out << " tmp" << m_objectIndex << "[tmp" << m_keyIndex << "] = tmp" << m_sourceIndex;
    }
#endif

private:
    InitArrayObjectIR(int targetIndex, int objectIndex, int keyIndex, int sourceIndex)
        : ESIR(ESIR::Opcode::InitArrayObject, targetIndex), m_objectIndex(objectIndex), m_keyIndex(keyIndex), m_sourceIndex(sourceIndex) { }
    int m_objectIndex;
    int m_keyIndex;
    int m_sourceIndex;
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
