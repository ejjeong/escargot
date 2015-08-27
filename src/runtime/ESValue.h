#ifndef ESValue_h
#define ESValue_h

#include "InternalString.h"

namespace JSC {
namespace Yarr {
    class BytecodePattern;
}
}

namespace escargot {

class ESUndefined;
class ESNull;
class ESBoolean;
class ESBooleanObject;
class ESNumberObject;
class ESString;
class ESChainString;
class ESObject;
class ESSlot;
class ESHiddenClass;
class ESFunctionObject;
class ESArrayObject;
class ESStringObject;
class ESDateObject;
class FunctionNode;
class ESVMInstance;
class ESPointer;
class ESRegExpObject;

union ValueDescriptor {
    int64_t asInt64;
#if ESCARGOT_32
    double asDouble;
#elif ESCARGOT_64
    ESPointer* ptr;
#endif
    struct {
        int32_t payload;
        int32_t tag;
    } asBits;
};

#define OBJECT_OFFSETOF(class, field) (reinterpret_cast<ptrdiff_t>(&(reinterpret_cast<class*>(0x4000)->field)) - 0x4000)

#define TagOffset (OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.tag))
#define PayloadOffset (OBJECT_OFFSETOF(EncodedValueDescriptor, asBits.payload))

#if ESCARGOT_64
#define CellPayloadOffset 0
#else
#define CellPayloadOffset PayloadOffset
#endif


class ESValue {
    //static void* operator new(size_t, void* p) = delete;
    //static void* operator new[](size_t, void* p) = delete;
    //static void* operator new(size_t size) = delete;
    //static void* operator new[](size_t size) = delete;

public:
#if ESCARGOT_32
    enum { Int32Tag =        0xffffffff };
    enum { BooleanTag =      0xfffffffe };
    enum { NullTag =         0xfffffffd };
    enum { UndefinedTag =    0xfffffffc };
    enum { PointerTag =      0xfffffffb };
    enum { EmptyValueTag =   0xfffffffa };
    enum { DeletedValueTag = 0xfffffff9 };

    enum { LowestTag =  DeletedValueTag };
#endif

    enum ESNullTag { ESNull };
    enum ESUndefinedTag { ESUndefined };
    enum ESEmptyValueTag { ESEmptyValue };
    enum ESTrueTag { ESTrue };
    enum ESFalseTag { ESFalse };
    enum EncodeAsDoubleTag { EncodeAsDouble };
    enum ESForceUninitializedTag { ESForceUninitialized };

    ESValue();
    ESValue(ESForceUninitializedTag);
    ESValue(ESNullTag);
    ESValue(ESUndefinedTag);
    ESValue(ESEmptyValueTag);
    ESValue(ESTrueTag);
    ESValue(ESFalseTag);
    ESValue(ESPointer* ptr);
    ESValue(const ESPointer* ptr);

    // Numbers
    ESValue(EncodeAsDoubleTag, double);
    explicit ESValue(double);
    explicit ESValue(bool);
    explicit ESValue(char);
    explicit ESValue(unsigned char);
    explicit ESValue(short);
    explicit ESValue(unsigned short);
    explicit ESValue(int);
    explicit ESValue(unsigned);
    explicit ESValue(long);
    explicit ESValue(unsigned long);
    explicit ESValue(long long);
    explicit ESValue(unsigned long long);

    bool operator==(const ESValue& other) const;
    bool operator!=(const ESValue& other) const;

    bool isInt32() const;
    bool isUInt32() const;
    bool isDouble() const;
    bool isTrue() const;
    bool isFalse() const;

    int32_t asInt32() const;
    uint32_t asUInt32() const;
    int64_t asMachineInt() const;
    double asDouble() const;
    bool asBoolean() const;
    double asNumber() const;

    int32_t asInt32ForArithmetic() const; // Boolean becomes an int, but otherwise like asInt32().

    // Querying the type.
    bool isEmpty() const;
    bool isFunction() const;
    bool isUndefined() const;
    bool isNull() const;
    bool isUndefinedOrNull() const
    {
        return isUndefined() || isNull();
    }
    bool isBoolean() const;
    bool isMachineInt() const;
    bool isNumber() const;
    bool isESString() const;
    bool isSymbol() const;
    bool isPrimitive() const;
    bool isGetterSetter() const;
    bool isCustomGetterSetter() const;
    bool isObject() const;

    enum PrimitiveTypeHint { PreferString, PreferNumber };
    ESValue toPrimitive(PrimitiveTypeHint = PreferNumber) const; //$7.1.1 ToPrimitive
    bool toBoolean() const; //$7.1.2 ToBoolean
    double toNumber() const; //$7.1.3 ToNumber
    double toInteger() const; //$7.1.4 ToInteger
    int32_t toInt32() const; //$7.1.5 ToInt32
    ESString* toString() const; //$7.1.12 ToString
    ESObject* toObject() const; //$7.1.13 ToObject
    double toLength() const; //$7.1.15 ToLength

    ESString* asESString() const;

    bool isESPointer() const;
    ESPointer* asESPointer() const;

    static ptrdiff_t offsetOfPayload() { return OBJECT_OFFSETOF(ESValue, u.asBits.payload); }
    static ptrdiff_t offsetOfTag() { return OBJECT_OFFSETOF(ESValue, u.asBits.tag); }

#if ESCARGOT_32
    uint32_t tag() const;
    int32_t payload() const;
#elif ESCARGOT_64
    // These values are #defines since using static const integers here is a ~1% regression!

    // This value is 2^48, used to encode doubles such that the encoded value will begin
    // with a 16-bit pattern within the range 0x0001..0xFFFE.
    #define DoubleEncodeOffset 0x1000000000000ll
    // If all bits in the mask are set, this indicates an integer number,
    // if any but not all are set this value is a double precision number.
    #define TagTypeNumber 0xffff000000000000ll

    // All non-numeric (bool, null, undefined) immediates have bit 2 set.
    #define TagBitTypeOther 0x2ll
    #define TagBitBool      0x4ll
    #define TagBitUndefined 0x8ll
    // Combined integer value for non-numeric immediates.
    #define ValueFalse     (TagBitTypeOther | TagBitBool | false)
    #define ValueTrue      (TagBitTypeOther | TagBitBool | true)
    #define ValueUndefined (TagBitTypeOther | TagBitUndefined)
    #define ValueNull      (TagBitTypeOther)

    // TagMask is used to check for all types of immediate values (either number or 'other').
    #define TagMask (TagTypeNumber | TagBitTypeOther)

    // These special values are never visible to JavaScript code; Empty is used to represent
    // Array holes, and for uninitialized ESValues. Deleted is used in hash table code.
    // These values would map to cell types in the ESValue encoding, but not valid GC cell
    // pointer should have either of these values (Empty is null, deleted is at an invalid
    // alignment for a GC cell, and in the zero page).
    #define ValueEmpty   0x0ll
    #define ValueDeleted 0x4ll
#endif

private:
    ValueDescriptor u;

public:
    bool abstractEqualsTo(const ESValue& val);
    bool equalsTo(const ESValue& val);

};


class ESPointer : public gc {
public:
    enum Type {
        ESString = 1 << 0,
        ESChainString = 1 << 1,
        ESObject = 1 << 2,
        ESFunctionObject = 1 << 3,
        ESArrayObject = 1 << 4,
        ESStringObject = 1 << 5,
        ESErrorObject = 1 << 6,
        ESDateObject = 1 << 7,
        ESNumberObject = 1 << 8,
        ESRegExpObject = 1 << 9,
        ESBooleanObject = 1 << 10,
        TypeMask = 0xffff
    };

protected:
    ESPointer(Type type)
    {
        m_type = type;
    }

public:
    ALWAYS_INLINE int type()
    {
        return m_type;
    }

