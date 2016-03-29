#ifndef ESValueInlines_h
#define ESValueInlines_h

namespace escargot {


// The fast double-to-(unsigned-)int conversion routine does not guarantee
// rounding towards zero.
// The result is unspecified if x is infinite or NaN, or if the rounded
// integer value is outside the range of type int.
ALWAYS_INLINE int FastD2I(double x)
{
    return static_cast<int32_t>(x);
}


ALWAYS_INLINE double FastI2D(int x)
{
    // There is no rounding involved in converting an integer to a
    // double, so this code should compile to a few instructions without
    // any FPU pipeline stalls.
    return static_cast<double>(x);
}

// ==============================================================================
// ===common architecture========================================================
// ==============================================================================

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

inline ESValue::ESValue(double d)
{
    const int32_t asInt32 = static_cast<int32_t>(d);
    if (asInt32 != d || (!asInt32 && std::signbit(d))) { // true for -0.0
        *this = ESValue(EncodeAsDouble, d);
        return;
    }
    *this = ESValue(static_cast<int32_t>(d));
}

inline ESValue::ESValue(char i)
{
    *this = ESValue(static_cast<int32_t>(i));
}

inline ESValue::ESValue(unsigned char i)
{
    *this = ESValue(static_cast<int32_t>(i));
}

inline ESValue::ESValue(short i)
{
    *this = ESValue(static_cast<int32_t>(i));
}

inline ESValue::ESValue(unsigned short i)
{
    *this = ESValue(static_cast<int32_t>(i));
}

inline ESValue::ESValue(unsigned i)
{
    if (static_cast<int32_t>(i) < 0) {
        *this = ESValue(EncodeAsDouble, static_cast<double>(i));
        return;
    }
    *this = ESValue(static_cast<int32_t>(i));
}

inline ESValue::ESValue(long i)
{
    if (static_cast<int32_t>(i) != i) {
        *this = ESValue(EncodeAsDouble, static_cast<double>(i));
        return;
    }
    *this = ESValue(static_cast<int32_t>(i));
}

inline ESValue::ESValue(unsigned long i)
{
    if (static_cast<uint32_t>(i) != i) {
        *this = ESValue(EncodeAsDouble, static_cast<double>(i));
        return;
    }
    *this = ESValue(static_cast<uint32_t>(i));
}

inline ESValue::ESValue(long long i)
{
    if (static_cast<int32_t>(i) != i) {
        *this = ESValue(EncodeAsDouble, static_cast<double>(i));
        return;
    }
    *this = ESValue(static_cast<int32_t>(i));
}

inline ESValue::ESValue(unsigned long long i)
{
    if (static_cast<uint32_t>(i) != i) {
        *this = ESValue(EncodeAsDouble, static_cast<double>(i));
        return;
    }
    *this = ESValue(static_cast<uint32_t>(i));
}

inline bool ESValue::isUInt32() const
{
    return isInt32() && asInt32() >= 0;
}

inline uint32_t ESValue::asUInt32() const
{
    ASSERT(isUInt32());
    return asInt32();
}

ALWAYS_INLINE double ESValue::asNumber() const
{
    ASSERT(isNumber());
    return isInt32() ? asInt32() : asDouble();
}

inline uint64_t ESValue::asRawData() const
{
    return u.asInt64;
}

ALWAYS_INLINE ESString* ESValue::toString() const
{
    if (isESString()) {
        return asESString();
    } else {
        return toStringSlowCase();
    }
}

inline ESObject* ESValue::toObject() const
{
    ESObject* object;
    if (LIKELY(isESPointer() && asESPointer()->isESObject())) {
        return asESPointer()->asESObject();
    } else if (isNumber()) {
        object = ESNumberObject::create(toNumber());
    } else if (isBoolean()) {
        object = ESBooleanObject::create(toBoolean());
    } else if (isESString()) {
        object = ESStringObject::create(asESPointer()->asESString());
    } else if (isNull()) {
        ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("cannot convert null into object"))));
    } else if (isUndefined()) {
        ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("cannot convert undefined into object"))));
    } else {
        RELEASE_ASSERT_NOT_REACHED();
    }
    return object;
}

ALWAYS_INLINE ESValue ESValue::toPrimitive(PrimitiveTypeHint preferredType) const
{
    if (UNLIKELY(!isPrimitive())) {
        return toPrimitiveSlowCase(preferredType);
    } else {
        return *this;
    }
}

inline double ESValue::toNumber() const
{
    // http://www.ecma-international.org/ecma-262/6.0/#sec-tonumber
#ifdef ESCARGOT_64
    auto n = u.asInt64 & TagTypeNumber;
    if (LIKELY(n)) {
        if (n == TagTypeNumber) {
            return FastI2D(asInt32());
        } else {
            return asDouble();
        }
    }
#else
    if (LIKELY(isInt32()))
        return FastI2D(asInt32());
    else if (isDouble())
        return asDouble();
#endif
    else if (isUndefined())
        return std::numeric_limits<double>::quiet_NaN();
    else if (isNull())
        return 0;
    else if (isBoolean())
        return asBoolean() ?  1 : 0;
    else {
        return toNumberSlowCase();
    }
}

inline double ESValue::toNumberSlowCase() const
{
    ASSERT(isESPointer());
    ESPointer* o = asESPointer();
    if (o->isESString() || o->isESStringObject()) {
        double val;
        ESString* data;
        if (LIKELY(o->isESString())) {
            data = o->asESString();
        } else {
            data = o->asESStringObject()->stringData();
        }

        // A StringNumericLiteral that is empty or contains only white space is converted to +0.
        if (data->length() == 0)
            return 0;

        char* end;
        char* buf = (char*)alloca(data->length() + 1);
        const size_t len = data->length();
        for (unsigned i = 0; i < len ; i ++) {
            char16_t c = data->charAt(i);
            if (c >= 128)
                c = 0;
            buf[i] = c;
        }
        buf[len] = 0;
        if (len >= 3 && strncmp("-0x", buf, 3) == 0) { // hex number with Unary Minus is invalid in JavaScript while it is valid in C
            val = std::numeric_limits<double>::quiet_NaN();
            return val;
        }
        val = strtod(buf, &end);
        if (end != buf + len) {
            auto isSpace = [] (char16_t c) -> bool {
                switch (c) {
                case 0x0009: case 0x000A: case 0x000B: case 0x000C:
                case 0x000D: case 0x0020: case 0x00A0: case 0x1680:
                case 0x180E: case 0x2000: case 0x2001: case 0x2002:
                case 0x2003: case 0x2004: case 0x2005: case 0x2006:
                case 0x2007: case 0x2008: case 0x2009: case 0x200A:
                case 0x2028: case 0x2029: case 0x202F: case 0x205F:
                case 0x3000: case 0xFEFF:
                    return true;
                default:
                    return false;
                }
            };

            unsigned ptr = 0;
            enum State { Initial, ReadingNumber, DoneReadingNumber, Invalid };
            State state = State::Initial;
            for (unsigned i = 0; i < len; i ++) {
                switch (state) {
                case Initial:
                    if (isSpace(data->charAt(i)))
                        break;
                    else
                        state = State::ReadingNumber;
                case ReadingNumber:
                    if (isSpace(data->charAt(i)))
                        state = State::DoneReadingNumber;
                    else {
                        char16_t c = data->charAt(i);
                        if (c >= 128)
                            c = 0;
                        buf[ptr++] = c;
                    }
                    break;
                case DoneReadingNumber:
                    if (!isSpace(data->charAt(i)))
                        state = State::Invalid;
                    break;
                case Invalid:
                    break;
                }
            }
            if (state != State::Invalid) {
                buf[ptr] = 0;
                val = strtod(buf, &end);
                if (end != buf + ptr)
                    val = std::numeric_limits<double>::quiet_NaN();
            } else
                val = std::numeric_limits<double>::quiet_NaN();
        }
        return val;
    } else {
        return toPrimitive().toNumber();
    }
}

ALWAYS_INLINE bool ESValue::toBoolean() const
{
#ifdef ESCARGOT_32
    if (LIKELY(isBoolean()))
        return payload();
#else
    if (*this == ESValue(true))
        return true;

    if (*this == ESValue(false))
        return false;
#endif

    if (isInt32())
        return asInt32();

    if (isDouble()) {
        double d = asDouble();
        if (isnan(d))
            return false;
        if (d == 0.0)
            return false;
        return true;
    }

    if (isUndefinedOrNull())
        return false;

    ASSERT(isESPointer());
    if (asESPointer()->isESString())
        return asESString()->length();
    return true;
}

// http://www.ecma-international.org/ecma-262/5.1/#sec-9.4
inline double ESValue::toInteger() const
{
    if (isInt32())
        return asInt32();
    double d = toNumber();
    if (isnan(d))
        return 0;
    if (d == 0 || d == std::numeric_limits<double>::infinity() || d == -std::numeric_limits<double>::infinity())
        return d;
    return (d < 0 ? -1 : 1) * std::floor(std::abs(d));
}

