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
class FunctionEnvironmentRecordWithArgumentsObject;
class ESHiddenClassPropertyInfo;

union ValueDescriptor {
    int64_t asInt64;
#ifdef ESCARGOT_32
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

#ifdef ESCARGOT_64
#define CellPayloadOffset 0
#else
#define CellPayloadOffset PayloadOffset
#endif

class ESValue {
public:
#ifdef ESCARGOT_32
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
    ALWAYS_INLINE bool isUndefined() const;
    ALWAYS_INLINE bool isNull() const;
    ALWAYS_INLINE bool isUndefinedOrNull() const
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
    ALWAYS_INLINE uint32_t toUint32() const; // http://www.ecma-international.org/ecma-262/5.1/#sec-9.6
    ALWAYS_INLINE ESString* toString() const; // $7.1.12 ToString
    ESString* toStringSlowCase() const; // $7.1.12 ToString
    inline ESObject* toObject() const; // $7.1.13 ToObject
    inline double toLength() const; // $7.1.15 ToLength

    enum { ESInvalidIndexValue = std::numeric_limits<uint32_t>::max() };
    ALWAYS_INLINE uint32_t toIndex() const; // http://www.ecma-international.org/ecma-262/5.1/#sec-15.4

    ALWAYS_INLINE ESString* asESString() const;

    ALWAYS_INLINE bool isESPointer() const;
    ALWAYS_INLINE ESPointer* asESPointer() const;

#ifdef ENABLE_ESJIT
    static ESValueInDouble toRawDouble(ESValue);
    static ESValue fromRawDouble(ESValueInDouble);
#endif

    static ptrdiff_t offsetOfPayload() { return OBJECT_OFFSETOF(ESValue, u.asBits.payload); }
    static ptrdiff_t offsetOfTag() { return OBJECT_OFFSETOF(ESValue, u.asBits.tag); }

#ifdef ESCARGOT_32
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
        TypeMask = 0x1ffff,
        TotalNumberOfTypes = 18
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

struct NullableUTF8String {
    NullableUTF8String(const char* buffer, const size_t& bufferSize)
    {
        m_buffer = buffer;
        m_bufferSize = bufferSize;
    }
    const char* m_buffer;
    size_t m_bufferSize;
};

struct NullableUTF16String {
    NullableUTF16String(const char16_t* buffer, const size_t& length)
    {
        m_buffer = buffer;
        m_length = length;
    }
    const char16_t* m_buffer;
    size_t m_length;
};

template <typename T>
size_t stringHash(const T* s, size_t len, size_t seed = 0)
{
    unsigned int hash = seed;
    for (size_t i = 0 ; i < len ; i ++) {
        hash = hash * 101  +  s[i];
    }
    return hash;
}

inline bool isAllASCII(const char* buf, const size_t& len)
{
    for (unsigned i = 0; i < len ; i ++) {
        if ((buf[i] & 0x80) != 0) {
            return false;
        }
    }
    return true;
}

inline bool isAllASCII(const char16_t* buf, const size_t& len)
{
    for (unsigned i = 0; i < len ; i ++) {
        if (buf[i] >= 128) {
            return false;
        }
    }
    return true;
}

static const char32_t offsetsFromUTF8[6] = { 0x00000000UL, 0x00003080UL, 0x000E2080UL, 0x03C82080UL, static_cast<char32_t>(0xFA082080UL), static_cast<char32_t>(0x82082080UL) };

inline char32_t readUTF8Sequence(const char*& sequence)
{
    unsigned length;
    const char sch = *sequence;
    if ((sch & 0x80) == 0)         length = 1;
    else if ((sch & 0xE0) == 0xC0) length = 2;
    else if ((sch & 0xF0) == 0xE0) length = 3;
    else if ((sch & 0xF8) == 0xF0) length = 4;
    else { RELEASE_ASSERT_NOT_REACHED(); }

    char32_t ch = 0;
    switch (length) {
        case 6: ch += static_cast<unsigned char>(*sequence++); ch <<= 6; // FALLTHROUGH;
        case 5: ch += static_cast<unsigned char>(*sequence++); ch <<= 6; // FALLTHROUGH;
        case 4: ch += static_cast<unsigned char>(*sequence++); ch <<= 6; // FALLTHROUGH;
        case 3: ch += static_cast<unsigned char>(*sequence++); ch <<= 6; // FALLTHROUGH;
        case 2: ch += static_cast<unsigned char>(*sequence++); ch <<= 6; // FALLTHROUGH;
        case 1: ch += static_cast<unsigned char>(*sequence++);
    }
    return ch - offsetsFromUTF8[length - 1];
}

inline UTF16String utf8StringToUTF16String(const char* buf, const size_t& len)
{
    UTF16String str;
    const char* source = buf;
    while (source < buf + len) {
        char32_t ch = readUTF8Sequence(source);
        if (((uint32_t)(ch)<=0xffff)) { // BMP
            if ((((ch)&0xfffff800)==0xd800)) { // SURROGATE
                str += 0xFFFD;
            } else {
                str += ch; // normal case
            }
        } else if (((uint32_t)((ch)-0x10000)<=0xfffff)) { // SUPPLEMENTARY
            str += (char16_t)(((ch)>>10)+0xd7c0);   // LEAD
            str += (char16_t)(((ch)&0x3ff)|0xdc00); // TRAIL
        } else {
            str += 0xFFFD;
        }
    }

    return UTF16String(std::move(str));
}

inline ASCIIString utf16StringToASCIIString(const char16_t* buf, const size_t& len)
{
    ASCIIString str;
    str.reserve(len);
    for (unsigned i = 0 ; i < len ; i ++) {
        ASSERT(buf[i] < 128);
        str += buf[i];
    }
    return ASCIIString(std::move(str));
}

ASCIIString dtoa(double number);

class ESString : public ESPointer {
    friend class ESScriptParser;
    friend class ESRopeString;
protected:
    ESString(ESPointer::Type type = Type::ESString)
        : ESPointer(type)
    { }
    ESString* data() const;
public:
    static ESString* create(ASCIIString&& src);
    static ESString* create(const ASCIIString& src);
    static ESString* create(UTF16String&& src);
    static ESString* createASCIIStringIfNeeded(const char16_t* src, size_t len)
    {
        if (isAllASCII(src, len)) {
            return ESString::create(utf16StringToASCIIString(src, len));
        }
        return ESString::create(std::move(UTF16String(src, &src[len])));
    }