    ALWAYS_INLINE bool isESString() const
    {
        return m_type & Type::ESString;
    }

    ALWAYS_INLINE ::escargot::ESString* asESString()
    {
#ifndef NDEBUG
        ASSERT(isESString());
#endif
        return reinterpret_cast<::escargot::ESString *>(this);
    }

    ALWAYS_INLINE bool isESChainString() const
    {
        return m_type & Type::ESChainString;
    }

    ALWAYS_INLINE ::escargot::ESChainString* asESChainString()
    {
#ifndef NDEBUG
        ASSERT(isESChainString());
#endif
        return reinterpret_cast<::escargot::ESChainString *>(this);
    }

    ALWAYS_INLINE bool isESObject() const
    {
        return m_type & Type::ESObject;
    }

    ALWAYS_INLINE ::escargot::ESObject* asESObject()
    {
#ifndef NDEBUG
        ASSERT(isESObject());
#endif
        return reinterpret_cast<::escargot::ESObject *>(this);
    }

    ALWAYS_INLINE bool isESFunctionObject()
    {
        return m_type & Type::ESFunctionObject;
    }

    ALWAYS_INLINE ::escargot::ESFunctionObject* asESFunctionObject()
    {
#ifndef NDEBUG
        ASSERT(isESFunctionObject());
#endif
        return reinterpret_cast<::escargot::ESFunctionObject *>(this);
    }

    ALWAYS_INLINE bool isESArrayObject() const
    {
        return m_type & Type::ESArrayObject;
    }

    ALWAYS_INLINE ::escargot::ESArrayObject* asESArrayObject()
    {
#ifndef NDEBUG
        ASSERT(isESArrayObject());
#endif
        return reinterpret_cast<::escargot::ESArrayObject *>(this);
    }

    ALWAYS_INLINE bool isESStringObject() const
    {
        return m_type & Type::ESStringObject;
    }

    ALWAYS_INLINE ::escargot::ESStringObject* asESStringObject()
    {
#ifndef NDEBUG
        ASSERT(isESStringObject());
#endif
        return reinterpret_cast<::escargot::ESStringObject *>(this);
    }

    ALWAYS_INLINE bool isESNumberObject() const
    {
        return m_type & Type::ESNumberObject;
    }

    ALWAYS_INLINE ::escargot::ESNumberObject* asESNumberObject()
    {
#ifndef NDEBUG
        ASSERT(isESNumberObject());
#endif
        return reinterpret_cast<::escargot::ESNumberObject *>(this);
    }

    ALWAYS_INLINE bool isESBooleanObject() const
    {
        return m_type & Type::ESBooleanObject;
    }

    ALWAYS_INLINE ::escargot::ESBooleanObject* asESBooleanObject()
    {
#ifndef NDEBUG
        ASSERT(isESBooleanObject());
#endif
        return reinterpret_cast<::escargot::ESBooleanObject *>(this);
    }

    ALWAYS_INLINE bool isESRegExpObject() const
    {
        return m_type & Type::ESRegExpObject;
    }

    ALWAYS_INLINE ::escargot::ESRegExpObject* asESRegExpObject()
    {
#ifndef NDEBUG
        ASSERT(isESRegExpObject());
#endif
        return reinterpret_cast<::escargot::ESRegExpObject *>(this);
    }

    ALWAYS_INLINE bool isESErrorObject() const
    {
        return m_type & Type::ESErrorObject;
    }

    ALWAYS_INLINE bool isESDateObject() const
    {
        return m_type & Type::ESDateObject;
    }

    ALWAYS_INLINE ::escargot::ESDateObject* asESDateObject()
    {
#ifndef NDEBUG
        ASSERT(isESDateObject());
#endif
        return reinterpret_cast<::escargot::ESDateObject *>(this);
    }

protected:
    // 0x******@@
    // * -> Data
    // @ -> tag
    int m_type;
};

class ESStringData : public u16string, public gc {
public:
    ESStringData()
    {
        m_hashData.m_isHashInited =  false;
    }
    ESStringData(const char16_t* str)
        : u16string(str)
    {
        m_hashData.m_isHashInited =  false;
    }
    ESStringData(u16string&& src)
        : u16string(std::move(src))
    {
        m_hashData.m_isHashInited =  false;
    }

    explicit ESStringData(int number)
        : ESStringData((double)number)
    {
    }

    explicit ESStringData(double number)
    {
        m_hashData.m_isHashInited =  false;
        char16_t buf[512];
        char chbuf[50];
        char* end = rapidjson::internal::dtoa(number, chbuf);
        int i = 0;
        for (char* p = chbuf; p != end; ++p) {
            buf[i++] = (char16_t) *p;
        }

        if(i >= 3 && buf[i-1] == '0' && buf[i-2] == '.') {
            i -= 2;
        }

        buf[i] = u'\0';

        reserve(i);
        append(&buf[0], &buf[i]);
    }

    explicit ESStringData(char16_t c)
        : u16string({c})
    {
        m_hashData.m_isHashInited =  false;
    }

    ESStringData(const char* s)
        : u16string(std::move(utf8ToUtf16(s, strlen(s))))
    {
        m_hashData.m_isHashInited =  false;
    }

    ESStringData(const ESStringData& s) = delete;
    void operator =(const ESStringData& s) = delete;

    ALWAYS_INLINE const char16_t* data() const
    {
        return u16string::data();
    }

    ALWAYS_INLINE size_t hashValue() const
    {
        initHash();
        return m_hashData.m_hashData;
    }

    ALWAYS_INLINE void initHash() const
    {
        if(!m_hashData.m_isHashInited) {
            m_hashData.m_isHashInited = true;
            std::hash<std::basic_string<char16_t> > hashFn;
            m_hashData.m_hashData = hashFn((std::basic_string<char16_t> &)*this);
        }
    }

protected:
#pragma pack(push, 1)
#ifdef ESCARGOT_64
    mutable struct {
        size_t m_hashData:63;
        bool m_isHashInited:1;
    } m_hashData;
#else
    mutable struct {
        size_t m_hashData:31;
        bool m_isHashInited:1;
    } m_hashData;
#endif
#pragma pack(pop)
};


class ESString : public ESPointer {
    friend class ESScriptParser;
protected:
    ESString(ESStringData* data)
        : ESPointer(Type::ESString)
    {
        m_string = data;
    }
    ESString(const char16_t* str)
        : ESPointer(Type::ESString)
    {
        m_string = new(GC) ESStringData(str);
    }

    ESString(u16string&& src)
        : ESPointer(Type::ESString)
    {
        m_string = new(GC) ESStringData(std::move(src));
    }

    ESString(int number)
        : ESPointer(Type::ESString)
    {
        m_string = new(GC) ESStringData(number);
    }

    ESString(double number)
        : ESPointer(Type::ESString)
    {
        m_string = new(GC) ESStringData(number);
    }

    ESString(char16_t number)
        : ESPointer(Type::ESString)
    {
        m_string = new(GC) ESStringData(number);
    }

    ESString(const char* str)
        : ESPointer(Type::ESString)
    {
        m_string = new(GC) ESStringData(str);
    }
public:
    static ESString* create(const char16_t* str)
    {
        return new ESString(str);
    }
    static ESString* create(u16string&& src)
    {
        return new ESString(std::move(src));
    }

    static ESString* create(int number)
    {
        return new ESString(number);
    }

    static ESString* create(double number)
    {
        return new ESString(number);
    }

    static ESString* create(char16_t c)
    {
        return new ESString(c);
    }