// http://www.ecma-international.org/ecma-262/5.1/#sec-9.5
ALWAYS_INLINE int32_t ESValue::toInt32() const
{
    // consume fast case
    if (LIKELY(isInt32()))
        return asInt32();

    return toInt32SlowCase();
}

// http://www.ecma-international.org/ecma-262/5.1/#sec-9.5
inline int32_t ESValue::toInt32SlowCase() const
{
    double num = toNumber();
    int64_t bits = bitwise_cast<int64_t>(num);
    int32_t exp = (static_cast<int32_t>(bits >> 52) & 0x7ff) - 0x3ff;

    // If exponent < 0 there will be no bits to the left of the decimal point
    // after rounding; if the exponent is > 83 then no bits of precision can be
    // left in the low 32-bit range of the result (IEEE-754 doubles have 52 bits
    // of fractional precision).
    // Note this case handles 0, -0, and all infinte, NaN, & denormal value.
    if (exp < 0 || exp > 83)
        return 0;

    // Select the appropriate 32-bits from the floating point mantissa. If the
    // exponent is 52 then the bits we need to select are already aligned to the
    // lowest bits of the 64-bit integer representation of tghe number, no need
    // to shift. If the exponent is greater than 52 we need to shift the value
    // left by (exp - 52), if the value is less than 52 we need to shift right
    // accordingly.
    int32_t result = (exp > 52)
        ? static_cast<int32_t>(bits << (exp - 52))
        : static_cast<int32_t>(bits >> (52 - exp));

    // IEEE-754 double precision values are stored omitting an implicit 1 before
    // the decimal point; we need to reinsert this now. We may also the shifted
    // invalid bits into the result that are not a part of the mantissa (the sign
    // and exponent bits from the floatingpoint representation); mask these out.
    if (exp < 32) {
        int32_t missingOne = 1 << exp;
        result &= missingOne - 1;
        result += missingOne;
    }

    // If the input value was negative (we could test either 'number' or 'bits',
    // but testing 'bits' is likely faster) invert the result appropriately.
    return bits < 0 ? -result : result;
}

ALWAYS_INLINE uint32_t ESValue::toUint32() const
{
    return toInt32();
}

inline bool ESValue::isObject() const
{
    return isESPointer() && asESPointer()->isESObject();
}

inline double ESValue::toLength() const
{
    double len = toInteger();
    if (len <= 0.0) {
        return 0.0;
    }
    if (len > 0 && std::isinf(len)) {
        return std::pow(2, 32) - 1;
    }
    return std::min(len, std::pow(2, 32) - 1);
}

ALWAYS_INLINE bool ESValue::isPrimitive() const
{
    // return isUndefined() || isNull() || isNumber() || isESString() || isBoolean();
    return !isESPointer() || asESPointer()->isESString();
}

ALWAYS_INLINE uint32_t ESValue::toIndex() const
{
    int32_t i;
    if (LIKELY(isInt32()) && LIKELY((i = asInt32()) >= 0)) {
        return i;
    } else {
        ESString* key = toString();
        return key->tryToUseAsIndex();
    }
}

#ifdef ENABLE_ESJIT
inline ESValueInDouble ESValue::toRawDouble(ESValue value)
{
    return bitwise_cast<ESValueInDouble>(value.u.asInt64);
}

ALWAYS_INLINE ESValue ESValue::fromRawDouble(ESValueInDouble value)
{
    ESValue val;
    val.u.asInt64 = bitwise_cast<uint64_t>(value);
    return val;
}
#endif

// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-abstract-equality-comparison
ALWAYS_INLINE bool ESValue::abstractEqualsTo(const ESValue& val)
{
    if (isInt32() && val.isInt32()) {
#ifdef ESCARGOT_64
        if (u.asInt64 == val.u.asInt64)
#else
        if (u.asBits.payload == val.u.asBits.payload)
#endif
            return true;
        return false;
    } else {
        return abstractEqualsToSlowCase(val);
    }
}

inline bool ESValue::equalsTo(const ESValue& val)
{
    if (isUndefined())
        return val.isUndefined();

    if (isNull())
        return val.isNull();

    if (isBoolean())
        return val.isBoolean() && asBoolean() == val.asBoolean();

    if (isNumber()) {
        if (!val.isNumber())
            return false;
        double a = asNumber();
        double b = val.asNumber();
        if (std::isnan(a) || std::isnan(b))
            return false;
        // we can pass [If x is +0 and y is −0, return true. If x is −0 and y is +0, return true.]
        // because
        // double a = -0.0;
        // double b = 0.0;
        // a == b; is true
        return a == b;
    }

    if (isESPointer()) {
        ESPointer* o = asESPointer();
        if (!val.isESPointer())
            return false;
        ESPointer* o2 = val.asESPointer();
        if (o->isESString()) {
            if (!o2->isESString())
                return false;
            return *o->asESString() == *o2->asESString();
        }
        return o == o2;
    }
    return false;
}

inline bool ESValue::equalsToByTheSameValueAlgorithm(const ESValue& val)
{
    if (isUndefined())
        return val.isUndefined();

    if (isNull())
        return val.isNull();

    if (isBoolean())
        return val.isBoolean() && asBoolean() == val.asBoolean();

    if (isNumber()) {
        if (!val.isNumber())
            return false;
        double a = asNumber();
        double b = val.asNumber();
        if (std::isnan(a) || std::isnan(b))
            return false;
        // we can pass [If x is +0 and y is −0, return true. If x is −0 and y is +0, return true.]
        // because
        // double a = -0.0;
        // double b = 0.0;
        // a == b; is true
        return a == b && std::signbit(a) == std::signbit(b);
    }

    if (isESPointer()) {
        ESPointer* o = asESPointer();
        if (!val.isESPointer())
            return false;
        ESPointer* o2 = val.asESPointer();
        if (o->isESString()) {
            if (!o2->isESString())
                return false;
            return *o->asESString() == *o2->asESString();
        }
        if (o == o2)
            return o == o2;
    }
    return false;
}

// ==============================================================================
// ===32-bit architecture========================================================
// ==============================================================================

#ifdef ESCARGOT_32

ALWAYS_INLINE ESValue::ESValue(ESForceUninitializedTag)
{

}

inline ESValue::ESValue()
{
    u.asBits.tag = UndefinedTag;
    u.asBits.payload = 0;
}

inline ESValue::ESValue(ESNullTag)
{
    u.asBits.tag = NullTag;
    u.asBits.payload = 0;
}

inline ESValue::ESValue(ESUndefinedTag)
{
    u.asBits.tag = UndefinedTag;
    u.asBits.payload = 0;
}

inline ESValue::ESValue(ESEmptyValueTag)
{
    u.asBits.tag = EmptyValueTag;
    u.asBits.payload = 0;
}

inline ESValue::ESValue(ESDeletedValueTag)
{
    u.asBits.tag = DeletedValueTag;
    u.asBits.payload = 0;
}

inline ESValue::ESValue(ESTrueTag)
{
    u.asBits.tag = BooleanTag;
    u.asBits.payload = 1;
}

inline ESValue::ESValue(ESFalseTag)
{
    u.asBits.tag = BooleanTag;
    u.asBits.payload = 0;
}

inline ESValue::ESValue(bool b)
{
    u.asBits.tag = BooleanTag;
    u.asBits.payload = b;
}

inline ESValue::ESValue(ESPointer* ptr)
{
    if (LIKELY(ptr != NULL)) {
        u.asBits.tag = PointerTag;
    } else {
        u.asBits.tag = EmptyValueTag;
    }
    u.asBits.payload = reinterpret_cast<int32_t>(ptr);
}

inline ESValue::ESValue(const ESPointer* ptr)
{
    if (LIKELY(ptr != NULL)) {
        u.asBits.tag = PointerTag;
    } else {
        u.asBits.tag = EmptyValueTag;
    }
    u.asBits.payload = reinterpret_cast<int32_t>(const_cast<ESPointer*>(ptr));
}

ALWAYS_INLINE ESValue::ESValue(EncodeAsDoubleTag, double d)
{
    u.asDouble = d;
}

inline ESValue::ESValue(int i)
{
    u.asBits.tag = Int32Tag;
    u.asBits.payload = i;
}

inline ESValue::operator bool() const
{
    return u.asInt64;
}

inline bool ESValue::operator==(const ESValue& other) const
{
    return u.asInt64 == other.u.asInt64;
}

inline bool ESValue::operator!=(const ESValue& other) const
{
    return u.asInt64 != other.u.asInt64;
}

inline uint32_t ESValue::tag() const
{
    return u.asBits.tag;
}

inline int32_t ESValue::payload() const
{
    return u.asBits.payload;
}