    static ESString* createASCIIStringIfNeeded(UTF16String&& src)
    {
        if (isAllASCII(src.data(), src.length())) {
            return ESString::create(utf16StringToASCIIString(src.data(), src.length()));
        }
        return ESString::create(std::move(src));
    }

    static ESString* createUTF16StringIfNeeded(const char* src, size_t len)
    {
        if (isAllASCII(src, len)) {
            return ESString::create(std::move(ASCIIString(src, &src[len])));
        }
        return ESString::create(utf8StringToUTF16String(src, len));
    }

    static ESString* createUTF16StringIfNeeded(ASCIIString&& src)
    {
        if (isAllASCII(src.data(), src.length())) {
            return ESString::create(std::move(src));
        }
        return ESString::create(utf8StringToUTF16String(src.data(), src.length()));
    }

    static ESString* create(const UTF16String& src);
    static ESString* create(int number);
    static ESString* create(double number);
    static ESString* create(char c);
    static ESString* create(char16_t c);
    static ESString* create(const char* str);
    static ESString* createAtomicString(const char* str);
    static ESString* concatTwoStrings(ESString* lstr, ESString* rstr);
    NullableUTF8String toNullableUTF8String() const;
    NullableUTF16String toNullableUTF16String() const;
    ASCIIString* asASCIIString() const;
    UTF16String* asUTF16String() const;
    ASCIIString* uncheckedAsASCIIString() const;
    UTF16String* uncheckedAsUTF16String() const;
    UTF16String toUTF16String() const;
    const char* asciiData() const;
    const char16_t* utf16Data() const;
    const char* utf8Data() const;
    bool isASCIIString() const;
    // for yarr
    bool is8Bit() const
    {
        return isASCIIString();
    }

    // convert ESString, ESRopeString into ESString
    ESString* unwrap() const
    {
        return data();
    }

    const char* characters8() const
    {
        ASSERT(is8Bit());
        return toNullableUTF8String().m_buffer;
    }

    const char16_t* characters16() const
    {
        ASSERT(!is8Bit());
        return toNullableUTF16String().m_buffer;
    }

    char16_t charAt(const size_t& idx) const;
    char16_t operator[](const size_t& idx) const
    {
        return charAt(idx);
    }

    uint32_t tryToUseAsIndex();
    bool hasOnlyDigit()
    {
        bool allOfCharIsDigit = true;
        size_t siz = length();
        for (unsigned i = 0; i < siz ; i ++) {
            char16_t c = charAt(i);
            if (c < '0' || c > '9') {
                allOfCharIsDigit = false;
                break;
            }
        }
        return allOfCharIsDigit;
    }

    ALWAYS_INLINE size_t length() const;
    ALWAYS_INLINE size_t hashValue() const;

    size_t find(ESString* str, size_t pos = 0)
    {
        const size_t srcStrLen = str->length();
        const size_t size = length();

        if (srcStrLen == 0)
            return pos <= size ? pos : -1;

        if (srcStrLen <= size) {
            char16_t src0 = str->charAt(0);
            for (; pos <= size - srcStrLen; ++pos) {
                if (charAt(pos) == src0) {
                    bool same = true;
                    for (size_t k = 1; k < srcStrLen; k++) {
                        if (charAt(pos + k) != str->charAt(k)) {
                            same = false;
                            break;
                        }
                    }
                    if (same)
                        return pos;
                }
            }
        }
        return -1;
    }

    size_t rfind(ESString* str, size_t pos = 0)
    {
        const size_t srcStrLen = str->length();
        const size_t size = length();

        if (srcStrLen == 0)
            return pos <= size ? pos : -1;

        if (srcStrLen <= size) {
            do {
                bool same = true;
                for (size_t k = 0; k < srcStrLen; k++) {
                    if (charAt(pos + k) != str->charAt(k)) {
                        same = false;
                        break;
                    }
                }
                if (same)
                    return pos;
            } while (pos-- > 0);
        }
        return -1;
    }
    ESString* substring(int from, int to) const;

    struct RegexMatchResult {
        struct RegexMatchResultPiece {
            unsigned m_start, m_end;
        };
        COMPILE_ASSERT((sizeof(RegexMatchResultPiece)) == (sizeof(unsigned) * 2), sizeof_RegexMatchResultPiece_wrong);
        int m_subPatternNum;
        std::vector<std::vector<RegexMatchResultPiece, pointer_free_allocator<RegexMatchResultPiece> >, gc_allocator<std::vector<RegexMatchResultPiece, pointer_free_allocator<RegexMatchResultPiece> >> > m_matchResults;
    };
    bool match(ESPointer* esptr, RegexMatchResult& result, bool testOnly = false, size_t startIndex = 0) const;