    static ESString* create(const char* str)
    {
        return new ESString(str);
    }

    ALWAYS_INLINE const char* utf8Data() const
    {
        return utf16ToUtf8(data());
    }

    template <typename Func>
    void wcharData(const Func& fn)
    {
        wchar_t* buf = (wchar_t *)alloca(sizeof(wchar_t) * (length()+1));
        for(unsigned i = 0 ; i < (unsigned)length() ; i ++) {
            buf[i] = data()[i];
        }
        buf[length()] = 0;

        fn(buf);
    }

    ALWAYS_INLINE const char16_t* data() const;
    ALWAYS_INLINE const u16string& string() const;
    ALWAYS_INLINE const ESStringData* stringData() const;
    ALWAYS_INLINE int length() const;

    ESString* substring(int from, int to) const
    {
        //NOTE to build normal string(for chain-string), we should call data();
        data();

        ASSERT(0 <= from && from <= to && to <= (int)m_string->length());
        u16string ret(std::move(m_string->substr(from, to-from)));
        return ESString::create(std::move(ret));
    }

    struct RegexMatchResult {
        struct RegexMatchResultPiece {
            unsigned m_start,m_end;
        };
        COMPILE_ASSERT((sizeof (RegexMatchResultPiece)) == (sizeof (unsigned) * 2),sizeof_RegexMatchResultPiece_wrong);
        int m_subPatternNum;
        std::vector< std::vector< RegexMatchResultPiece > > m_matchResults;
    };
    bool match(ESPointer* esptr, RegexMatchResult& result, bool testOnly = false) const;

    ESString(const ESString& s) = delete;
    void operator =(const ESString& s) = delete;

    ALWAYS_INLINE friend bool operator == (const ESString& a,const char16_t* b);
    ALWAYS_INLINE friend bool operator != (const ESString& a,const char16_t* b);
    ALWAYS_INLINE friend bool operator == (const ESString& a,const ESString& b);
    ALWAYS_INLINE friend bool operator < (const ESString& a,const ESString& b);
    ALWAYS_INLINE friend bool operator > (const ESString& a,const ESString& b);
    ALWAYS_INLINE friend bool operator <= (const ESString& a,const ESString& b);
    ALWAYS_INLINE friend bool operator >= (const ESString& a,const ESString& b);

#ifndef NDEBUG
    void show() const
    {
        printf("%s\n",utf8Data());
    }
#endif
protected:
    ESStringData* m_string;
};

ALWAYS_INLINE bool operator == (const ESString& a,const char16_t* b)
{
    return a.string() == b;
}

ALWAYS_INLINE bool operator != (const ESString& a,const char16_t* b)
{
    return a.string() != b;
}

ALWAYS_INLINE bool operator == (const ESString& a,const ESString& b)
{
    if(a.length() == b.length()) {
        if(a.stringData()->hashValue() == b.stringData()->hashValue()) {
            return a.string() == b.string();
        }
    }
    return false;
}

ALWAYS_INLINE bool operator < (const ESString& a,const ESString& b)
{
    return a.string() < b.string();
}

ALWAYS_INLINE bool operator > (const ESString& a,const ESString& b)
{
    return a.string() > b.string();
}

ALWAYS_INLINE bool operator <= (const ESString& a,const ESString& b)
{
    return a.string() <= b.string();
}

ALWAYS_INLINE bool operator >= (const ESString& a,const ESString& b)
{
    return a.string() >= b.string();
}

typedef std::vector<ESString *,gc_allocator<ESString *> > ESStringVector;
typedef std::vector<ESStringData *,gc_allocator<ESStringData *> > ESChainStringVector;

class ESChainString : public ESString {
protected:
    ESChainString()
        : ESString((ESStringData *)nullptr)
    {
        m_type = m_type | ESPointer::ESChainString;
        m_chainSize = 0;
        m_contentLength = 0;
    }
public:
    static const unsigned ESChainStringCreateMinLimit = 256;
    static const unsigned ESChainStringDefaultSize = 8;
    static const unsigned ESChainStringMaximumSize = 128;
    static ESChainString* create()
    {
        return new ESChainString();
    }
    void convertIntoNormalString()
    {
#ifndef NDEBUG
        ASSERT(m_type & ESPointer::ESChainString);
        if(m_string) {
            ASSERT(m_chainSize == 0);
        }
#endif
        u16string result;
        result.reserve(m_contentLength);
        for(unsigned i = 0; i < m_chainSize ; i ++) {
            result.append(static_cast<const u16string &>(*m_chain[i]));
            m_chain[i] = NULL;
        }

        m_string = new(GC) ESStringData(std::move(result));
        m_chainSize = 0;
        m_contentLength = 0;
    }

    void append(ESString* str)
    {
        if(m_string) {
            ASSERT(m_chainSize == 0);
            m_chain[m_chainSize++] = m_string;
            m_contentLength = m_string->length();
            m_string = NULL;
        }

        if(str->isESChainString()) {
            ESChainString* cs = str->asESChainString();
            if(cs->m_string) {
                if(m_chainSize + 1 >= m_chain.size()) {
                    size_t newSize  = m_chain.size() * 2;
                    if(newSize < ESChainStringDefaultSize)
                        newSize = ESChainStringDefaultSize;
                    m_chain.resize(newSize);
                }
                m_chain[m_chainSize++] = const_cast<ESStringData *>(str->stringData());
                m_contentLength += str->length();
            }
            else {
                if(m_chainSize + cs->m_chainSize >= m_chain.size()) {
                    size_t newSize  = (m_chain.size() + cs->m_chainSize) * 1.25f;
                    if(newSize < ESChainStringDefaultSize)
                        newSize = ESChainStringDefaultSize;
                    m_chain.resize(newSize);
                }
                for(unsigned i = 0; i < cs->m_chainSize ; i ++) {
                    m_chain[i + m_chainSize] = cs->m_chain[i];
                }
                m_chainSize += cs->m_chainSize;
                m_contentLength += cs->m_contentLength;
            }
        } else {
            if(m_chainSize + 1 >= m_chain.size()) {
                size_t newSize  = m_chain.size() * 2;
                if(newSize < ESChainStringDefaultSize)
                    newSize = ESChainStringDefaultSize;
                m_chain.resize(newSize);
            }
            m_chain[m_chainSize++] = const_cast<ESStringData *>(str->stringData());
            m_contentLength += str->length();
        }

        if(m_chainSize > ESChainStringMaximumSize) {
            convertIntoNormalString();
        }
    }

    int contentLength()
    {
        return m_contentLength;
    }

protected:
    ESChainStringVector m_chain;
    unsigned m_chainSize;
    unsigned m_contentLength;
};



ALWAYS_INLINE const char16_t* ESString::data() const
{
    if(UNLIKELY(m_string == NULL)) {
        const_cast<ESString *>(this)->asESChainString()->convertIntoNormalString();
    }
    return m_string->data();
}

ALWAYS_INLINE const u16string& ESString::string() const
{
    if(UNLIKELY(m_string == NULL)) {
        const_cast<ESString *>(this)->asESChainString()->convertIntoNormalString();
    }
    return static_cast<const u16string&>(*m_string);
}

ALWAYS_INLINE const ESStringData* ESString::stringData() const
{
    if(UNLIKELY(m_string == NULL)) {
        const_cast<ESString *>(this)->asESChainString()->convertIntoNormalString();
    }
    return m_string;
}

ALWAYS_INLINE int ESString::length() const
{
    if(UNLIKELY(m_string == NULL)) {
        escargot::ESChainString* chain = (escargot::ESChainString *)this;
        return chain->contentLength();
    }
    return m_string->length();
}

}