inline bool ESValue::isInt32() const
{
    return tag() == Int32Tag;
}

inline bool ESValue::isDouble() const
{
    return tag() < LowestTag;
}

inline int32_t ESValue::asInt32() const
{
    ASSERT(isInt32());
    return u.asBits.payload;
}

inline bool ESValue::asBoolean() const
{
    ASSERT(isBoolean());
    return u.asBits.payload;
}

inline double ESValue::asDouble() const
{
    ASSERT(isDouble());
    return u.asDouble;
}

inline bool ESValue::isEmpty() const
{
    return tag() == EmptyValueTag;
}

inline bool ESValue::isDeleted() const
{
    return tag() == DeletedValueTag;
}

inline bool ESValue::isNumber() const
{
    return isInt32() || isDouble();
}

inline bool ESValue::isESPointer() const
{
    return tag() == PointerTag;
}

ALWAYS_INLINE bool ESValue::isUndefined() const
{
    return tag() == UndefinedTag;
}

ALWAYS_INLINE bool ESValue::isNull() const
{
    return tag() == NullTag;
}

ALWAYS_INLINE bool ESValue::isBoolean() const
{
    return tag() == BooleanTag;
}

ALWAYS_INLINE ESPointer* ESValue::asESPointer() const
{
    ASSERT(isESPointer());
    return reinterpret_cast<ESPointer*>(u.asBits.payload);
}

inline bool ESValue::isESString() const
{
    return isESPointer() && asESPointer()->isESString();
}

inline ESString* ESValue::asESString() const
{
    ASSERT(isESString());
    return asESPointer()->asESString();
}

#else

// ==============================================================================
// ===64-bit architecture========================================================
// ==============================================================================


ALWAYS_INLINE ESValue::ESValue()
{
    u.asInt64 = ValueUndefined;
}

ALWAYS_INLINE ESValue::ESValue(ESForceUninitializedTag)
{
}

ALWAYS_INLINE ESValue::ESValue(ESNullTag)
{
    u.asInt64 = ValueNull;
}

ALWAYS_INLINE ESValue::ESValue(ESUndefinedTag)
{
    u.asInt64 = ValueUndefined;
}

ALWAYS_INLINE ESValue::ESValue(ESEmptyValueTag)
{
    u.asInt64 = ValueEmpty;
}

ALWAYS_INLINE ESValue::ESValue(ESDeletedValueTag)
{
    u.asInt64 = ValueDeleted;
}

ALWAYS_INLINE ESValue::ESValue(ESTrueTag)
{
    u.asInt64 = ValueTrue;
}

ALWAYS_INLINE ESValue::ESValue(ESFalseTag)
{
    u.asInt64 = ValueFalse;
}

ALWAYS_INLINE ESValue::ESValue(bool b)
{
    u.asInt64 = (TagBitTypeOther | TagBitBool | b);
}

ALWAYS_INLINE ESValue::ESValue(ESPointer* ptr)
{
    u.ptr = ptr;
}

ALWAYS_INLINE ESValue::ESValue(const ESPointer* ptr)
{
    u.ptr = const_cast<ESPointer*>(ptr);
}

ALWAYS_INLINE int64_t reinterpretDoubleToInt64(double value)
{
    return bitwise_cast<int64_t>(value);
}
ALWAYS_INLINE double reinterpretInt64ToDouble(int64_t value)
{
    return bitwise_cast<double>(value);
}

ALWAYS_INLINE ESValue::ESValue(EncodeAsDoubleTag, double d)
{
    u.asInt64 = reinterpretDoubleToInt64(d) + DoubleEncodeOffset;
}

ALWAYS_INLINE ESValue::ESValue(int i)
{
    u.asInt64 = TagTypeNumber | static_cast<uint32_t>(i);
}

ALWAYS_INLINE ESValue::operator bool() const
{
    ASSERT(u.asInt64 != ValueDeleted);
    return u.asInt64 != ValueEmpty;
}

ALWAYS_INLINE bool ESValue::operator==(const ESValue& other) const
{
    return u.asInt64 == other.u.asInt64;
}

ALWAYS_INLINE bool ESValue::operator!=(const ESValue& other) const
{
    return u.asInt64 != other.u.asInt64;
}

inline bool ESValue::isInt32() const
{
#ifdef ESCARGOT_LITTLE_ENDIAN
    ASSERT(sizeof(short) == 2);
    unsigned short* firstByte = (unsigned short *)&u.asInt64;
    return firstByte[3] == 0xffff;
#else
    return (u.asInt64 & TagTypeNumber) == TagTypeNumber;
#endif
}

inline bool ESValue::isDouble() const
{
    return isNumber() && !isInt32();
}

inline int32_t ESValue::asInt32() const
{
    ASSERT(isInt32());
    return static_cast<int32_t>(u.asInt64);
}

inline bool ESValue::asBoolean() const
{
    ASSERT(isBoolean());
    return u.asInt64 == ValueTrue;
}

inline double ESValue::asDouble() const
{
    ASSERT(isDouble());
    return reinterpretInt64ToDouble(u.asInt64 - DoubleEncodeOffset);
}

ALWAYS_INLINE bool ESValue::isEmpty() const
{
    return u.asInt64 == ValueEmpty;
}

inline bool ESValue::isDeleted() const
{
    return u.asInt64 == ValueDeleted;
}

inline bool ESValue::isNumber() const
{
#ifdef ESCARGOT_LITTLE_ENDIAN
    ASSERT(sizeof(short) == 2);
    unsigned short* firstByte = (unsigned short *)&u.asInt64;
    return firstByte[3];
#else
    return u.asInt64 & TagTypeNumber;
#endif
}

inline bool ESValue::isESString() const
{
    return isESPointer() && asESPointer()->isESString();
}

inline ESString* ESValue::asESString() const
{
    ASSERT(isESString());
    return asESPointer()->asESString();
}

ALWAYS_INLINE bool ESValue::isESPointer() const
{
    return !(u.asInt64 & TagMask);
}

ALWAYS_INLINE bool ESValue::isUndefined() const
{
    return u.asInt64 == ValueUndefined;
}

ALWAYS_INLINE bool ESValue::isNull() const
{
    return u.asInt64 == ValueNull;
}

ALWAYS_INLINE bool ESValue::isBoolean() const
{
    return u.asInt64 == ValueTrue || u.asInt64 == ValueFalse;
}

ALWAYS_INLINE ESPointer* ESValue::asESPointer() const
{
    ASSERT(isESPointer());
    return u.ptr;
}


#endif
inline ESString* ESString::create(double number)
{
#ifdef ENABLE_DTOACACHE
    auto cache = ESVMInstance::currentInstance()->dtoaCache();
    auto iter = cache->find(number);
    if (iter != cache->end()) {
        return iter->second;
    }
#endif
    ESString* str = ESString::create(dtoa(number));
#ifdef ENABLE_DTOACACHE
    cache->insert(std::make_pair(number, str));
#endif
    return str;
}

inline ESString* ESString::create(const char* str)
{
    unsigned l = strlen(str);
    if (l == 1) {
        return strings->asciiTable[(size_t)str[0]].string();
    }
    return new ESASCIIString(std::move(ASCIIString(str)));
}


inline ESString* ESString::createAtomicString(const char* str)
{
    InternalAtomicString as(str);
    return as.string();
}


ALWAYS_INLINE ESValue ESPropertyAccessorData::value(::escargot::ESObject* obj, ESValue originalObj, ESString* propertyName)
{
    if (m_nativeGetter) {
        ASSERT(!m_jsGetter);
        ASSERT(originalObj.isObject());
        return m_nativeGetter(obj, originalObj.asESPointer()->asESObject(), propertyName);
    }
    if (m_jsGetter) {
        ASSERT(!m_nativeGetter);
        return ESFunctionObject::call(ESVMInstance::currentInstance(), m_jsGetter, originalObj, NULL, 0, false);
    }
    return ESValue();
}

ALWAYS_INLINE bool ESPropertyAccessorData::setValue(::escargot::ESObject* obj, ESValue originalObj, ESString* propertyName, const ESValue& value)
{
    if (m_nativeSetter) {
        ASSERT(!m_jsSetter);
        ASSERT(originalObj.isObject());
        m_nativeSetter(obj, originalObj.asESPointer()->asESObject(), propertyName, value);
        return true;
    }
    if (m_jsSetter) {
        ASSERT(!m_nativeSetter);
        ESValue arg[] = {value};
        ESFunctionObject::call(ESVMInstance::currentInstance(), m_jsSetter,
            originalObj , arg, 1, false);
        return true;
    }
    return false;
}