    ESString(const ESString& s) = delete;
    void operator =(const ESString& s) = delete;

    ALWAYS_INLINE friend bool operator == (const ESString& a, const char* b);
    ALWAYS_INLINE friend bool operator != (const ESString& a, const char* b);
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

protected:
#ifdef ESCARGOT_64
    struct {
        bool m_isASCIIString:1;
        size_t m_hashData:63;
    } m_data;
#else
    struct {
        bool m_isASCIIString:1;
        size_t m_hashData:31;
    } m_data;
#endif
#ifndef NDEBUG
    ASCIIString* m_asciiString;
    UTF16String* m_utf16String;
#endif
};

class ESASCIIString : public ESString, public ASCIIString {
    friend class ESString;
protected:
    ESASCIIString(ASCIIString&& src)
        : ESString()
        , ASCIIString(std::move(src))
    {
        m_data.m_isASCIIString = true;
        m_bufferRoot = ASCIIString::data();
#ifndef NDEBUG
        m_asciiString = asASCIIString();
#endif
        m_data.m_hashData = stringHash(ASCIIString::data(), ASCIIString::length());
    }

    // FIXME
    // for protect string buffer
    // gcc stores buffer of basic_string with special way
    // so we should root buffer manually
    const void* m_bufferRoot;
};

class ESUTF16String : public ESString, public UTF16String {
    friend class ESString;
protected:
    ESUTF16String(UTF16String&& src)
        : ESString()
        , UTF16String(std::move(src))
    {
        m_data.m_isASCIIString = false;
        m_bufferRoot = UTF16String::data();
#ifndef NDEBUG
        m_utf16String = asUTF16String();
#endif
        m_data.m_hashData = stringHash(UTF16String::data(), UTF16String::length());
    }

    // FIXME
    // for protect string buffer
    // gcc stores buffer of basic_string with special way
    // so we should root buffer manually
    const void* m_bufferRoot;
};

template <typename T>
ALWAYS_INLINE bool stringEqual(const T* s, const T* s1, const size_t& len)
{
    return memcmp(s, s1, sizeof(T) * len) == 0;
}

ALWAYS_INLINE bool stringEqual(const char16_t* s, const char* s1, const size_t& len)
{
    for (size_t i = 0; i < len ; i ++) {
        if (s[i] != s1[i]) {
            return false;
        }
    }
    return true;
}

ALWAYS_INLINE bool operator == (const ESString& a, const char* b)
{
    if (a.length() == strlen(b)) {
        if (a.isASCIIString()) {
            return stringEqual(a.asciiData(), b, a.length());
        } else {
            return stringEqual(a.utf16Data(), b, a.length());
        }
    }
    return false;
}

ALWAYS_INLINE bool operator != (const ESString& a, const char* b)
{
    return !(a == b);
}

ALWAYS_INLINE bool operator == (const ESString& a, const ESString& b)
{
    size_t lenA = a.length();
    size_t lenB = b.length();
    if (lenA == lenB) {
        if (a.hashValue() == b.hashValue()) {
            bool aa = a.isASCIIString();
            bool bb = b.isASCIIString();
            ESString* dataA = a.unwrap();
            ESString* dataB = b.unwrap();
            if (aa && bb) {
                return stringEqual(dataA->uncheckedAsASCIIString()->data(), dataB->uncheckedAsASCIIString()->data(), lenA);
            } else if (aa && !bb) {
                return stringEqual(dataB->uncheckedAsUTF16String()->data(), dataA->uncheckedAsASCIIString()->data(), lenA);
            } else if (!aa && bb) {
                return stringEqual(dataA->uncheckedAsUTF16String()->data(), dataB->uncheckedAsASCIIString()->data(), lenA);
            } else {
                return stringEqual(dataA->uncheckedAsUTF16String()->data(), dataB->uncheckedAsUTF16String()->data(), lenA);
            }
        }
    }
    return false;
}

ALWAYS_INLINE bool operator != (const ESString& a, const ESString& b)
{
    return !operator ==(a, b);
}

template<typename T1, typename T2>
ALWAYS_INLINE int stringCompare(size_t l1, size_t l2, const T1* c1, const T2* c2)
{
    const unsigned lmin = l1 < l2 ? l1 : l2;
    unsigned pos = 0;
    while (pos < lmin && *c1 == *c2) {
        ++c1;
        ++c2;
        ++pos;
    }

    if (pos < lmin)
        return (c1[0] > c2[0]) ? 1 : -1;

    if (l1 == l2)
        return 0;

    return (l1 > l2) ? 1 : -1;
}

ALWAYS_INLINE int stringCompare(const ESString& a, const ESString& b)
{
    size_t lenA = a.length();
    bool aa = a.isASCIIString();
    size_t lenB = b.length();
    bool bb = b.isASCIIString();

    if (aa && bb) {
        return stringCompare(lenA, lenB, a.asciiData(), b.asciiData());
    } else if (!aa && bb) {
        return stringCompare(lenA, lenB, a.utf16Data(), b.asciiData());
    } else if (aa && !bb) {
        return stringCompare(lenA, lenB, a.asciiData(), b.utf16Data());
    } else {
        return stringCompare(lenA, lenB, a.utf16Data(), b.utf16Data());
    }
}

ALWAYS_INLINE bool operator < (const ESString& a, const ESString& b)
{
    return stringCompare(a, b) < 0;
}

ALWAYS_INLINE bool operator > (const ESString& a, const ESString& b)
{
    return stringCompare(a, b) > 0;
}

ALWAYS_INLINE bool operator <= (const ESString& a, const ESString& b)
{
    return stringCompare(a, b) <= 0;
}

ALWAYS_INLINE bool operator >= (const ESString& a, const ESString& b)
{
    return stringCompare(a, b) >= 0;
}

typedef std::vector<ESString *, gc_allocator<ESString *> > ESStringVector;

class ESRopeString : public ESString {
    friend class ESString;
protected:
    ESRopeString()
        : ESString((ESPointer::Type)(ESPointer::ESString | ESPointer::ESRopeString))
    {
        m_contentLength = 0;
        m_hasNonASCIIString = false;
        m_left = nullptr;
        m_right = nullptr;
        m_content = nullptr;
    }
public:
    static const unsigned ESRopeStringCreateMinLimit = 24;
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