namespace std
{
template<> struct hash<escargot::ESString *>
{
    size_t operator()(escargot::ESString * const &x) const
    {
        return x->stringData()->hashValue();
    }
};

template<> struct equal_to<escargot::ESString *>
{
    bool operator()(escargot::ESString * const &a, escargot::ESString * const &b) const
    {
        if(a == b) {
            return true;
        }
        return a->string() == b->string();
    }
};

}


#include "runtime/InternalAtomicString.h"
#include "runtime/StaticStrings.h"

namespace escargot {

struct ESAccessorData : public gc {
public:
    std::function<ESValue (::escargot::ESObject* obj)> m_getter;
    std::function<void (::escargot::ESObject* obj, const ESValue& value)> m_setter;
};

class ESSlot {
public:
    ESSlot()
    {
        m_isWritable = true;
        m_isEnumerable = true;
        m_isConfigurable = true;
        m_isDataProperty = true;
    }

    ESSlot(const ::escargot::ESValue& value,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        m_data = ESValue(value);
        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = true;
    }

    ESSlot(::escargot::ESObject* object,
            std::function<ESValue (::escargot::ESObject* obj)> getter = nullptr,
            std::function<void (::escargot::ESObject* obj, const ESValue& value)> setter = nullptr,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        ESAccessorData* data = new ESAccessorData;
        data->m_getter = getter;
        data->m_setter = setter;
        m_data = ESValue((ESPointer *)data);

        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = false;
    }

    ESSlot(::escargot::ESObject* object,ESAccessorData* data,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        m_data = ESValue((ESPointer *)data);
        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = false;
    }

    friend class DeclarativeEnvironmentRecord;
    //DO NOT USE THIS FUNCITON
    void init(const ::escargot::ESValue& value,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        m_data = ESValue(value);
        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = true;
    }

    ALWAYS_INLINE void setValue(const ::escargot::ESValue& value, ::escargot::ESObject* object = NULL);

    ALWAYS_INLINE ESValue value(::escargot::ESObject* object = NULL) const;

    ALWAYS_INLINE void setDataProperty(const ::escargot::ESValue& value);

    ALWAYS_INLINE bool isConfigurable() const
    {
        return m_isConfigurable;
    }

    ALWAYS_INLINE bool isEnumerable() const
    {
        return m_isEnumerable;
    }

    ALWAYS_INLINE bool isWritable() const
    {
        return m_isWritable;
    }

    ALWAYS_INLINE bool isDataProperty() const
    {
        return m_isDataProperty;
    }

    ALWAYS_INLINE void setConfigurable(bool b)
    {
        m_isConfigurable = b;
    }

    ALWAYS_INLINE void setEnumerable(bool b)
    {
        m_isEnumerable = b;
    }

    ALWAYS_INLINE void setWritable(bool b)
    {
        m_isWritable = b;
    }

    ALWAYS_INLINE void setAsDataProperty()
    {
        m_isDataProperty = true;
        m_data = ESValue();
    }

    ALWAYS_INLINE ESAccessorData* accessorData() const
    {
        ASSERT(!m_isDataProperty);
        return (ESAccessorData *)m_data.asESPointer();
    }

    ALWAYS_INLINE ESValue* data()
    {
        return &m_data;
    }

protected:
#pragma pack(push, 1)
    bool m_isDataProperty:1;
    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-property-attributes
    bool m_isWritable:1;
    bool m_isEnumerable:1;
    bool m_isConfigurable:1;
#pragma pack(pop)

    ESValue m_data;
};

#ifdef ESCARGOT_64
ASSERT_STATIC(sizeof(ESSlot) == 2 * sizeof(void*), "sizeof(ESSlot) should be 16 bytes");
#else
ASSERT_STATIC(false, "sizeof(ESSlot) should be re-considered");
#endif

class ESSlotAccessor {
public:
    ESSlotAccessor()
    {
        m_data = nullptr;
    }

    explicit ESSlotAccessor(ESValue* data)
    {
        m_data = data;
        m_isDataProperty = true;
    }

    explicit ESSlotAccessor(ESSlot* slot)
    {
        if (slot) {
            if(slot->isDataProperty()) {
                m_data = slot->data();
                m_isDataProperty = true;
            } else {
                m_data = (ESValue *)slot->accessorData();
                m_isDataProperty = true;
            }

        } else {
            m_data = nullptr;
        }
    }

    explicit ESSlotAccessor(ESAccessorData* data)
    {
        m_isDataProperty = false;
        m_data = (ESValue *)data;
    }

    ALWAYS_INLINE bool hasData() const
    {
        return !!m_data;
    }

    ALWAYS_INLINE bool isDataProperty() const
    {
        return m_isDataProperty;
    }

    ALWAYS_INLINE ESAccessorData* accessorData()
    {
        ASSERT(!m_isDataProperty);
        return (ESAccessorData *)m_data;
    }

    ALWAYS_INLINE void setValue(const ::escargot::ESValue& value, ::escargot::ESObject* object = NULL)
    {
        ASSERT(m_data);
        if(LIKELY(m_isDataProperty)) {
            *m_data = value;
        } else {
            ASSERT(object);
            if(((ESAccessorData *)m_data)->m_setter) {
                ((ESAccessorData *)m_data)->m_setter(object, value);
            }
        }
    }

    ALWAYS_INLINE ESValue value(::escargot::ESObject* object = NULL) const
    {
        ASSERT(m_data);
        if(LIKELY(m_isDataProperty)) {
            return *m_data;
        } else {
            ASSERT(object);
            if(((ESAccessorData *)m_data)->m_getter) {
                return ((ESAccessorData *)m_data)->m_getter(object);
            }
            return ESValue();
        }
    }

    ALWAYS_INLINE const ESValue& readDataProperty() const
    {
        ASSERT(m_data);
        ASSERT(m_isDataProperty);
        return *m_data;
    }

    ALWAYS_INLINE void setDataProperty(const ::escargot::ESValue& value)
    {
        ASSERT(m_isDataProperty);
        *m_data = value;
    }

public:
    ESValue* m_data;
    bool m_isDataProperty;
};

struct ESHiddenClassPropertyInfo {
    ESHiddenClassPropertyInfo(bool isData, bool isWritable, bool isEnumerable, bool isConfigurable)
    {
        m_isDataProperty = isData;
        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
    }
#pragma pack(push, 1)
    bool m_isDataProperty:1;
    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-property-attributes
    bool m_isWritable:1;
    bool m_isEnumerable:1;
    bool m_isConfigurable:1;
#pragma pack(pop)
};

typedef std::unordered_map<ESString*, ::escargot::ESSlot,
                std::hash<ESString*>,std::equal_to<ESString*>,
                gc_allocator<std::pair<const ESString*, ::escargot::ESSlot> > > ESObjectMapStd;

typedef std::vector<std::pair<::escargot::ESString*, ::escargot::ESHiddenClassPropertyInfo>,
        gc_allocator<std::pair<::escargot::ESString*, ::escargot::ESHiddenClassPropertyInfo> > > ESHiddenClassDataStd;

typedef std::unordered_map<ESString*, std::pair<::escargot::ESHiddenClass *, ::escargot::ESHiddenClass *>,
                std::hash<ESString*>,std::equal_to<ESString*>,
                gc_allocator<std::pair<const ESString*, std::pair<::escargot::ESHiddenClass *, ::escargot::ESHiddenClass *> > > > ESHiddenClassTransitionDataStd;

typedef std::vector<::escargot::ESValue, gc_allocator<::escargot::ESValue> > ESObjectVectorStd;
typedef std::vector<::escargot::ESValue, gc_allocator<::escargot::ESValue> > ESVectorStd;

/*
typedef std::map<InternalString, ::escargot::ESSlot *,
            std::less<InternalString>,
            gc_allocator<std::pair<const InternalString, ::escargot::ESSlot *> > > ESObjectMapStd;
*/
class ESObjectMap : public ESObjectMapStd {
public:
    ESObjectMap(size_t siz)
        : ESObjectMapStd(siz) { }

};

class ESVector : public ESVectorStd {
public:
    ESVector(size_t siz)
        : ESVectorStd(siz) { }
};

class ESHiddenClass : public gc {
    static const unsigned ESHiddenClassSizeLimit = 24;
    friend class ESVMInstance;
    friend class ESObject;
public:
    size_t findProperty(ESString* name)
    {
        size_t siz = m_data.size();
        for(size_t i = 0; i < siz ; i ++) {
            if(m_data[i].first == name || *m_data[i].first == *name)
                return i;
        }

        return SIZE_MAX;
    }


private:
    ESHiddenClass()
        : m_transitionData(4)
    {

    }