inline ESHiddenClass* ESHiddenClass::removeProperty(size_t idx)
{
    ASSERT(idx != SIZE_MAX);
    // ASSERT(m_propertyInfo[idx].m_flags.m_isConfigurable);

    if (m_flags.m_isVectorMode) {
        ESHiddenClass* ret;
        ASSERT(*m_propertyInfo[0].m_name == *strings->__proto__.string());
        unsigned i;
        if (idx == 0) {
            // Deleting __proto__
            ret = new ESHiddenClass;
            i = 0;
        } else {
            // Deleting ordinary property
            ret = ESVMInstance::currentInstance()->initialHiddenClassForObject();
            i = 1;
        }
        for (; i < m_propertyInfo.size(); i ++) {
            if (idx != i) {
                ret = ret->defineProperty(m_propertyInfo[i].m_name, m_propertyInfo[i].m_flags.m_isDataProperty
                    , m_propertyInfo[i].m_flags.m_isWritable, m_propertyInfo[i].m_flags.m_isEnumerable, m_propertyInfo[i].m_flags.m_isConfigurable);
            }
        }
        return ret;
    } else {
        if (!m_flags.m_forceNonVectorMode && m_propertyInfo.size() < ESHiddenClassVectorModeSizeLimit / 2) {
            // convert into vector mode
            ESHiddenClass* ret = ESVMInstance::currentInstance()->initialHiddenClassForObject();
            for (unsigned i = 1; i < m_propertyInfo.size(); i ++) {
                if (idx != i) {
                    ret = ret->defineProperty(m_propertyInfo[i].m_name, m_propertyInfo[i].m_flags.m_isDataProperty
                        , m_propertyInfo[i].m_flags.m_isWritable, m_propertyInfo[i].m_flags.m_isEnumerable, m_propertyInfo[i].m_flags.m_isConfigurable);
                }
            }
            return ret;
        }
        ESHiddenClass* cls = new ESHiddenClass;
        cls->m_flags.m_forceNonVectorMode = m_flags.m_forceNonVectorMode;
        cls->m_flags.m_isVectorMode = false;
        for (unsigned i = 0; i < m_propertyInfo.size(); i ++) {
            if (i == idx)
                continue;
            cls->m_propertyInfo.push_back(m_propertyInfo[i]);
            cls->m_flags.m_hasReadOnlyProperty |= (!m_propertyInfo[i].m_flags.m_isWritable);
            cls->m_flags.m_hasIndexedProperty |= m_propertyInfo[i].m_name->hasOnlyDigit();
            cls->m_flags.m_hasIndexedReadOnlyProperty |= (!m_propertyInfo[i].m_flags.m_isWritable && m_propertyInfo[i].m_name->hasOnlyDigit());
        }

        cls->fillHashMapInfo();
        // TODO
        // delete this;
        return cls;
    }
}

// IS FUNCTION IS FOR GLOBAL OBJECT
inline ESHiddenClass* ESHiddenClass::removePropertyWithoutIndexChange(size_t idx)
{
    ASSERT(idx != SIZE_MAX);
    // can not delete __proto__
    ASSERT(idx != 0);
    ASSERT(m_propertyInfo[idx].m_flags.m_isConfigurable);
    ASSERT(m_flags.m_forceNonVectorMode);
    ASSERT(!m_flags.m_isVectorMode);

    ESHiddenClass* cls = new ESHiddenClass;
    cls->m_flags.m_forceNonVectorMode = true;
    cls->m_flags.m_isVectorMode = false;
    for (unsigned i = 0; i < m_propertyInfo.size(); i ++) {
        if (i == idx) {
            cls->m_propertyInfo.push_back(ESHiddenClassPropertyInfo());
            cls->m_propertyInfo.back().m_name = m_propertyInfo[i].m_name;
        } else {
            cls->m_propertyInfo.push_back(m_propertyInfo[i]);
            if (m_propertyInfo[i].m_flags.m_isDeletedValue)
                continue;
            cls->m_flags.m_hasReadOnlyProperty |= (!m_propertyInfo[i].m_flags.m_isWritable);
            cls->m_flags.m_hasIndexedProperty |= m_propertyInfo[i].m_name->hasOnlyDigit();
            cls->m_flags.m_hasIndexedReadOnlyProperty |= (!m_propertyInfo[i].m_flags.m_isWritable && m_propertyInfo[i].m_name->hasOnlyDigit());
        }
    }

    cls->fillHashMapInfo(true);

    // TODO
    // delete this;
    return cls;
}

inline ESHiddenClass* ESHiddenClass::morphToNonVectorMode()
{
    ESHiddenClass* cls = new ESHiddenClass;
    cls->m_flags.m_isVectorMode = false;
    cls->m_propertyInfo.assign(m_propertyInfo.begin(), m_propertyInfo.end());

    cls->m_flags.m_hasReadOnlyProperty = m_flags.m_hasReadOnlyProperty;
    cls->m_flags.m_hasIndexedProperty = m_flags.m_hasIndexedProperty;
    cls->m_flags.m_hasIndexedReadOnlyProperty = m_flags.m_hasIndexedReadOnlyProperty;

    cls->fillHashMapInfo();

    return cls;
}

inline ESHiddenClass* ESHiddenClass::forceNonVectorMode()
{
    ESHiddenClass* cls = morphToNonVectorMode();
    cls->m_flags.m_forceNonVectorMode = true;
    return cls;
}

inline ESHiddenClass* ESHiddenClass::defineProperty(ESString* name, bool isData, bool isWritable, bool isEnumerable, bool isConfigurable, bool forceNewHiddenClass)
{
    ASSERT(findProperty(name) == SIZE_MAX);
    if (m_flags.m_isVectorMode) {
        if (m_propertyInfo.size() > ESHiddenClassVectorModeSizeLimit) {
            ESHiddenClass* cls = new ESHiddenClass;
            cls->m_flags.m_isVectorMode = false;
            cls->m_propertyInfo.reserve(m_propertyInfo.size() + 1);
            cls->m_propertyInfo.assign(m_propertyInfo.begin(), m_propertyInfo.end());
            size_t resultIndex = cls->m_propertyInfo.size();
            cls->m_propertyInfo.push_back(ESHiddenClassPropertyInfo(name, isData, isWritable, isEnumerable, isConfigurable));

            ASSERT(cls->m_propertyInfo.size() - 1 == resultIndex);

            cls->m_flags.m_hasReadOnlyProperty = m_flags.m_hasReadOnlyProperty | (!isWritable);
            cls->m_flags.m_hasIndexedProperty = m_flags.m_hasIndexedProperty | name->hasOnlyDigit();
            cls->m_flags.m_hasIndexedReadOnlyProperty = m_flags.m_hasIndexedReadOnlyProperty | (!isWritable && name->hasOnlyDigit());
            cls->fillHashMapInfo();
            return cls;
        }
        ESHiddenClass* cls;
        auto iter = m_transitionData.find(name);
        size_t pid = m_propertyInfo.size();
        char flag = assembleHidenClassPropertyInfoFlags(isData, isWritable, isEnumerable, isConfigurable);
        if (iter == m_transitionData.end()) {
            ESHiddenClass** vec = (escargot::ESHiddenClass**)GC_malloc(sizeof(ESHiddenClass*) * 16);
            memset(vec, 0, sizeof(ESHiddenClass *) * 16);
            cls = new ESHiddenClass;
            cls->m_propertyInfo.reserve(m_propertyInfo.size() + 1);
            cls->m_propertyInfo.assign(m_propertyInfo.begin(), m_propertyInfo.end());
            cls->m_propertyInfo.push_back(ESHiddenClassPropertyInfo(name, isData, isWritable, isEnumerable, isConfigurable));

            cls->m_flags.m_hasReadOnlyProperty = m_flags.m_hasReadOnlyProperty | (!isWritable);
            cls->m_flags.m_hasIndexedProperty = m_flags.m_hasIndexedProperty | name->hasOnlyDigit();
            cls->m_flags.m_hasIndexedReadOnlyProperty = m_flags.m_hasIndexedReadOnlyProperty | (!isWritable && name->hasOnlyDigit());
            vec[(size_t)flag] = cls;
            m_transitionData.insert(std::make_pair(name, vec));
        } else {
            cls = iter->second[(size_t)flag];
            if (cls) {
                ASSERT(flag == cls->m_propertyInfo.back().flags());
            } else {
                cls = new ESHiddenClass;
                cls->m_propertyInfo.reserve(m_propertyInfo.size() + 1);
                cls->m_propertyInfo.assign(m_propertyInfo.begin(), m_propertyInfo.end());
                cls->m_propertyInfo.push_back(ESHiddenClassPropertyInfo(name, isData, isWritable, isEnumerable, isConfigurable));

                cls->m_flags.m_hasReadOnlyProperty = m_flags.m_hasReadOnlyProperty | (!isWritable);
                cls->m_flags.m_hasIndexedProperty = m_flags.m_hasIndexedProperty | name->hasOnlyDigit();
                cls->m_flags.m_hasIndexedReadOnlyProperty = m_flags.m_hasIndexedReadOnlyProperty | (!isWritable && name->hasOnlyDigit());
                iter->second[(size_t)flag] = cls;
            }
        }
        ASSERT(cls->m_propertyInfo.size() - 1 == pid);
        return cls;
    } else {
        ESHiddenClass* cls = (forceNewHiddenClass || m_flags.m_hasEverSetAsPrototypeObjectHiddenClass) ? new ESHiddenClass(*this) : this;
        cls->m_propertyInfo.push_back(ESHiddenClassPropertyInfo(name, isData, isWritable, isEnumerable, isConfigurable));
        cls->m_flags.m_hasReadOnlyProperty = cls->m_flags.m_hasReadOnlyProperty | (!isWritable);
        cls->m_flags.m_hasIndexedProperty = cls->m_flags.m_hasIndexedProperty | name->hasOnlyDigit();
        cls->m_flags.m_hasIndexedReadOnlyProperty = cls->m_flags.m_hasIndexedReadOnlyProperty | (!isWritable && name->hasOnlyDigit());

        cls->appendHashMapInfo();
        return cls;
    }
}

