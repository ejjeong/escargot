#ifndef ESValue_h
#define ESValue_h

#include "InternalString.h"

namespace JSC {
namespace Yarr {
class YarrPattern;
class BytecodePattern;
}
}

namespace escargot {

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
class ESMathObject;
// ES6 Typed Array
class ESArrayBufferObject;
class ESArrayBufferView;
template<typename TypeArg>
class ESTypedArrayObject;
class ESTypedArrayObjectWrapper;
class ESDataViewObject;
class ESArgumentsObject;
class ESControlFlowRecord;
class CodeBlock;

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

#if ESCARGOT_64
typedef uint64_t ESValueInDouble;
#else
typedef double ESValueInDouble;
#endif

class ESValue {
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
    enum ESDeletedValueTag { ESDeletedValue };
    enum ESTrueTag { ESTrue };
    enum ESFalseTag { ESFalse };
    enum EncodeAsDoubleTag { EncodeAsDouble };
    enum ESForceUninitializedTag { ESForceUninitialized };

    ESValue();
    ESValue(ESForceUninitializedTag);
    ESValue(ESNullTag);
    ESValue(ESUndefinedTag);
    ESValue(ESEmptyValueTag);
    ESValue(ESDeletedValueTag);
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

    ALWAYS_INLINE bool operator==(const ESValue& other) const;
    ALWAYS_INLINE bool operator!=(const ESValue& other) const;

    bool isInt32() const;
    bool isUInt32() const;
    bool isDouble() const;
    bool isTrue() const;
    bool isFalse() const;

    int32_t asInt32() const;
    uint32_t asUInt32() const;
    double asDouble() const;
    bool asBoolean() const;
    double asNumber() const;
    uint64_t asRawData() const;

    // Querying the type.
    ALWAYS_INLINE bool isEmpty() const;
    bool isDeleted() const;
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
    ALWAYS_INLINE bool isPrimitive() const;
    bool isGetterSetter() const;
    bool isCustomGetterSetter() const;
    bool isObject() const;

    enum PrimitiveTypeHint { PreferString, PreferNumber };
    ALWAYS_INLINE ESValue toPrimitive(PrimitiveTypeHint = PreferNumber) const; // $7.1.1 ToPrimitive
    ESValue toPrimitiveSlowCase(PrimitiveTypeHint = PreferNumber) const;
    ALWAYS_INLINE bool toBoolean() const; // $7.1.2 ToBoolean
    ALWAYS_INLINE double toNumber() const; // $7.1.3 ToNumber
    inline double toNumberSlowCase() const; // $7.1.3 ToNumber
    inline double toInteger() const; // $7.1.4 ToInteger
    ALWAYS_INLINE int32_t toInt32() const; // $7.1.5 ToInt32
    inline int32_t toInt32SlowCase() const; // $7.1.5 ToInt32
    inline uint32_t toUint32() const; // http://www.ecma-international.org/ecma-262/5.1/#sec-9.6
    ALWAYS_INLINE ESString* toString() const; // $7.1.12 ToString
    ESString* toStringSlowCase() const; // $7.1.12 ToString
    inline ESObject* toObject() const; // $7.1.13 ToObject
    inline double toLength() const; // $7.1.15 ToLength

    enum { ESInvalidIndexValue = std::numeric_limits<uint32_t>::max() };
    ALWAYS_INLINE uint32_t toIndex() const; // http://www.ecma-international.org/ecma-262/5.1/#sec-15.4

    ALWAYS_INLINE ESString* asESString() const;

    ALWAYS_INLINE bool isESPointer() const;
    ALWAYS_INLINE ESPointer* asESPointer() const;

    static ESValueInDouble toRawDouble(ESValue);
    static ESValue fromRawDouble(ESValueInDouble);

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

#ifdef ENABLE_ESJIT
    static size_t offsetOfAsInt64() { return offsetof(ESValue, u.asInt64); }
#endif

private:
    ValueDescriptor u;

public:
    ALWAYS_INLINE bool abstractEqualsTo(const ESValue& val);
    bool abstractEqualsToSlowCase(const ESValue& val);
    inline bool equalsTo(const ESValue& val);
    inline bool equalsToByTheSameValueAlgorithm(const ESValue& val);

};

typedef ESValue (*NativeFunctionType)(ESVMInstance*);


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
        ESMathObject = 1 << 10,
        ESBooleanObject = 1 << 11,
        ESArrayBufferObject = 1 << 12,
        ESArrayBufferView = 1 << 13,
        ESTypedArrayObject = 1 << 14,
        ESDataViewObject = 1 << 15,
        ESArgumentsObject = 1 << 16,
        ESControlFlowRecord = 1 << 17,
        ESJSONObject = 1 << 18,
        TypeMask = 0x1ffff
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
        return m_type == (Type::ESArrayObject | Type::ESObject);
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

    ALWAYS_INLINE bool isESJSONObject() const
    {
        return m_type & Type::ESJSONObject;
    }

    ALWAYS_INLINE ::escargot::ESRegExpObject* asESRegExpObject()
    {
#ifndef NDEBUG
        ASSERT(isESRegExpObject());
#endif
        return reinterpret_cast<::escargot::ESRegExpObject *>(this);
    }

    ALWAYS_INLINE bool isESMathObject() const
    {
        return m_type & Type::ESMathObject;
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

    ALWAYS_INLINE bool isESArgumentsObject() const
    {
        return m_type & Type::ESArgumentsObject;
    }

    ALWAYS_INLINE ::escargot::ESArgumentsObject* asESArgumentsObject()
    {
#ifndef NDEBUG
        ASSERT(isESArgumentsObject());
#endif
        return reinterpret_cast<::escargot::ESArgumentsObject *>(this);
    }

    ALWAYS_INLINE bool isESControlFlowRecord() const
    {
        return m_type & Type::ESControlFlowRecord;
    }

    ALWAYS_INLINE ::escargot::ESControlFlowRecord* asESControlFlowRecord()
    {
#ifndef NDEBUG
        ASSERT(isESControlFlowRecord());
#endif
        return reinterpret_cast<::escargot::ESControlFlowRecord *>(this);
    }

#ifdef ENABLE_ESJIT
    static size_t offsetOfType() { return offsetof(ESPointer, m_type); }
#endif

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
#ifdef ENABLE_ESJIT
        m_length = 0;
#endif
    }

