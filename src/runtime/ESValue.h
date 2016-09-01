/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2007, 2008, 2012 Apple Inc. All rights reserved.
 *  Copyright (C) 2016 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

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
#ifdef USE_ES6_FEATURE
// ES6 Typed Array
class ESArrayBufferObject;
class ESArrayBufferView;
template<typename TypeArg>
class ESTypedArrayObject;
class ESTypedArrayObjectWrapper;
class ESDataViewObject;
class ESPromiseObject;
#endif
class ESArgumentsObject;
class ESControlFlowRecord;
class CodeBlock;
class FunctionEnvironmentRecordWithArgumentsObject;
class ESHiddenClassPropertyInfo;
class GlobalObject;

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

template<typename ToType, typename FromType>
inline ToType bitwise_cast(FromType from)
{
    ASSERT(sizeof(FromType) == sizeof(ToType));
    union {
        FromType from;
        ToType to;
    } u;
    u.from = from;
    return u.to;
}

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

    ALWAYS_INLINE operator bool() const;
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
#ifdef USE_ES6_FEATURE
    bool isIterable() const;
#endif

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
    ALWAYS_INLINE ESString* toStringOrEmptyString() const;
    ESString* toStringSlowCase(bool emptyStringOnError) const; // $7.1.12 ToString
    ALWAYS_INLINE ESObject* toObject() const; // $7.1.13 ToObject
    inline ESObject* toObjectSlowPath() const; // $7.1.13 ToObject
    inline ESObject* toTransientObject(GlobalObject* globalObject = nullptr) const; // ES5 $8.7.2 transient object
    inline ESObject* toTransientObjectSlowPath(GlobalObject* globalObject = nullptr) const; // ES5 $8.7.2 transient object
    inline double toLength() const; // $7.1.15 ToLength

    enum { ESInvalidIndexValue = std::numeric_limits<uint32_t>::max() };
    ALWAYS_INLINE uint32_t toIndex() const; // http://www.ecma-international.org/ecma-262/5.1/#sec-15.4

    ALWAYS_INLINE ESObject* asObject() const;
    ALWAYS_INLINE ESFunctionObject* asFunction() const;
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

protected:
    ESValue toPrimitiveSlowCaseHelper(PrimitiveTypeHint preferredType) const;

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
#ifdef USE_ES6_FEATURE
        ESArrayBufferObject = 1 << 12,
        ESArrayBufferView = 1 << 13,
        ESTypedArrayObject = 1 << 14,
        ESDataViewObject = 1 << 15,
        ESPromiseObject = 1 << 16,
#endif
        ESArgumentsObject = 1 << 17,
        ESControlFlowRecord = 1 << 18,
        ESJSONObject = 1 << 19,
        TypeMask = 0x3ffff,
        TotalNumberOfTypes = 19
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
        return reinterpret_cast< ::escargot::ESString *>(this);
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
        return reinterpret_cast< ::escargot::ESRopeString *>(this);
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
        return reinterpret_cast< ::escargot::ESObject *>(this);
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
        return reinterpret_cast< ::escargot::ESFunctionObject *>(this);
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
        return reinterpret_cast< ::escargot::ESArrayObject *>(this);
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
        return reinterpret_cast< ::escargot::ESStringObject *>(this);
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
        return reinterpret_cast< ::escargot::ESNumberObject *>(this);
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
        return reinterpret_cast< ::escargot::ESBooleanObject *>(this);
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
        return reinterpret_cast< ::escargot::ESRegExpObject *>(this);
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
        return reinterpret_cast< ::escargot::ESErrorObject *>(this);
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
        return reinterpret_cast< ::escargot::ESDateObject *>(this);
    }

#ifdef USE_ES6_FEATURE
    ALWAYS_INLINE bool isESArrayBufferObject() const
    {
        return m_type & Type::ESArrayBufferObject;
    }

    ALWAYS_INLINE ::escargot::ESArrayBufferObject* asESArrayBufferObject()
    {
#ifndef NDEBUG
        ASSERT(isESArrayBufferObject());
#endif
        return reinterpret_cast< ::escargot::ESArrayBufferObject *>(this);
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
        return reinterpret_cast< ::escargot::ESArrayBufferView *>(this);
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
        return reinterpret_cast< ::escargot::ESTypedArrayObject<TypeArg> *>(this);
    }

    ALWAYS_INLINE ::escargot::ESTypedArrayObjectWrapper* asESTypedArrayObjectWrapper()
    {
#ifndef NDEBUG
        ASSERT(isESTypedArrayObject());
#endif
        return reinterpret_cast< ::escargot::ESTypedArrayObjectWrapper *>(this);
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
        return reinterpret_cast< ::escargot::ESDataViewObject *>(this);
    }

    ALWAYS_INLINE bool isESPromiseObject() const
    {
        return m_type & Type::ESPromiseObject;
    }

    ALWAYS_INLINE ::escargot::ESPromiseObject* asESPromiseObject()
    {
#ifndef NDEBUG
        ASSERT(isESPromiseObject());
#endif
        return reinterpret_cast< ::escargot::ESPromiseObject *>(this);
    }