        bool hasNonASCIIChild = false;
        if (lstr->isESRopeString()) {
            hasNonASCIIChild |= lstr->asESRopeString()->hasNonASCIIChild();
        } else {
            hasNonASCIIChild |= !lstr->isASCIIString();
        }

        if (rstr->isESRopeString()) {
            hasNonASCIIChild |= rstr->asESRopeString()->hasNonASCIIChild();
        } else {
            hasNonASCIIChild |= !rstr->isASCIIString();
        }

        rope->m_hasNonASCIIString = hasNonASCIIChild;
        return rope;
    }

    bool hasNonASCIIChild() const
    {
        return m_hasNonASCIIString;
    }

    ESString* string()
    {
        ASSERT(isESRopeString());
        if (m_content) {
            return m_content;
        }
        if (m_hasNonASCIIString) {
            UTF16String result;
            // TODO: should reduce unnecessary append operations in std::string::resize
            result.resize(m_contentLength);
            std::vector<ESString *, gc_allocator<ESString *>> queue;
            queue.push_back(m_left);
            queue.push_back(m_right);
            size_t pos = m_contentLength;
            while (!queue.empty()) {
                ESString* cur = queue.back();
                queue.pop_back();
                if (cur->isESRopeString() && (cur->asESRopeString()->m_content == nullptr)) {
                    ESRopeString* rs = cur->asESRopeString();
                    ASSERT(rs->m_left);
                    ASSERT(rs->m_right);
                    queue.push_back(rs->m_left);
                    queue.push_back(rs->m_right);
                } else {
                    ESString* str = cur;
                    if (cur->isESRopeString()) {
                        str = cur->asESRopeString()->m_content;
                    }
                    pos -= str->length();
                    char16_t* buf = const_cast<char16_t *>(result.data());
                    for (size_t i = 0 ; i < str->length() ; i ++) {
                        buf[i + pos] = str->charAt(i);
                    }
                }
            }
            m_content =  ESString::create(std::move(result));
            m_left = nullptr;
            m_right = nullptr;
            return m_content;
        } else {
            ASCIIString result;
            // TODO: should reduce unnecessary append operations in std::string::resize
            result.resize(m_contentLength);
            std::vector<ESString *, gc_allocator<ESString *>> queue;
            queue.push_back(m_left);
            queue.push_back(m_right);
            int pos = m_contentLength;
            while (!queue.empty()) {
                ESString* cur = queue.back();
                queue.pop_back();
                if (cur && cur->isESRopeString() && cur->asESRopeString()->m_content == nullptr) {
                    ESRopeString* rs = cur->asESRopeString();
                    ASSERT(rs->m_left);
                    ASSERT(rs->m_right);
                    queue.push_back(rs->m_left);
                    queue.push_back(rs->m_right);
                } else {
                    ESString* str = cur;
                    if (cur->isESRopeString()) {
                        str = cur->asESRopeString()->m_content;
                    }
                    ASSERT(str->isASCIIString());
                    pos -= str->length();
                    memcpy((void*)(result.data() + pos), str->asciiData(), str->length() * sizeof(char));
                }
            }
            m_content = ESString::create(std::move(result));
            m_left = nullptr;
            m_right = nullptr;
            return m_content;
        }
    }

#ifdef ENABLE_ESJIT
    static size_t offsetOfContentLength() { return offsetof(ESRopeString, m_contentLength); }
#endif

public:
    size_t contentLength()
    {
        return m_contentLength;
    }

protected:
    ESString* m_left;
    ESString* m_right;
    ESString* m_content;
    size_t m_contentLength;
    bool m_hasNonASCIIString;
};


inline ESString* ESString::create(ASCIIString&& src)
{
    return new ESASCIIString(std::move(src));
}

inline ESString* ESString::create(const ASCIIString& src)
{
    return new ESASCIIString(std::move(ASCIIString(src)));
}

inline ESString* ESString::create(UTF16String&& src)
{
    return new ESUTF16String(std::move(src));
}

inline ESString* ESString::create(const UTF16String& src)
{
    return new ESUTF16String(std::move(UTF16String(src)));
}

inline ESString* ESString::create(int number)
{
    return ESString::create((double)number);
}

inline ESString* ESString::create(char c)
{
    return new ESASCIIString(std::move(ASCIIString({c})));
}

inline ESString* ESString::create(char16_t c)
{
    return new ESUTF16String(std::move(UTF16String({c})));
}

inline ESString* ESString::data() const
{
    if (UNLIKELY(isESRopeString())) {
        return ((escargot::ESRopeString*)this)->string();
    } else {
        return (ESString*)this;
    }
}

inline char16_t ESString::charAt(const size_t& idx) const
{
    ESString* s = data();
    if (LIKELY(s->m_data.m_isASCIIString)) {
        return (*s->uncheckedAsASCIIString())[idx];
    } else {
        return (*s->uncheckedAsUTF16String())[idx];
    }
}

inline ASCIIString* ESString::asASCIIString() const
{
    ASSERT(isASCIIString());
    size_t ptr = (size_t)data();
    ptr += sizeof(ESString);
    return (ASCIIString*)ptr;
}

inline UTF16String* ESString::asUTF16String() const
{
    ASSERT(!isASCIIString());
    size_t ptr = (size_t)data();
    ptr += sizeof(ESString);
    return (UTF16String*)ptr;
}

inline ASCIIString* ESString::uncheckedAsASCIIString() const
{
    ASSERT(isASCIIString());
    size_t ptr = (size_t)this;
    ptr += sizeof(ESString);
    return (ASCIIString*)ptr;
}

inline UTF16String* ESString::uncheckedAsUTF16String() const
{
    ASSERT(!isASCIIString());
    size_t ptr = (size_t)this;
    ptr += sizeof(ESString);
    return (UTF16String*)ptr;
}

inline size_t ESString::length() const
{
    if (UNLIKELY(isESRopeString())) {
        escargot::ESRopeString* rope = (escargot::ESRopeString *)this;
        return rope->contentLength();
    }

    // NOTE std::basic_string has internal length variable.
    // so we should not check its type.
    // but this implemetion only checked with gcc
    size_t ptr = (size_t)this;
    ptr += sizeof(ESString);
    return ((ASCIIString*)ptr)->length();
/*
    // original code
    if (isASCIIString()) {
        return asASCIIString()->length();
    } else {
        return asUTF16String()->length();
    }
*/
}

inline size_t ESString::hashValue() const
{
    if (UNLIKELY(isESRopeString())) {
        escargot::ESRopeString* rope = (escargot::ESRopeString *)this;
        return rope->string()->hashValue();
    }
    return m_data.m_hashData;
}

inline const char* ESString::asciiData() const
{
    return asASCIIString()->data();
}

inline const char16_t* ESString::utf16Data() const
{
    return asUTF16String()->data();
}

inline const char* ESString::utf8Data() const
{
    ESString* s = data();
    if (s->isASCIIString()) {
        return s->asASCIIString()->data();
    } else {
        return utf16ToUtf8(s->asUTF16String()->data(), s->asUTF16String()->length());
    }
}

inline NullableUTF8String ESString::toNullableUTF8String() const
{
    ESString* s = data();
    if (s->isASCIIString()) {
        return NullableUTF8String(s->asASCIIString()->data(), s->asASCIIString()->length());
    } else {
        size_t siz;
        const char* buf = utf16ToUtf8(s->asUTF16String()->data(), s->asUTF16String()->length(), &siz);
        return NullableUTF8String(buf, siz - 1);
    }
}

inline NullableUTF16String ESString::toNullableUTF16String() const
{
    ESString* s = data();
    if (s->isASCIIString()) {
        size_t siz = s->asASCIIString()->length();
        char16_t* v = (char16_t *)GC_MALLOC_ATOMIC(sizeof(char16_t) * siz);
        const char* src = s->asASCIIString()->data();
        for (size_t i = 0; i < siz ; i ++) {
            v[i] = src[i];
        }
        return NullableUTF16String(v, siz);
    } else {
        return NullableUTF16String(s->asUTF16String()->data(), s->asUTF16String()->length());
    }
}

inline UTF16String ESString::toUTF16String() const
{
    ESString* ss = data();
    if (isASCIIString()) {
        UTF16String str;
        size_t s = ss->asASCIIString()->length();
        str.reserve(s);
        const char* src = ss->asASCIIString()->data();
        for (size_t i = 0; i < s ; i ++) {
            str += src[i];
        }
        return std::move(str);
    } else {
        return *(ss->asUTF16String());
    }
}

inline bool ESString::isASCIIString() const
{
    if (UNLIKELY(isESRopeString())) {
        return !((escargot::ESRopeString*)this)->m_hasNonASCIIString;
    } else {
        return m_data.m_isASCIIString;
    }
}


}