    ESStringData(const u16string& src)
        : u16string(src)
    {
        m_hashData.m_isHashInited =  false;
#ifdef ENABLE_ESJIT
        m_length = src.length();
#endif
    }

    ESStringData(u16string&& src)
        : u16string(std::move(src))
    {
        m_hashData.m_isHashInited =  false;
#ifdef ENABLE_ESJIT
        m_length = u16string::length();
#endif
    }

    ESStringData(std::u16string& src)
    {
        assign(src.begin(), src.end());
        m_hashData.m_isHashInited =  false;
#ifdef ENABLE_ESJIT
        m_length = u16string::length();
#endif
    }

    explicit ESStringData(int number)
        : ESStringData((double)number)
    {
#ifdef ENABLE_ESJIT
        m_length = u16string::length();
#endif
    }

    explicit ESStringData(double number);

    explicit ESStringData(char16_t c)
        : u16string({c})
    {
        m_hashData.m_isHashInited =  false;
#ifdef ENABLE_ESJIT
        m_length = u16string::length();
#endif
    }

    ESStringData(const char* s)
        : u16string(std::move(utf8ToUtf16(s, strlen(s))))
    {
        m_hashData.m_isHashInited =  false;
#ifdef ENABLE_ESJIT
        m_length = u16string::length();
#endif
    }

    ESStringData(const ESStringData& s) = delete;
    void operator =(const ESStringData& s) = delete;

    ALWAYS_INLINE size_t length() const
    {
#ifdef ENABLE_ESJIT
        return m_length;
#endif
        return u16string::length();
    }

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
        if (!m_hashData.m_isHashInited) {
            m_hashData.m_isHashInited = true;
            std::hash<std::basic_string<char16_t> > hashFn;
            m_hashData.m_hashData = hashFn((std::basic_string<char16_t> &)*this);
        }
    }

#ifdef ENABLE_ESJIT
    // static size_t offsetOfData() { return offsetof(ESStringData, _M_dataplus._M_p); }
    static size_t offsetOfData() { return 0; }
    static size_t offsetOfLength() { return offsetof(ESStringData, m_length); }
#endif

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
#ifdef ENABLE_ESJIT
    size_t m_length;
#endif
};

struct NullableUTF8String {
    const char* m_buffer;
    size_t m_bufferSize;
};

class ESString : public ESPointer {
    friend class ESScriptParser;
protected:
    ESString(ESStringData* data)
        : ESPointer(Type::ESString)
    {
        m_string = data;
    }