    ALWAYS_INLINE std::pair<ESHiddenClass *, size_t> defineProperty(ESObject* obj, ESString* name, bool isData, bool isWritable, bool isEnumerable, bool isConfigurable);
    ALWAYS_INLINE ESValue readWithoutCheck(ESObject* obj, size_t idx);
    ALWAYS_INLINE ESValue read(ESObject* obj, ESString* name);
    ALWAYS_INLINE ESValue read(ESObject* obj, size_t idx);

    ESHiddenClassDataStd m_data;
    ESHiddenClassTransitionDataStd m_transitionData;
};

class ESObject : public ESPointer {
    friend class ESSlot;
    friend class ESHiddenClass;
protected:
    ESObject(ESPointer::Type type = ESPointer::Type::ESObject);
public:

    //DO NOT USE THIS FUNCTION
    //NOTE rooted ESSlot has short life time.
    escargot::ESSlotAccessor definePropertyOrThrow(escargot::ESString* key, bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true)
    {
        if(UNLIKELY(m_map != NULL)){
            auto iter = m_map->find(key);

            if(iter == m_map->end()) {
                return ESSlotAccessor(&m_map->insert(std::make_pair(key, ::escargot::ESSlot(ESValue(), isWritable, isEnumerable, isConfigurable))).first->second);
            } else {
            }
            ASSERT(iter->second.isDataProperty());
            return ESSlotAccessor(&iter->second);
        } else {
            std::pair<ESHiddenClass *, size_t> ret = m_hiddenClass->defineProperty(this, key, true, isWritable, isEnumerable, isConfigurable);
            m_hiddenClass = ret.first;
            if(m_hiddenClass->m_data.size() > ESHiddenClass::ESHiddenClassSizeLimit) {
                convertIntoMapMode();
                return definePropertyOrThrow(key, isWritable, isEnumerable, isConfigurable);
            }
            if(m_hiddenClass->m_data[ret.second].second.m_isDataProperty) {
                return escargot::ESSlotAccessor(&m_hiddenClassData[ret.second]);
            } else {
                return escargot::ESSlotAccessor((ESAccessorData *)m_hiddenClassData[ret.second].asESPointer());
            }
        }
    }

    void defineAccessorProperty(escargot::ESString* key,std::function<ESValue (::escargot::ESObject* obj)> getter = nullptr,
            std::function<void (::escargot::ESObject* obj, const ESValue& value)> setter = nullptr,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        if(UNLIKELY(m_map != NULL)){
            auto iter = m_map->find(key);
            if(iter != m_map->end()) {
                m_map->erase(iter);
            }
            m_map->insert(std::make_pair(key, escargot::ESSlot(this, getter, setter, isWritable, isEnumerable, isConfigurable)));
        } else {
            std::pair<ESHiddenClass *, size_t> ret = m_hiddenClass->defineProperty(this, key, false, isWritable, isEnumerable, isConfigurable);
            m_hiddenClass = ret.first;
            ASSERT(!m_hiddenClass->m_data[ret.second].second.m_isDataProperty);
            ESAccessorData* data = new ESAccessorData();
            data->m_getter = getter;
            data->m_setter = setter;
            m_hiddenClassData[ret.second] = ESValue((ESPointer *)data);
        }
    }

    void defineAccessorProperty(escargot::ESString* key,ESAccessorData* data,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        if(UNLIKELY(m_map != NULL)){
            auto iter = m_map->find(key);
            if(iter != m_map->end()) {
                m_map->erase(iter);
            }
            m_map->insert(std::make_pair(key, escargot::ESSlot(this, data, isWritable, isEnumerable, isConfigurable)));
        } else {
            std::pair<ESHiddenClass *, size_t> ret = m_hiddenClass->defineProperty(this, key, false, isWritable, isEnumerable, isConfigurable);
            m_hiddenClass = ret.first;
            ASSERT(!m_hiddenClass->m_data[ret.second].second.m_isDataProperty);
            m_hiddenClassData[ret.second] = ESValue((ESPointer *)data);
        }
    }

    bool hasOwnProperty(escargot::ESString* key) {
        if(UNLIKELY(m_map != NULL)){
            auto iter = m_map->find(key);
            if(iter == m_map->end())
                return false;
            return true;
        } else {
            return m_hiddenClass->findProperty(key) != SIZE_MAX;
        }
    }

    //$6.1.7.2 Object Internal Methods and Internal Slots
    bool isExtensible() {
        return true;
    }

    bool isHiddenClassMode()
    {
        return !m_map;
    }

    ESHiddenClass* hiddenClass()
    {
        return m_hiddenClass;
    }

    static ESObject* create()
    {
        return new ESObject();
    }

    ESSlotAccessor readHiddenClass(size_t idx)
    {
        ASSERT(!m_map);
        if(LIKELY(m_hiddenClass->m_data[idx].second.m_isDataProperty))
            return ESSlotAccessor(&m_hiddenClassData[idx]);
        else
            return ESSlotAccessor((ESAccessorData *)m_hiddenClassData[idx].asESPointer());
    }

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-get-o-p
    ESValue get(escargot::ESString* key, bool searchPrototype = false)
    {
        ESSlotAccessor ac = find(key, searchPrototype);
        if(LIKELY(ac.hasData())) {
            return ac.value(this);
        } else {
            return ESValue();
        }
    }

    //DO NOT USE THIS FUNCTION
    //NOTE rooted ESSlot has short life time.
    escargot::ESSlotAccessor find(escargot::ESString* key, bool searchPrototype = false)
    {
        if(UNLIKELY(m_map != NULL)){
            auto iter = m_map->find(key);
            if(iter == m_map->end()) {
                if(searchPrototype) {
                    if(UNLIKELY(this->m___proto__.isUndefined())) {
                        return ESSlotAccessor();
                    }
                    ESObject* target = this->m___proto__.asESPointer()->asESObject();
                    while(true) {
                        escargot::ESSlotAccessor s = target->find(key, false);
                        if (s.hasData())
                            return s;
                        ESValue proto = target->__proto__();
                        if (proto.isESPointer() && proto.asESPointer()->isESObject()) {
                            target = proto.asESPointer()->asESObject();
                        } else {
                            break;
                        }
                    }
                } else
                    return escargot::ESSlotAccessor();
            }
            return escargot::ESSlotAccessor(&iter->second);
        } else {
            size_t idx = m_hiddenClass->findProperty(key);
            if(idx == SIZE_MAX) {
                if(searchPrototype) {
                    if(UNLIKELY(this->m___proto__.isUndefined())) {
                        return ESSlotAccessor();
                    }
                    ESObject* target = this->m___proto__.asESPointer()->asESObject();
                    while(true) {
                        escargot::ESSlotAccessor s = target->find(key, false);
                        if (s.hasData())
                            return s;
                        ESValue proto = target->__proto__();
                        if (proto.isESPointer() && proto.asESPointer()->isESObject()) {
                            target = proto.asESPointer()->asESObject();
                        } else {
                            break;
                        }
                    }
                    return ESSlotAccessor();
                } else {
                    return ESSlotAccessor();
                }
            } else {
                if(LIKELY(m_hiddenClass->m_data[idx].second.m_isDataProperty))
                    return ESSlotAccessor(&m_hiddenClassData[idx]);
                else
                    return ESSlotAccessor((ESAccessorData *)m_hiddenClassData[idx].asESPointer());
            }
        }
    }