ALWAYS_INLINE ESValue ESHiddenClass::read(ESObject* obj, ESValue originalObject, ESString* propertyName, ESString* name)
{
    return read(obj, originalObject, propertyName, findProperty(name));
}

ALWAYS_INLINE ESValue ESHiddenClass::read(ESObject* obj, ESValue originalObject, ESString* propertyName, size_t idx)
{
    if (LIKELY(m_propertyInfo[idx].m_flags.m_isDataProperty)) {
        return obj->m_hiddenClassData[idx];
    } else {
        ESPropertyAccessorData* data = (ESPropertyAccessorData *)obj->m_hiddenClassData[idx].asESPointer();
        return data->value(obj, originalObject, propertyName);
    }
}

ALWAYS_INLINE bool ESHiddenClass::write(ESObject* obj, ESValue originalObject, ESString* propertyName, ESString* name, const ESValue& val)
{
    return write(obj, originalObject, propertyName, findProperty(name), val);
}

ALWAYS_INLINE bool ESHiddenClass::write(ESObject* obj, ESValue originalObject, ESString* propertyName, size_t idx, const ESValue& val)
{
    if (LIKELY(m_propertyInfo[idx].m_flags.m_isDataProperty)) {
        if (UNLIKELY(!m_propertyInfo[idx].m_flags.m_isWritable)) {
            return false;
        }
        obj->m_hiddenClassData[idx] = val;
    } else {
        if (!obj->accessorData(idx)->getJSGetter() && !obj->accessorData(idx)->getJSSetter() && !m_propertyInfo[idx].m_flags.m_isWritable)
            return false;
        ESPropertyAccessorData* data = (ESPropertyAccessorData *)obj->m_hiddenClassData[idx].asESPointer();
        return data->setValue(obj, originalObject, propertyName, val);
    }
    return true;
}

inline void ESObject::set__proto__(const ESValue& obj)
{
    // for global init
    if (obj.isEmpty())
        return;
    if (UNLIKELY(!isExtensible()))
        return;
    ASSERT(obj.isObject() || obj.isUndefinedOrNull());
    ESValue it = obj;
    while (it.isObject()) {
        ESValue proto = it.toObject()->__proto__();
        if (proto.isObject() && proto.toObject() == this)
            ESVMInstance::currentInstance()->throwError(TypeError::create(ESString::create("cyclic __proto__")));
        it = proto;
    }
    m___proto__ = obj;
    setValueAsProtoType(obj);
}

inline bool ESObject::defineDataProperty(const escargot::ESValue& key, bool isWritable, bool isEnumerable, bool isConfigurable, const ESValue& initialValue, bool force, bool forceNewHiddenClass)
{
    if (isESArrayObject() && asESArrayObject()->isFastmode()) {
        uint32_t i = key.toIndex();
        if (i != ESValue::ESInvalidIndexValue) {
            if (isWritable && isEnumerable && isConfigurable) {
                size_t len = asESArrayObject()->length();
                if (asESArrayObject()->shouldConvertToSlowMode(i+1)) {
                    asESArrayObject()->convertToSlowMode();
                } else {
                    if (i >= len) {
                        if (UNLIKELY(!isExtensible() && !force))
                            return false;
                        asESArrayObject()->setLength(i+1);
                    }
                    asESArrayObject()->m_vector[i] = initialValue;
                    return true;
                }
            } else {
                asESArrayObject()->convertToSlowMode();
            }
        }
    }
    if (isESTypedArrayObject()) {
        uint32_t i = key.toIndex();
        if (i != ESValue::ESInvalidIndexValue) {
            ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("cannot redefine property"))));
        }
    }
    if (isESStringObject()) {
        uint32_t i = key.toIndex();
        if (i != ESValue::ESInvalidIndexValue && i < asESStringObject()->length()) {
            // Indexed properties of string object is non-configurable
            ASSERT(!force);
            return false;
        }
    }

    escargot::ESString* keyString = key.toString();
    if (m_flags.m_isEverSetAsPrototypeObject && keyString->hasOnlyDigit()) {
        ESVMInstance::currentInstance()->globalObject()->somePrototypeObjectDefineIndexedProperty();
    }
    size_t oldIdx = m_hiddenClass->findProperty(keyString);
    if (oldIdx == SIZE_MAX) {
        if (UNLIKELY(!isExtensible() && !force))
            return false;
        m_hiddenClass = m_hiddenClass->defineProperty(keyString, true, isWritable, isEnumerable, isConfigurable, forceNewHiddenClass);
        m_hiddenClassData.push_back(initialValue);
        if (UNLIKELY(m_flags.m_isGlobalObject)) {
            ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
            ESVMInstance::currentInstance()->globalObject()->propertyDefined(m_hiddenClassData.size() - 1, keyString);
        }
        if (isESArrayObject()) {
            uint32_t i = key.toIndex();
            if (i != ESValue::ESInvalidIndexValue) {
                if (i >= asESArrayObject()->length())
                    asESArrayObject()->setLength(i+1);
            }
        }
        return true;
    } else {
        ASSERT(!forceNewHiddenClass);
        if (!m_hiddenClass->m_propertyInfo[oldIdx].m_flags.m_isConfigurable && !force && !m_hiddenClassData[oldIdx].isDeleted()) {
            if (oldIdx == 0) { // for __proto__
                ESHiddenClassPropertyInfo& info = m_hiddenClass->m_propertyInfo[oldIdx];
                if (!info.m_flags.m_isWritable || info.m_flags.m_isEnumerable)
                    return false;
                else {
                    set__proto__(initialValue);
                    return true;
                }
            } else {
                ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("cannot redefine property"))));
            }
        }
        if (LIKELY(!m_flags.m_isGlobalObject)) {
            m_hiddenClass = m_hiddenClass->removeProperty(oldIdx);
            m_hiddenClassData.erase(m_hiddenClassData.begin() + oldIdx);
            m_hiddenClass = m_hiddenClass->defineProperty(keyString, true, isWritable, isEnumerable, isConfigurable);
            m_hiddenClassData.push_back(initialValue);
        } else {
            ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
            m_hiddenClass = m_hiddenClass->removePropertyWithoutIndexChange(oldIdx);
            m_hiddenClass->m_propertyInfo[oldIdx].m_flags.m_isDeletedValue = true;
            m_hiddenClassData[oldIdx] = ESValue(ESValue::ESDeletedValue);
            ESVMInstance::currentInstance()->globalObject()->propertyDeleted(oldIdx);

            m_hiddenClass = m_hiddenClass->defineProperty(keyString, true, isWritable, isEnumerable, isConfigurable);
            m_hiddenClassData.push_back(initialValue);
            ESVMInstance::currentInstance()->globalObject()->propertyDefined(m_hiddenClassData.size() - 1, keyString);
        }
        return true;
    }
}