    ESString(const u16string& src)
        : ESPointer(Type::ESString)
    {
        m_string = new(GC) ESStringData(src);
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

    static ESString* create(const u16string& src)
    {
        return new ESString(src);
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

    const char* utf8Data() const
    {
        return utf16ToUtf8(data());
    }

    u16string* utf16Data() const
    {
        if (UNLIKELY(m_string == NULL)) {
            ensureNormalString();
        }
        return m_string;
    }

    NullableUTF8String toNullableUTF8String()
    {
        size_t len;
        NullableUTF8String str;
        str.m_buffer = utf16ToUtf8(data(), &str.m_bufferSize);
        return str;
    }

    template <typename Func>
    void wcharData(const Func& fn)
    {
#ifdef ANDROID
        RELEASE_ASSERT_NOT_REACHED();
#endif
        wchar_t* buf = (wchar_t *)alloca(sizeof(wchar_t) * (length()+1));
        for (unsigned i = 0 ; i < (unsigned)length() ; i ++) {
            buf[i] = data()[i];
        }
        buf[length()] = 0;

        fn(buf, length());
    }

    uint32_t tryToUseAsIndex();
    bool hasOnlyDigit()
    {
        const u16string& s = string();
        bool allOfCharIsDigit = true;
        for (unsigned i = 0; i < s.length() ; i ++) {
            char16_t c = s[i];
            if (c < '0' || c > '9') {
                allOfCharIsDigit = false;
                break;
            }
        }
        return allOfCharIsDigit;
    }

    ALWAYS_INLINE void ensureNormalString() const;
    ALWAYS_INLINE const char16_t* data() const;
    ALWAYS_INLINE const u16string& string() const;
    ALWAYS_INLINE const ESStringData* stringData() const;
    ALWAYS_INLINE size_t length() const;
    ALWAYS_INLINE size_t hashValue() const
    {
        return m_string->hashValue();
    }

    ESString* substring(int from, int to) const;

    struct RegexMatchResult {
        struct RegexMatchResultPiece {
            unsigned m_start, m_end;
        };
        COMPILE_ASSERT((sizeof(RegexMatchResultPiece)) == (sizeof(unsigned) * 2), sizeof_RegexMatchResultPiece_wrong);
        int m_subPatternNum;
        std::vector< std::vector< RegexMatchResultPiece > > m_matchResults;
    };
    bool match(ESPointer* esptr, RegexMatchResult& result, bool testOnly = false, size_t startIndex = 0) const;

    ESString(const ESString& s) = delete;
    void operator =(const ESString& s) = delete;

    ALWAYS_INLINE friend bool operator == (const ESString& a, const char16_t* b);
    ALWAYS_INLINE friend bool operator != (const ESString& a, const char16_t* b);
    ALWAYS_INLINE friend bool operator == (const ESString& a, const ESString& b);
    ALWAYS_INLINE friend bool operator != (const ESString& a, const ESString& b);
    ALWAYS_INLINE friend bool operator < (const ESString& a, const ESString& b);
    ALWAYS_INLINE friend bool operator > (const ESString& a, const ESString& b);
    ALWAYS_INLINE friend bool operator <= (const ESString& a, const ESString& b);
    ALWAYS_INLINE friend bool operator >= (const ESString& a, const ESString& b);

#ifndef NDEBUG
    void show() const
    {
        printf("%s\n", utf8Data());
    }
#endif

#ifdef ENABLE_ESJIT
    static size_t offsetOfStringData() { return offsetof(ESString, m_string); }
#endif

protected:
    ESStringData* m_string;
};

ALWAYS_INLINE bool operator == (const ESString& a, const char16_t* b)
{
    return a.string() == b;
}

ALWAYS_INLINE bool operator != (const ESString& a, const char16_t* b)
{
    return a.string() != b;
}

ALWAYS_INLINE bool operator == (const ESString& a, const ESString& b)
{
    const ESStringData* dataA = a.stringData();
    const ESStringData* dataB = b.stringData();

    if (dataA == dataB)
        return true;

    if (dataA->length() == dataB->length()) {
        if (dataA->hashValue() == dataB->hashValue()) {
            return *dataA == *dataB;
        }
    }
    return false;
}

ALWAYS_INLINE bool operator != (const ESString& a, const ESString& b)
{
    return !operator ==(a, b);
}

ALWAYS_INLINE bool operator < (const ESString& a, const ESString& b)
{
    return a.string() < b.string();
}

ALWAYS_INLINE bool operator > (const ESString& a, const ESString& b)
{
    return a.string() > b.string();
}

ALWAYS_INLINE bool operator <= (const ESString& a, const ESString& b)
{
    return a.string() <= b.string();
}

ALWAYS_INLINE bool operator >= (const ESString& a, const ESString& b)
{
    return a.string() >= b.string();
}

typedef std::vector<ESString *, gc_allocator<ESString *> > ESStringVector;

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
    ESStringData* stringData()
    {
#ifndef NDEBUG
        ASSERT(m_type & ESPointer::ESRopeString);
        if (m_string) {
            ASSERT(m_contentLength == 0);
        }
#endif
        u16string result;
        // TODO: should reduce unnecessary append operations in std::string::resize
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
    size_t contentLength()
    {
        return m_contentLength;
    }

protected:
    ESString* m_left;
    ESString* m_right;
    size_t m_contentLength;
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
    if (UNLIKELY(m_string == NULL)) {
        const_cast<ESString *>(this)->asESRopeString()->convertIntoNormalString();
    }
}

ALWAYS_INLINE const char16_t* ESString::data() const
{
    if (UNLIKELY(m_string == NULL)) {
        ensureNormalString();
    }
    return m_string->data();
}

ALWAYS_INLINE const u16string& ESString::string() const
{
    if (UNLIKELY(m_string == NULL)) {
        ensureNormalString();
    }
    return static_cast<const u16string&>(*m_string);
}

ALWAYS_INLINE const ESStringData* ESString::stringData() const
{
    if (UNLIKELY(m_string == NULL)) {
        return const_cast<ESString *>(this)->asESRopeString()->stringData();
    }
    return m_string;
}

ALWAYS_INLINE size_t ESString::length() const
{
    if (UNLIKELY(m_string == NULL)) {
        escargot::ESRopeString* rope = (escargot::ESRopeString *)this;
        return rope->contentLength();
    }
    return m_string->length();
}

}

namespace std {
template<> struct hash<escargot::ESString *> {
    size_t operator()(escargot::ESString * const &x) const
    {
        return x->stringData()->hashValue();
    }
};

template<> struct equal_to<escargot::ESString *> {
    bool operator()(escargot::ESString * const &a, escargot::ESString * const &b) const
    {
        if (a == b) {
            return true;
        }
        return a->string() == b->string();
    }
};

}


#include "runtime/InternalAtomicString.h"
#include "runtime/StaticStrings.h"

namespace escargot {

typedef ESValue (*ESNativeGetter)(::escargot::ESObject* obj, ::escargot::ESObject* originalObj);
typedef void (*ESNativeSetter)(::escargot::ESObject* obj, ::escargot::ESObject* originalObj, const ESValue& value);

class ESPropertyAccessorData : public gc {
public:
    ESPropertyAccessorData(ESFunctionObject* getter, ESFunctionObject* setter)
    {
        m_nativeGetter = nullptr;
        m_nativeSetter = nullptr;
        m_jsGetter = getter;
        m_jsSetter = setter;
    }
    ESPropertyAccessorData(ESNativeGetter getter, ESNativeSetter setter)
    {
        m_nativeGetter = getter;
        m_nativeSetter = setter;
        m_jsGetter = nullptr;
        m_jsSetter = nullptr;
    }
    ESPropertyAccessorData()
    {
        m_nativeGetter = nullptr;
        m_nativeSetter = nullptr;
        m_jsGetter = nullptr;
        m_jsSetter = nullptr;
    }

    ALWAYS_INLINE ESValue value(::escargot::ESObject* obj, ::escargot::ESObject* originalObj);
    ALWAYS_INLINE void setValue(::escargot::ESObject* obj, ::escargot::ESObject* originalObj, const ESValue& value);

    ESNativeSetter getNativeSetter()
    {
        return m_nativeSetter;
    }

    ESNativeGetter getNativeGetter()
    {
        return m_nativeGetter;
    }

    void setSetter(ESNativeSetter setter)
    {
        m_nativeSetter = setter;
        m_jsSetter = nullptr;
    }

    void setGetter(ESNativeGetter getter)
    {
        m_nativeGetter = getter;
        m_jsGetter = nullptr;
    }

    ESFunctionObject* getJSSetter()
    {
        return m_jsSetter;
    }

    void setJSSetter(ESFunctionObject* setter)
    {
        m_nativeSetter = nullptr;
        m_jsSetter = setter;
    }

    ESFunctionObject* getJSGetter()
    {
        return m_jsGetter;
    }

    void setJSGetter(ESFunctionObject* getter)
    {
        m_nativeGetter = nullptr;
        m_jsGetter = getter;
    }