    //NOTE rooted ESSlot has short life time.
    escargot::ESSlotAccessor findOnlyPrototype(escargot::ESString* key)
    {
        if(UNLIKELY(m_map != NULL)){
            auto iter = m_map->find(key);
            if(iter == m_map->end()) {
                if(UNLIKELY(this->m___proto__.isUndefined())) {
                    return ESSlotAccessor();
                }
                ESObject* target = this->m___proto__.asESPointer()->asESObject();
                while(true) {
                    escargot::ESSlotAccessor s = target->find(key, false);
                    if (s.hasData())
                        return s;
                    ESValue proto = target->__proto__();
                    if (proto.isESPointer() && proto.asESPointer()->isESObject()) {
                        target = proto.asESPointer()->asESObject();
                    } else {
                        break;
                    }
                }
            }
            return escargot::ESSlotAccessor(&iter->second);
        } else {
            if(UNLIKELY(this->m___proto__.isUndefined())) {
                return ESSlotAccessor();
            }
            ESObject* target = this->m___proto__.asESPointer()->asESObject();
            while(true) {
                escargot::ESSlotAccessor s = target->find(key, false);
                if (s.hasData())
                    return s;
                ESValue proto = target->__proto__();
                if (proto.isESPointer() && proto.asESPointer()->isESObject()) {
                    target = proto.asESPointer()->asESObject();
                } else {
                    break;
                }
            }

            return escargot::ESSlotAccessor();
        }

    }

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-set-o-p-v-throw
    void set(escargot::ESString* key, const ESValue& val, bool shouldThrowException = false)
    {
        if(UNLIKELY(m_map != NULL)){
            //TODO Assert: IsPropertyKey(P) is true.
            //TODO Assert: Type(Throw) is ESBoolean.
            //TODO shouldThrowException
            auto iter = m_map->find(key);
            if(iter == m_map->end()) {
                //TODO set flags
                m_map->insert(std::make_pair(key, escargot::ESSlot(val, true, true, true)));
            } else {
                iter->second.setValue(val, this);
            }
        } else {
            //TODO is this flags right?
            std::pair<ESHiddenClass *, size_t> ret = m_hiddenClass->defineProperty(this, key, true, true, true, true);
            m_hiddenClass = ret.first;
            if(m_hiddenClass->m_data[ret.second].second.m_isDataProperty) {
                m_hiddenClassData[ret.second] = val;
            } else {
                ESAccessorData* data = ((ESAccessorData *)m_hiddenClassData[ret.second].asESPointer());
                if(data->m_setter)
                    data->m_setter(this, val);
            }
        }

    }

    //FUNCTION FOR DEBUG.
    template <typename Functor>
    void enumeration(Functor t)
    {
        if(UNLIKELY(m_map != NULL)){
            auto iter = m_map->begin();
            while(iter != m_map->end()) {
                if(iter->second.isEnumerable()) {
                    t((*iter).first,ESSlotAccessor(&(*iter).second));
                }
                iter++;
            }
        } else {
            for(unsigned i = 0; i < m_hiddenClass->m_data.size() ; i ++) {
                if(m_hiddenClass->m_data[i].second.m_isEnumerable) {
                    t(m_hiddenClass->m_data[i].first, find(m_hiddenClass->m_data[i].first, false));
                }
            }
        }
    }

    ALWAYS_INLINE ESValue __proto__()
    {
        return m___proto__;
    }

    ALWAYS_INLINE void set__proto__(const ESValue& obj)
    {
        m___proto__ = obj;
    }

    ALWAYS_INLINE ESValue constructor()
    {
        return get(strings->constructor);
    }

    ALWAYS_INLINE void setConstructor(const ESValue& obj)
    {
        set(strings->constructor, obj);
    }

    bool hasValueOf()
    {
        if(isESArrayObject())
            return false;
        else
            return true;
    }

    ESValue valueOf();

    void convertIntoMapMode()
    {
        if(!m_map) {
            m_map = new(GC) ESObjectMap(GC);
            for(size_t i = 0; i < m_hiddenClass->m_data.size() ; i ++) {
                if(m_hiddenClass->m_data[i].second.m_isDataProperty) {
                    m_map->insert(std::make_pair(m_hiddenClass->m_data[i].first,
                            ESSlot(
                                    m_hiddenClassData[i],
                                    m_hiddenClass->m_data[i].second.m_isWritable,
                                    m_hiddenClass->m_data[i].second.m_isEnumerable,
                                    m_hiddenClass->m_data[i].second.m_isConfigurable
                                    )
                            ));
                } else {
                    m_map->insert(std::make_pair(m_hiddenClass->m_data[i].first,
                            ESSlot(this,
                                    (ESAccessorData *)m_hiddenClassData[i].asESPointer(),
                                    m_hiddenClass->m_data[i].second.m_isWritable,
                                    m_hiddenClass->m_data[i].second.m_isEnumerable,
                                    m_hiddenClass->m_data[i].second.m_isConfigurable
                                    )
                            ));
                }
            }
            m_hiddenClassData.clear();
            m_hiddenClass = nullptr;
        }
    }

protected:
    ESObjectMap* m_map;
    ESHiddenClass* m_hiddenClass;
    ESObjectVectorStd m_hiddenClassData;