#endif

    ALWAYS_INLINE bool isESArgumentsObject() const
    {
        return m_type & Type::ESArgumentsObject;
    }

    ALWAYS_INLINE ::escargot::ESArgumentsObject* asESArgumentsObject()
    {
#ifndef NDEBUG
        ASSERT(isESArgumentsObject());
#endif
        return reinterpret_cast< ::escargot::ESArgumentsObject *>(this);
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
        return reinterpret_cast< ::escargot::ESControlFlowRecord *>(this);
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

inline char32_t readUTF8Sequence(const char*& sequence, bool& valid, int& charlen)
{
    unsigned length;
    const char sch = *sequence;
    valid = true;
    if ((sch & 0x80) == 0)
        length = 1;
    else {
        unsigned char ch2 = static_cast<unsigned char>(*(sequence + 1));
        if ((sch & 0xE0) == 0xC0
            && (ch2 & 0xC0) == 0x80)
            length = 2;
        else {
            unsigned char ch3 = static_cast<unsigned char>(*(sequence + 2));
            if ((sch & 0xF0) == 0xE0
                && (ch2 & 0xC0) == 0x80
                && (ch3 & 0xC0) == 0x80)
                length = 3;
            else {
                unsigned char ch4 = static_cast<unsigned char>(*(sequence + 3));
                if ((sch & 0xF8) == 0xF0
                    && (ch2 & 0xC0) == 0x80
                    && (ch3 & 0xC0) == 0x80
                    && (ch4 & 0xC0) == 0x80)
                    length = 4;
                else {
                    valid = false;
                    (*sequence++);
                    return -1;
                }
            }
        }
    }

    charlen = length;
    char32_t ch = 0;
    switch (length) {
    case 4:
        ch += static_cast<unsigned char>(*sequence++);
        ch <<= 6; // FALLTHROUGH;
    case 3:
        ch += static_cast<unsigned char>(*sequence++);
        ch <<= 6; // FALLTHROUGH;
    case 2:
        ch += static_cast<unsigned char>(*sequence++);
        ch <<= 6; // FALLTHROUGH;
    case 1:
        ch += static_cast<unsigned char>(*sequence++);
    }
    return ch - offsetsFromUTF8[length - 1];
}

inline UTF16String utf8StringToUTF16String(const char* buf, const size_t& len)
{
    UTF16String str;
    const char* source = buf;
    int charlen;
    bool valid;
    while (source < buf + len) {
        char32_t ch = readUTF8Sequence(source, valid, charlen);
        if (!valid) { // Invalid sequence
            str += 0xFFFD;
        } else if (((uint32_t)(ch) <= 0xffff)) { // BMP
            if ((((ch) & 0xfffff800) == 0xd800)) { // SURROGATE
                str += 0xFFFD;
                source -= (charlen - 1);
            } else {
                str += ch; // normal case
            }
        } else if (((uint32_t)((ch) - 0x10000) <= 0xfffff)) { // SUPPLEMENTARY
            str += (char16_t)(((ch) >> 10) + 0xd7c0); // LEAD
            str += (char16_t)(((ch) & 0x3ff) | 0xdc00); // TRAIL
        } else {
            str += 0xFFFD;
            source -= (charlen - 1);
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

struct RegexMatchResult {
    struct RegexMatchResultPiece {
        unsigned m_start, m_end;
    };
    COMPILE_ASSERT((sizeof(RegexMatchResultPiece)) == (sizeof(unsigned) * 2), sizeof_RegexMatchResultPiece_wrong);
    int m_subPatternNum;
    std::vector<std::vector<RegexMatchResultPiece, pointer_free_allocator<RegexMatchResultPiece> >, gc_allocator<std::vector<RegexMatchResultPiece, pointer_free_allocator<RegexMatchResultPiece> >> > m_matchResults;
};

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
    static ESString* create(icu::UnicodeString& src);
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
    icu::UnicodeString toUnicodeString() const;
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

    ESString(const ESString& s) = delete;
    void operator =(const ESString& s) = delete;
    static constexpr size_t maxLength() { return options::MaximumStringLength; }

    ALWAYS_INLINE friend bool operator == (const ESString& a, const char* b);
    ALWAYS_INLINE friend bool operator != (const ESString& a, const char* b);
    ALWAYS_INLINE friend bool operator == (const ESString& a, const ESString& b);
    ALWAYS_INLINE friend bool operator != (const ESString& a, const ESString& b);
    ALWAYS_INLINE friend bool operator < (const ESString& a, const ESString& b);
    ALWAYS_INLINE friend bool operator > (const ESString& a, const ESString& b);
    ALWAYS_INLINE friend bool operator <= (const ESString& a, const ESString& b);
    ALWAYS_INLINE friend bool operator >= (const ESString& a, const ESString& b);

    size_t createRegexMatchResult(escargot::ESRegExpObject* regexp, RegexMatchResult& result);
    escargot::ESArrayObject* createMatchedArray(escargot::ESRegExpObject* regexp, RegexMatchResult& result);

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

    static ESRopeString* createAndConcat(ESString* lstr, ESString* rstr);

    bool hasNonASCIIChild() const
    {
        return m_hasNonASCIIString;
    }

    ESString* string();

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

inline ESString* ESString::create(icu::UnicodeString& src)
{
    UTF16String str;
    size_t srclen = src.length();
    for (size_t i = 0; i < srclen; i++) {
        str.push_back(src.charAt(i));
    }
    return new ESUTF16String(std::move(str));
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

inline icu::UnicodeString ESString::toUnicodeString() const
{
    if (isASCIIString()) {
        return icu::UnicodeString(asASCIIString()->data(), asASCIIString()->length(), US_INV);
    } else {
        return icu::UnicodeString((const UChar*)asUTF16String()->data(), asUTF16String()->length());
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

    ALWAYS_INLINE ESValue value(::escargot::ESObject* obj, ESValue originalObj, ::escargot::ESString* propertyName);
    ALWAYS_INLINE bool setValue(::escargot::ESObject* obj, ESValue originalObj, ::escargot::ESString* propertyName, const ESValue& value);

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

struct ESHiddenClassPropertyInfo {
    ESHiddenClassPropertyInfo(ESString* name, unsigned attributes)
    {
        m_name = name;
        m_attributes = attributes;
    }

    ESHiddenClassPropertyInfo();

    static unsigned buildAttributes(unsigned property, bool writable, bool enumerable, bool configurable);
    static unsigned hiddenClassPopretyInfoVecIndex(bool isData, bool writable, bool enumerable, bool configurable);
    static constexpr unsigned hiddenClassPopretyInfoVecSize() { return 16; }

    ALWAYS_INLINE void setWritable(bool writable);
    ALWAYS_INLINE void setEnumerable(bool enumerable);
    ALWAYS_INLINE void setConfigurable(bool configurable);
    ALWAYS_INLINE void setDeleted(bool deleted);
    ALWAYS_INLINE void setDataProperty(bool data);
    ALWAYS_INLINE void setJSAccessorProperty(bool jsAccessor);
    ALWAYS_INLINE void setNativeAccessorProperty(bool nativeAccessor);
    ALWAYS_INLINE void setName(ESString* name) { m_name = name; }

    ALWAYS_INLINE bool writable() const;
    ALWAYS_INLINE bool enumerable() const;
    ALWAYS_INLINE bool configurable() const;
    ALWAYS_INLINE bool isDeleted() const;
    ALWAYS_INLINE bool isDataProperty() const;
    ALWAYS_INLINE bool isJSAccessorProperty() const;
    ALWAYS_INLINE bool isNativeAccessorProperty() const;
    ALWAYS_INLINE ESString* name() const { return m_name; }
    ALWAYS_INLINE unsigned attributes() const { return m_attributes; }
    ALWAYS_INLINE unsigned property() const;

    static ESHiddenClassPropertyInfo s_dummyPropertyInfo;
private:
    ESString* m_name;
    unsigned m_attributes;
};

typedef std::unordered_map< ::escargot::ESString*, size_t,
    std::hash<ESString*>, std::equal_to<ESString*>,
    gc_allocator<std::pair<const ::escargot::ESString*, size_t> > > ESHiddenClassPropertyIndexHashMapInfoStd;

class ESHiddenClassPropertyIndexHashMapInfo : public ESHiddenClassPropertyIndexHashMapInfoStd, public gc { };

typedef std::vector< ::escargot::ESHiddenClassPropertyInfo, gc_allocator< ::escargot::ESHiddenClassPropertyInfo> > ESHiddenClassPropertyInfoVectorStd;

class ESHiddenClassPropertyInfoVector : public ESHiddenClassPropertyInfoVectorStd {
    public:
#ifdef ENABLE_ESJIT
    static size_t offsetOfData() { return offsetof(ESHiddenClassPropertyInfoVector, _M_impl._M_start); }
#endif
};

typedef std::unordered_map<ESString*, ::escargot::ESHiddenClass **,
std::hash<ESString*>, std::equal_to<ESString*>,
gc_allocator<std::pair<ESString*, ::escargot::ESHiddenClass **> > > ESHiddenClassTransitionDataStd;

typedef std::vector< ::escargot::ESValue, gc_allocator< ::escargot::ESValue> > ESValueVectorStd;

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
                if (*m_propertyInfo[i].name() == *name)
                    return i;
            }
            return SIZE_MAX;
        } else {
            ASSERT(m_propertyIndexHashMapInfo);
            auto iter = m_propertyIndexHashMapInfo->find(const_cast<ESString *>(name));
            if (iter == m_propertyIndexHashMapInfo->end())
                return SIZE_MAX;
            if (m_propertyInfo[iter->second].isDeleted())
                return SIZE_MAX;
            return iter->second;
        }
    }

    void appendHashMapInfo(bool force = false)
    {
        if (m_propertyIndexHashMapInfo) {
            size_t idx = m_propertyInfo.size() - 1;
            m_propertyIndexHashMapInfo->insert(std::make_pair(m_propertyInfo[idx].name(), idx));
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
                if (!m_propertyInfo[i].isDeleted())
                    m_propertyIndexHashMapInfo->insert(std::make_pair(m_propertyInfo[i].name(), i));
            }

            // ASSERT(m_propertyIndexHashMapInfo->size() == m_propertyInfo.size());
        }
    }

    inline ESHiddenClass* defineProperty(ESString* name, unsigned attributes, bool forceNewHiddenClass = false);
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
    void setHasEverSetAsPrototypeObjectHiddenClass() { m_flags.m_hasEverSetAsPrototypeObjectHiddenClass = true; }

    ALWAYS_INLINE ESValue read(ESObject* obj, ESValue originalObject, ESString* propertyName, ESString* name);
    ALWAYS_INLINE ESValue read(ESObject* obj, ESValue originalObject, ESString* propertyName, size_t index);

    ALWAYS_INLINE bool write(ESObject* obj, ESValue originalObject, ESString* propertyName, ESString* name, const ESValue& val);
    ALWAYS_INLINE bool write(ESObject* obj, ESValue originalObject, ESString* propertyName, size_t index, const ESValue& val);

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
        m_flags.m_hasEverSetAsPrototypeObjectHiddenClass = false;
        m_propertyIndexHashMapInfo = NULL;
    }

    ESHiddenClass(const ESHiddenClass& other)
        : m_propertyIndexHashMapInfo(other.m_propertyIndexHashMapInfo)
        , m_propertyInfo(std::move(other.m_propertyInfo))
        , m_transitionData(0)
    {
        ASSERT(other.m_flags.m_isVectorMode == false); // this function is currently used only for vector mode
        m_flags = other.m_flags;
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
        bool m_hasEverSetAsPrototypeObjectHiddenClass:1;
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

enum Attribute {
    None              = 0,
    Writable          = 1 << 1, // property can be only read, not written
    Enumerable        = 1 << 2, // property doesn't appear in (for .. in ..)
    Configurable      = 1 << 3, // property can't be deleted
    Deleted           = 1 << 4, // property is deleted
    Data              = 1 << 5, // property is a data property
    JSAccessor        = 1 << 6, // property is a JS getter/setter property
    NativeAccessor    = 1 << 7, // property is a native getter/setter property
};

class PropertyDescriptor {
public:
    PropertyDescriptor(ESValue value, unsigned attributes)
        : m_value(value)
        , m_getter(ESValue::ESEmptyValue)
        , m_setter(ESValue::ESEmptyValue)
        , m_attributes(attributes)
        , m_seenAttributes(EnumerablePresent | ConfigurablePresent | WritablePresent)
    {
        ASSERT(m_value);
        checkValidity();
    }
    PropertyDescriptor(ESObject* obj);
    bool writable() const;
    bool enumerable() const;
    bool configurable() const;
    bool isDataDescriptor() const;
    bool isGenericDescriptor() const;
    bool isAccessorDescriptor() const;
    unsigned attributes() const { return m_attributes; }
    ESValue value() const { return m_value.isEmpty()? ESValue(ESValue::ESUndefined) : m_value; }
    ESFunctionObject* getterFunction() const;
    ESFunctionObject* setterFunction() const;
    void setWritable(bool);
    void setEnumerable(bool);
    void setConfigurable(bool);
    void setValue(ESValue value) { m_value = value; }
    void setSetter(ESValue);
    void setGetter(ESValue);
    bool hasWritable() const { return m_seenAttributes & WritablePresent; }
    bool hasEnumerable() const { return m_seenAttributes & EnumerablePresent; }
    bool hasConfigurable() const { return m_seenAttributes & ConfigurablePresent; }
    bool hasValue() const { return !m_value.isEmpty();}
    bool hasSetter() const { return !m_setter.isEmpty(); }
    bool hasGetter() const { return !m_getter.isEmpty(); }
    static ESValue fromPropertyDescriptor(ESObject* descSrc, ESString* propertyName, size_t idx);
    static ESValue fromPropertyDescriptorForIndexedProperties(ESObject* obj, uint32_t index);

    static unsigned defaultAttributes;

private:
    void checkValidity() const;

    enum PresentAttribute {
        WritablePresent         = 1 << 1, // property can be only read, not written
        EnumerablePresent       = 1 << 2, // property doesn't appear in (for .. in ..)
        ConfigurablePresent     = 1 << 3, // property can't be deleted
    };
    // May be a getter/setter
    ESValue m_value;
    ESValue m_getter;
    ESValue m_setter;
    unsigned m_attributes;
    unsigned m_seenAttributes;
};

typedef ESValueVector (*PropertyEnumerationCallback)(ESObject* obj);
typedef ESValue (*PropertyReadCallback)(const ESValue& key, ESObject* obj);
typedef bool (*PropertyWriteCallback)(const ESValue& key, const ESValue& value, ESObject* obj);

struct ESObjectRareData : public gc {
    ESObjectRareData()
    {
        m_hasPropertyInterceptor = false;
        m_isProhibitCreateIndexedProperty = false;
        m_propertyEnumerationCallback = nullptr;
        m_propertyReadCallback = nullptr;
        m_propertyWriteCallback = nullptr;
        m_extraPointerData = nullptr;
        m_internalSlot = nullptr;
    }

    bool m_hasPropertyInterceptor;
    bool m_isProhibitCreateIndexedProperty; // only works when m_hasPropertyInterceptor is true
    PropertyEnumerationCallback m_propertyEnumerationCallback;
    PropertyReadCallback m_propertyReadCallback;
    PropertyWriteCallback m_propertyWriteCallback;
    void* m_extraPointerData;
    ESObject* m_internalSlot; // Use this with caution: many internal slots are already implemented implicitly in source code.
};

class ESObject : public ESPointer {
    friend class ESHiddenClass;
    friend class ESFunctionObject;
    friend ALWAYS_INLINE void setObjectPreComputedCaseOperation(ESValue* willBeObject, ::escargot::ESString* keyString, const ESValue& value
        , ESHiddenClassChain* cachedHiddenClassChain, size_t* cachedHiddenClassIndex, ESHiddenClass** hiddenClassWillBe);
    friend class GlobalObject;
protected:
    ESObject(ESPointer::Type type, ESValue __proto__, size_t initialKeyCount = 6);
public:
    static ESObject* create(size_t initialKeyCount = 6);

    inline bool defineDataProperty(InternalAtomicString name,
        bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true, const ESValue& initalValue = ESValue(), bool force = false)
    {
        return defineDataProperty(name.string(), isWritable, isEnumerable, isConfigurable, initalValue, force);
    }
    inline bool defineDataProperty(const ESValue& key,
        bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true, const ESValue& initalValue = ESValue(), bool force = false, bool forceNewHiddenClass = false);
    inline bool defineAccessorProperty(const ESValue& key, ESPropertyAccessorData* data,
        bool isWritable = true, bool isEnumerable = true, bool isConfigurable = true, bool force = false);
    inline bool defineAccessorProperty(const ESValue& key, ESNativeGetter getter, ESNativeSetter setter,
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

    inline bool deletePropertyWithException(const ESValue& key, bool force = false);
    inline bool deleteProperty(const ESValue& key, bool force = false);
    inline bool deletePropertySlowPath(const ESValue& key, bool force = false);
    inline void propertyFlags(const ESValue& key, bool& exists, bool& isDataProperty, bool& isWritable, bool& isEnumerable, bool& isConfigurable);
    inline bool hasProperty(const ESValue& key);
    inline bool hasOwnProperty(const ESValue& key, bool shouldCheckPropertyInterceptor = true);

    bool defineOwnProperty(const ESValue& key, const PropertyDescriptor& desc, bool throwFlag);
    bool defineOwnProperty(const ESValue& key, ESObject* obj, bool throwFlag);

    // $6.1.7.2 Object Internal Methods and Internal Slots
    bool isExtensible()
    {
        return m_flags.m_isExtensible;
    }

    void setExtensible(bool extensible)
    {
        m_flags.m_isExtensible = extensible;
    }

    bool isGlobalObject()
    {
        return m_flags.m_isGlobalObject;
    }

    ESHiddenClass* hiddenClass()
    {
        return m_hiddenClass;
    }

    ESValueVectorStd hiddenClassData()
    {
        return m_hiddenClassData;
    }

    uint32_t extraData()
    {
        return m_flags.m_extraData;
    }

    // NOTE extraData has 24-bit length
    void setExtraData(uint32_t e)
    {
        m_flags.m_extraData = e;
    }

    void* extraPointerData()
    {
        ASSERT(m_objectRareData);
        return m_objectRareData->m_extraPointerData;
    }

    void setExtraPointerData(void* e)
    {
        ensureRareData();
        m_objectRareData->m_extraPointerData = e;
    }

    escargot::ESObject* internalSlot()
    {
        ASSERT(m_objectRareData);
        return m_objectRareData->m_internalSlot;
    }

    void setInternalSlot(escargot::ESObject* object)
    {
        ensureRareData();
        m_objectRareData->m_internalSlot = object;
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
        ASSERT(!m_hiddenClass->propertyInfo(idx).isDataProperty());
        return (ESPropertyAccessorData *)m_hiddenClassData[idx].asESPointer();
    }

    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-get-o-p
    ALWAYS_INLINE ESValue get(escargot::ESValue key, ESValue* receiver = nullptr);
    ALWAYS_INLINE ESValue getOwnProperty(escargot::ESValue key);
    inline ESValue getOwnPropertyFastPath(escargot::ESValue key);
    inline ESValue getOwnPropertySlowPath(escargot::ESValue key);

    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-set-o-p-v-throw
    inline bool set(const escargot::ESValue& key, const ESValue& val, ESValue* receiver = nullptr);
    ALWAYS_INLINE bool set(escargot::ESString* key, const ESValue& val, ESValue* receiver = nullptr)
    {
        return set(ESValue(key), val, receiver);
    }
    ALWAYS_INLINE void set(escargot::ESString* key, const ESValue& val, bool throwException, ESValue* receiver = nullptr)
    {
        set(ESValue(key), val, throwException, receiver);
    }

    ALWAYS_INLINE void set(const ESValue& key, const ESValue& val, bool throwExeption, ESValue* receiver = nullptr);
    NEVER_INLINE bool setSlowPath(const ESValue& key, const ESValue& val, ESValue* receiver = nullptr);

    ALWAYS_INLINE uint32_t length();
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
    inline void relocateIndexesForward(double start, double end, unsigned offset);
    inline void relocateIndexesBackward(double start, double end, unsigned offset);

    void setPropertyInterceptor(PropertyReadCallback read, PropertyWriteCallback write, PropertyEnumerationCallback enumeration, bool prohibitCreateIndexedProperty = false)
    {
        ASSERT(!isESArrayObject());
        ensureRareData();
        forceNonVectorHiddenClass(true);
        m_objectRareData->m_hasPropertyInterceptor = true;
        m_objectRareData->m_propertyEnumerationCallback = enumeration;
        m_objectRareData->m_propertyReadCallback = read;
        m_objectRareData->m_propertyWriteCallback = write;
        m_objectRareData->m_isProhibitCreateIndexedProperty = prohibitCreateIndexedProperty;
    }

    ALWAYS_INLINE bool hasPropertyInterceptor()
    {
        return !isESArrayObject() && m_objectRareData && m_objectRareData->m_hasPropertyInterceptor;
    }

    bool isProhibitCreateIndexedProperty()
    {
        ASSERT(hasPropertyInterceptor());
        return m_objectRareData->m_isProhibitCreateIndexedProperty;
    }

    ESValue readKeyForPropertyInterceptor(const ESValue& key)
    {
        ASSERT(hasPropertyInterceptor());
        return m_objectRareData->m_propertyReadCallback(key, this);
    }

    bool writeKeyForPropertyInterceptor(const ESValue& key, const ESValue& val)
    {
        ASSERT(hasPropertyInterceptor());
        return m_objectRareData->m_propertyWriteCallback(key, val, this);
    }

    bool hasOwnPropertyForPropertyInterceptor(const ESValue& key)
    {
        ASSERT(hasPropertyInterceptor());
        ESValue v = readKeyForPropertyInterceptor(key);
        if (!v.isDeleted())
            return true;
        return false;
    }

#ifdef ENABLE_ESJIT
    static size_t offsetOfHiddenClassData() { return offsetof(ESObject, m_hiddenClassData); }
    static size_t offsetOfHiddenClass() { return offsetof(ESObject, m_hiddenClass); }
    static size_t offsetOf__proto__() { return offsetof(ESObject, m___proto__); }
#endif
protected:

    void setValueAsProtoType(const ESValue& obj);

    void ensureRareData()
    {
        ASSERT(!isESArrayObject());
        if (m_objectRareData == nullptr) {
            m_objectRareData = new ESObjectRareData();
        }
    }

    ESValueVector propertyEnumerationForPropertyInterceptor()
    {
        ASSERT(hasPropertyInterceptor());
        return m_objectRareData->m_propertyEnumerationCallback(this);
    }

    ESHiddenClass* m_hiddenClass;
    ESValueVectorStd m_hiddenClassData;

    ESValue m___proto__;

    union {
        ESObjectRareData* m_objectRareData;

        // used only for ESArrayObject (to make accessing didSomePrototypeObjectDefineIndexedProperty() faster)
        GlobalObject* m_globalObjectForESArrayObject;
    };
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

        // property deletion counter
        uint32_t m_deleteCount: 2;

        // extra data
        uint32_t m_extraData: 24;

        size_t m_margin: 32;
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

        // property deletion counter
        uint32_t m_deleteCount: 2;

        // extra data
        uint32_t m_extraData: 24;

        // size_t m_margin: 0;
    } m_flags;
#endif
};

ALWAYS_INLINE ESObject* ESObjectCreate()
{
    return ESObject::create();
}

class ESBindingSlot {
public:
    ALWAYS_INLINE ESBindingSlot()
        : m_slot(nullptr)
        , m_isGlobalBinding(false)
        , m_isDataBinding(true)
        , m_isBindingMutable(true)
        , m_isBindingConfigurable(false)
        , m_isVirtual(false) { }

    ALWAYS_INLINE ESBindingSlot(ESValue* slot, bool isDataBinding = true, bool isBindingMutable = true, bool isBindingConfigurable = false, bool isGlobalBinding = false, bool isVirtual = false)
        : m_slot(slot)
        , m_isGlobalBinding(isGlobalBinding)
        , m_isDataBinding(isDataBinding)
        , m_isBindingMutable(isBindingMutable)
        , m_isBindingConfigurable(isBindingConfigurable)
        , m_isVirtual(isVirtual)
    {
#ifndef NDEBUG
        m_isInitialized = true;
#endif
    }

    void* operator new(std::size_t) = delete;
    ALWAYS_INLINE ESValue* operator->() const { return m_slot; }
    ALWAYS_INLINE operator bool() const { return m_slot; }
    ALWAYS_INLINE bool operator==(const ESValue* other) const { return m_slot == other; }

    ALWAYS_INLINE ESValue* getSlot()
    {
        ASSERT(m_isDataBinding);
        ASSERT(m_isInitialized);
        return m_slot;
    }

    ALWAYS_INLINE void setSlot(const ESValue& value)
    {
        ASSERT(m_isDataBinding);
        *m_slot = value;
    }

    ALWAYS_INLINE bool isGlobalBinding() { return m_isGlobalBinding; }
    ALWAYS_INLINE bool isDataBinding() { return m_isDataBinding; }
    ALWAYS_INLINE bool isMutable() { return m_isBindingMutable; }
    ALWAYS_INLINE bool isConfigurable() { return m_isBindingConfigurable; }
    ALWAYS_INLINE bool isVirtual() { return m_isVirtual; }

private:
    ESValue* m_slot;
    bool m_isGlobalBinding:1;
    bool m_isDataBinding:1;
    bool m_isBindingMutable:1;
    bool m_isBindingConfigurable:1;
    bool m_isVirtual:1;
#ifndef NDEBUG
    bool m_isInitialized:1;
#endif
};

class ESErrorObject : public ESObject {
public:
    enum Code {
        None,
        ReferenceError,
        TypeError,
        SyntaxError,
        RangeError,
        URIError,
        EvalError
    };

    Code errorCode() const { return m_code; }

    static ESErrorObject* create(escargot::ESString* message = strings->emptyString.string(), Code code = None);
protected:
    Code m_code;

    ESErrorObject(escargot::ESString* message, Code code);
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

typedef int64_t time64_t;
typedef double time64IncludingNaN;
class ESDateObject : public ESObject {
protected:
    ESDateObject(ESPointer::Type type = ESPointer::Type::ESDateObject);
public:
    static ESDateObject* create()
    {
        return new ESDateObject();
    }

    static time64IncludingNaN parseStringToDate(escargot::ESString* istr);

    void setTimeValue();
    void setTimeValue(time64IncludingNaN t);
    void setTimeValue(const ESValue str);
    void setTimeValue(int year, int month, int date, int hour, int minute, int64_t second, int64_t millisecond, bool convertToUTC = true);
    void setTimeValueAsNaN()
    {
        m_hasValidDate = false;
    }

    inline bool isValid()
    {
        return m_hasValidDate;
    }

    time64IncludingNaN timeValueAsDouble()
    {
        if (m_hasValidDate) {
            return (time64IncludingNaN) m_primitiveValue;
        } else {
            return std::numeric_limits<double>::quiet_NaN();
        }
    }

    static time64IncludingNaN applyLocalTimezoneOffset(time64_t t);
    static time64_t ymdhmsToSeconds(int year, int month, int day, int hour, int minute, int64_t second);

    static time64IncludingNaN timeClip(double V)
    {
        if (std::isinf(V) || std::isnan(V)) {
            return nan("0");
        } else if (std::abs(V) >= 8640000000000000.0) {
            return nan("0");
        } else {
            return ESValue(V).toInteger();
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
    int getTimezoneOffset();
    void setTime(time64IncludingNaN t);
    int getUTCDate();
    int getUTCDay();
    int getUTCFullYear();
    int getUTCHours();
    int getUTCMilliseconds();
    int getUTCMinutes();
    int getUTCMonth();
    int getUTCSeconds();

private:

    struct tm m_cachedTM; // it stores time disregarding timezone
    long long m_primitiveValue; // it stores timevalue regarding timezone
    bool m_isCacheDirty;
    bool m_hasValidDate; // function get***() series (in ESValue.cpp) should check if the timevalue is valid with this flag
    int m_timezone;

    void resolveCache();
    static time64IncludingNaN parseStringToDate_1(escargot::ESString* istr, bool& haveTZ, int& offset);
    static time64IncludingNaN parseStringToDate_2(escargot::ESString* istr, bool& haveTZ);

    static constexpr double hoursPerDay = 24.0;
    static constexpr double minutesPerHour = 60.0;
    static constexpr double secondsPerMinute = 60.0;
    static constexpr double secondsPerHour = secondsPerMinute * minutesPerHour;
    static constexpr double msPerSecond = 1000.0;
    static constexpr double msPerMinute = msPerSecond * secondsPerMinute;
    static constexpr double msPerHour = msPerSecond * secondsPerHour;
    static constexpr double msPerDay = msPerHour * hoursPerDay;

    static double day(time64_t t) { return floor(t / msPerDay); }
    static int timeWithinDay(time64_t t) { return t % (int) msPerDay; }
    static int daysInYear(int year);
    static int dayFromYear(int year);
    static time64_t timeFromYear(int year) { return msPerDay * dayFromYear(year); }
    static int yearFromTime(time64_t t);
    static int inLeapYear(time64_t t);
    static int dayFromMonth(int year, int month);
    static int monthFromTime(time64_t t);
    static int dateFromTime(time64_t t);
    static int daysFromTime(time64_t t); // return the number of days after 1970.1.1
    static void computeLocalTime(time64_t, struct tm& tm);
    static time64_t makeDay(int year, int month, int date);
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

class ESArrayObject : public ESObject {
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

    ALWAYS_INLINE void convertToSlowMode()
    {
        // wprintf(L"CONVERT TO SLOW MODE!!!  \n");
        if (!m_flags.m_isFastMode)
            return;
        convertToSlowModeSlowPath();
    }

    NEVER_INLINE void convertToSlowModeSlowPath() {
        forceNonVectorHiddenClass();
        m_flags.m_isFastMode = false;
        uint32_t len = length();
        if (len == 0)
            return;

        ESValue* dataPtr = m_vector.data();
        for (uint32_t i = 0; i < len; i++) {
            if (dataPtr[i] != ESValue(ESValue::ESEmptyValue)) {
                m_hiddenClass = m_hiddenClass->defineProperty(ESValue(i).toString(), Data | PropertyDescriptor::defaultAttributes, false);
                m_hiddenClassData.push_back(dataPtr[i]);
            }
        }
        m_vector.clear();
    }

    void set(const uint32_t& i, const ESValue& val)
    {
        ESObject::set(ESValue(i), val);
    }

    void setLength(unsigned newLength);

    ALWAYS_INLINE bool isFastmode();

    const uint32_t& length()
    {
        return (const uint32_t &)m_length;
    }

    bool defineOwnProperty(const ESValue& key, const PropertyDescriptor& desc, bool throwFlag);
    bool defineOwnProperty(const ESValue& key, ESObject* desc, bool throwFlag);

    static double nextIndexForward(ESObject* obj, const double cur, const double len, const bool skipUndefined);
    static double nextIndexBackward(ESObject* obj, const double cur, const double end, const bool skipUndefined);

    ALWAYS_INLINE GlobalObject* globalObject() { return m_globalObjectForESArrayObject; }
    ALWAYS_INLINE void setGlobalObject(GlobalObject* globalObject) { m_globalObjectForESArrayObject = globalObject; }

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

enum ExecutableType { GlobalCode, FunctionCode, EvalCode };

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

    static ESFunctionObject* createBoundFunction(ESVMInstance* instance, escargot::ESFunctionObject* boundTargetFunction, escargot::ESValue boundThis, const escargot::ESValue* boundArguments = nullptr, size_t boundArgumentsCount = 0);

    void initialize(LexicalEnvironment*, CodeBlock*, escargot::ESString*, unsigned, bool, bool);

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
        return get(strings->name.string()).toString();
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

    // The largest finite floating point number is 1.mantissa * 2^(0x7fe-0x3ff).
    // Since 2^N in binary is a one bit followed by N zero bits. 1 * 2^3ff requires
    // at most 1024 characters to the left of a decimal point, in base 2 (1025 if
    // we include a minus sign). For the fraction, a value with an exponent of 0
    // has up to 52 bits to the right of the decimal point. Each decrement of the
    // exponent down to a minimum of -0x3fe adds an additional digit to the length
    // of the fraction. As such the maximum fraction size is 1075 (1076 including
    // a point). We pick a buffer size such that can simply place the point in the
    // center of the buffer, and are guaranteed to have enough space in each direction
    // fo any number of digits an IEEE number may require to represent.
    typedef char RadixBuffer[2180];
    static char* toStringWithRadix(RadixBuffer& buffer, double number, unsigned radix);

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
public:
    enum Option {
        None = 0,
        Global = 1,
        IgnoreCase = 1 << 1,
        MultiLine = 1 << 2,
        Sticky = 1 << 3,
    };

    struct RegExpCacheKey {
        RegExpCacheKey(const escargot::ESString* body, Option option)
            : m_body(body)
            , m_multiline(option & ESRegExpObject::Option::MultiLine)
            , m_ignoreCase(option & ESRegExpObject::Option::IgnoreCase) { }

        bool operator == (const RegExpCacheKey& otherKey) const
        {
            return (m_body == otherKey.m_body) && (m_multiline == otherKey.m_multiline) && (m_ignoreCase == otherKey.m_ignoreCase);
        }
        const escargot::ESString* m_body;
        const bool m_multiline;
        const bool m_ignoreCase;
    };

    struct RegExpCacheEntry {
        RegExpCacheEntry(const char* yarrError = nullptr, JSC::Yarr::YarrPattern* yarrPattern = nullptr, JSC::Yarr::BytecodePattern* bytecodePattern = nullptr)
            : m_yarrError(yarrError)
            , m_yarrPattern(yarrPattern)
            , m_bytecodePattern(bytecodePattern) { }

        const char* m_yarrError;
        JSC::Yarr::YarrPattern* m_yarrPattern;
        JSC::Yarr::BytecodePattern* m_bytecodePattern;
    };

    static ESRegExpObject* create(const escargot::ESValue patternStr, const escargot::ESValue optionStr);
    static ESRegExpObject* create(escargot::ESString* source, const Option& option)
    {
        return new ESRegExpObject(source, option);
    }

    ALWAYS_INLINE Option option() { return m_option; }
    ALWAYS_INLINE const escargot::ESString* source() { return m_source; }
    ALWAYS_INLINE ESValue lastIndex() { return m_lastIndex; }
    void setSource(escargot::ESString* src);
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

    static RegExpCacheEntry& getCacheEntryAndCompileIfNeeded(escargot::ESString* source, const Option& option);
    static Option parseOption(escargot::ESString* optionString);

    bool match(const escargot::ESString* str, RegexMatchResult& result, bool testOnly = false, size_t startIndex = 0);
    bool matchNonGlobally(const escargot::ESString* str, RegexMatchResult& result, bool testOnly = false, size_t startIndex = 0)
    {
        Option prevOption = option();
        setOption((Option)(prevOption & ~Option::Global));
        bool ret = match(str, result, testOnly, startIndex);
        setOption(prevOption);
        return ret;
    }

    escargot::ESArrayObject* createRegExpMatchedArray(const RegexMatchResult& result, const escargot::ESString* input);
    escargot::ESArrayObject* pushBackToRegExpMatchedArray(escargot::ESArrayObject* array, size_t& index, const size_t limit, const RegexMatchResult& result, const escargot::ESString* str);

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
    const escargot::ESString* m_lastExecutedString;
};

}

namespace std {

template<> struct hash<escargot::ESRegExpObject::RegExpCacheKey> {
    size_t operator()(escargot::ESRegExpObject::RegExpCacheKey const &x) const
    {
        return x.m_body->hashValue();
    }
};

template<> struct equal_to<escargot::ESRegExpObject::RegExpCacheKey> {
    bool operator()(escargot::ESRegExpObject::RegExpCacheKey const &a, escargot::ESRegExpObject::RegExpCacheKey const &b) const
    {
        return a == b;
    }
};

}

namespace escargot {

#ifdef USE_ES6_FEATURE
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

    // buffer is must be allocated by bdwgc
    void attachArrayBuffer(void* buffer, size_t length)
    {
        ASSERT(isDetachedBuffer());
        m_data = buffer;
        m_bytelength = length;
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

    void copyDataFrom(ESArrayBufferObject* other, unsigned start, unsigned length)
    {
        memcpy((int8_t*)other->m_data + start, m_data, length);
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

template<typename Adapter, TypedArrayType type>
struct TypedArrayAdaptor {
    typedef typename Adapter::Type Type;
    static const TypedArrayType typeVal = type;
    static Type toNative(ESValue val)
    {
        if (val.isInt32()) {
            return Adapter::toNativeFromInt32(val.asInt32());
        } else if (val.isDouble()) {
            return Adapter::toNativeFromDouble(val.asDouble());
        }
        return static_cast<Type>(val.toNumber());
    }
};

template<typename TypeArg>
struct IntegralTypedArrayAdapter {
    typedef TypeArg Type;
    static TypeArg toNativeFromInt32(int32_t value)
    {
        return static_cast<TypeArg>(value);
    }

    static TypeArg toNativeFromDouble(double value)
    {
        int32_t result = static_cast<int32_t>(value);
        if (static_cast<double>(result) != value)
            result = ESValue(value).toInt32();
        return static_cast<TypeArg>(result);
    }
};

template<typename TypeArg>
struct FloatTypedArrayAdaptor {
    typedef TypeArg Type;
    static TypeArg toNativeFromInt32(int32_t value)
    {
        return static_cast<TypeArg>(value);
    }

    static TypeArg toNativeFromDouble(double value)
    {
        return value;
    }
};

struct Int8Adaptor: TypedArrayAdaptor<IntegralTypedArrayAdapter<int8_t>, TypedArrayType::Int8Array> {
};
struct Int16Adaptor: TypedArrayAdaptor<IntegralTypedArrayAdapter<int16_t>, TypedArrayType::Int16Array> {
};
struct Int32Adaptor: TypedArrayAdaptor<IntegralTypedArrayAdapter<int32_t>, TypedArrayType::Int32Array> {
};
struct Uint8Adaptor: TypedArrayAdaptor<IntegralTypedArrayAdapter<uint8_t>, TypedArrayType::Uint8Array> {
};
struct Uint16Adaptor: TypedArrayAdaptor<IntegralTypedArrayAdapter<uint16_t>, TypedArrayType::Uint16Array> {
};
struct Uint32Adaptor: TypedArrayAdaptor<IntegralTypedArrayAdapter<uint32_t>, TypedArrayType::Uint32Array> {
};
struct Uint8ClampedAdaptor: TypedArrayAdaptor<IntegralTypedArrayAdapter<uint8_t>, TypedArrayType::Uint8ClampedArray> {
};
struct Float32Adaptor: TypedArrayAdaptor<FloatTypedArrayAdaptor<float>, TypedArrayType::Float32Array> {
};
struct Float64Adaptor: TypedArrayAdaptor<FloatTypedArrayAdaptor<double>, TypedArrayType::Float64Array> {
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
#endif

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

    ESControlFlowRecord(const ControlFlowReason& reason, const ESValue& value, const ESValue& tryDupCount)
        : ESPointer(ESPointer::ESControlFlowRecord)
    {
        m_reason = reason;
        m_value = value;
        m_tryDupCount = tryDupCount;
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

    static ESControlFlowRecord* create(const ControlFlowReason& reason, const ESValue& value, const ESValue& tryDupCount)
    {
        return new ESControlFlowRecord(reason, value, tryDupCount);
    }

    const ControlFlowReason& reason()
    {
        return m_reason;
    }

    const ESValue& value()
    {
        return m_value;
    }

    const ESValue& tryDupCount()
    {
        return m_tryDupCount;
    }

    void setValue(const ESValue& value)
    {
        m_value = value;
    }

    void setTryDupCount(const ESValue& tryDupCount)
    {
        m_tryDupCount = tryDupCount;
    }

protected:
    ControlFlowReason m_reason;
    ESValue m_value;
    ESValue m_tryDupCount;
};

#ifdef USE_ES6_FEATURE

struct PromiseReaction {
public:
    struct Capability {
    public:
        Capability()
            : m_resolveFunction(nullptr)
            , m_rejectFunction(nullptr) { }

        Capability(escargot::ESFunctionObject* resolveFunction, escargot::ESFunctionObject* rejectFunction)
            : m_resolveFunction(resolveFunction)
            , m_rejectFunction(rejectFunction) { }

        Capability(const Capability& other)
            : m_resolveFunction(other.m_resolveFunction)
            , m_rejectFunction(other.m_rejectFunction) { }


        escargot::ESFunctionObject* m_resolveFunction;
        escargot::ESFunctionObject* m_rejectFunction;
    };

    PromiseReaction()
        : m_capability()
        , m_handler(nullptr) { }

    PromiseReaction(escargot::ESFunctionObject* handler, escargot::ESFunctionObject* resolveFunction, escargot::ESFunctionObject* rejectFunction)
        : m_capability(resolveFunction, rejectFunction)
        , m_handler(handler) { }

    PromiseReaction(escargot::ESFunctionObject* handler, const Capability& capability)
        : m_capability(capability)
        , m_handler(handler) { }


    Capability m_capability;
    escargot::ESFunctionObject* m_handler;
};

class ESPromiseObject : public ESObject {
public:
    enum PromiseState {
        Pending,
        FulFilled,
        Rejected
    };

protected:
    ESPromiseObject(escargot::ESFunctionObject* executor)
        : ESObject((Type)(Type::ESObject | Type::ESPromiseObject), ESValue())
        , m_executor(executor)
        , m_state(PromiseState::Pending)
    {
        m_counter = (s_counter++);
    }

    static int s_counter;
    int m_counter;

public:
    static ESPromiseObject* create()
    {
        return new ESPromiseObject(nullptr);
    }

    static ESPromiseObject* create(escargot::ESFunctionObject* executor)
    {
        return new ESPromiseObject(executor);
    }

    void setExecutor(escargot::ESFunctionObject* executor)
    {
        ASSERT(!m_executor);
        m_executor = executor;
    }

    void fulfillPromise(escargot::ESVMInstance* instance, ESValue value);
    void rejectPromise(escargot::ESVMInstance* instance, ESValue reason);

    typedef std::vector<escargot::PromiseReaction, gc_allocator<escargot::PromiseReaction> > Reactions;
    void triggerPromiseReactions(escargot::ESVMInstance* instance, Reactions& reactions);

    void appendReaction(escargot::ESFunctionObject* onFulfilled, escargot::ESFunctionObject* onRejected, escargot::PromiseReaction::Capability capability)
    {
        // ESCARGOT_LOG_INFO("appendReaction into %p\n", this);
        m_fulfillReactions.push_back(PromiseReaction(onFulfilled, capability));
        m_rejectReactions.push_back(PromiseReaction(onRejected, capability));
    }

    void setCapability(escargot::PromiseReaction::Capability capability) { m_capability = capability; }

    void createResolvingFunctions(escargot::ESVMInstance* instance, escargot::ESFunctionObject*& resolveFunction, escargot::ESFunctionObject*& rejectFunction);
    static escargot::ESObject* resolvingFunctionAlreadyResolved(escargot::ESFunctionObject* callee);

    escargot::ESFunctionObject* executor() { return m_executor; }
    PromiseState state() { return m_state; }
    escargot::ESValue promiseResult() { ASSERT(m_state != PromiseState::Pending); return m_promiseResult; }
    escargot::PromiseReaction::Capability capability() { return m_capability; }

protected:
    escargot::ESFunctionObject* m_executor;
    PromiseState m_state;
    escargot::ESValue m_promiseResult;
    Reactions m_fulfillReactions;
    Reactions m_rejectReactions;
    escargot::PromiseReaction::Capability m_capability;
};
#endif

}
#include "vm/ESVMInstance.h"
#include "ESValueInlines.h"

#endif