namespace std {
template<> struct hash<escargot::ESString *> {
    size_t operator()(escargot::ESString * const &x) const
    {
        return x->hashValue();
    }
};

template<> struct equal_to<escargot::ESString *> {
    bool operator()(escargot::ESString * const &a, escargot::ESString * const &b) const
    {
        if (a == b) {
            return true;
        }
        return *a == *b;
    }
};

}


#include "runtime/InternalAtomicString.h"
#include "runtime/StaticStrings.h"

namespace escargot {

typedef ESValue (*ESNativeGetter)(::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName);
typedef void (*ESNativeSetter)(::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName, const ESValue& value);

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

    ALWAYS_INLINE ESValue value(::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName);
    ALWAYS_INLINE bool setValue(::escargot::ESObject* obj, ::escargot::ESObject* originalObj, ::escargot::ESString* propertyName, const ESValue& value);

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

    void setGetterAndSetterTo(ESObject* obj, const ESHiddenClassPropertyInfo* propertyInfo);

    // http://www.ecma-international.org/ecma-262/5.1/#sec-8.10.1
    bool isAccessorDescriptor()
    {
        return m_jsGetter || m_jsSetter;
    }

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
    gc_allocator<std::pair<const ::escargot::ESString*, size_t> > > ESHiddenClassPropertyIndexHashMapInfoStd;

class ESHiddenClassPropertyIndexHashMapInfo : public ESHiddenClassPropertyIndexHashMapInfoStd, public gc { };

typedef std::vector<::escargot::ESHiddenClassPropertyInfo, gc_allocator<::escargot::ESHiddenClassPropertyInfo> > ESHiddenClassPropertyInfoVectorStd;

class ESHiddenClassPropertyInfoVector : public ESHiddenClassPropertyInfoVectorStd {
    public:
#ifdef ENABLE_ESJIT
    static size_t offsetOfData() { return offsetof(ESHiddenClassPropertyInfoVector, _M_impl._M_start); }
#endif
};

typedef std::unordered_map<ESString*, ::escargot::ESHiddenClass **,
std::hash<ESString*>, std::equal_to<ESString*>,
gc_allocator<std::pair<ESString*, ::escargot::ESHiddenClass **> > > ESHiddenClassTransitionDataStd;

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
    static const unsigned ESHiddenClassVectorModeSizeLimit = 32;
    friend class ESVMInstance;
    friend class ESObject;
public:
    ALWAYS_INLINE size_t findProperty(const ESString* name)
    {
        if (LIKELY(m_propertyIndexHashMapInfo == NULL)) {
            size_t siz = m_propertyInfo.size();
            for (size_t i = 0 ; i < siz ; i ++) {
                if (*m_propertyInfo[i].m_name == *name)
                    return i;
            }
            return SIZE_MAX;
        } else {
            ASSERT(m_propertyIndexHashMapInfo);
            auto iter = m_propertyIndexHashMapInfo->find(const_cast<ESString *>(name));
            if (iter == m_propertyIndexHashMapInfo->end())
                return SIZE_MAX;
            return iter->second;
        }
    }