inline bool ESObject::defineAccessorProperty(const escargot::ESValue& key, ESPropertyAccessorData* data, bool isWritable, bool isEnumerable, bool isConfigurable, bool force)
{
    if (isESArrayObject() && asESArrayObject()->isFastmode()) {
        uint32_t i = key.toIndex();
        if (i != ESValue::ESInvalidIndexValue) {
            asESArrayObject()->convertToSlowMode();
        }
    }
    if (isESTypedArrayObject()) {
        uint32_t i = key.toIndex();
        if (i != ESValue::ESInvalidIndexValue) {
            ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("cannot redefine property"))));
        }
    }
    if (isESStringObject()) {
        uint32_t i = key.toIndex();
        if (i != ESValue::ESInvalidIndexValue && i < asESStringObject()->length()) {
            // Indexed properties of string object is non-configurable
            ASSERT(!force);
            return false;
        }
    }

    if (UNLIKELY(m_flags.m_isGlobalObject))
        ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();

    escargot::ESString* keyString = key.toString();
    if (m_flags.m_isEverSetAsPrototypeObject && keyString->hasOnlyDigit()) {
        ESVMInstance::currentInstance()->globalObject()->somePrototypeObjectDefineIndexedProperty();
    }
    size_t oldIdx = m_hiddenClass->findProperty(keyString);
    if (oldIdx == SIZE_MAX) {
        if (UNLIKELY(!isExtensible() && !force))
            return false;
        m_hiddenClass = m_hiddenClass->defineProperty(keyString, false, isWritable, isEnumerable, isConfigurable);
        m_hiddenClassData.push_back((ESPointer *)data);
        if (isESArrayObject()) {
            uint32_t i = key.toIndex();
            if (i != ESValue::ESInvalidIndexValue) {
                if (i >= asESArrayObject()->length())
                    asESArrayObject()->setLength(i+1);
            }
        }
        if (UNLIKELY(m_flags.m_isGlobalObject)) {
            ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
            ESVMInstance::currentInstance()->globalObject()->propertyDefined(m_hiddenClassData.size() - 1, keyString);
        }
        return true;
    } else {
        if (!m_hiddenClass->m_propertyInfo[oldIdx].m_flags.m_isConfigurable && !force && !m_hiddenClassData[oldIdx].isDeleted()) {
            if (oldIdx == 0) // for __proto__
                return false;
            ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("cannot redefine property"))));
        }
        if (LIKELY(!m_flags.m_isGlobalObject)) {
            m_hiddenClass = m_hiddenClass->removeProperty(oldIdx);
            m_hiddenClassData.erase(m_hiddenClassData.begin() + oldIdx);
            m_hiddenClass = m_hiddenClass->defineProperty(keyString, false, isWritable, isEnumerable, isConfigurable);
            m_hiddenClassData.push_back((ESPointer *)data);
        } else {
            ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
            m_hiddenClass = m_hiddenClass->removePropertyWithoutIndexChange(oldIdx);
            m_hiddenClass->m_propertyInfo[oldIdx].m_flags.m_isDeletedValue = true;
            m_hiddenClassData[oldIdx] = ESValue(ESValue::ESDeletedValue);
            ESVMInstance::currentInstance()->globalObject()->propertyDeleted(oldIdx);

            m_hiddenClass = m_hiddenClass->defineProperty(keyString, true, isWritable, isEnumerable, isConfigurable);
            m_hiddenClassData.push_back((ESPointer *)data);
            ESVMInstance::currentInstance()->globalObject()->propertyDefined(m_hiddenClassData.size() - 1, keyString);
        }
        return true;
    }
}

inline bool ESObject::deletePropertyWithException(const ESValue& key, bool force)
{
    if (!deleteProperty(key, force)) {
        ESVMInstance::currentInstance()->throwError(TypeError::create(ESString::create(u"Unable to delete property.")));
    }
    return true;
}

// $9.1.10
inline bool ESObject::deleteProperty(const ESValue& key, bool force)
{
    if (isESArrayObject() && asESArrayObject()->isFastmode()) {
        uint32_t i = key.toIndex();
        if (i != ESValue::ESInvalidIndexValue) {
            if (i < asESArrayObject()->length()) {
                asESArrayObject()->m_vector[i] = ESValue(ESValue::ESEmptyValue);
                return true;
            }
            return true;
        }
    }
    if (isESTypedArrayObject()) {
        uint32_t i = key.toIndex();
        if (i != ESValue::ESInvalidIndexValue) {
            return true;
        }
    }
    if (isESStringObject()) {
        uint32_t i = key.toIndex();
        if (i != ESValue::ESInvalidIndexValue) {
            if (i < asESStringObject()->length()) {
                return false;
            }
        }
    }

    size_t idx = m_hiddenClass->findProperty(key.toString());
    if (idx == SIZE_MAX) // if undefined, return true
        return true;

    if (!m_hiddenClass->m_propertyInfo[idx].m_flags.m_isConfigurable && !force) {
        return false;
    }
    if (UNLIKELY(m_flags.m_isGlobalObject)) {
        ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
        m_hiddenClass = m_hiddenClass->removePropertyWithoutIndexChange(idx);
        m_hiddenClass->m_propertyInfo[idx].m_flags.m_isDeletedValue = true;
        m_hiddenClassData[idx] = ESValue(ESValue::ESDeletedValue);
        ESVMInstance::currentInstance()->globalObject()->propertyDeleted(idx);
    } else {
        m_hiddenClass = m_hiddenClass->removeProperty(idx);
        m_hiddenClassData.erase(m_hiddenClassData.begin() + idx);
    }

    return true;
}

ALWAYS_INLINE bool ESObject::hasProperty(const escargot::ESValue& key)
{
    ESObject* target = this;
    escargot::ESString* keyString = NULL;
    while (true) {
        if (target->isESArrayObject() && target->asESArrayObject()->isFastmode()) {
            uint32_t idx = key.toIndex();
            if (idx != ESValue::ESInvalidIndexValue) {
                if (LIKELY(idx < target->asESArrayObject()->length())) {
                    ESValue e = target->asESArrayObject()->m_vector[idx];
                    if (LIKELY(!e.isEmpty()))
                        return true;
                }
            }
        } else if (target->isESTypedArrayObject()) {
            uint32_t idx = key.toIndex();
            if (idx != ESValue::ESInvalidIndexValue && (uint32_t)idx < target->asESTypedArrayObjectWrapper()->length())
                return true;
        } else if (target->isESStringObject()) {
            uint32_t idx = key.toIndex();
            if ((uint32_t)idx < target->asESStringObject()->length())
                return true;
        }

        if (!keyString) {
            keyString = key.toString();
        }
        size_t t = target->m_hiddenClass->findProperty(keyString);
        if (t != SIZE_MAX)
            return true;

        if (target->__proto__().isESPointer() && target->__proto__().asESPointer()->isESObject()) {
            target = target->__proto__().asESPointer()->asESObject();
        } else {
            return false;
        }
    }
}


ALWAYS_INLINE bool ESObject::hasOwnProperty(const escargot::ESValue& key)
{
    if ((isESArrayObject() && asESArrayObject()->isFastmode()) || isESTypedArrayObject()) {
        uint32_t idx = key.toIndex();
        if (idx != ESValue::ESInvalidIndexValue) {
            if (LIKELY(isESArrayObject())) {
                if (idx < asESArrayObject()->length() && asESArrayObject()->m_vector[idx] != ESValue(ESValue::ESEmptyValue)) {
                    return true;
                } else {
                    return false;
                }
            } else {
                if ((uint32_t)idx < asESTypedArrayObjectWrapper()->length()) {
                    return true;
                } else {
                    return false;
                }
            }
        } else {
        }
    }
    if (isESStringObject()) {
        uint32_t idx = key.toIndex();
        if (idx != ESValue::ESInvalidIndexValue) {
            if ((uint32_t)idx < asESStringObject()->length()) {
                return true;
            }
        }
    }
    return m_hiddenClass->findProperty(key.toString()) != SIZE_MAX;
}

// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-get-o-p
ALWAYS_INLINE ESValue ESObject::get(escargot::ESValue key, escargot::ESValue* receiver)
{
    ESObject* target = this;
    escargot::ESString* keyString = NULL;
    while (true) {
        if (target->isESArrayObject() && target->asESArrayObject()->isFastmode()) {
            uint32_t idx = key.toIndex();
            if (LIKELY(idx < target->asESArrayObject()->length())) {
                ESValue e = target->asESArrayObject()->m_vector[idx];
                if (LIKELY(!e.isEmpty()))
                    return e;
            }
        } else if (target->isESTypedArrayObject()) {
            uint32_t idx = key.toIndex();
            if (idx != ESValue::ESInvalidIndexValue)
                return target->asESTypedArrayObjectWrapper()->get(idx);
        } else if (target->isESStringObject()) {
            uint32_t idx = key.toIndex();
            if (idx < target->asESStringObject()->stringData()->length()) {
                char16_t c = target->asESStringObject()->stringData()->charAt(idx);
                if (LIKELY(c < ESCARGOT_ASCII_TABLE_MAX)) {
                    return strings->asciiTable[c].string();
                } else {
                    return ESString::create(c);
                }
            }
        }

        if (!keyString) {
            keyString = key.toString();
        }
        size_t t = target->m_hiddenClass->findProperty(keyString);
        if (t != SIZE_MAX) {
            ESValue receiverVal(this);
            if (receiver)
                receiverVal = *receiver;
            ESValue ret = target->m_hiddenClass->read(target, receiverVal, keyString, t);
            if (!UNLIKELY(ret.isDeleted())) {
                if (LIKELY(!ret.isEmpty()))
                    return ret;
            }
        }
        if (target->__proto__().isESPointer() && target->__proto__().asESPointer()->isESObject()) {
            target = target->__proto__().asESPointer()->asESObject();
        } else {
            return ESValue();
        }
    }
}