    void setGetterAndSetterTo(ESObject* obj);

protected:
    ESNativeGetter m_nativeGetter;
    ESNativeSetter m_nativeSetter;
    ESFunctionObject* m_jsGetter;
    ESFunctionObject* m_jsSetter;
};

inline char assembleHidenClassPropertyInfoFlags(bool isData, bool isWritable, bool isEnumerable, bool isConfigurable)
{
    return isData | ((int)isWritable << 1) | ((int)isEnumerable << 2) | ((int)isConfigurable << 3);
}

struct ESHiddenClassPropertyInfo {
    ESHiddenClassPropertyInfo(ESString* name, bool isData, bool isWritable, bool isEnumerable, bool isConfigurable)
    {
        m_name = name;
        m_flags.m_isDataProperty = isData;
        m_flags.m_isWritable = isWritable;
        m_flags.m_isEnumerable = isEnumerable;
        m_flags.m_isConfigurable = isConfigurable;
        m_flags.m_isDeletedValue = false;
    }

    ESHiddenClassPropertyInfo()
    {
        m_name = NULL;
        m_flags.m_isDataProperty = true;
        m_flags.m_isWritable = false;
        m_flags.m_isEnumerable = false;
        m_flags.m_isConfigurable = false;
        m_flags.m_isDeletedValue = true;
    }

    char flags()
    {
        return assembleHidenClassPropertyInfoFlags(m_flags.m_isDataProperty, m_flags.m_isWritable, m_flags.m_isEnumerable, m_flags.m_isConfigurable);
    }

    ESString* m_name;

    struct {
        bool m_isDataProperty:1;
        // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-property-attributes
        bool m_isWritable:1;
        bool m_isEnumerable:1;
        bool m_isConfigurable:1;
        bool m_isDeletedValue:1;
    } m_flags;
};

typedef std::unordered_map<::escargot::ESString*, size_t,
    std::hash<ESString*>, std::equal_to<ESString*>,
    gc_allocator< std::pair<const ::escargot::ESString*, size_t> > > ESHiddenClassPropertyIndexHashMapInfoStd;
typedef std::vector<::escargot::ESHiddenClassPropertyInfo, gc_allocator<::escargot::ESHiddenClassPropertyInfo> > ESHiddenClassPropertyInfoVectorStd;

class ESHiddenClassPropertyInfoVector : public ESHiddenClassPropertyInfoVectorStd {
    public:
#ifdef ENABLE_ESJIT
    static size_t offsetOfData() { return offsetof(ESHiddenClassPropertyInfoVector, _M_impl._M_start); }
#endif
};

typedef std::unordered_map<ESString*, ::escargot::ESHiddenClass **,
std::hash<ESString*>, std::equal_to<ESString*>,
gc_allocator<std::pair<const ESString*, std::pair<::escargot::ESHiddenClass *, ::escargot::ESHiddenClass **> > > > ESHiddenClassTransitionDataStd;

typedef std::vector<::escargot::ESValue, gc_allocator<::escargot::ESValue> > ESValueVectorStd;

class ESValueVector : public ESValueVectorStd {
public:
    ESValueVector(size_t siz)
        : ESValueVectorStd(siz) { }

#ifdef ENABLE_ESJIT
    static size_t offsetOfData() { return offsetof(ESValueVector, _M_impl._M_start); }
#endif
};

class ESHiddenClass : public gc {
    static const unsigned ESHiddenClassVectorModeSizeLimit = 64;
    friend class ESVMInstance;
    friend class ESObject;
public:
    size_t findProperty(const ESString* name)
    {
        auto iter = m_propertyIndexHashMapInfo.find(const_cast<ESString *>(name));
        if (iter == m_propertyIndexHashMapInfo.end())
            return SIZE_MAX;
        return iter->second;
    }

    ALWAYS_INLINE ESHiddenClass* defineProperty(ESString* name, bool isData, bool isWritable, bool isEnumerable, bool isConfigurable);
    ALWAYS_INLINE ESHiddenClass* removeProperty(ESString* name)
    {
        return removeProperty(findProperty(name));
    }
    ALWAYS_INLINE ESHiddenClass* removeProperty(size_t idx);
    ALWAYS_INLINE ESHiddenClass* removePropertyWithoutIndexChange(size_t idx);
    ALWAYS_INLINE ESHiddenClass* morphToNonVectorMode();
    ALWAYS_INLINE ESHiddenClass* forceNonVectorMode();
    bool isVectorMode()
    {
        return m_flags.m_isVectorMode;
    }

    ALWAYS_INLINE ESValue read(ESObject* obj, ESObject* originalObject, ESString* name);
    ALWAYS_INLINE ESValue read(ESObject* obj, ESObject* originalObject, size_t index);

    ALWAYS_INLINE bool write(ESObject* obj, ESObject* originalObject, ESString* name, const ESValue& val);
    ALWAYS_INLINE bool write(ESObject* obj, ESObject* originalObject, size_t index, const ESValue& val);

    const ESHiddenClassPropertyInfo& propertyInfo(const size_t& idx)
    {
        return m_propertyInfo[idx];
    }

    const ESHiddenClassPropertyInfoVector& propertyInfo()
    {
        return m_propertyInfo;
    }

    size_t propertyCount()
    {
        return m_propertyInfo.size();
    }

    ALWAYS_INLINE bool hasReadOnlyProperty()
    {
        return m_flags.m_hasReadOnlyProperty;
    }

    ALWAYS_INLINE bool hasIndexedProperty()
    {
        return m_flags.m_hasIndexedProperty;
    }

    ALWAYS_INLINE bool hasIndexedReadOnlyProperty()
    {
        return m_flags.m_hasIndexedReadOnlyProperty;
    }


#ifdef ENABLE_ESJIT
    static size_t offsetOfFlags() { return offsetof(ESHiddenClass, m_flags); }
    static size_t offsetOfPropertyInfo() { return offsetof(ESHiddenClass, m_propertyInfo); }
#endif

private:
    ESHiddenClass()
        : m_transitionData(4)
    {
        m_flags.m_isVectorMode = true;
        m_flags.m_forceNonVectorMode = false;
        m_flags.m_hasReadOnlyProperty = false;
        m_flags.m_hasIndexedProperty = false;
        m_flags.m_hasIndexedReadOnlyProperty = false;
    }