    ALWAYS_INLINE size_t findPropertyCheckDeleted(const ESString* name)
    {
        if (LIKELY(m_propertyIndexHashMapInfo == NULL)) {
            size_t siz = m_propertyInfo.size();
            for (size_t i = 0 ; i < siz ; i ++) {
                if (*m_propertyInfo[i].m_name == *name)
                    return i;
            }
            return SIZE_MAX;
        } else {
            ASSERT(m_propertyIndexHashMapInfo);
            auto iter = m_propertyIndexHashMapInfo->find(const_cast<ESString *>(name));
            if (iter == m_propertyIndexHashMapInfo->end())
                return SIZE_MAX;
            if (m_propertyInfo[iter->second].m_flags.m_isDeletedValue)
                return SIZE_MAX;
            return iter->second;
        }
    }

    void appendHashMapInfo(bool force = false)
    {
        if (m_propertyIndexHashMapInfo) {
            size_t idx = m_propertyInfo.size() - 1;
            m_propertyIndexHashMapInfo->insert(std::make_pair(m_propertyInfo[idx].m_name, idx));
        } else {
            fillHashMapInfo(force);
        }
    }

    void fillHashMapInfo(bool force = false)
    {
        if (force || m_propertyInfo.size() > 32) {
            if (m_propertyIndexHashMapInfo) {
                m_propertyIndexHashMapInfo->clear();
            } else {
                m_propertyIndexHashMapInfo = new ESHiddenClassPropertyIndexHashMapInfo();
            }

            for (unsigned i = 0; i < m_propertyInfo.size(); i ++) {
                m_propertyIndexHashMapInfo->insert(std::make_pair(m_propertyInfo[i].m_name, i));
            }

            ASSERT(m_propertyIndexHashMapInfo->size() == m_propertyInfo.size());
        }
    }

    inline ESHiddenClass* defineProperty(ESString* name, bool isData, bool isWritable, bool isEnumerable, bool isConfigurable);
    inline ESHiddenClass* removeProperty(ESString* name)
    {
        return removeProperty(findProperty(name));
    }
    inline ESHiddenClass* removeProperty(size_t idx);
    inline ESHiddenClass* removePropertyWithoutIndexChange(size_t idx);
    inline ESHiddenClass* morphToNonVectorMode();
    inline ESHiddenClass* forceNonVectorMode();
    bool isVectorMode()
    {
        return m_flags.m_isVectorMode;
    }

    ALWAYS_INLINE ESValue read(ESObject* obj, ESObject* originalObject, ESString* propertyName, ESString* name);
    ALWAYS_INLINE ESValue read(ESObject* obj, ESObject* originalObject, ESString* propertyName, size_t index);