ALWAYS_INLINE ESValue ESObject::getOwnProperty(escargot::ESValue key)
{
    if (isESArrayObject() && asESArrayObject()->isFastmode()) {
        uint32_t idx = key.toIndex();
        if (idx != ESValue::ESInvalidIndexValue) {
            if (LIKELY(idx < asESArrayObject()->length())) {
                ESValue e = asESArrayObject()->m_vector[idx];
                if (LIKELY(!e.isEmpty()))
                    return e;
            }
        }
    }
    if (isESTypedArrayObject()) {
        uint32_t idx = key.toIndex();
        if (idx != ESValue::ESInvalidIndexValue) {
            return asESTypedArrayObjectWrapper()->get(idx);
        }
    }
    if (isESStringObject()) {
        uint32_t idx = key.toIndex();
        if (idx != ESValue::ESInvalidIndexValue) {
            if (LIKELY(idx < asESStringObject()->length())) {
                return asESStringObject()->getCharacterAsString(idx);
            }
        }
    }

    escargot::ESString* keyString = key.toString();
    size_t t = m_hiddenClass->findProperty(keyString);
    if (t != SIZE_MAX) {
        return m_hiddenClass->read(this, this, keyString, t);
    } else {
        return ESValue();
    }
}

ALWAYS_INLINE uint32_t ESObject::length()
{
    if (LIKELY(isESArrayObject()))
        return asESArrayObject()->length();
    else
        return get(strings->length.string()).toUint32();
}
ALWAYS_INLINE ESValue ESObject::pop()
{
    uint32_t len = length();
    if (len == 0) {
        if (!set(strings->length.string(), ESValue(0))) {
            ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Cannot assign to read only property 'length'"))));
        }
        return ESValue();
    }
    if (LIKELY(isESArrayObject() && asESArrayObject()->isFastmode())) {
        ESValue ret = asESArrayObject()->fastPop();
        if (!ret.isEmpty())
            return ret;
    }
    ESValue ret = get(ESValue(len-1));
    deletePropertyWithException(ESValue(len-1));
    set(strings->length.string(), ESValue(len - 1), true);
    return ret;
}

// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-set-o-p-v-throw
ALWAYS_INLINE bool ESObject::set(const escargot::ESValue& key, const ESValue& val, escargot::ESValue* receiver)
{
    if (isESArrayObject() && asESArrayObject()->isFastmode()) {
        uint32_t idx = key.toIndex();
        if (idx != ESValue::ESInvalidIndexValue) {
            if (idx >= asESArrayObject()->length()) {
                if (UNLIKELY(!isExtensible()))
                    return false;
                if (asESArrayObject()->shouldConvertToSlowMode(idx + 1)) {
                    asESArrayObject()->convertToSlowMode();
                    asESArrayObject()->setLength(idx + 1);
                    goto array_fastmode_fail;
                } else {
                    asESArrayObject()->setLength(idx + 1);
                }
            }

            if (LIKELY(!asESArrayObject()->m_vector[idx].isEmpty())) {
                asESArrayObject()->m_vector[idx] = val;
                return true;
            } else {
                // if hole, check prototypes.
                ESValue target = __proto__();
                while (true) {
                    if (!target.isObject()) {
                        break;
                    }
                    if (target.asESPointer()->asESObject()->hiddenClass()->hasIndexedReadOnlyProperty()) {
                        size_t t = target.asESPointer()->asESObject()->hiddenClass()->findProperty(key.toString());
                        if (t != SIZE_MAX) {
                            if (!target.asESPointer()->asESObject()->hiddenClass()->m_propertyInfo[t].m_flags.m_isWritable)
                            return false;
                        }
                    }
                    target = target.asESPointer()->asESObject()->__proto__();
                }
                asESArrayObject()->m_vector[idx] = val;
                return true;
            }
        }
    }
    if (isESTypedArrayObject()) {
        uint32_t idx = key.toIndex();
        asESTypedArrayObjectWrapper()->set(idx, val);
        return true;
    }
    if (isESStringObject()) {
        uint32_t idx = key.toIndex();
        if (idx != ESValue::ESInvalidIndexValue)
            if (idx < asESStringObject()->length())
                return false;
    }
    array_fastmode_fail:
    escargot::ESString* keyString = key.toString();
    size_t idx = m_hiddenClass->findProperty(keyString);
    if (idx == SIZE_MAX) {
        if (UNLIKELY(!isExtensible()))
            return false;
        ESValue target = __proto__();
        bool foundInPrototype = false; // for GetObjectPreComputedCase vector mode cache
        while (true) {
            if (!target.isObject()) {
                break;
            }
            size_t t = target.asESPointer()->asESObject()->hiddenClass()->findProperty(keyString);
            if (t != SIZE_MAX) {
                foundInPrototype = true;
                // http://www.ecma-international.org/ecma-262/5.1/#sec-8.12.5
                // If IsAccessorDescriptor(desc) is true, then
                // Let setter be desc.[[Set]] which cannot be undefined.
                // Call the [[Call]] internal method of setter providing O as the this value and providing V as the sole argument.
                if (!target.asESPointer()->asESObject()->hiddenClass()->m_propertyInfo[t].m_flags.m_isDataProperty) {
                    ESPropertyAccessorData* data = target.asESPointer()->asESObject()->accessorData(t);
                    ESValue receiverVal(this);
                    if (receiver)
                        receiverVal = *receiver;
                    if (data->isAccessorDescriptor()) {
                        if (data->getJSSetter()) {
                            ESValue args[] = {val};
                            ESFunctionObject::call(ESVMInstance::currentInstance(), data->getJSSetter(), receiverVal, args, 1, false);
                            return true;
                        }
                        return false;
                    }
                }

                if (!target.asESPointer()->asESObject()->hiddenClass()->m_propertyInfo[t].m_flags.m_isWritable)
                    return false;
            }
            target = target.asESPointer()->asESObject()->__proto__();
        }
        m_hiddenClass = m_hiddenClass->defineProperty(keyString, true, true, true, true, foundInPrototype);
        m_hiddenClassData.push_back(val);

        if (UNLIKELY(m_flags.m_isGlobalObject))
            ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
        if (UNLIKELY(isESArrayObject())) {
            uint32_t index = key.toIndex();
            uint32_t oldLen = asESArrayObject()->length();
            if (index != ESValue::ESInvalidIndexValue && index >= oldLen)
                asESArrayObject()->setLength(index+1);
        }
        return true;
    } else {
        return m_hiddenClass->write(this, this, keyString, idx, val);
    }
}

ALWAYS_INLINE void ESObject::set(const escargot::ESValue& key, const ESValue& val, bool throwExpetion, escargot::ESValue* receiver)
{
    if (!set(key, val, receiver) && throwExpetion) {
        ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Attempted to assign to readonly property."))));
    }
}

ALWAYS_INLINE size_t ESObject::keyCount()
{
    size_t siz = 0;
    if (isESArrayObject() && asESArrayObject()->isFastmode()) {
        siz += asESArrayObject()->length();
    }
    if (isESTypedArrayObject()) {
        siz += asESTypedArrayObjectWrapper()->length();
    }
    if (isESStringObject()) {
        siz += asESStringObject()->length();
    }
    siz += m_hiddenClassData.size();
    return siz;
}

template <typename Functor>
ALWAYS_INLINE void ESObject::enumeration(Functor t)
{
    if (isESArrayObject() && asESArrayObject()->isFastmode()) {
        for (uint32_t i = 0; i < asESArrayObject()->length(); i++) {
            if (asESArrayObject()->m_vector[i].isEmpty())
                continue;
            t(ESValue(i).toString());
        }
    }

    if (isESTypedArrayObject()) {
        for (uint32_t i = 0; i < asESTypedArrayObjectWrapper()->length(); i++) {
            t(ESValue(i).toString());
        }
    }

    if (isESStringObject()) {
        for (uint32_t i = 0; i < asESStringObject()->length(); i++) {
            t(ESValue(i).toString());
        }
    }

    auto iter = m_hiddenClass->m_propertyInfo.begin();
    while (iter != m_hiddenClass->m_propertyInfo.end()) {
        if (iter->m_flags.m_isEnumerable) {
            t(ESValue(iter->m_name));
        }
        iter++;
    }
}