    ESValue m___proto__;
};

ALWAYS_INLINE std::pair<ESHiddenClass *, size_t> ESHiddenClass::defineProperty(ESObject* obj, ESString* name, bool isData, bool isWritable, bool isEnumerable, bool isConfigurable)
{
    size_t idx = findProperty(name);
    if(idx == SIZE_MAX) {
        ESHiddenClass* cls;
        auto iter = m_transitionData.find(name);
        if(iter == m_transitionData.end()) {
            if(isData) {
                cls = new ESHiddenClass;
                cls->m_data = m_data;
                cls->m_data.push_back(std::make_pair(name, ESHiddenClassPropertyInfo(isData, isWritable, isEnumerable, isConfigurable)));
                m_transitionData.insert(std::make_pair(name, std::make_pair(cls, nullptr)));
                //printf("%d %s\n",(int)m_data.size(), name->utf8Data());
            } else {
                cls = new ESHiddenClass;
                cls->m_data = m_data;
                cls->m_data.push_back(std::make_pair(name, ESHiddenClassPropertyInfo(isData, isWritable, isEnumerable, isConfigurable)));
                m_transitionData.insert(std::make_pair(name, std::make_pair(nullptr, cls)));
                //printf("%d %s\n",(int)m_data.size(), name->utf8Data());
            }
        } else {
            if(isData) {
                if(iter->second.first) {
                    cls = iter->second.first;
                } else {
                    cls = new ESHiddenClass;
                    cls->m_data = m_data;
                    cls->m_data.push_back(std::make_pair(name, ESHiddenClassPropertyInfo(isData, isWritable, isEnumerable, isConfigurable)));
                    iter->second.first = cls;
                    //printf("%d %s\n",(int)m_data.size(), name->utf8Data());
                }
            } else {
                if(iter->second.second) {
                    cls = iter->second.second;
                } else {
                    cls = new ESHiddenClass;
                    cls->m_data = m_data;
                    cls->m_data.push_back(std::make_pair(name, ESHiddenClassPropertyInfo(isData, isWritable, isEnumerable, isConfigurable)));
                    iter->second.second = cls;
                    //printf("%d %s\n",(int)m_data.size(), name->utf8Data());
                }
            }
        }
        obj->m_hiddenClassData.resize(cls->m_data.size());
        return std::make_pair(cls, cls->m_data.size() - 1);
    } else {
        return std::make_pair(this, idx);
    }
}

ALWAYS_INLINE ESValue ESHiddenClass::readWithoutCheck(ESObject* obj, size_t idx)
{
    if(LIKELY(m_data[idx].second.m_isDataProperty)) {
        return obj->m_hiddenClassData[idx];
    } else {
        ESAccessorData* ac = (ESAccessorData *)obj->m_hiddenClassData[idx].asESPointer();
        return ac->m_getter(obj);
    }
}

ALWAYS_INLINE ESValue ESHiddenClass::read(ESObject* obj, ESString* name)
{
    return read(obj, findProperty(name));
}

ALWAYS_INLINE ESValue ESHiddenClass::read(ESObject* obj, size_t idx)
{
    if(idx < m_data.size()) {
        if(LIKELY(m_data[idx].second.m_isDataProperty)) {
            return obj->m_hiddenClassData[idx];
        } else {
            ESAccessorData* ac = (ESAccessorData *)obj->m_hiddenClassData[idx].asESPointer();
            return ac->m_getter(obj);
        }
    } else {
        return ESValue();
    }
}

class ESErrorObject : public ESObject {
protected:
    ESErrorObject(escargot::ESString* message);
public:
    static ESErrorObject* create(escargot::ESString* message = strings->emptyESString)
    {
        return new ESErrorObject(message);
    }

    escargot::ESString* message() { return m_message; }

protected:
    escargot::ESString* m_message;
};

class ReferenceError : public ESErrorObject {
public:
    ReferenceError(escargot::ESString* message = strings->emptyESString)
        : ESErrorObject(message)
    {
    }
};

class TypeError : public ESErrorObject {
public:
    TypeError(escargot::ESString* message = strings->emptyESString)
        : ESErrorObject(message)
    {
    }
};

class SyntaxError : public ESErrorObject {
public:
    SyntaxError(escargot::ESString* message = strings->emptyESString)
        : ESErrorObject(message)
    {
    }
};

class RangeError : public ESErrorObject {
public:
    RangeError(escargot::ESString* message = strings->emptyESString)
        : ESErrorObject(message)
    {
    }
};

class ESDateObject : public ESObject {
protected:
    ESDateObject(ESPointer::Type type = ESPointer::Type::ESDateObject)
           : ESObject((Type)(Type::ESObject | Type::ESDateObject)) {}

public:
    static ESDateObject* create()
    {
        return new ESDateObject();
    }

    static ESDateObject* create(ESObject* proto)
    {
        //TODO
        ESDateObject* date = new ESDateObject();
        if(proto != NULL)
            date->set__proto__(proto);
        return date;
    }

    void parseStringToDate(struct tm* timeinfo, escargot::ESString* istr);

    void setTimeValue(ESValue str);

    double getTimeAsMilisec() {
        return m_time.tv_sec*1000 + floor(m_time.tv_nsec/1000000);
    }

    int getDate();
    int getDay();
    int getFullYear();
    int getHours();
    int getMinutes();
    int getMonth();
    int getSeconds();
    int getTimezoneOffset();
    void setTime(double t);

    ESValue valueOf()
    {
        return ESValue(getTimeAsMilisec());
    }

private:
    struct timespec m_time;
};

class ESArrayObject : public ESObject {
protected:
    ESArrayObject();
public:
    static ESArrayObject* create()
    {
        return ESArrayObject::create(0);
    }

    // $9.4.2.2
    static ESArrayObject* create(int length, ESObject* proto = NULL)
    {
        //TODO
        ESArrayObject* arr = new ESArrayObject();
        if (length == -1)
            arr->convertToSlowMode();
        else
            arr->setLength(length);
        //if(proto == NULL)
        //    proto = global->arrayPrototype();
        if(proto != NULL)
            arr->set__proto__(proto);
        return arr;
    }

    void set(escargot::ESString* key, ESValue val, bool shouldThrowException = false)
    {
        ESObject::set(key, val, shouldThrowException);
    }

    //DO NOT USE RETURN VALUE OF THIS FUNCTION
    escargot::ESSlotAccessor definePropertyOrThrow(ESValue key, bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true)
    {
        int i;
        if (key.isInt32()) {
            i = key.asInt32();
            int len = length().asInt32();
            if (i == len && m_fastmode) {
                setLength(len+1);
            }
            else if (i >= len) {
                if (shouldConvertToSlowMode(i)) convertToSlowMode();
                setLength(i+1);
            }
            if (m_fastmode)
                return ESSlotAccessor(&m_vector[i]);
        }
        return ESObject::definePropertyOrThrow(key.toString(), isWritable, isEnumerable, isConfigurable);
    }

    void set(ESValue key, const ESValue& val, bool shouldThrowException = false)
    {
        int i;
        if (key.isInt32()) {
            i = key.asInt32();
            int len = length().asInt32();
            if (i == len && m_fastmode) {
                setLength(len+1);
            }
            else if (i >= len) {
                if (shouldConvertToSlowMode(i)) convertToSlowMode();
                setLength(i+1);
            }
            if (m_fastmode) {
                m_vector[i] = val;
                return;
            }
        }
        ESObject::set(key.toString(), val, shouldThrowException);
    }

    void set(int i, const ESValue& val, bool shouldThrowException = false)
    {
        int len = length().asInt32();
        if (i == len && m_fastmode) {
            setLength(len+1);
        }
        else if (i >= len) {
            if (shouldConvertToSlowMode(i)) convertToSlowMode();
            setLength(i+1);
        }
        if (m_fastmode) {
            m_vector[i] = val;
        } else {
            ESObject::set(ESValue(i).toString(), val, shouldThrowException);
        }
    }

    ESValue get(int key)
    {
        if (m_fastmode) {
            if(key >= 0 && key < m_length)
                return m_vector[key];
            else
                return ESValue();
        }
        return ESObject::get(ESValue(key).toString());
    }

    ESValue get(ESValue key, bool searchPrototype = false)
    {
        if (m_fastmode && key.isInt32()) {
            int idx = key.asInt32();
            if(LIKELY(idx >= 0 && idx < m_length))
                return m_vector[idx];
            else
                return ESValue();
        }
        return ESObject::get(key.toString(), searchPrototype);
    }

    //DO NOT USE THIS FUNCTION
    escargot::ESSlotAccessor findOnlyIndex(int key)
    {
        if (LIKELY(m_fastmode && key >= 0 && key < m_length)) {
            return ESSlotAccessor(&m_vector[key]);
        }
        return ESSlotAccessor();
    }

    //DO NOT USE THIS FUNCTION
    escargot::ESSlotAccessor find(ESValue key)
    {
        if (m_fastmode && key.isInt32()) {
            int idx = key.asInt32();
            if(LIKELY(idx >= 0 && idx < m_length))
                return ESSlotAccessor(&m_vector[idx]);
            else
                return ESSlotAccessor();
        }
        return ESSlotAccessor(ESObject::find(key.toString()));
    }