    ALWAYS_INLINE bool write(ESObject* obj, ESObject* originalObject, ESString* propertyName, ESString* name, const ESValue& val);
    ALWAYS_INLINE bool write(ESObject* obj, ESObject* originalObject, ESString* propertyName, size_t index, const ESValue& val);

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
        m_propertyIndexHashMapInfo = NULL;
    }

    ESHiddenClassPropertyIndexHashMapInfo* m_propertyIndexHashMapInfo;
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

    inline bool defineDataProperty(InternalAtomicString name,
        bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true, const ESValue& initalValue = ESValue(), bool force = false)
    {
        return defineDataProperty(name.string(), isWritable, isEnumerable, isConfigurable, initalValue, force);
    }
    inline bool defineDataProperty(const escargot::ESValue& key,
        bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true, const ESValue& initalValue = ESValue(), bool force = false);
    inline bool defineAccessorProperty(const escargot::ESValue& key, ESPropertyAccessorData* data,
        bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true, bool force = false);
    inline bool defineAccessorProperty(const escargot::ESValue& key, ESNativeGetter getter, ESNativeSetter setter,
        bool isWritable, bool isEnumerable, bool isConfigurable, bool force = false)
    {
        return defineAccessorProperty(key, new ESPropertyAccessorData(getter, setter), isWritable, isEnumerable, isConfigurable, force);
    }
    inline bool defineAccessorProperty(escargot::ESString* key, ESNativeGetter getter,
        ESNativeSetter setter,
        bool isWritable, bool isEnumerable, bool isConfigurable, bool force = false)
    {
        return defineAccessorProperty(key, new ESPropertyAccessorData(getter, setter), isWritable, isEnumerable, isConfigurable, force);
    }

    inline bool deleteProperty(const ESValue& key, bool forced = false);
    inline void propertyFlags(const ESValue& key, bool& exists, bool& isDataProperty, bool& isWritable, bool& isEnumerable, bool& isConfigurable);
    inline bool hasProperty(const escargot::ESValue& key);
    inline bool hasOwnProperty(const escargot::ESValue& key);

    bool defineOwnProperty(ESValue& key, ESObject* desc, bool throwFlag);

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

    uint16_t extraData()
    {
        return m_flags.m_extraData;
    }

    void setExtraData(uint16_t e)
    {
        m_flags.m_extraData = e;
    }

    void forceNonVectorHiddenClass(bool forceFillHiddenClassInfo = false)
    {
        if (m_hiddenClass->m_flags.m_forceNonVectorMode)
            return;
        m_hiddenClass = m_hiddenClass->forceNonVectorMode();
        if (forceFillHiddenClassInfo) {
            m_hiddenClass->fillHashMapInfo(true);
        }
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
        // object
        bool m_isGlobalObject: 1;
        bool m_isExtensible: 1;
        bool m_isEverSetAsPrototypeObject: 1;

        // array
        bool m_isFastMode: 1;

        // function
        bool m_nonConstructor: 1;
        bool m_isBoundFunction: 1;

        // extra data
        uint16_t m_extraData: 16;

        size_t m_margin: 42;
    } m_flags;
#endif
#ifdef ESCARGOT_32
    struct {
        bool m_isGlobalObject: 1;
        bool m_isExtensible: 1;
        bool m_isEverSetAsPrototypeObject: 1;

        // array
        bool m_isFastMode: 1;

        // function
        bool m_nonConstructor: 1;
        bool m_isBoundFunction: 1;

        // extra data
        uint16_t m_extraData: 16;

        size_t m_margin: 10;
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

    static double parseStringToDate(escargot::ESString* istr);
    static void parseYmdhmsToDate(struct tm* timeinfo, int year, int month, int date, int hour, int minute, int second);

    void setTimeValue();
    void setTimeValue(double t);
    void setTimeValue(const ESValue str);
    void setTimeValue(int year, int month, int date, int hour, int minute, int second, int millisecond);
    void setTimeValueAsNaN()
    {
        m_hasValidDate = false;
    }

    double timeValueAsDouble()
    {
        if (m_hasValidDate) {
            return m_primitiveValue;
        } else {
            return std::numeric_limits<double>::quiet_NaN();
        }
    }

    escargot::ESString* toDateString();
    escargot::ESString* toTimeString();
    escargot::ESString* toFullString();
    int getDate();
    int getDay();
    int getFullYear();
    int getHours();
    int getMilliseconds();
    int getMinutes();
    int getMonth();
    int getSeconds();
    static long getTimezoneOffset();
    void setTime(double t);
    int getUTCDate();
    int getUTCDay();
    int getUTCFullYear();
    int getUTCHours();
    int getUTCMilliseconds();
    int getUTCMinutes();
    int getUTCMonth();
    int getUTCSeconds();
    static double toUTC(double t);
    static double ymdhmsToSeconds(long year, int month, int day, int hour, int minute, double second);

    static double timeClip(double V)
    {
        if (std::isinf(V) || std::isnan(V)) {
            return nan("0");
        } else if (std::abs(V) > 8.64 * std::pow(10, 15)) {
            return nan("0");
        } else {
            return ESValue(V).toInteger();
        }
    }

private:
    void resolveCache();
    struct tm m_cachedTM; // it stores time disregarding timezone
    long long m_primitiveValue; // it stores timevalue regarding timezone
    bool m_isCacheDirty;
    bool m_hasValidDate; // function get***() series (in ESValue.cpp) should check if the timevalue is valid with this flag

    static constexpr double hoursPerDay = 24.0;
    static constexpr double minutesPerHour = 60.0;
    static constexpr double secondsPerMinute = 60.0;
    static constexpr double secondsPerHour = secondsPerMinute * minutesPerHour;
    static constexpr double msPerSecond = 1000.0;
    static constexpr double msPerMinute = msPerSecond * secondsPerMinute;
    static constexpr double msPerHour = msPerSecond * secondsPerHour;
    static constexpr double msPerDay = msPerHour * hoursPerDay;

    static double day(long long t) { return floor(t / msPerDay); }
    static double timeWithinDay(long long t) { return (int) t % (int) msPerDay; }
    static int daysInYear(long year);
    static int dayFromYear(long year);
    static double timeFromYear(long year) { return msPerDay * dayFromYear(year); }
    static long yearFromTime(long long t);
    static int inLeapYear(long long t);
    static int dayFromMonth(long year, int month);
    static int monthFromTime(long long t);
    static int dateFromTime(long long t);
    static double makeDay(long year, int month, int date);
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
        if (m_flags.m_isFastMode) {
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
        if (m_flags.m_isFastMode) {
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
        if (m_flags.m_isFastMode && i > MAX_FASTMODE_SIZE)
            return true;
        return false;
    }

    void convertToSlowMode()
    {
        // wprintf(L"CONVERT TO SLOW MODE!!!  \n");
        if (!m_flags.m_isFastMode)
            return;
        forceNonVectorHiddenClass();
        m_flags.m_isFastMode = false;
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

    void set(const uint32_t& i, const ESValue& val)
    {
        ESObject::set(ESValue(i), val);
    }


    void setLength(unsigned newLength)
    {
        if (m_flags.m_isFastMode) {
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
                    size_t reservedSpace = std::min(MAX_FASTMODE_SIZE, (unsigned)(newLength*1.5f));
                    m_vector.reserve(reservedSpace);
                }
                m_vector.resize(newLength, ESValue(ESValue::ESEmptyValue));
            }
        } else {
            unsigned currentLength = m_length;
            if (newLength < currentLength) {
                std::vector<unsigned> indexes;
                enumeration([&](ESValue key) {
                    uint32_t index = key.toIndex();
                    if (index != ESValue::ESInvalidIndexValue) {
                        if (index >= newLength && index < currentLength)
                            indexes.push_back(index);
                    }
                });
                for (auto index : indexes)
                    deleteProperty(ESValue(index));
            }
        }
        m_length = newLength;
    }

    bool isFastmode()
    {
        return m_flags.m_isFastMode;
    }

    const uint32_t& length()
    {
        return (const uint32_t &)m_length;
    }

    bool defineOwnProperty(ESValue& key, ESObject* desc, bool throwFlag);

#ifdef ENABLE_ESJIT
    static size_t offsetOfVectorData() { return offsetof(ESArrayObject, m_vector); }
    static size_t offsetOfLength() { return offsetof(ESArrayObject, m_length); }
    // static size_t offsetOfIsFastMode() { return offsetof(ESArrayObject, m_flags.m_isFastMode); }
#endif

protected:
    uint32_t m_length;
    ESValueVector m_vector;
    static const unsigned MAX_FASTMODE_SIZE = 65536 * 2;
};