    ESHiddenClassPropertyIndexHashMapInfoStd m_propertyIndexHashMapInfo;
    ESHiddenClassPropertyInfoVector m_propertyInfo;
    ESHiddenClassTransitionDataStd m_transitionData;

    struct {
        bool m_isVectorMode:1;
        bool m_forceNonVectorMode:1;
        bool m_hasReadOnlyProperty:1;
        bool m_hasIndexedProperty:1;
        bool m_hasIndexedReadOnlyProperty:1;
    } m_flags;
};

typedef std::vector<ESHiddenClass*, pointer_free_allocator<ESHiddenClass*> > ESHiddenClassChainStd;
class ESHiddenClassChain : public ESHiddenClassChainStd {
public:
    ESHiddenClassChain()
        : ESHiddenClassChainStd() { }

#ifdef ENABLE_ESJIT
    static size_t offsetOfData() { return offsetof(ESHiddenClassChain, _M_impl._M_start); }
    static size_t offsetOfDataEnd() { return offsetof(ESHiddenClassChain, _M_impl._M_finish); }
#endif
};

class ESObject : public ESPointer {
    friend class ESHiddenClass;
    friend class ESFunctionObject;
    friend ALWAYS_INLINE void setObjectPreComputedCaseOperation(ESValue* willBeObject, ::escargot::ESString* keyString, const ESValue& value
        , ESHiddenClassChain* cachedHiddenClassChain, size_t* cachedHiddenClassIndex, ESHiddenClass** hiddenClassWillBe);
protected:
    ESObject(ESPointer::Type type, ESValue __proto__, size_t initialKeyCount = 6);
public:
    static ESObject* create(size_t initialKeyCount = 6);

    inline bool defineDataProperty(InternalAtomicString name, bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true, const ESValue& initalValue = ESValue())
    {
        return defineDataProperty(name.string(), isWritable, isEnumerable, isConfigurable, initalValue);
    }
    inline bool defineDataProperty(const escargot::ESValue& key, bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true, const ESValue& initalValue = ESValue());
    inline bool defineAccessorProperty(const escargot::ESValue& key, ESPropertyAccessorData* data, bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true);
    inline bool defineAccessorProperty(const escargot::ESValue& key, ESNativeGetter getter, ESNativeSetter setter,
        bool isWritable, bool isEnumerable, bool isConfigurable)
    {
        return defineAccessorProperty(key, new ESPropertyAccessorData(getter, setter), isWritable, isEnumerable, isConfigurable);
    }
    inline bool defineAccessorProperty(escargot::ESString* key, ESNativeGetter getter,
        ESNativeSetter setter,
        bool isWritable, bool isEnumerable, bool isConfigurable)
    {
        return defineAccessorProperty(key, new ESPropertyAccessorData(getter, setter), isWritable, isEnumerable, isConfigurable);
    }

    inline bool deleteProperty(const ESValue& key, bool forced = false);
    inline void propertyFlags(const ESValue& key, bool& exists, bool& isDataProperty, bool& isWritable, bool& isEnumerable, bool& isConfigurable);
    inline bool hasProperty(const escargot::ESValue& key);
    inline bool hasOwnProperty(const escargot::ESValue& key);

    bool DefineOwnProperty(ESValue& key, ESObject* desc, bool throwFlag);

    // $6.1.7.2 Object Internal Methods and Internal Slots
    bool isExtensible()
    {
        return m_flags.m_isExtensible;
    }

    void setExtensible(bool extensible)
    {
        m_flags.m_isExtensible = extensible;
    }

    ESHiddenClass* hiddenClass()
    {
        return m_hiddenClass;
    }

    void forceNonVectorHiddenClass()
    {
        ASSERT(!m_hiddenClass->m_flags.m_forceNonVectorMode);
        m_hiddenClass = m_hiddenClass->forceNonVectorMode();
    }

    ESPropertyAccessorData* accessorData(escargot::ESString* key)
    {
        size_t idx = m_hiddenClass->findProperty(key);
        return accessorData(idx);
    }

    ESPropertyAccessorData* accessorData(size_t idx)
    {
        ASSERT(!m_hiddenClass->propertyInfo(idx).m_flags.m_isDataProperty);
        return (ESPropertyAccessorData *)m_hiddenClassData[idx].asESPointer();
    }

    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-get-o-p
    ALWAYS_INLINE ESValue get(escargot::ESValue key);
    ALWAYS_INLINE ESValue getOwnProperty(escargot::ESValue key);

    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-set-o-p-v-throw
    ALWAYS_INLINE bool set(const escargot::ESValue& key, const ESValue& val);
    ALWAYS_INLINE bool set(escargot::ESString* key, const ESValue& val)
    {
        return set(ESValue(key), val);
    }

    ALWAYS_INLINE void set(const escargot::ESValue& key, const ESValue& val, bool throwExpetion);

    ALWAYS_INLINE uint32_t length();
    ALWAYS_INLINE ESValue pop();
    ALWAYS_INLINE void eraseValues(uint32_t, int);

    template <typename Functor>
    ALWAYS_INLINE void enumeration(Functor t);
    template <typename Functor>
    ALWAYS_INLINE void enumerationWithNonEnumerable(Functor t);

    template <typename Comp>
    ALWAYS_INLINE void sort(const Comp& c);

    ALWAYS_INLINE const ESValue& __proto__()
    {
        return m___proto__;
    }

    inline void set__proto__(const ESValue& obj);
    ALWAYS_INLINE size_t keyCount();

#ifdef ENABLE_ESJIT
    static size_t offsetOfHiddenClassData() { return offsetof(ESObject, m_hiddenClassData); }
    static size_t offsetOfHiddenClass() { return offsetof(ESObject, m_hiddenClass); }
    static size_t offsetOf__proto__() { return offsetof(ESObject, m___proto__); }
#endif
protected:
    void setValueAsProtoType(const ESValue& obj);