    bool hasOwnProperty(escargot::ESString* key) {
        if (!m_fastmode)
            return ESObject::hasOwnProperty(key);

        wchar_t *end;
        const wchar_t *bufAddress;
        long key_int;
        key->wcharData([&](const wchar_t* buf){
            bufAddress = buf;
            key_int = wcstol(buf, &end, 10);
        });

        if (end != bufAddress + key->length())
            return ESObject::hasOwnProperty(key);
        if (key_int < m_length)
            return true;
        return false;
    }

    template <typename Functor>
    void enumeration(Functor t)
    {
        if (m_fastmode) {
            for (int i = 0; i < m_length; i++) {
                //FIXME: check if index i exists or not
                t(ESValue(i), ESSlotAccessor(&m_vector[i]));
            }
        }
    }

    void push(const ESValue& val)
    {
        set(m_length, val);
    }

    void insertValue(int idx, const ESValue& val)
    {
        if (m_fastmode) {
            m_vector.insert(m_vector.begin()+idx, val);
            setLength(length().asInt32() + 1);
        } else {
            // TODO
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    void eraseValues(int idx, int cnt)
    {
        if (m_fastmode) {
            m_vector.erase(m_vector.begin()+idx, m_vector.begin()+idx+cnt);
        } else {
            for (int k = 0, i = idx; i < length().asInt32() && k < cnt; i++, k++) {
                set(i, get(i+cnt));
            }
        }
        setLength(length().asInt32() - cnt);
    }

    bool shouldConvertToSlowMode(int i) {
        if (m_fastmode && i > MAX_FASTMODE_SIZE)
            return true;
        return false;
    }

    void convertToSlowMode()
    {
        //wprintf(L"CONVERT TO SLOW MODE!!!  \n");
        m_fastmode = false;
        int len = length().asInt32();
        if (len == 0) return;

        for (int i = 0; i < len; i++) {
            ESObject::set(ESValue(i).toString(), m_vector[i]);
        }
        m_vector.clear();
    }

    void setLength(ESValue len)
    {
        ASSERT(len.isInt32());
        int32_t newLength = len.asInt32();
        if (len.asInt32() < m_length) {
            //TODO : delete elements
        } else if (m_fastmode && newLength > m_length) {
            m_vector.resize(newLength);
        }
        m_length = newLength;
    }

    void setLength(int len)
    {
        ESValue length = ESValue(len);
        setLength(length);
    }

    bool isFastmode()
    {
        return m_fastmode;
    }

    ESValue length()
    {
        return ESValue(m_length);
    }

    void sort()
    {
        RELEASE_ASSERT(isFastmode());
        //TODO non fast mode sort

        std::sort(m_vector.begin(), m_vector.end(), [](const ::escargot::ESValue& a, const ::escargot::ESValue& b) -> bool {
            ::escargot::ESString* vala = a.toString();
            ::escargot::ESString* valb = b.toString();
            return vala->string() < valb->string();
        });
    }

    template <typename Comp>
    void sort(const Comp& c)
    {
        RELEASE_ASSERT(isFastmode());
        //TODO non fast mode sort

        std::sort(m_vector.begin(), m_vector.end(),c);
    }


protected:
    int32_t m_length;
    ESVector m_vector;
    bool m_fastmode;
    static const int MAX_FASTMODE_SIZE = 65536;
};

class LexicalEnvironment;
class Node;
class ESFunctionObject : public ESObject {
protected:
    ESFunctionObject(LexicalEnvironment* outerEnvironment, FunctionNode* functionAST, ESObject* proto);
public:
    static ESFunctionObject* create(LexicalEnvironment* outerEnvironment, FunctionNode* functionAST, ESObject* proto = NULL)
    {
        ESFunctionObject* ret = new ESFunctionObject(outerEnvironment, functionAST, proto);
        return ret;
    }

    ALWAYS_INLINE ESValue protoType()
    {
        return m_protoType;
    }

    ALWAYS_INLINE void setProtoType(ESValue obj)
    {
        m_protoType = obj;
    }

    FunctionNode* functionAST() { return m_functionAST; }
    LexicalEnvironment* outerEnvironment() { return m_outerEnvironment; }

    static ESValue call(ESValue callee, ESValue receiver, ESValue arguments[], size_t argumentCount, ESVMInstance* ESVMInstance, bool isNewExpression = false);
protected:
    LexicalEnvironment* m_outerEnvironment;
    FunctionNode* m_functionAST;
    ESValue m_protoType;
    //ESObject functionObject;
    //HomeObject
    ////ESObject newTarget
    //BindThisValue(V);
    //GetThisBinding();
};

class ESStringObject : public ESObject {
protected:
    ESStringObject(escargot::ESString* str);
public:
    static ESStringObject* create(escargot::ESString* str)
    {
        return new ESStringObject(str);
    }

    static ESStringObject* create()
    {
        return new ESStringObject(strings->emptyESString);
    }

    ALWAYS_INLINE ::escargot::ESString* getStringData()
    {
        return m_stringData;
    }

    ALWAYS_INLINE void setString(::escargot::ESString* str)
    {
        m_stringData = str;
    }

    ESValue valueOf()
    {
        return ESValue(m_stringData);
    }

private:
    ::escargot::ESString* m_stringData;
};

class ESNumberObject : public ESObject {
protected:
    ESNumberObject(double value)
        : ESObject((Type)(Type::ESObject | Type::ESNumberObject))
    {
        m_primitiveValue = value;
    }

public:
    static ESNumberObject* create(double value)
    {
        return new ESNumberObject(value);
    }

    ALWAYS_INLINE double numberData() { return m_primitiveValue; }
    ALWAYS_INLINE void setNumberData(double d)
    {
        m_primitiveValue = d;
    }

private:
    double m_primitiveValue;
};

class ESBooleanObject : public ESObject {
protected:
    ESBooleanObject(const ESValue& value)
        : ESObject((Type)(Type::ESObject | Type::ESBooleanObject))
    {
        m_primitiveValue = value;
    }

public:
    static ESBooleanObject* create(const ESValue& value)
    {
        return new ESBooleanObject(value);
    }

    void setBooleanData(const ESValue& value) { m_primitiveValue = value; }
    ALWAYS_INLINE ESValue booleanData() { return m_primitiveValue; }

private:
    ESValue m_primitiveValue;
};

class ESRegExpObject : public ESObject {
    friend class ESString;
public:
    enum Option {
        None = 0,
        Global = 1,
        IgnoreCase = 1 << 1,
        MultiLine = 1 << 2,
        Sticky = 1 << 3,
    };
    static ESRegExpObject* create(escargot::ESString* source, const Option& option, ESObject* proto = NULL)
    {
        ESRegExpObject* ret = new ESRegExpObject(source, option);
        if (proto != NULL)
            ret->set__proto__(proto);

        return ret;
    }

    ALWAYS_INLINE Option option() { return m_option; }
    ALWAYS_INLINE const escargot::ESString* source() { return m_source; }
    void setSource(escargot::ESString* src);
    void setOption(const Option& option);

    JSC::Yarr::BytecodePattern* bytecodePattern()
    {
        return m_bytecodePattern;
    }

private:
    void setBytecodePattern(JSC::Yarr::BytecodePattern* pattern)
    {
        m_bytecodePattern = pattern;
    }
    ESRegExpObject(escargot::ESString* source, const Option& option);

    escargot::ESString* m_source;
    JSC::Yarr::BytecodePattern* m_bytecodePattern;
    Option m_option;
};

}

#include "ESValueInlines.h"

#endif
