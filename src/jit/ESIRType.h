#ifndef ESIRType_h
#define ESIRType_h

#ifdef ENABLE_ESJIT

namespace escargot {
namespace ESJIT {

#define FOR_EACH_PRIMITIVE_TYPES(F) \
    F(Int32, 1) \
    F(Boolean, 2) \
    F(Double, 3) \
    F(Null, 4) \
    F(Undefined, 5) \
    F(Empty, 6) \
    F(Deleted, 7) \
    F(Pointer, 8) \
    F(String, 9) \
    F(RopeString, 10) \
    F(Object, 11) \
    F(FunctionObject, 12) \
    F(ArrayObject, 13) \
    F(StringObject, 14) \
    F(ErrorObject, 15) \
    F(DateObject, 16) \
    F(NumberObject, 17) \
    F(BooleanObject, 18) \

#define FOR_EACH_ESIR_TYPES(F) \
    FOR_EACH_PRIMITIVE_TYPES(F) \
    F(Number, 0) \
    F(Bottom, 0) \
    F(Top, 0)

/* Primitive types */
#define DECLARE_TYPE_CONSTANT(type, shift) \
const uint64_t Type##type = 0x1 << shift;
FOR_EACH_PRIMITIVE_TYPES(DECLARE_TYPE_CONSTANT)
#undef DECLARE_TYPE_CONSTANT

/* Complex types */
const uint64_t TypeNumber = TypeInt32 | TypeDouble;

const uint64_t TypeBottom = 0x0000000000000000;
const uint64_t TypeTop = 0xffffffffffffffff;

inline const char* getESIRTypeName(uint64_t m_type)
{
    #define DECLARE_ESIR_TYPE_NAME_STRING(type, offset) \
    if (m_type == 0x1 << offset) \
        return #type;
    FOR_EACH_ESIR_TYPES(DECLARE_ESIR_TYPE_NAME_STRING)
    #undef DECLARE_ESIR_TYPE_NAME_STRING
    if (m_type == TypeBottom)
        return "Bottom";
    if (m_type == TypeTop)
        return "Top";
    return "Complex";
}

class Type
{
public:
    Type(uint64_t type = TypeBottom) : m_type(type) { }
    Type(const Type& type) : m_type(type.m_type) { }

    void mergeType(Type otherType)
    {
        m_type |= otherType.m_type;
    }

    static Type getType(ESValue value)
    {
        if (value.isInt32())
            return TypeInt32;
        else if (value.isDouble())
            return TypeDouble;
        else if (value.isUndefined())
            return TypeUndefined;
        else if (value.isESPointer()) {
            return TypePointer;
#if 0
            ESPointer* p = value.asESPointer();
            if (p->isESFunctionObject())
                return TypeFunctionObject;
            else
                return TypeObject;
#endif
        } else {
#ifndef NDEBUG
            if (ESVMInstance::currentInstance()->m_verboseJIT)
                printf("WARNING: Reading type of unhandled ESValue '%s'. Returning Top.\n", value.toString()->utf8Data());
#endif
            return TypeTop;
        }
    }

    void dump(std::ostream& out) {
        out << "[Type: 0x" << std::hex << m_type << std::dec;
        out << " : " << getESIRTypeName(m_type) << "]";
    }

#define DECLARE_IS_TYPE(type, unused) \
    bool is##type##Type() \
    { \
        return (m_type & Type##type) == Type##type; \
    }
    FOR_EACH_ESIR_TYPES(DECLARE_IS_TYPE)
#undef DECLARE_IS_TYPE

#define DECLARE_HAS_FLAG(type, unused) \
    bool has##type##Flag() \
    { \
        return (m_type & Type##type) != 0; \
    }
    FOR_EACH_ESIR_TYPES(DECLARE_HAS_FLAG)
#undef DECLARE_HAS_FLAG

private:
    uint64_t m_type;
};

COMPILE_ASSERT((sizeof (Type)) == (sizeof (uint64_t)), sizeof ESJIT::Type should be equal to sizeof uint64_t);

}}
#endif
#endif
