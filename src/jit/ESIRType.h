#ifndef ESIRType_h
#define ESIRType_h

#ifdef ENABLE_ESJIT

namespace escargot {
namespace ESJIT {

#define FOR_EACH_ESIR_TYPES(F) \
    F(Int32,          0x1 << 1) \
    F(Boolean,        0x1 << 2) \
    F(Double,         0x1 << 3) \
    F(Number,         TypeInt32 | TypeDouble) \
    F(Null,           0x1 << 4) \
    F(Undefined,      0x1 << 5) \
    F(Empty,          0x1 << 6) \
    F(Deleted,        0x1 << 7) \
    F(String,         0x1 << 8) \
    F(RopeString,     0x1 << 9) \
    F(FunctionObject, 0x1 << 10) \
    F(ArrayObject,    0x1 << 11) \
    F(StringObject,   0x1 << 12) \
    F(ErrorObject,    0x1 << 13) \
    F(DateObject,     0x1 << 14) \
    F(NumberObject,   0x1 << 15) \
    F(BooleanObject,  0x1 << 16) \
    F(Object,         TypeFunctionObject | TypeArrayObject | TypeStringObject | TypeErrorObject | TypeDateObject | TypeNumberObject | TypeBooleanObject) \
    F(Pointer,        TypeString | TypeObject) \
    F(Bottom,         0x0) \
    F(Top,            ~0x0)

#define DECLARE_TYPE_CONSTANT(type, encoding) \
        const uint64_t Type##type = encoding;
FOR_EACH_ESIR_TYPES(DECLARE_TYPE_CONSTANT)
#undef DECLARE_TYPE_CONSTANT

void logVerboseJIT(const char* fmt...);

#ifndef LOG_VJ
#ifndef NDEBUG
#define LOG_VJ(...) ::escargot::ESJIT::logVerboseJIT(__VA_ARGS__)
#else
#define LOG_VJ(...)
#endif
#endif

class Type {
public:
    Type(uint64_t type = TypeBottom)
        : m_type(type) { }
    Type(const Type& type)
        : m_type(type.m_type) { }

    void mergeType(Type otherType)
    {
        // FIXME workaround for running SunSpider
        // m_type = m_type | otherType.m_type;
        if (isInt32Type()) {
            if (otherType.isDoubleType()) {
                m_type = otherType.m_type;
            }
        } else if (isDoubleType()) {
        } else {
            m_type = otherType.m_type;
        }

    }

    static Type getType(ESValue value)
    {
        if (value.isBoolean())
            return TypeBoolean;
        else if (value.isInt32())
            return TypeInt32;
        else if (value.isDouble())
            return TypeDouble;
        else if (value.isNull())
            return TypeNull;
        else if (value.isUndefined())
            return TypeUndefined;
        else if (value.isEmpty()) // FIXME what if the profiled value was empty? (!= not profiled)
            return TypeBottom;
        else if (value.isESPointer()) {
            ESPointer* p = value.asESPointer();
            if (p->isESArrayObject()) {
                return TypeArrayObject;
            } else if (p->isESString()) {
                return TypeString;
            } else if (p->isESFunctionObject()) {
                return TypeFunctionObject;
            } else if (p->isESObject()) {
                return TypeObject;
            } else {
                LOG_VJ("WARNING: Reading type of unhandled ESValue '%s'. Returning Top.\n", value.toString()->utf8Data());
                return TypePointer;
            }
        } else {
            LOG_VJ("WARNING: Reading type of unhandled ESValue '%s'. Returning Top.\n", value.toString()->utf8Data());
            return TypeTop;
        }
    }

    uint64_t type() { return m_type; }

    bool operator==(Type& otherType)
    {
        return m_type == otherType.m_type;
    }

    bool operator!=(Type& otherType)
    {
        return !operator==(otherType);
    }

#ifndef NDEBUG
    const char* getESIRTypeName()
    {
        if (m_type == TypeBottom)
            return "Bottom";
#define DECLARE_ESIR_TYPE_NAME_STRING(type, offset) \
        if (is##type##Type()) \
            return #type;
        FOR_EACH_ESIR_TYPES(DECLARE_ESIR_TYPE_NAME_STRING)
#undef DECLARE_ESIR_TYPE_NAME_STRING
        return "Unknown";
    }

    void dump(std::ostream& out)
    {
        out << "[Type: 0x" << std::hex << m_type << std::dec;
        out << " : " << getESIRTypeName() << "]";
    }
#endif

#define DECLARE_IS_TYPE(type, unused) \
    bool is##type##Type() \
    { \
        return Type##type == TypeBottom? (m_type == TypeBottom) : ((m_type != TypeBottom) && ((m_type | Type##type) == Type##type)); \
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

COMPILE_ASSERT((sizeof(Type)) == (sizeof(uint64_t)), sizeof ESJIT::Type should be equal to sizeof uint64_t);

}}
#endif
#endif
