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
class ESRopeString;
class ESObject;
class ESSlot;
class ESHiddenClass;
class ESFunctionObject;
class ESArrayObject;
class ESStringObject;
class ESErrorObject;
class ESDateObject;
class ESVMInstance;
class ESPointer;
class ESRegExpObject;
// ES6 Typed Array
class ESArrayBufferObject;
class ESArrayBufferView;
template<typename TypeArg>
class ESTypedArrayObject;
class ESTypedArrayObjectWrapper;
class ESDataViewObject;

class CodeBlock;
typedef ESValue (*NativeFunctionType)(ESVMInstance*);

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
    inline ESValue toPrimitive(PrimitiveTypeHint = PreferNumber) const; //$7.1.1 ToPrimitive
    inline bool toBoolean() const; //$7.1.2 ToBoolean
    inline double toNumber() const; //$7.1.3 ToNumber
    inline double toInteger() const; //$7.1.4 ToInteger
    inline int32_t toInt32() const; //$7.1.5 ToInt32
    inline ESString* toString() const; //$7.1.12 ToString
    inline ESObject* toObject() const; //$7.1.13 ToObject
    inline double toLength() const; //$7.1.15 ToLength

    inline ESObject* toFunctionReceiverObject() const;

    inline ESString* asESString() const;

    inline bool isESPointer() const;
    inline ESPointer* asESPointer() const;

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
        ESRopeString = 1 << 1,
        ESObject = 1 << 2,
        ESFunctionObject = 1 << 3,
        ESArrayObject = 1 << 4,
        ESStringObject = 1 << 5,
        ESErrorObject = 1 << 6,
        ESDateObject = 1 << 7,
        ESNumberObject = 1 << 8,
        ESRegExpObject = 1 << 9,
        ESBooleanObject = 1 << 10,
        ESArrayBufferObject = 1 << 11,
        ESArrayBufferView = 1 << 12,
        ESTypedArrayObject = 1 << 13,
        ESDataViewObject = 1 << 14,
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

    ALWAYS_INLINE bool isESRopeString() const
    {
        return m_type & Type::ESRopeString;
    }

    ALWAYS_INLINE ::escargot::ESRopeString* asESRopeString()
    {
#ifndef NDEBUG
        ASSERT(isESRopeString());
#endif
        return reinterpret_cast<::escargot::ESRopeString *>(this);
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

    ALWAYS_INLINE ::escargot::ESErrorObject* asESErrorObject()
    {
#ifndef NDEBUG
        ASSERT(isESErrorObject());
#endif
        return reinterpret_cast<::escargot::ESErrorObject *>(this);
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

    ALWAYS_INLINE bool isESArrayBufferObject() const
    {
        return m_type & Type::ESArrayBufferObject;
    }

    ALWAYS_INLINE ::escargot::ESArrayBufferObject* asESArrayBufferObject()
    {
#ifndef NDEBUG
        ASSERT(isESArrayBufferObject());
#endif
        return reinterpret_cast<::escargot::ESArrayBufferObject *>(this);
    }

    ALWAYS_INLINE bool isESArrayBufferView() const
    {
        return m_type & Type::ESArrayBufferView;
    }

    ALWAYS_INLINE ::escargot::ESArrayBufferView* asESArrayBufferView()
    {
#ifndef NDEBUG
        ASSERT(isESArrayBufferView());
#endif
        return reinterpret_cast<::escargot::ESArrayBufferView *>(this);
    }

    ALWAYS_INLINE bool isESTypedArrayObject() const
    {
        return m_type & Type::ESTypedArrayObject;
    }

    template<typename TypeArg>
    ALWAYS_INLINE ::escargot::ESTypedArrayObject<TypeArg>* asESTypedArrayObject()
    {
#ifndef NDEBUG
        ASSERT(isESTypedArrayObject());
#endif
        return reinterpret_cast<::escargot::ESTypedArrayObject<TypeArg> *>(this);
    }

    ALWAYS_INLINE ::escargot::ESTypedArrayObjectWrapper* asESTypedArrayObjectWrapper()
    {
#ifndef NDEBUG
        ASSERT(isESTypedArrayObject());
#endif
        return reinterpret_cast<::escargot::ESTypedArrayObjectWrapper *>(this);
    }

    ALWAYS_INLINE bool isESDataViewObject() const
    {
        return m_type & Type::ESDataViewObject;
    }

    ALWAYS_INLINE ::escargot::ESDataViewObject* asESDataViewObject()
    {
#ifndef NDEBUG
        ASSERT(isESDataViewObject());
#endif
        return reinterpret_cast<::escargot::ESDataViewObject *>(this);
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
    ESStringData(u16string&& src)
        : u16string(std::move(src))
    {
        m_hashData.m_isHashInited =  false;
    }
    ESStringData(std::u16string& src)
    {
        assign(src.begin(), src.end());
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

    ESString(u16string&& src)
        : ESPointer(Type::ESString)
    {
        m_string = new(GC) ESStringData(std::move(src));
    }

    ESString(std::u16string& src)
        : ESPointer(Type::ESString)
    {
        m_string = new(GC) ESStringData(src);
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
    static ESString* create(u16string&& src)
    {
        return new ESString(std::move(src));
    }

    static ESString* create(std::u16string& src)
    {
        return new ESString(src);
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

    static ESString* concatTwoStrings(ESString* lstr, ESString* rstr);

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

        fn(buf, length());
    }

    size_t tryToUseAsIndex()
    {
        const u16string& s = string();
        bool allOfCharIsDigit = true;
        unsigned number = 0;
        for(unsigned i = 0; i < s.length() ; i ++) {
            char16_t c = s[i];
            if(c < '0' || c > '9') {
                allOfCharIsDigit = false;
                break;
            } else {
                number = number*10 + (c-'0');
            }
        }
        if(allOfCharIsDigit) {
            return number;
        }
        return SIZE_MAX;
    }

    ALWAYS_INLINE void ensureNormalString() const;
    ALWAYS_INLINE const char16_t* data() const;
    ALWAYS_INLINE const u16string& string() const;
    ALWAYS_INLINE const ESStringData* stringData() const;
    ALWAYS_INLINE int length() const;

    ESString* substring(int from, int to) const;

    struct RegexMatchResult {
        struct RegexMatchResultPiece {
            unsigned m_start,m_end;
        };
        COMPILE_ASSERT((sizeof (RegexMatchResultPiece)) == (sizeof (unsigned) * 2),sizeof_RegexMatchResultPiece_wrong);
        int m_subPatternNum;
        std::vector< std::vector< RegexMatchResultPiece > > m_matchResults;
    };
    bool match(ESPointer* esptr, RegexMatchResult& result, bool testOnly = false, size_t startIndex = 0) const;

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

class ESRopeString : public ESString {
    friend class ESString;
protected:
    ESRopeString()
        : ESString((ESStringData *)nullptr)
    {
        m_type = m_type | ESPointer::ESRopeString;
        m_contentLength = 0;
    }
public:
    static const unsigned ESRopeStringCreateMinLimit = 256;
    static ESRopeString* create()
    {
        return new ESRopeString();
    }
    static ESRopeString* createAndConcat(ESString* lstr, ESString* rstr)
    {
        ESRopeString* rope = ESRopeString::create();
        rope->m_contentLength = lstr->length() + rstr->length();
        rope->m_left = lstr;
        rope->m_right = rstr;
        return rope;
    }
    ESStringData* stringData() {
#ifndef NDEBUG
        ASSERT(m_type & ESPointer::ESRopeString);
        if(m_string) {
            ASSERT(m_contentLength == 0);
        }
#endif
        u16string result;
        //TODO: should reduce unnecessary append operations in std::string::resize
        result.resize(m_contentLength);
        std::vector<ESString *> queue;
        queue.push_back(m_left);
        queue.push_back(m_right);
        int pos = m_contentLength;
        while (!queue.empty()) {
            ESString* cur = queue.back();
            queue.pop_back();
            if (cur && cur->isESRopeString() && cur->asESRopeString()->m_contentLength != 0) {
                ESRopeString* rs = cur->asESRopeString();
                queue.push_back(rs->m_left);
                queue.push_back(rs->m_right);
            } else {
                pos -= cur->length();
                memcpy((void*)(result.data() + pos), cur->data(), cur->length() * sizeof(char16_t));
            }
        }
        return new(GC) ESStringData(std::move(result));
    }
    void convertIntoNormalString()
    {
        m_string = stringData();
        m_left = nullptr;
        m_right = nullptr;
        m_contentLength = 0;
    }
public:
    int contentLength()
    {
        return m_contentLength;
    }

protected:
    ESString* m_left;
    ESString* m_right;
    unsigned m_contentLength;
};

ALWAYS_INLINE ESString* ESString::concatTwoStrings(ESString* lstr, ESString* rstr)
{
    int llen = lstr->length();
    if (llen == 0)
        return rstr;
    int rlen = rstr->length();
    if (rlen == 0)
        return lstr;

    if (UNLIKELY(llen + rlen >= (int)ESRopeString::ESRopeStringCreateMinLimit)) {
        return ESRopeString::createAndConcat(lstr, rstr);
    } else {
        u16string str;
        str.reserve(llen + rlen);
        str.append(lstr->string());
        str.append(rstr->string());
        return ESString::create(std::move(str));
    }
}

ALWAYS_INLINE void ESString::ensureNormalString() const
{
    if(UNLIKELY(m_string == NULL)) {
        const_cast<ESString *>(this)->asESRopeString()->convertIntoNormalString();
    }
}

ALWAYS_INLINE const char16_t* ESString::data() const
{
    if(UNLIKELY(m_string == NULL)) {
        ensureNormalString();
    }
    return m_string->data();
}

ALWAYS_INLINE const u16string& ESString::string() const
{
    if(UNLIKELY(m_string == NULL)) {
        ensureNormalString();
    }
    return static_cast<const u16string&>(*m_string);
}

ALWAYS_INLINE const ESStringData* ESString::stringData() const
{
    if(UNLIKELY(m_string == NULL)) {
        return const_cast<ESString *>(this)->asESRopeString()->stringData();
    }
    return m_string;
}

ALWAYS_INLINE int ESString::length() const
{
    if(UNLIKELY(m_string == NULL)) {
        escargot::ESRopeString* rope = (escargot::ESRopeString *)this;
        return rope->contentLength();
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

class ESAccessorData : public gc {
public:
    ESAccessorData()
    {
        m_getter = nullptr;
        m_setter = nullptr;
        m_jsGetter = nullptr;
        m_jsSetter = nullptr;
    }

    ALWAYS_INLINE ESValue value(::escargot::ESObject* obj);
    ALWAYS_INLINE void setValue(::escargot::ESObject* obj, const ESValue& value);

    void setSetter(void (*setter)(::escargot::ESObject* obj, const ESValue& value))
    {
        m_setter = setter;
        m_jsSetter = nullptr;
    }

    void setGetter(ESValue (*getter)(::escargot::ESObject* obj))
    {
        m_getter = getter;
        m_jsGetter = nullptr;
    }

    void setJSSetter(ESFunctionObject* setter)
    {
        m_setter = nullptr;
        m_jsSetter = setter;
    }

    void setJSGetter(ESFunctionObject* getter)
    {
        m_getter = nullptr;
        m_jsGetter = getter;
    }

protected:
    ESValue (*m_getter)(::escargot::ESObject* obj);
    void (*m_setter)(::escargot::ESObject* obj, const ESValue& value);
    ESFunctionObject* m_jsGetter;
    ESFunctionObject* m_jsSetter;
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

    ESSlot(ESValue (*getter)(::escargot::ESObject* obj) = nullptr,
            void (*setter)(::escargot::ESObject* obj, const ESValue& value) = nullptr,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        ESAccessorData* data = new ESAccessorData;
        data->setGetter(getter);
        data->setSetter(setter);
        m_data = ESValue((ESPointer *)data);

        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = false;
    }

    ESSlot(ESFunctionObject* getter = nullptr,
            ESFunctionObject* setter = nullptr,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        ESAccessorData* data = new ESAccessorData;
        data->setJSGetter(getter);
        data->setJSSetter(setter);
        m_data = ESValue((ESPointer *)data);

        m_isWritable = isWritable;
        m_isEnumerable = isEnumerable;
        m_isConfigurable = isConfigurable;
        m_isDataProperty = false;
    }

    ESSlot(ESAccessorData* data,
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
        m_propertyValue = ESValue(ESValue::ESEmptyValue);
        m_targetObject = NULL;
    }

    explicit ESSlotAccessor(ESValue* data)
    {
        m_propertyValue = ESValue((ESPointer *)data);
        m_targetObject = NULL;
    }

    explicit ESSlotAccessor(ESObject* obj, const ESValue& propertyValue)
    {
        m_propertyValue = propertyValue;
        m_targetObject = obj;
    }

    ALWAYS_INLINE bool hasData() const
    {
        return m_propertyValue != ESValue(ESValue::ESEmptyValue);
    }

    ALWAYS_INLINE void setValue(const ::escargot::ESValue& value);
    ALWAYS_INLINE ESValue value() const;

    ALWAYS_INLINE const ESValue& readDataProperty() const
    {
        ASSERT(hasData());
        ASSERT(m_targetObject == NULL);
        return *((ESValue *)m_propertyValue.asESPointer());
    }

    ALWAYS_INLINE void setDataProperty(const ::escargot::ESValue& value)
    {
        ASSERT(hasData());
        ASSERT(m_targetObject == NULL);
        *((ESValue *)m_propertyValue.asESPointer()) = value;
    }

    ALWAYS_INLINE void switchOwner(ESObject* obj)
    {
        if(m_targetObject)
            m_targetObject = obj;
    }

    ESValue* dataAddress()
    {
        ASSERT(hasData());
        ASSERT(m_targetObject == NULL);
        return ((ESValue *)m_propertyValue.asESPointer());
    }

public:
    ESObject* m_targetObject;
    ESValue m_propertyValue;
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

inline char assembleHidenClassPropertyInfoFlags(bool isData, bool isWritable, bool isEnumerable, bool isConfigurable)
{
    return isData | ((int)isWritable << 1) | ((int)isEnumerable << 2) | ((int)isConfigurable << 3);
}

typedef std::unordered_map<ESString*, ::escargot::ESSlot,
                std::hash<ESString*>,std::equal_to<ESString*>,
                gc_allocator<std::pair<const ESString*, ::escargot::ESSlot> > > ESObjectMapStd;

typedef std::unordered_map<::escargot::ESString*, size_t,
                std::hash<ESString*>,std::equal_to<ESString*>,
                gc_allocator< std::pair<const ::escargot::ESString*, size_t> > > ESHiddenClassPropertyInfoStd;

typedef std::vector<::escargot::ESHiddenClassPropertyInfo, gc_allocator<::escargot::ESHiddenClassPropertyInfo> > ESHiddenClassPropertyFlagInfoStd;

typedef std::unordered_map<ESString*, ::escargot::ESHiddenClass **,
                std::hash<ESString*>,std::equal_to<ESString*>,
                gc_allocator<std::pair<const ESString*, std::pair<::escargot::ESHiddenClass *, ::escargot::ESHiddenClass **> > > > ESHiddenClassTransitionDataStd;

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
    static const unsigned ESHiddenClassSizeLimit = 64;
    friend class ESVMInstance;
    friend class ESObject;
public:
    size_t findProperty(const ESString* name)
    {
        auto iter = m_propertyInfo.find(const_cast<ESString *>(name));
        if(iter == m_propertyInfo.end())
            return SIZE_MAX;
        return iter->second;
    }

private:
    ESHiddenClass()
        : m_transitionData(4)
    {

    }

    ALWAYS_INLINE size_t defineProperty(ESObject* obj, ESString* name, bool isData, bool isWritable, bool isEnumerable, bool isConfigurable);
    ALWAYS_INLINE ESValue readWithoutCheck(ESObject* obj, size_t idx);
    ALWAYS_INLINE ESValue read(ESObject* obj, ESString* name);
    ALWAYS_INLINE ESValue read(ESObject* obj, size_t idx);

    ESHiddenClassPropertyInfoStd m_propertyInfo;
    ESHiddenClassPropertyFlagInfoStd m_propertyFlagInfo;
    ESHiddenClassTransitionDataStd m_transitionData;
};

class ESObject : public ESPointer {
    friend class ESSlot;
    friend class ESHiddenClass;
protected:
    ESObject(ESPointer::Type type = ESPointer::Type::ESObject, size_t initialKeyCount = 6);
public:

    //DO NOT USE THIS FUNCTION
    //NOTE rooted ESSlot has short life time.
    inline escargot::ESSlotAccessor definePropertyOrThrow(escargot::ESValue key, bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true, const ESValue& initalValue = ESValue());
    void defineAccessorProperty(escargot::ESString* key,ESValue (*getter)(::escargot::ESObject* obj) = nullptr,
            void (*setter)(::escargot::ESObject* obj, const ESValue& value)  = nullptr,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        //TODO consider array
        if(UNLIKELY(m_map != NULL)){
            auto iter = m_map->find(key);
            if(iter != m_map->end()) {
                m_map->erase(iter);
            }
            m_map->insert(std::make_pair(key, escargot::ESSlot(getter, setter, isWritable, isEnumerable, isConfigurable)));
        } else {
            size_t ret = m_hiddenClass->defineProperty(this, key, false, isWritable, isEnumerable, isConfigurable);
            ASSERT(!m_hiddenClass->m_propertyFlagInfo[m_hiddenClass->findProperty(key)].m_isDataProperty);
            ESAccessorData* data = new ESAccessorData();
            data->setGetter(getter);
            data->setSetter(setter);
            m_hiddenClassData[ret] = ESValue((ESPointer *)data);
        }
    }

    void defineAccessorProperty(escargot::ESString* key,escargot::ESFunctionObject* getter = nullptr,
            escargot::ESFunctionObject* setter = nullptr,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        //TODO consider array
        if(UNLIKELY(m_map != NULL)){
            auto iter = m_map->find(key);
            if(iter != m_map->end()) {
                m_map->erase(iter);
            }
            m_map->insert(std::make_pair(key, escargot::ESSlot(getter, setter, isWritable, isEnumerable, isConfigurable)));
        } else {
            size_t ret = m_hiddenClass->defineProperty(this, key, false, isWritable, isEnumerable, isConfigurable);
            ASSERT(!m_hiddenClass->m_propertyFlagInfo[m_hiddenClass->findProperty(key)].m_isDataProperty);
            ESAccessorData* data = new ESAccessorData();
            data->setJSGetter(getter);
            data->setJSSetter(setter);
            m_hiddenClassData[ret] = ESValue((ESPointer *)data);
        }
    }

    void defineAccessorProperty(escargot::ESString* key,ESAccessorData* data,
            bool isWritable = false, bool isEnumerable = false, bool isConfigurable = false)
    {
        //TODO consider array
        if(UNLIKELY(m_map != NULL)){
            auto iter = m_map->find(key);
            if(iter != m_map->end()) {
                m_map->erase(iter);
            }
            m_map->insert(std::make_pair(key, escargot::ESSlot(data, isWritable, isEnumerable, isConfigurable)));
        } else {
            size_t ret = m_hiddenClass->defineProperty(this, key, false, isWritable, isEnumerable, isConfigurable);
            ASSERT(!m_hiddenClass->m_propertyFlagInfo[m_hiddenClass->findProperty(key)].m_isDataProperty);
            m_hiddenClassData[ret] = ESValue((ESPointer *)data);
        }
    }

    inline void deletePropety(const ESValue& key);
    inline void propertyFlags(const ESValue& key, bool& exists, bool& isDataProperty, bool& isWritable, bool& isEnumerable, bool& isConfigurable);
    inline ESAccessorData* accessorData(const ESValue& key);

    ALWAYS_INLINE bool hasOwnProperty(escargot::ESValue key);
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

    static ESObject* create(size_t initialKeyCount = 6)
    {
        return new ESObject(ESPointer::Type::ESObject, initialKeyCount);
    }

    ALWAYS_INLINE ESValue readHiddenClass(size_t idx);
    ALWAYS_INLINE void writeHiddenClass(size_t idx, const ESValue& value);

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-get-o-p
    ALWAYS_INLINE ESValue get(escargot::ESValue key, bool searchPrototype = false);
    inline escargot::ESValue find(const escargot::ESValue& key, bool searchPrototype = false, escargot::ESObject* realObj = nullptr);
    ALWAYS_INLINE escargot::ESValue findOnlyPrototype(escargot::ESValue key);

    //DO NOT USE THIS FUNCTION
    //NOTE rooted ESSlot has short life time.
    ESSlotAccessor addressOfProperty(escargot::ESValue key);

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-set-o-p-v-throw
    ALWAYS_INLINE void set(escargot::ESValue key, const ESValue& val, bool shouldThrowException = false);

    void set(escargot::ESString* key, const ESValue& val, bool shouldThrowException = false)
    {
        set(ESValue(key), val, shouldThrowException);
    }

    ALWAYS_INLINE const int32_t length();
    ALWAYS_INLINE ESValue pop();
    ALWAYS_INLINE void eraseValues(int, int);

    template <typename Functor>
    ALWAYS_INLINE void enumeration(Functor t);

    ALWAYS_INLINE ESValue __proto__()
    {
        return m___proto__;
    }

    ALWAYS_INLINE void set__proto__(const ESValue& obj)
    {
        ASSERT(obj.isObject() || obj.isUndefined());
        m___proto__ = obj;
    }

    ALWAYS_INLINE ESValue constructor()
    {
        return get(ESValue(strings->constructor));
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
        ASSERT(!m_map);
        if(!m_map) {
            m_map = new(GC) ESObjectMap(128);
            auto iter = m_hiddenClass->m_propertyInfo.begin();
            while(iter != m_hiddenClass->m_propertyInfo.end()) {
                size_t idx = iter->second;
                ASSERT(!hasOwnProperty(iter->first));
                if(m_hiddenClass->m_propertyFlagInfo[idx].m_isDataProperty) {
                    m_map->insert(std::make_pair(iter->first,
                        ESSlot(
                                m_hiddenClassData[idx],
                                m_hiddenClass->m_propertyFlagInfo[idx].m_isWritable,
                                m_hiddenClass->m_propertyFlagInfo[idx].m_isEnumerable,
                                m_hiddenClass->m_propertyFlagInfo[idx].m_isConfigurable
                                )
                        ));
                } else {
                    m_map->insert(std::make_pair(iter->first,
                            ESSlot((ESAccessorData *)m_hiddenClassData[idx].asESPointer(),
                                    m_hiddenClass->m_propertyFlagInfo[idx].m_isWritable,
                                    m_hiddenClass->m_propertyFlagInfo[idx].m_isEnumerable,
                                    m_hiddenClass->m_propertyFlagInfo[idx].m_isConfigurable
                                    )
                            ));
                }
                iter++;
            }
            m_hiddenClass = nullptr;
            m_hiddenClassData.clear();
        }
    }

    ALWAYS_INLINE size_t keyCount();
protected:
    ESObjectMap* m_map;
    ESHiddenClass* m_hiddenClass;
    ESObjectVectorStd m_hiddenClassData;

    ESValue m___proto__;
};

class ESErrorObject : public ESObject {
protected:
    ESErrorObject(escargot::ESString* message);
public:
    static ESErrorObject* create(escargot::ESString* message = strings->emptyESString)
    {
        return new ESErrorObject(message);
    }
protected:
};

class ReferenceError : public ESErrorObject {
protected:
    ReferenceError(escargot::ESString* message = strings->emptyESString);
public:
    static ReferenceError* create(escargot::ESString* message = strings->emptyESString)
    {
        return new ReferenceError(message);
    }
};

class TypeError : public ESErrorObject {
protected:
    TypeError(escargot::ESString* message = strings->emptyESString);
public:
    static TypeError* create(escargot::ESString* message = strings->emptyESString)
    {
        return new TypeError(message);
    }
};

class SyntaxError : public ESErrorObject {
protected:
    SyntaxError(escargot::ESString* message = strings->emptyESString);
public:

    static SyntaxError* create(escargot::ESString* message = strings->emptyESString)
    {
        return new SyntaxError(message);
    }
};

class RangeError : public ESErrorObject {
protected:
    RangeError(escargot::ESString* message = strings->emptyESString);
public:
    static RangeError* create(escargot::ESString* message = strings->emptyESString)
    {
        return new RangeError(message);
    }
};

class ESDateObject : public ESObject {
protected:
    ESDateObject(ESPointer::Type type = ESPointer::Type::ESDateObject)
           : ESObject((Type)(Type::ESObject | Type::ESDateObject)) {
        m_isCacheDirty = true;
    }

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
    void resolveCache();
    struct timespec m_time;
    struct tm m_cachedTM;
    bool m_isCacheDirty;
};

class ESJSONObject : public ESObject {
protected:
    ESJSONObject();
public:
    static ESJSONObject* create()
    {
        return new ESJSONObject();
    }
protected:
};

class ESArrayObject : public ESObject {
    friend class ESObject;
protected:
    ESArrayObject(int length);
public:
    static ESArrayObject* create()
    {
        return ESArrayObject::create(0);
    }

    // $9.4.2.2
    static ESArrayObject* create(int length, ESObject* proto = NULL)
    {
        //TODO
        ESArrayObject* arr = new ESArrayObject(length);
        //if(proto == NULL)
        //    proto = global->arrayPrototype();
        if(proto != NULL)
            arr->set__proto__(proto);
        return arr;
    }

    ESValue get(unsigned key)
    {
        if (m_fastmode) {
            if(key >= 0 && key < m_length)
                return m_vector[key];
            else
                return ESValue();
        }
        return ESObject::get(ESValue(key));
    }

    void push(const ESValue& val)
    {
        set(m_length, val);
    }

    ESValue fastPop()
    {
        ASSERT(isFastmode());
        if(m_length == 0)
            return ESValue();
        ESValue ret = m_vector[m_vector.size() - 1];
        //TODO delete ret from m_vector
        setLength(length() - 1);
        return ret;
    }

    void insertValue(int idx, const ESValue& val)
    {
        if (m_fastmode) {
            m_vector.insert(m_vector.begin()+idx, val);
            setLength(length() + 1);
        } else {
            // TODO
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    void eraseValues(unsigned idx, unsigned cnt)
    {
        if (m_fastmode) {
            m_vector.erase(m_vector.begin()+idx, m_vector.begin()+idx+cnt);
        } else {
            for (int k = 0, i = idx; i < length() && k < (int)cnt; i++, k++) {
                set(i, get(i+cnt));
            }
        }
        setLength(length() - cnt);
    }

    bool shouldConvertToSlowMode(unsigned i) {
        if (m_fastmode && i > MAX_FASTMODE_SIZE)
            return true;
        return false;
    }

    void convertToSlowMode()
    {
        //wprintf(L"CONVERT TO SLOW MODE!!!  \n");
        m_fastmode = false;
        int len = length();
        if (len == 0) return;

        for (int i = 0; i < len; i++) {
            ESObject::set(ESValue(i).toString(), m_vector[i]);
        }
        m_vector.clear();
    }

    void set(int i, const ESValue& val, bool shouldThrowException = false)
    {
        int len = length();
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
            ESObject::set(ESValue(i), val, shouldThrowException);
        }
    }

    void setLength(unsigned newLength)
    {
        if (newLength < m_length) {
            //TODO : delete elements
        } else if (m_fastmode && newLength > m_length) {
            if(m_vector.capacity() < newLength) {
                size_t reservedSpace = std::min(MAX_FASTMODE_SIZE, newLength*2);
                m_vector.reserve(reservedSpace);
            }
            m_vector.resize(newLength, ESValue(ESValue::ESEmptyValue));
        }
        m_length = newLength;
    }

    bool isFastmode()
    {
        return m_fastmode;
    }

    const int32_t length()
    {
        return (const int32_t)m_length;
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
    unsigned m_length;
    ESVector m_vector;
    bool m_fastmode;
    static const unsigned MAX_FASTMODE_SIZE = 65536;
};

class LexicalEnvironment;
class Node;
class ESFunctionObject : public ESObject {
protected:
    ESFunctionObject(LexicalEnvironment* outerEnvironment, CodeBlock* codeBlock, escargot::ESString* name, ESObject* proto);
    ESFunctionObject(LexicalEnvironment* outerEnvironment, NativeFunctionType fn, escargot::ESString* name, ESObject* proto);
public:
    static ESFunctionObject* create(LexicalEnvironment* outerEnvironment, CodeBlock* codeBlock, escargot::ESString* name, ESObject* proto = NULL)
    {
        ESFunctionObject* ret = new ESFunctionObject(outerEnvironment, codeBlock, name, proto);
        return ret;
    }

    static ESFunctionObject* create(LexicalEnvironment* outerEnvironment, const NativeFunctionType& fn, escargot::ESString* name, ESObject* proto = NULL)
    {
        ESFunctionObject* ret = new ESFunctionObject(outerEnvironment, fn, name, proto);
        return ret;
    }

    void initialize(LexicalEnvironment* outerEnvironment, CodeBlock* codeBlock)
    {
        m_outerEnvironment = outerEnvironment;
        m_codeBlock = codeBlock;
    }

    ALWAYS_INLINE ESValue protoType()
    {
        return m_protoType;
    }

    ALWAYS_INLINE void setProtoType(ESValue obj)
    {
        m_protoType = obj;
    }

    CodeBlock* codeBlock() { return m_codeBlock; }
    LexicalEnvironment* outerEnvironment() { return m_outerEnvironment; }

    ALWAYS_INLINE escargot::ESString* name()
    {
        return m_name;
    }

    static ESValue call(ESVMInstance* instance, const ESValue& callee, const ESValue& receiver, ESValue arguments[], const size_t& argumentCount, bool isNewExpression);
protected:
    LexicalEnvironment* m_outerEnvironment;
    ESValue m_protoType;

    CodeBlock* m_codeBlock;
    escargot::ESString* m_name;
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
    friend class GlobalObject;
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
    ALWAYS_INLINE unsigned lastIndex() { return m_lastIndex; }
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

    unsigned m_lastIndex;
    escargot::ESString* m_lastExecutedString;
};

enum TypedArrayType {
    Int8Array,
    Uint8Array,
    Uint8ClampedArray,
    Int16Array,
    Uint16Array,
    Int32Array,
    Uint32Array,
    Float32Array,
    Float64Array
};

class ESArrayBufferObject : public ESObject {
protected:
    ESArrayBufferObject(ESPointer::Type type = ESPointer::Type::ESArrayBufferObject)
           : ESObject((Type)(Type::ESObject | Type::ESArrayBufferObject)),
             m_data(NULL),
             m_bytelength(0) {
    }

public:
    static ESArrayBufferObject* create()
    {
        return new ESArrayBufferObject();
    }
    static ESArrayBufferObject* create(ESObject* proto)
    {
        ESArrayBufferObject* obj = new ESArrayBufferObject();
        if (proto != NULL)
            obj->set__proto__(proto);
        return obj;
    }
    static ESArrayBufferObject* createAndAllocate(unsigned bytelength)
    {
        ESArrayBufferObject* obj = new ESArrayBufferObject();
        obj->allocateArrayBuffer(bytelength);
        return obj;
    }

    void allocateArrayBuffer(unsigned bytelength)
    {
        m_bytelength = bytelength;
        m_data = GC_malloc(bytelength);
    }
    bool isDetachedBuffer()
    {
        if (data() == NULL) return true;
        return false;
    }
    void detachArrayBuffer()
    {
        m_data = NULL;
        m_bytelength = 0;
    }

    ALWAYS_INLINE void* data() { return m_data; }
    ALWAYS_INLINE unsigned bytelength() { return m_bytelength; }

    //$24.1.1.5
    template<typename Type>
    ESValue getValueFromBuffer(unsigned byteindex, TypedArrayType typeVal, int isLittleEndian = -1)
    {
        ASSERT(byteindex >= 0);
        if (isLittleEndian != -1) {
            //TODO
            RELEASE_ASSERT_NOT_REACHED();
        }
        //If isLittleEndian is not present, set isLittleEndian to either true or false.
        void* rawStart = (int8_t*)m_data + byteindex;
        return ESValue( *((Type*) rawStart) );
    }
    //$24.1.1.6
    template<typename TypeAdaptor>
    bool setValueInBuffer(unsigned byteindex, TypedArrayType typeVal, ESValue val, int isLittleEndian = -1)
    {
        ASSERT(byteindex >= 0);
        if (isLittleEndian != -1) {
            //TODO
            RELEASE_ASSERT_NOT_REACHED();
        }
        //If isLittleEndian is not present, set isLittleEndian to either true or false.
        void* rawStart = (int8_t*)m_data + byteindex;
        *((typename TypeAdaptor::Type*) rawStart) = (typename TypeAdaptor::Type) TypeAdaptor::toNative(val);
        return true;
    }

private:
    void* m_data;
    unsigned m_bytelength;
};

class ESArrayBufferView : public ESObject {
protected:
    ESArrayBufferView(ESPointer::Type type = ESPointer::Type::ESArrayBufferView)
           : ESObject((Type)(Type::ESObject | Type::ESArrayBufferView | type)) {
    }

public:
    static ESArrayBufferView* create()
    {
        return new ESArrayBufferView();
    }
    ALWAYS_INLINE escargot::ESArrayBufferObject* buffer() { return m_buffer; }
    ALWAYS_INLINE void setBuffer(escargot::ESArrayBufferObject* bo) { m_buffer = bo; }
    ALWAYS_INLINE unsigned bytelength() { return m_bytelength; }
    ALWAYS_INLINE void setBytelength(unsigned l) { m_bytelength = l; }
    ALWAYS_INLINE unsigned byteoffset() { return m_byteoffset; }
    ALWAYS_INLINE void setByteoffset(unsigned o) { m_byteoffset = o; }

protected:
    escargot::ESArrayBufferObject* m_buffer;
    unsigned m_bytelength;
    unsigned m_byteoffset;
};

template<typename TypeArg, TypedArrayType type>
struct TypedArrayAdaptor {
    typedef TypeArg Type;
    static const TypedArrayType typeVal = type;
    static TypeArg toNative(ESValue val) {
        return static_cast<TypeArg>(val.toNumber());
    }
};
struct Int8Adaptor: TypedArrayAdaptor<int8_t, TypedArrayType::Int8Array> {
};
struct Int16Adaptor: TypedArrayAdaptor<int16_t, TypedArrayType::Int16Array> {
};
struct Int32Adaptor: TypedArrayAdaptor<int32_t, TypedArrayType::Int32Array> {
};
struct Uint8Adaptor: TypedArrayAdaptor<uint8_t, TypedArrayType::Uint8Array> {
};
struct Uint16Adaptor: TypedArrayAdaptor<uint16_t, TypedArrayType::Uint16Array> {
};
struct Uint32Adaptor: TypedArrayAdaptor<uint32_t, TypedArrayType::Uint32Array> {
};
struct Uint8ClampedAdaptor: TypedArrayAdaptor<uint8_t, TypedArrayType::Uint8ClampedArray> {
};
struct Float32Adaptor: TypedArrayAdaptor<float, TypedArrayType::Float32Array> {
};
struct Float64Adaptor: TypedArrayAdaptor<double, TypedArrayType::Float64Array> {
};

class ESTypedArrayObjectWrapper : public ESArrayBufferView {
protected:
    ESTypedArrayObjectWrapper(TypedArrayType arraytype, ESPointer::Type type)
           : ESArrayBufferView(type),
             m_arraytype(arraytype) {
    }
public:
    unsigned elementSize()
    {
        switch (m_arraytype) {
        case TypedArrayType::Int8Array:
        case TypedArrayType::Uint8Array:
        case TypedArrayType::Uint8ClampedArray:
            return 1;
        case TypedArrayType::Int16Array:
        case TypedArrayType::Uint16Array:
            return 2;
        case TypedArrayType::Int32Array:
        case TypedArrayType::Uint32Array:
        case TypedArrayType::Float32Array:
            return 4;
        case TypedArrayType::Float64Array:
            return 8;
        }
        RELEASE_ASSERT_NOT_REACHED();
    }

    void allocateTypedArray(unsigned length)
    {
        m_arraylength = length;
        m_byteoffset = 0;
        m_bytelength = length * elementSize();
        setBuffer(ESArrayBufferObject::createAndAllocate(m_bytelength));
    }

    ESValue get(int key);
    bool set(int key, ESValue val);
    ALWAYS_INLINE unsigned arraylength() { return m_arraylength; }
    ALWAYS_INLINE void setArraylength(unsigned length) { m_arraylength = length; }
    ALWAYS_INLINE TypedArrayType arraytype() { return m_arraytype; }
    /*
    ALWAYS_INLINE void setArraytype(TypedArrayType t) {
        m_arraytype = t;
    }
    */

private:
    unsigned m_arraylength;
    TypedArrayType m_arraytype;
};

template<typename TypeAdaptor>
class ESTypedArrayObject : public ESTypedArrayObjectWrapper {
public:

protected:
    ESTypedArrayObject(TypedArrayType arraytype,
                       ESPointer::Type type = ESPointer::Type::ESTypedArrayObject)
           : ESTypedArrayObjectWrapper(arraytype,
                                       (Type)(Type::ESObject | Type::ESTypedArrayObject)) {
    }

public:
    static ESTypedArrayObject* create()
    {
        return new ESTypedArrayObject(TypeAdaptor::typeVal);
    }
    static ESTypedArrayObject* create(ESObject* proto)
    {
        ESTypedArrayObject<TypeAdaptor>* obj = new ESTypedArrayObject(TypeAdaptor::typeVal);
        if (proto != NULL)
            obj->set__proto__(proto);
        return obj;
    }
    ESValue get(int key)
    {
        if (key >= 0 && key < arraylength()) {
            unsigned idxPosition = key * elementSize() + byteoffset();
            escargot::ESArrayBufferObject* b = buffer();
            return b->getValueFromBuffer<typename TypeAdaptor::Type>(idxPosition, arraytype());
        }
        return ESValue();
    }
    bool set(int key, ESValue val)
    {
        if (key < 0 || key >= arraylength()) return false;
        unsigned idxPosition = key * elementSize() + byteoffset();
        escargot::ESArrayBufferObject* b = buffer();
        return b->setValueInBuffer<TypeAdaptor>(idxPosition, arraytype(), val);
    }
};
typedef ESTypedArrayObject<Int8Adaptor> ESInt8Array;
typedef ESTypedArrayObject<Int16Adaptor> ESInt16Array;
typedef ESTypedArrayObject<Int32Adaptor> ESInt32Array;
typedef ESTypedArrayObject<Uint8Adaptor> ESUint8Array;
typedef ESTypedArrayObject<Uint16Adaptor> ESUint16Array;
typedef ESTypedArrayObject<Uint32Adaptor> ESUint32Array;
typedef ESTypedArrayObject<Uint8ClampedAdaptor> ESUint8ClampedArray;
typedef ESTypedArrayObject<Float32Adaptor> ESFloat32Array;
typedef ESTypedArrayObject<Float64Adaptor> ESFloat64Array;

class ESDataViewObject : public ESArrayBufferView {
protected:
    ESDataViewObject(ESPointer::Type type = ESPointer::Type::ESDataViewObject)
           : ESArrayBufferView((Type)(Type::ESObject | Type::ESDataViewObject)) {
    }

public:
    static ESDataViewObject* create()
    {
        return new ESDataViewObject();
    }
};

}
#include "vm/ESVMInstance.h"
#include "ESValueInlines.h"

#endif