    ESHiddenClass* m_hiddenClass;
    ESValueVectorStd m_hiddenClassData;

    ESValue m___proto__;

#ifdef ESCARGOT_64
    struct {
        bool m_isGlobalObject: 1;
        bool m_isExtensible: 1;
        bool m_isEverSetAsPrototypeObject: 1;
        size_t m_margin: 61;
    } m_flags;
#endif
#ifdef ESCARGOT_32
    struct {
        bool m_isGlobalObject: 1;
        bool m_isExtensible: 1;
        bool m_isEverSetAsPrototypeObject: 1;
        size_t m_margin: 29;
    } m_flags;
#endif
};

class ESErrorObject : public ESObject {
protected:
    ESErrorObject(escargot::ESString* message);
public:
    static ESErrorObject* create(escargot::ESString* message = strings->emptyString.string())
    {
        return new ESErrorObject(message);
    }
protected:
};

class ReferenceError : public ESErrorObject {
protected:
    ReferenceError(escargot::ESString* message = strings->emptyString.string());
public:
    static ReferenceError* create(escargot::ESString* message = strings->emptyString.string())
    {
        return new ReferenceError(message);
    }
};

class TypeError : public ESErrorObject {
protected:
    TypeError(escargot::ESString* message = strings->emptyString.string());
public:
    static TypeError* create(escargot::ESString* message = strings->emptyString.string())
    {
        return new TypeError(message);
    }
};

class SyntaxError : public ESErrorObject {
protected:
    SyntaxError(escargot::ESString* message = strings->emptyString.string());
public:

    static SyntaxError* create(escargot::ESString* message = strings->emptyString.string())
    {
        return new SyntaxError(message);
    }
};

class RangeError : public ESErrorObject {
protected:
    RangeError(escargot::ESString* message = strings->emptyString.string());
public:
    static RangeError* create(escargot::ESString* message = strings->emptyString.string())
    {
        return new RangeError(message);
    }
};

class URIError : public ESErrorObject {
protected:
    URIError(escargot::ESString* message = strings->emptyString.string());
public:
    static URIError* create(escargot::ESString* message = strings->emptyString.string())
    {
        return new URIError(message);
    }
};

class EvalError : public ESErrorObject {
protected:
    EvalError(escargot::ESString* message = strings->emptyString.string());
public:
    static EvalError* create(escargot::ESString* message = strings->emptyString.string())
    {
        return new EvalError(message);
    }
};

class ESDateObject : public ESObject {
protected:
    ESDateObject(ESPointer::Type type = ESPointer::Type::ESDateObject);
public:
    static ESDateObject* create()
    {
        return new ESDateObject();
    }

    void parseStringToDate(struct tm* timeinfo, escargot::ESString* istr);
    void parseYmdhmsToDate(struct tm* timeinfo, int year, int month, int date, int hour, int minute, int second);

    void setTimeValue();
    void setTimeValue(const ESValue str);
    void setTimeValue(int year, int month, int date, int hour, int minute, int second, int millisecond);

    double getTimeAsMilisec()
    {
        return m_time.tv_sec*1000 + floor(m_time.tv_nsec / 1000000);
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

    void setPrimitiveValue(double primitiveVale) { m_primitiveValue = primitiveVale; }
    double getPrimitiveValue() { return m_primitiveValue; }
    static double timeClip(double V)
    {
        if (std::isinf(V)) {
            return nan("0");
        } else if (std::abs(V) > 8.64 * std::pow(10, 15)) {
            return nan("0");
        } else {
            return ESValue(V).toInteger();
        }
    }

    tm* getGmtTime();


private:
    void resolveCache();
    struct timespec m_time;
    struct tm m_cachedTM;
    bool m_isCacheDirty;
    double m_primitiveValue;
};

class ESMathObject : public ESObject {
protected:
    ESMathObject(ESPointer::Type type = ESPointer::Type::ESMathObject);
public:
    static ESMathObject* create()
    {
        return new ESMathObject();
    }
protected:
};

class ESJSONObject : public ESObject {
protected:
    ESJSONObject(ESPointer::Type type = ESPointer::Type::ESJSONObject);
public:
    static ESJSONObject* create()
    {
        return new ESJSONObject();
    }
protected:
};

class ESArrayObject final : public ESObject {
    friend class ESObject;
protected:
    ESArrayObject(int length);
public:

    // $9.4.2.2
    static ESArrayObject* create(int length = 0)
    {
        return new ESArrayObject(length);
    }

    ALWAYS_INLINE ESValue* data()
    {
        ASSERT(isFastmode());
        return m_vector.data();
    }

    ESValue get(unsigned key)
    {
        return ESObject::get(ESValue(key));
    }

    void push(const ESValue& val)
    {
        set(m_length, val);
    }

    ESValue fastPop()
    {
        ASSERT(isFastmode());
        if (m_length == 0)
            return ESValue();
        ESValue ret = m_vector[m_vector.size() - 1];
        setLength(length() - 1);
        return ret;
    }

    // Insert 1 element val at idx
    void insertValue(int idx, const ESValue& val)
    {
        if (m_fastmode) {
            m_vector.insert(m_vector.begin()+idx, val);
            setLength(length() + 1);
        } else {
            for (int i = length(); i >= idx; i--) {
                set(i, get(i-1));
            }
            set(idx - 1, val);
        }
    }

    // Erase #cnt elements from idx
    void eraseValues(unsigned idx, unsigned cnt)
    {
        if (m_fastmode) {
            m_vector.erase(m_vector.begin()+idx, m_vector.begin()+idx+cnt);
        } else {
            for (size_t k = 0, i = idx; i < length() && k < cnt; i++, k++) {
                set(i, get(i+cnt));
            }
        }
        setLength(length() - cnt);
    }

    bool shouldConvertToSlowMode(unsigned i)
    {
        if (m_fastmode && i > MAX_FASTMODE_SIZE)
            return true;
        return false;
    }