extern ESHiddenClassPropertyInfo dummyPropertyInfo;
template <typename Functor>
ALWAYS_INLINE void ESObject::enumerationWithNonEnumerable(Functor t)
{
    if (isESArrayObject() && asESArrayObject()->isFastmode()) {
        for (uint32_t i = 0; i < asESArrayObject()->length(); i++) {
            if (asESArrayObject()->m_vector[i].isEmpty())
                continue;
            t(ESValue(i).toString(), &dummyPropertyInfo);
        }
    }

    if (isESTypedArrayObject()) {
        for (uint32_t i = 0; i < asESTypedArrayObjectWrapper()->length(); i++) {
            t(ESValue(i).toString(), &dummyPropertyInfo);
        }
    }

    if (isESStringObject()) {
        for (uint32_t i = 0; i < asESStringObject()->length(); i++) {
            ESHiddenClassPropertyInfo propertyInfo;
            propertyInfo.m_flags.m_isEnumerable = true;
            propertyInfo.m_flags.m_isDeletedValue = false;
            t(ESValue(i).toString(), &propertyInfo);

            // temporary propertyInfo should be unchaged
            ASSERT(propertyInfo.m_flags.m_isDataProperty == true);
            ASSERT(propertyInfo.m_flags.m_isWritable == false);
            ASSERT(propertyInfo.m_flags.m_isEnumerable == true);
            ASSERT(propertyInfo.m_flags.m_isConfigurable == false);
            ASSERT(propertyInfo.m_flags.m_isDeletedValue == false);
        }
    }

    auto iter = m_hiddenClass->m_propertyInfo.begin();
    while (iter != m_hiddenClass->m_propertyInfo.end()) {
        if (iter->m_name != strings->__proto__)
            t(ESValue(iter->m_name), &(*iter));
        iter++;
    }
}

template <typename Comp>
ALWAYS_INLINE void ESObject::sort(const Comp& c)
{
    if (isESArrayObject() && asESArrayObject()->isFastmode()) {
        std::sort(asESArrayObject()->m_vector.begin(), asESArrayObject()->m_vector.end(), c);
    } else {
        uint32_t len = get(strings->length.string()).toUint32();
        ESValueVector selected(len);
        uint32_t n = 0;
        for (uint32_t i = 0; i < len; i++) {
            if (hasProperty(ESValue(i))) {
                selected.push_back(get(ESValue(i)));
                n++;
            }
        }
        std::sort(selected.begin(), selected.end(), c);
        uint32_t i;
        for (i = 0; i < n; i++) {
            if (!set(ESValue(i), selected[i]))
                ESVMInstance::currentInstance()->throwError(TypeError::create(ESString::create("Attempted to assign to readonly property.")));

        }
        while (i != len) {
            deleteProperty(ESValue(i));
            i++;
        }
    }
}

template<>
inline ESTypedArrayObject<Int8Adaptor>::ESTypedArrayObject(TypedArrayType arraytype, ESPointer::Type type)
    : ESTypedArrayObjectWrapper(arraytype,
    (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->int8ArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Int16Adaptor>::ESTypedArrayObject(TypedArrayType arraytype, ESPointer::Type type)
    : ESTypedArrayObjectWrapper(arraytype,
    (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->int16ArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Int32Adaptor>::ESTypedArrayObject(TypedArrayType arraytype, ESPointer::Type type)
    : ESTypedArrayObjectWrapper(arraytype,
    (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->int32ArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Uint8Adaptor>::ESTypedArrayObject(TypedArrayType arraytype, ESPointer::Type type)
    : ESTypedArrayObjectWrapper(arraytype,
    (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->uint8ArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Uint16Adaptor>::ESTypedArrayObject(TypedArrayType arraytype, ESPointer::Type type)
    : ESTypedArrayObjectWrapper(arraytype,
    (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->uint16ArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Uint32Adaptor>::ESTypedArrayObject(TypedArrayType arraytype, ESPointer::Type type)
    : ESTypedArrayObjectWrapper(arraytype,
    (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->uint32ArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Uint8ClampedAdaptor>::ESTypedArrayObject(TypedArrayType arraytype, ESPointer::Type type)
    : ESTypedArrayObjectWrapper(arraytype,
    (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->uint8ClampedArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Float32Adaptor>::ESTypedArrayObject(TypedArrayType arraytype, ESPointer::Type type)
    : ESTypedArrayObjectWrapper(arraytype,
    (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->float32ArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Float64Adaptor>::ESTypedArrayObject(TypedArrayType arraytype, ESPointer::Type type)
    : ESTypedArrayObjectWrapper(arraytype,
    (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->float64ArrayPrototype())
{
}

#define ESStringBuilderInlineStorageMax 12

class ESStringBuilder {
    struct ESStringBuilderPiece {
        ESString* m_string;
        size_t m_start, m_end;
    };

    void appendPiece(ESString* str, size_t s, size_t e)
    {
        ESStringBuilderPiece piece;
        piece.m_string = str;
        piece.m_start = s;
        piece.m_end = e;
        if (!str->isASCIIString()) {
            m_isASCIIString = false;
        }
        m_contentLength += e - s;
        if (m_piecesInlineStorageUsage < ESStringBuilderInlineStorageMax) {
            m_piecesInlineStorage[m_piecesInlineStorageUsage++] = piece;
        } else
            m_pieces.push_back(piece);
    }
public:
    ESStringBuilder()
    {
        m_isASCIIString = true;
        m_contentLength = 0;
        m_piecesInlineStorageUsage = 0;
    }

    void appendString(const char* str)
    {
        appendPiece(ESString::create(str), 0, strlen(str));
    }

    void appendChar(char16_t ch)
    {
        if (ch < 128) {
            appendString(strings->asciiTable[ch].string());
        } else {
            appendString(ESString::create(ch));
        }
    }

    void appendString(ESString* str)
    {
        appendPiece(str, 0, str->length());
    }

    void appendSubString(ESString* str, size_t s, size_t e)
    {
        appendPiece(str, s, e);
    }

    ESString* finalize()
    {
        if (m_isASCIIString) {
            ASCIIString ret;
            ret.resize(m_contentLength);

            size_t currentLength = 0;
            for (size_t i = 0; i < m_piecesInlineStorageUsage; i ++) {
                const ESString* data = m_piecesInlineStorage[i].m_string;
                const ASCIIString& str = *data->asASCIIString();
                size_t s = m_piecesInlineStorage[i].m_start;
                size_t e = m_piecesInlineStorage[i].m_end;
                memcpy((void *)(ret.data() + currentLength), &str[s], e - s);
                currentLength += e - s;
            }

            for (size_t i = 0; i < m_pieces.size(); i ++) {
                const ESString* data = m_pieces[i].m_string;
                const ASCIIString& str = *data->asASCIIString();
                size_t s = m_pieces[i].m_start;
                size_t e = m_pieces[i].m_end;
                memcpy((void *)(ret.data() + currentLength), &str[s], e - s);
                currentLength += e - s;
            }

            return ESString::create(std::move(ret));
        } else {
            UTF16String ret;
            ret.reserve(m_contentLength);

            for (size_t i = 0; i < m_piecesInlineStorageUsage; i ++) {
                const ESString* data = m_piecesInlineStorage[i].m_string;
                if (data->isASCIIString()) {
                    const ASCIIString& str = *data->asASCIIString();
                    ret.append(&str[m_piecesInlineStorage[i].m_start], &str[m_piecesInlineStorage[i].m_end]);
                } else {
                    const UTF16String& str = *data->asUTF16String();
                    ret.append(&str[m_piecesInlineStorage[i].m_start], &str[m_piecesInlineStorage[i].m_end]);
                }
            }

            for (size_t i = 0; i < m_pieces.size(); i ++) {
                const ESString* data = m_pieces[i].m_string;
                if (data->isASCIIString()) {
                    const ASCIIString& str = *data->asASCIIString();
                    ret.append(&str[m_pieces[i].m_start], &str[m_pieces[i].m_end]);
                } else {
                    const UTF16String& str = *data->asUTF16String();
                    ret.append(&str[m_pieces[i].m_start], &str[m_pieces[i].m_end]);
                }
            }

            return ESString::create(std::move(ret));
        }
    }

protected:
    ESStringBuilderPiece m_piecesInlineStorage[ESStringBuilderInlineStorageMax];
    size_t m_piecesInlineStorageUsage;
    std::vector<ESStringBuilderPiece, gc_allocator<ESStringBuilderPiece> > m_pieces;
    size_t m_contentLength;
    bool m_isASCIIString;
};

inline ESString* ESString::concatTwoStrings(ESString* lstr, ESString* rstr)
{
    int llen = lstr->length();
    if (llen == 0)
        return rstr;
    int rlen = rstr->length();
    if (rlen == 0)
        return lstr;

    if (llen > std::numeric_limits<std::int32_t>::max() - rlen)
        ESVMInstance::currentInstance()->throwError(ESValue(RangeError::create(ESString::create("Out of memory"))));

    if (UNLIKELY(llen + rlen >= (int)ESRopeString::ESRopeStringCreateMinLimit)) {
        return ESRopeString::createAndConcat(lstr, rstr);
    } else {
        ESStringBuilder builder;
        builder.appendString(lstr);
        builder.appendString(rstr);
        return builder.finalize();
    }
}

}

#endif
