#ifndef ESIRType_h
#define ESIRType_h

namespace escargot {
namespace ESJIT {

#define FOR_EACH_ESVALUE_TYPE(F) \
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

typedef enum {
    #define DECLARE_TYPE(name) name,
    FOR_EACH_ESVALUE_TYPE(DECLARE_TYPE)
    #undef DECLARE_TYPE
    Last
} ESValueType;

typedef uint64_t Type;

}}
#endif