    void convertToSlowMode()
    {
        // wprintf(L"CONVERT TO SLOW MODE!!!  \n");
        if (!m_fastmode)
            return;
        m_fastmode = false;
        uint32_t len = length();
        if (len == 0)
            return;

        ESValue* dataPtr = m_vector.data();
        for (uint32_t i = 0; i < len; i++) {
            if (dataPtr[i] != ESValue(ESValue::ESEmptyValue))
                ESObject::set(ESValue(i).toString(), dataPtr[i]);
        }
        m_vector.clear();
    }

    void set(int i, const ESValue& val)
    {
        ESObject::set(ESValue(i), val);
    }


    void setLength(unsigned newLength)
    {
        if (m_fastmode) {
            if (shouldConvertToSlowMode(newLength)) {
                convertToSlowMode();
                ESObject::set(strings->length, ESValue(newLength));
                m_length = newLength;
                return;
            }
            if (newLength < m_length) {
                m_vector.resize(newLength);
            } else if (newLength > m_length) {
                if (m_vector.capacity() < newLength) {
                    size_t reservedSpace = std::min(MAX_FASTMODE_SIZE, newLength*2);
                    m_vector.reserve(reservedSpace);
                }
                m_vector.resize(newLength, ESValue(ESValue::ESEmptyValue));
            }
        } else {
            if (newLength < m_length) {
                for (unsigned i = newLength; i < m_length; i++) {
                    deleteProperty(ESValue(i));
                }
            }
        }
        m_length = newLength;
    }

    const bool& isFastmode()
    {
        return m_fastmode;
    }

    const uint32_t& length()
    {
        return (const uint32_t &)m_length;
    }

    bool DefineOwnProperty(ESValue& key, ESObject* desc, bool throwFlag);

#ifdef ENABLE_ESJIT
    static size_t offsetOfVectorData() { return offsetof(ESArrayObject, m_vector); }
    static size_t offsetOfLength() { return offsetof(ESArrayObject, m_length); }
    static size_t offsetOfIsFastMode() { return offsetof(ESArrayObject, m_fastmode); }
#endif

protected:
    uint32_t m_length;
    ESValueVector m_vector;
    bool m_fastmode;
    static const unsigned MAX_FASTMODE_SIZE = 65536 * 2;
};

class LexicalEnvironment;
class Node;
class ESFunctionObject : public ESObject {
protected:
    ESFunctionObject(LexicalEnvironment* outerEnvironment, CodeBlock* codeBlock, escargot::ESString* name, unsigned length, bool hasPrototype = true);
    ESFunctionObject(LexicalEnvironment* outerEnvironment, NativeFunctionType fn, escargot::ESString* name, unsigned length, bool isConstructor);
public:
    static ESFunctionObject* create(LexicalEnvironment* outerEnvironment, CodeBlock* codeBlock, escargot::ESString* name, unsigned length = 0, bool hasPrototype = true)
    {
        ESFunctionObject* ret = new ESFunctionObject(outerEnvironment, codeBlock, name, length, hasPrototype);
        return ret;
    }

    // Built-in functions that are not constructors do not have prototype property(ECMA 5.1 $15)
    static ESFunctionObject* create(LexicalEnvironment* outerEnvironment, const NativeFunctionType& fn, escargot::ESString* name, unsigned length = 0, bool isConstructor = false)
    {
        ESFunctionObject* ret = new ESFunctionObject(outerEnvironment, fn, name, length, isConstructor);
        return ret;
    }

    void initialize(LexicalEnvironment* outerEnvironment, CodeBlock* codeBlock)
    {
        m_outerEnvironment = outerEnvironment;
        m_codeBlock = codeBlock;
    }

    ALWAYS_INLINE const ESValue& protoType()
    {
        return m_protoType;
    }

    ALWAYS_INLINE void setProtoType(const ESValue& obj)
    {
        m_protoType = obj;
        setValueAsProtoType(obj);
    }

    CodeBlock* codeBlock() { return m_codeBlock; }

    LexicalEnvironment* outerEnvironment() { return m_outerEnvironment; }

    ALWAYS_INLINE escargot::ESString* name()
    {
        return m_name;
    }

    bool nonConstructor() { return m_nonConstructor; }
    void setBoundFunc()
    {
        m_is_bound_func = true;
    }
    bool isBoundFunc()
    {
        return m_is_bound_func;
    }

    static ESValue call(ESVMInstance* instance, const ESValue& callee, const ESValue& receiver, ESValue arguments[], const size_t& argumentCount, bool isNewExpression);
protected:
    LexicalEnvironment* m_outerEnvironment;
    ESValue m_protoType;

    CodeBlock* m_codeBlock;
    escargot::ESString* m_name;
    bool m_nonConstructor;
    bool m_is_bound_func;
    // ESObject functionObject;
    // HomeObject
    // //ESObject newTarget
    // BindThisValue(V);
    // GetThisBinding();
};

class ESStringObject : public ESObject {
protected:
    ESStringObject(escargot::ESString* str);
public:
    static ESStringObject* create(escargot::ESString* str = strings->emptyString.string())
    {
        return new ESStringObject(str);
    }

    ALWAYS_INLINE size_t length()
    {
        return m_stringData->length();
    }

    ALWAYS_INLINE ::escargot::ESString* stringData()
    {
        return m_stringData;
    }

    ALWAYS_INLINE void setStringData(::escargot::ESString* str)
    {
        m_stringData = str;
    }

    ALWAYS_INLINE ::escargot::ESString* getCharacterAsString(size_t index)
    {
        ASSERT(index < m_stringData->length());
        if (m_stringData->data()[index] < ESCARGOT_ASCII_TABLE_MAX)
            return strings->asciiTable[m_stringData->data()[index]].string();
        else
            return ESString::create(m_stringData->data()[index]);
    }

#ifdef ENABLE_ESJIT
    static size_t offsetOfStringData() { return offsetof(ESStringObject, m_stringData); }
#endif

private:
    ::escargot::ESString* m_stringData;
};

class ESNumberObject : public ESObject {
protected:
    ESNumberObject(double value);

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
    ESBooleanObject(bool value);

public:
    static ESBooleanObject* create(bool value)
    {
        return new ESBooleanObject(value);
    }