class LexicalEnvironment;
class Node;
class ESFunctionObject : public ESObject {
protected:
    ESFunctionObject(LexicalEnvironment* outerEnvironment, CodeBlock* codeBlock, escargot::ESString* name, unsigned length, bool hasPrototype = true, bool isBuiltIn = false);
    ESFunctionObject(LexicalEnvironment* outerEnvironment, NativeFunctionType fn, escargot::ESString* name, unsigned length, bool isConstructor, bool isBuiltIn = true);
public:
    static ESFunctionObject* create(LexicalEnvironment* outerEnvironment, CodeBlock* codeBlock, escargot::ESString* name, unsigned length = 0, bool hasPrototype = true)
    {
        ESFunctionObject* ret = new ESFunctionObject(outerEnvironment, codeBlock, name, length, hasPrototype);
        return ret;
    }

    // Built-in functions that are not constructors do not have prototype property(ECMA 5.1 $15)
    static ESFunctionObject* create(LexicalEnvironment* outerEnvironment, const NativeFunctionType& fn, escargot::ESString* name, unsigned length = 0, bool isConstructor = false, bool isBuiltIn = true)
    {
        ESFunctionObject* ret = new ESFunctionObject(outerEnvironment, fn, name, length, isConstructor, isBuiltIn);
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

    bool nonConstructor() { return m_flags.m_nonConstructor; }
    void setBoundFunc()
    {
        m_flags.m_isBoundFunction = true;
    }
    bool isBoundFunc()
    {
        return m_flags.m_isBoundFunction;
    }

    static ESValue call(ESVMInstance* instance, const ESValue& callee, const ESValue& receiver, ESValue arguments[], const size_t& argumentCount, bool isNewExpression);
protected:
    LexicalEnvironment* m_outerEnvironment;
    ESValue m_protoType;

    CodeBlock* m_codeBlock;
    escargot::ESString* m_name;
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
        char16_t ch = m_stringData->charAt(index);
        if (ch < ESCARGOT_ASCII_TABLE_MAX)
            return strings->asciiTable[ch].string();
        else
            return ESString::create(ch);
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

    ESValue get(uint32_t key);
    bool set(uint32_t key, ESValue val);
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

    ESValue get(uint32_t key)
    {
        if ((unsigned)key < arraylength()) {
            unsigned idxPosition = key * elementSize() + byteoffset();
            escargot::ESArrayBufferObject* b = buffer();
            return b->getValueFromBuffer<typename TypeAdaptor::Type>(idxPosition, arraytype());
        }
        return ESValue();
    }
    bool set(uint32_t key, ESValue val)
    {
        if ((unsigned)key >= arraylength())
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
    ESArgumentsObject(FunctionEnvironmentRecordWithArgumentsObject* environment);

public:
    static ESArgumentsObject* create(FunctionEnvironmentRecordWithArgumentsObject* environment)
    {
        return new ESArgumentsObject(environment);
    }

    FunctionEnvironmentRecordWithArgumentsObject* environment() { return m_environment; }

private:
    FunctionEnvironmentRecordWithArgumentsObject* m_environment;
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