    void setBooleanData(bool value) { m_primitiveValue = value; }
    ALWAYS_INLINE bool booleanData() { return m_primitiveValue; }

private:
    bool m_primitiveValue;
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
    static ESRegExpObject* create(escargot::ESString* source, const Option& option)
    {
        ESRegExpObject* ret = new ESRegExpObject(source, option);
        return ret;
    }

    ALWAYS_INLINE Option option() { return m_option; }
    ALWAYS_INLINE const escargot::ESString* source() { return m_source; }
    ALWAYS_INLINE ESValue lastIndex() { return m_lastIndex; }
    bool setSource(escargot::ESString* src);
    void setOption(const Option& option);
    void setLastIndex(const ESValue& lastIndex) { m_lastIndex = lastIndex; }

    JSC::Yarr::YarrPattern* yarrPattern()
    {
        return m_yarrPattern;
    }

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
    JSC::Yarr::YarrPattern* m_yarrPattern;
    JSC::Yarr::BytecodePattern* m_bytecodePattern;
    Option m_option;

    ESValue m_lastIndex;
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
    ESArrayBufferObject(ESPointer::Type type = ESPointer::Type::ESArrayBufferObject);

public:
    static ESArrayBufferObject* create()
    {
        return new ESArrayBufferObject();
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
        m_data = GC_MALLOC_ATOMIC(bytelength);
        memset(m_data, 0, bytelength);
    }

    bool isDetachedBuffer()
    {
        if (data() == NULL)
            return true;
        return false;
    }

    void detachArrayBuffer()
    {
        m_data = NULL;
        m_bytelength = 0;
    }

    ALWAYS_INLINE void* data() { return m_data; }
    ALWAYS_INLINE unsigned bytelength() { return m_bytelength; }

    // $24.1.1.5
    template<typename Type>
    ESValue getValueFromBuffer(unsigned byteindex, TypedArrayType typeVal, int isLittleEndian = -1)
    {
        ASSERT(!isDetachedBuffer());
        ASSERT(byteindex >= 0 && byteindex + sizeof(Type) <= m_bytelength);
        if (isLittleEndian != -1) {
            // TODO
            RELEASE_ASSERT_NOT_REACHED();
        }
        // If isLittleEndian is not present, set isLittleEndian to either true or false.
        void* rawStart = (int8_t*)m_data + byteindex;
        return ESValue( *((Type*) rawStart) );
    }
    // $24.1.1.6
    template<typename TypeAdaptor>
    bool setValueInBuffer(unsigned byteindex, TypedArrayType typeVal, ESValue val, int isLittleEndian = -1)
    {
        ASSERT(!isDetachedBuffer());
        ASSERT(byteindex >= 0 && byteindex + sizeof(typename TypeAdaptor::Type) <= m_bytelength);
        if (isLittleEndian != -1) {
            // TODO
            RELEASE_ASSERT_NOT_REACHED();
        }
        // If isLittleEndian is not present, set isLittleEndian to either true or false.
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
    ESArrayBufferView(ESPointer::Type type, ESValue __proto__);
public:
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
    static TypeArg toNative(ESValue val)
    {
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
    ESTypedArrayObjectWrapper(TypedArrayType arraytype, ESPointer::Type type, ESValue __proto__)
        : ESArrayBufferView(type, __proto__)
        , m_arraytype(arraytype)
    {
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
    inline ESTypedArrayObject(TypedArrayType arraytype,
        ESPointer::Type type = ESPointer::Type::ESTypedArrayObject);

public:
    static ESTypedArrayObject* create()
    {
        return new ESTypedArrayObject(TypeAdaptor::typeVal);
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
        if (key < 0 || key >= arraylength())
            return false;
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
        : ESArrayBufferView((Type)(Type::ESObject | Type::ESDataViewObject), ESValue()) // TODO set __proto__ properly
    {
        // TODO
        RELEASE_ASSERT_NOT_REACHED();
    }

public:
    static ESDataViewObject* create()
    {
        return new ESDataViewObject();
    }
};

class ESArgumentsObject : public ESObject {
protected:
    ESArgumentsObject(ESPointer::Type type = ESPointer::Type::ESArgumentsObject);

public:
    static ESArgumentsObject* create()
    {
        return new ESArgumentsObject();
    }
};

class ESControlFlowRecord : public ESPointer {
public:
    enum ControlFlowReason {
        NeedsReturn,
        NeedsJump,
        NeedsThrow,
    };
protected:
    ESControlFlowRecord(const ControlFlowReason& reason, const ESValue& value)
        : ESPointer(ESPointer::ESControlFlowRecord)
    {
        m_reason = reason;
        m_value = value;
    }

    ESControlFlowRecord(const ControlFlowReason& reason, const ESValue& value, const ESValue& value2)
        : ESPointer(ESPointer::ESControlFlowRecord)
    {
        m_reason = reason;
        m_value = value;
        m_value2 = value2;
    }

public:
    ESControlFlowRecord* clone()
    {
        return new ESControlFlowRecord(*this);
    }

    static ESControlFlowRecord* create(const ControlFlowReason& reason, const ESValue& value)
    {
        return new ESControlFlowRecord(reason, value);
    }

    static ESControlFlowRecord* create(const ControlFlowReason& reason, const ESValue& value, const ESValue& value2)
    {
        return new ESControlFlowRecord(reason, value, value2);
    }

    const ControlFlowReason& reason()
    {
        return m_reason;
    }

    const ESValue& value()
    {
        return m_value;
    }

    const ESValue& value2()
    {
        return m_value2;
    }

    void setValue(const ESValue& v)
    {
        m_value = v;
    }

    void setValue2(const ESValue& v)
    {
        m_value2 = v;
    }

protected:
    ControlFlowReason m_reason;
    ESValue m_value;
    ESValue m_value2;
};

}
#include "vm/ESVMInstance.h"
#include "ESValueInlines.h"

#endif
