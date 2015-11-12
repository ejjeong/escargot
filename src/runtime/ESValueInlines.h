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

inline double ESValue::asNumber() const
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
    ESFunctionObject* function;
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
        throw ESValue(TypeError::create(ESString::create(u"cannot convert null into object")));
    } else if (isUndefined()) {
        throw ESValue(TypeError::create(ESString::create(u"cannot convert undefined into object")));
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
    RELEASE_ASSERT_NOT_REACHED();
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

#ifndef ANDROID
        data->wcharData([&val, &data](const wchar_t* buf, unsigned size) {
            wchar_t* end;
            val = wcstod(buf, &end);
            if (end != buf + size) {
                const u16string& str = data->string();
                bool isOnlyWhiteSpace = true;
                for (unsigned i = 0; i < str.length(); i ++) {
                    // FIXME we shold not use isspace function. implement javascript isspace function.
                    if (!isspace(str[i])) {
                        isOnlyWhiteSpace = false;
                        break;
                    }
                }
                if (isOnlyWhiteSpace) {
                    val = 0;
                } else
                    val = std::numeric_limits<double>::quiet_NaN();
            }
        });

#else
        char* end;
        NullableUTF8String s = data->toNullableUTF8String();
        const char* buf = s.m_buffer;
        val = strtod(buf, &end);
        if (end != buf + s.m_bufferSize-1) {
            const u16string& str = data->string();
            bool isOnlyWhiteSpace = true;
            for (unsigned i = 0; i < str.length(); i ++) {
                // FIXME we shold not use isspace function. implement javascript isspace function.
                if (!isspace(str[i])) {
                    isOnlyWhiteSpace = false;
                    break;
                }
            }
            if (isOnlyWhiteSpace) {
                val = 0;
            } else
                val = std::numeric_limits<double>::quiet_NaN();
        }
#endif
        return val;
    } else {
        return toPrimitive().toNumber();
    }
}

ALWAYS_INLINE bool ESValue::toBoolean() const
{
    if (*this == ESValue(true))
        return true;

    if (*this == ESValue(false))
        return false;

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
    // Let number be the result of calling ToNumber on the input argument.
    double num = toNumber();

    // If number is NaN, +0, −0, +∞, or −∞, return +0.
    if (UNLIKELY(std::isnan(num) || num == 0.0 || std::isinf(0.0))) {
        return 0;
    }

    // Let posInt be sign(number) * floor(abs(number)).
    long long int posInt = (num < 0 ? -1 : 1) * std::floor(std::abs(num));
    // Let int32bit be posInt modulo 232; that is, a finite integer value k of Number type with positive sign
    // and less than 232 in magnitude such that the mathematical difference of posInt and k is mathematically an integer multiple of 232.
    long long int int32bit = posInt % 0x100000000;
    int res;
    // If int32bit is greater than or equal to 231, return int32bit − 232, otherwise return int32bit.
    if (int32bit >= 0x80000000)
        res = int32bit - 0x100000000;
    else
        res = int32bit;
    return res;
}

inline uint32_t ESValue::toUint32() const
{
    // consume fast case
    if (LIKELY(isInt32()))
        return asInt32();

    // Let number be the result of calling ToNumber on the input argument.
    double num = toNumber();

    // If number is NaN, +0, −0, +∞, or −∞, return +0.
    if (UNLIKELY(std::isnan(num) || num == 0.0 || std::isinf(0.0))) {
        return 0;
    }

    // Let posInt be sign(number) × floor(abs(number)).
    long long int posInt = (num < 0 ? -1 : 1) * std::floor(std::abs(num));

    // Let int32bit be posInt modulo 232; that is, a finite integer value k of Number type with positive sign and
    // less than 232 in magnitude such that the mathematical difference of posInt and k is mathematically an integer multiple of 232.
    long long int int32bit = posInt % 0x100000000;
    return int32bit;
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

// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-abstract-equality-comparison
ALWAYS_INLINE bool ESValue::abstractEqualsTo(const ESValue& val)
{
    if (isInt32() && val.isInt32()) {
        if (u.asInt64 == val.u.asInt64)
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
            return o->asESString()->string() == o2->asESString()->string();
        }
        if (o == o2)
            return o == o2;
    }
    return false;
}

// ==============================================================================
// ===32-bit architecture========================================================
// ==============================================================================

#if ESCARGOT_32

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

inline bool ESValue::isUndefined() const
{
    return tag() == UndefinedTag;
}

inline bool ESValue::isNull() const
{
    return tag() == NullTag;
}

inline bool ESValue::isBoolean() const
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

inline bool ESValue::isESPointer() const
{
    return !(u.asInt64 & TagMask);
}

inline bool ESValue::isUndefined() const
{
    return u.asInt64 == ValueUndefined;
}

inline bool ESValue::isNull() const
{
    return u.asInt64 == ValueNull;
}

inline bool ESValue::isBoolean() const
{
    return u.asInt64 == ValueTrue || u.asInt64 == ValueFalse;
}

ALWAYS_INLINE ESPointer* ESValue::asESPointer() const
{
    ASSERT(isESPointer());
    return u.ptr;
}


#endif

ALWAYS_INLINE ESValue ESPropertyAccessorData::value(::escargot::ESObject* obj, ::escargot::ESObject* originalObj)
{
    if (m_nativeGetter) {
        ASSERT(!m_jsGetter);
        return m_nativeGetter(obj, originalObj);
    }
    if (m_jsGetter) {
        ASSERT(!m_nativeGetter);
        return ESFunctionObject::call(ESVMInstance::currentInstance(), m_jsGetter, originalObj, NULL, 0, false);
    }
    return ESValue();
}

ALWAYS_INLINE void ESPropertyAccessorData::setValue(::escargot::ESObject* obj, ::escargot::ESObject* originalObj, const ESValue& value)
{
    if (m_nativeSetter) {
        ASSERT(!m_jsSetter);
        m_nativeSetter(obj, originalObj, value);
    }
    if (m_jsSetter) {
        ASSERT(!m_nativeSetter);
        ESValue arg[] = {value};
        ESFunctionObject::call(ESVMInstance::currentInstance(), m_jsSetter,
            originalObj , arg, 1, false);
    }
}

ALWAYS_INLINE ESHiddenClass* ESHiddenClass::removeProperty(size_t idx)
{
    ASSERT(idx != SIZE_MAX);
    // can not delete __proto__
    ASSERT(idx != 0);
    ASSERT(m_propertyInfo[idx].m_flags.m_isConfigurable);

    if (m_flags.m_isVectorMode) {
        ESHiddenClass* ret = ESVMInstance::currentInstance()->initialHiddenClassForObject();
        ASSERT(*m_propertyInfo[0].m_name == *strings->__proto__.string());
        for (unsigned i = 1; i < m_propertyInfo.size(); i ++) {
            if (idx != i) {
                size_t d;
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
                    size_t d;
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
            if (i < idx) {
                cls->m_propertyIndexHashMapInfo.insert(std::make_pair(m_propertyInfo[i].m_name, i));
            } else if (i > idx) {
                cls->m_propertyIndexHashMapInfo.insert(std::make_pair(m_propertyInfo[i].m_name, i - 1));
            }
            cls->m_flags.m_hasReadOnlyProperty |= (!m_propertyInfo[i].m_flags.m_isWritable);
            cls->m_flags.m_hasIndexedProperty |= m_propertyInfo[i].m_name->hasOnlyDigit();
            cls->m_flags.m_hasIndexedReadOnlyProperty |= (!m_propertyInfo[i].m_flags.m_isWritable && m_propertyInfo[i].m_name->hasOnlyDigit());
        }

        // TODO
        // delete this;
        return cls;
    }
}

// IS FUNCTION IS FOR GLOBAL OBJECT
ALWAYS_INLINE ESHiddenClass* ESHiddenClass::removePropertyWithoutIndexChange(size_t idx)
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
        } else {
            cls->m_propertyInfo.push_back(m_propertyInfo[i]);
            if (m_propertyInfo[i].m_flags.m_isDeletedValue)
                continue;
            cls->m_propertyIndexHashMapInfo.insert(std::make_pair(m_propertyInfo[i].m_name, i));
            cls->m_flags.m_hasReadOnlyProperty |= (!m_propertyInfo[i].m_flags.m_isWritable);
            cls->m_flags.m_hasIndexedProperty |= m_propertyInfo[i].m_name->hasOnlyDigit();
            cls->m_flags.m_hasIndexedReadOnlyProperty |= (!m_propertyInfo[i].m_flags.m_isWritable && m_propertyInfo[i].m_name->hasOnlyDigit());
        }
    }

    // TODO
    // delete this;
    return cls;
}

ALWAYS_INLINE ESHiddenClass* ESHiddenClass::morphToNonVectorMode()
{
    ESHiddenClass* cls = new ESHiddenClass;
    cls->m_flags.m_isVectorMode = false;
    for (unsigned i = 0; i < m_propertyInfo.size(); i ++) {
        cls->m_propertyIndexHashMapInfo.insert(std::make_pair(m_propertyInfo[i].m_name, i));
    }
    cls->m_propertyInfo.assign(m_propertyInfo.begin(), m_propertyInfo.end());

    ASSERT(cls->m_propertyIndexHashMapInfo.size() == cls->m_propertyInfo.size());
    cls->m_flags.m_hasReadOnlyProperty = m_flags.m_hasReadOnlyProperty;
    cls->m_flags.m_hasIndexedProperty = m_flags.m_hasIndexedProperty;
    cls->m_flags.m_hasIndexedReadOnlyProperty = m_flags.m_hasIndexedReadOnlyProperty;

    return cls;
}

ALWAYS_INLINE ESHiddenClass* ESHiddenClass::forceNonVectorMode()
{
    ESHiddenClass* cls = morphToNonVectorMode();
    cls->m_flags.m_forceNonVectorMode = true;
    return cls;
}

ALWAYS_INLINE ESHiddenClass* ESHiddenClass::defineProperty(ESString* name, bool isData, bool isWritable, bool isEnumerable, bool isConfigurable)
{
    ASSERT(findProperty(name) == SIZE_MAX);
    if (m_flags.m_isVectorMode) {
        if (m_propertyInfo.size() > ESHiddenClassVectorModeSizeLimit) {
            ESHiddenClass* cls = new ESHiddenClass;
            cls->m_flags.m_isVectorMode = false;
            for (unsigned i = 0; i < m_propertyInfo.size(); i ++) {
                cls->m_propertyIndexHashMapInfo.insert(std::make_pair(m_propertyInfo[i].m_name, i));
            }
            cls->m_propertyInfo.assign(m_propertyInfo.begin(), m_propertyInfo.end());
            size_t resultIndex = cls->m_propertyInfo.size();
            cls->m_propertyIndexHashMapInfo.insert(std::make_pair(name, resultIndex));
            cls->m_propertyInfo.push_back(ESHiddenClassPropertyInfo(name, isData, isWritable, isEnumerable, isConfigurable));

            ASSERT(cls->m_propertyIndexHashMapInfo.size() == cls->m_propertyInfo.size());
            ASSERT(cls->m_propertyInfo.size() - 1 == resultIndex);

            cls->m_flags.m_hasReadOnlyProperty = m_flags.m_hasReadOnlyProperty | (!isWritable);
            cls->m_flags.m_hasIndexedProperty = m_flags.m_hasIndexedProperty | name->hasOnlyDigit();
            cls->m_flags.m_hasIndexedReadOnlyProperty = m_flags.m_hasIndexedReadOnlyProperty | (!isWritable && name->hasOnlyDigit());
            return cls;
        }
        ESHiddenClass* cls;
        auto iter = m_transitionData.find(name);
        size_t pid = m_propertyInfo.size();
        char flag = assembleHidenClassPropertyInfoFlags(isData, isWritable, isEnumerable, isConfigurable);
        if (iter == m_transitionData.end()) {
            ESHiddenClass** vec = new(GC) ESHiddenClass*[16];
            memset(vec, 0, sizeof(ESHiddenClass *) * 16);
            cls = new ESHiddenClass;
            cls->m_propertyIndexHashMapInfo.insert(m_propertyIndexHashMapInfo.begin(), m_propertyIndexHashMapInfo.end());
            cls->m_propertyIndexHashMapInfo.insert(std::make_pair(name, pid));
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
                cls->m_propertyIndexHashMapInfo.insert(m_propertyIndexHashMapInfo.begin(), m_propertyIndexHashMapInfo.end());
                cls->m_propertyIndexHashMapInfo.insert(std::make_pair(name, pid));
                cls->m_propertyInfo.assign(m_propertyInfo.begin(), m_propertyInfo.end());
                cls->m_propertyInfo.push_back(ESHiddenClassPropertyInfo(name, isData, isWritable, isEnumerable, isConfigurable));

                cls->m_flags.m_hasReadOnlyProperty = m_flags.m_hasReadOnlyProperty | (!isWritable);
                cls->m_flags.m_hasIndexedProperty = m_flags.m_hasIndexedProperty | name->hasOnlyDigit();
                cls->m_flags.m_hasIndexedReadOnlyProperty = m_flags.m_hasIndexedReadOnlyProperty | (!isWritable && name->hasOnlyDigit());
                iter->second[(size_t)flag] = cls;
            }
        }
        ASSERT(cls->m_propertyIndexHashMapInfo.size() == cls->m_propertyInfo.size());
        ASSERT(cls->m_propertyInfo.size() - 1 == pid);
        return cls;
    } else {
        size_t idx = m_propertyInfo.size();
        m_propertyIndexHashMapInfo.insert(std::make_pair(name, idx));
        m_propertyInfo.push_back(ESHiddenClassPropertyInfo(name, isData, isWritable, isEnumerable, isConfigurable));
        m_flags.m_hasReadOnlyProperty = m_flags.m_hasReadOnlyProperty | (!isWritable);
        m_flags.m_hasIndexedProperty = m_flags.m_hasIndexedProperty | name->hasOnlyDigit();
        m_flags.m_hasIndexedReadOnlyProperty = m_flags.m_hasIndexedReadOnlyProperty | (!isWritable && name->hasOnlyDigit());
        return this;
    }
}

ALWAYS_INLINE ESValue ESHiddenClass::read(ESObject* obj, ESObject* originalObject, ESString* name)
{
    return read(obj, originalObject, findProperty(name));
}

ALWAYS_INLINE ESValue ESHiddenClass::read(ESObject* obj, ESObject* originalObject, size_t idx)
{
    if (LIKELY(m_propertyInfo[idx].m_flags.m_isDataProperty)) {
        return obj->m_hiddenClassData[idx];
    } else {
        ESPropertyAccessorData* data = (ESPropertyAccessorData *)obj->m_hiddenClassData[idx].asESPointer();
        return data->value(obj, originalObject);
    }
}

ALWAYS_INLINE bool ESHiddenClass::write(ESObject* obj, ESObject* originalObject, ESString* name, const ESValue& val)
{
    return write(obj, originalObject, findProperty(name), val);
}

ALWAYS_INLINE bool ESHiddenClass::write(ESObject* obj, ESObject* originalObject, size_t idx, const ESValue& val)
{
    if (LIKELY(m_propertyInfo[idx].m_flags.m_isDataProperty)) {
        if (UNLIKELY(!m_propertyInfo[idx].m_flags.m_isWritable)) {
            return false;
        }
        obj->m_hiddenClassData[idx] = val;
    } else {
        ESPropertyAccessorData* data = (ESPropertyAccessorData *)obj->m_hiddenClassData[idx].asESPointer();
        data->setValue(obj, originalObject, val);
    }
    return true;
}

inline void ESObject::set__proto__(const ESValue& obj)
{
    // for global init
    if (obj.isEmpty())
        return;
    ASSERT(obj.isObject() || obj.isUndefinedOrNull());
    m___proto__ = obj;
    setValueAsProtoType(obj);
}

inline bool ESObject::defineDataProperty(const escargot::ESValue& key, bool isWritable, bool isEnumerable, bool isConfigurable, const ESValue& initialValue)
{
    if (isESArrayObject() && asESArrayObject()->isFastmode()) {
        uint32_t i = key.toIndex();
        if (i != ESValue::ESInvalidIndexValue) {
            if (isWritable && isEnumerable && isConfigurable) {
                int len = asESArrayObject()->length();
                if (asESArrayObject()->shouldConvertToSlowMode(i)) {
                    asESArrayObject()->convertToSlowMode();
                } else {
                    if (i >= len) {
                        if (UNLIKELY(!isExtensible()))
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
            throw ESValue(TypeError::create(ESString::create("cannot redefine property")));
        }
    }
    if (isESStringObject()) {
        uint32_t i = key.toIndex();
        if (i != ESValue::ESInvalidIndexValue && i < asESStringObject()->length()) {
            // Indexed properties of string object is non-configurable
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
        if (UNLIKELY(!isExtensible()))
            return false;
        m_hiddenClass = m_hiddenClass->defineProperty(keyString, true, isWritable, isEnumerable, isConfigurable);
        m_hiddenClassData.push_back(initialValue);
        if (isESArrayObject()) {
            uint32_t i = key.toIndex();
            if (i != ESValue::ESInvalidIndexValue) {
                if (i >= asESArrayObject()->length())
                    asESArrayObject()->setLength(i+1);
            }
        }
        return true;
    } else {
        if (!m_hiddenClass->m_propertyInfo[oldIdx].m_flags.m_isConfigurable) {
            throw ESValue(TypeError::create(ESString::create("cannot redefine property")));
        }
        m_hiddenClass = m_hiddenClass->removeProperty(oldIdx);
        m_hiddenClassData.erase(m_hiddenClassData.begin() + oldIdx);
        m_hiddenClass = m_hiddenClass->defineProperty(keyString, true, isWritable, isEnumerable, isConfigurable);
        m_hiddenClassData.push_back(initialValue);
        return true;
    }
}

inline bool ESObject::defineAccessorProperty(const escargot::ESValue& key, ESPropertyAccessorData* data, bool isWritable, bool isEnumerable, bool isConfigurable)
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
            throw ESValue(TypeError::create(ESString::create("cannot redefine property")));
        }
    }
    if (isESStringObject()) {
        uint32_t i = key.toIndex();
        if (i != ESValue::ESInvalidIndexValue && i < asESStringObject()->length()) {
            // Indexed properties of string object is non-configurable
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
        if (UNLIKELY(!isExtensible()))
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
        return true;
    } else {
        if (!m_hiddenClass->m_propertyInfo[oldIdx].m_flags.m_isConfigurable) {
            throw ESValue(TypeError::create(ESString::create("cannot redefine property")));
        }
        m_hiddenClass = m_hiddenClass->removeProperty(oldIdx);
        m_hiddenClassData.erase(m_hiddenClassData.begin() + oldIdx);
        m_hiddenClass = m_hiddenClass->defineProperty(keyString, false, isWritable, isEnumerable, isConfigurable);
        m_hiddenClassData.push_back((ESPointer *)data);
        return true;
    }
}

// $9.1.10
inline bool ESObject::deleteProperty(const ESValue& key)
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

    if (!m_hiddenClass->m_propertyInfo[idx].m_flags.m_isConfigurable) {
        return false;
    }
    if (UNLIKELY(m_flags.m_isGlobalObject)) {
        ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
        m_hiddenClass = m_hiddenClass->removePropertyWithoutIndexChange(idx);
        m_hiddenClassData[idx] = ESValue(ESValue::ESDeletedValue);
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
                if (LIKELY((int)idx < target->asESArrayObject()->length())) {
                    ESValue e = target->asESArrayObject()->m_vector[idx];
                    if (LIKELY(!e.isEmpty()))
                        return true;
                }
            }
        } else if (target->isESTypedArrayObject()) {
            uint32_t idx = key.toIndex();
            if ((uint32_t)idx < target->asESTypedArrayObjectWrapper()->length())
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
                if ((int)idx < asESArrayObject()->length() && asESArrayObject()->m_vector[idx] != ESValue(ESValue::ESEmptyValue)) {
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
ALWAYS_INLINE ESValue ESObject::get(escargot::ESValue key)
{
    ESObject* target = this;
    escargot::ESString* keyString = NULL;
    while (true) {
        if (target->isESArrayObject() && target->asESArrayObject()->isFastmode()) {
            uint32_t idx = key.toIndex();
            if (idx != ESValue::ESInvalidIndexValue) {
                if (LIKELY((int)idx < target->asESArrayObject()->length())) {
                    ESValue e = target->asESArrayObject()->m_vector[idx];
                    if (LIKELY(!e.isEmpty()))
                        return e;
                }
            }
        } else if (target->isESTypedArrayObject()) {
            uint32_t idx = key.toIndex();
            if (idx != ESValue::ESInvalidIndexValue) {
                return target->asESTypedArrayObjectWrapper()->get(idx);
            }
        } else if (target->isESStringObject()) {
            uint32_t idx = key.toIndex();
            if (idx != ESValue::ESInvalidIndexValue) {
                if (idx < target->asESStringObject()->stringData()->length()) {
                    char16_t c = target->asESStringObject()->stringData()->string()[idx];
                    if (LIKELY(c < ESCARGOT_ASCII_TABLE_MAX)) {
                        return strings->asciiTable[c].string();
                    } else {
                        return ESString::create(c);
                    }
                }
            }
        }

        if (!keyString) {
            keyString = key.toString();
        }
        size_t t = target->m_hiddenClass->findProperty(keyString);
        if (t != SIZE_MAX) {
            return target->m_hiddenClass->read(target, this, t);
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
            if (LIKELY((int)idx < asESArrayObject()->length())) {
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
            if (LIKELY((int)idx < asESStringObject()->length())) {
                return asESStringObject()->getCharacterAsString(idx);
            }
        }
    }

    escargot::ESString* keyString = key.toString();
    size_t t = m_hiddenClass->findProperty(keyString);
    if (t != SIZE_MAX) {
        return m_hiddenClass->read(this, this, t);
    } else {
        return ESValue();
    }
}

ALWAYS_INLINE const uint32_t ESObject::length()
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
        set(strings->length.string(), ESValue(0));
        return ESValue();
    }
    if (LIKELY(isESArrayObject() && asESArrayObject()->isFastmode())) {
        ESValue ret = asESArrayObject()->fastPop();
        if (!ret.isEmpty())
            return ret;
    }
    if (isESStringObject()) {
        RELEASE_ASSERT_NOT_REACHED();
    }
    ESValue ret = get(ESValue(len-1));
    deleteProperty(ESValue(len-1));
    set(strings->length.string(), ESValue(len - 1));
    return ret;
}
ALWAYS_INLINE void ESObject::eraseValues(int idx, int cnt)
{
    if (LIKELY(isESArrayObject())) {
        asESArrayObject()->eraseValues(idx, cnt);
        return;
    }
    if (isESStringObject()) {
        if (idx < asESStringObject()->length())
            return;
    }
    for (uint32_t i = idx; i < length(); i++) {
        if (i+cnt < length())
            set(ESValue(i), get(ESValue(i+cnt)));
        else
            deleteProperty(ESValue(i));
    }
    // NOTE: length is set in Array.splice
}

// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-set-o-p-v-throw
ALWAYS_INLINE bool ESObject::set(const escargot::ESValue& key, const ESValue& val)
{
    if (isESArrayObject() && asESArrayObject()->isFastmode()) {
        uint32_t idx = key.toIndex();
        if (idx != ESValue::ESInvalidIndexValue) {
            if (idx >= asESArrayObject()->length()) {
                if (UNLIKELY(!isExtensible()))
                    return false;
                if (asESArrayObject()->shouldConvertToSlowMode(idx)) {
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
        if (idx != ESValue::ESInvalidIndexValue) {
            if (idx < asESTypedArrayObjectWrapper()->length())
                asESTypedArrayObjectWrapper()->set(idx, val);
            return true;
        }
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
        while (true) {
            if (!target.isObject()) {
                break;
            }
            if (target.asESPointer()->asESObject()->hiddenClass()->hasReadOnlyProperty()) {
                size_t t = target.asESPointer()->asESObject()->hiddenClass()->findProperty(key.toString());
                if (t != SIZE_MAX) {
                    if (!target.asESPointer()->asESObject()->hiddenClass()->m_propertyInfo[t].m_flags.m_isWritable)
                        return false;
                }
            }
            target = target.asESPointer()->asESObject()->__proto__();
        }
        m_hiddenClass = m_hiddenClass->defineProperty(keyString, true, true, true, true);
        m_hiddenClassData.push_back(val);

        if (UNLIKELY(m_flags.m_isGlobalObject))
            ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
        return true;
    } else {
        return m_hiddenClass->write(this, this, idx, val);
    }
}

ALWAYS_INLINE void ESObject::set(const escargot::ESValue& key, const ESValue& val, bool throwExpetion)
{
    if (!set(key, val) && throwExpetion) {
        throw ESValue(TypeError::create(ESString::create("Cannot assign to readonly property")));
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
        for (int i = 0; i < asESArrayObject()->length(); i++) {
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
        for (int i = 0; i < asESStringObject()->length(); i++) {
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
        for (int i = 0; i < asESArrayObject()->length(); i++) {
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
        for (int i = 0; i < asESStringObject()->length(); i++) {
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

ALWAYS_INLINE void ESObject::sort()
{
    if (isESArrayObject()) {
        std::sort(asESArrayObject()->m_vector.begin(), asESArrayObject()->m_vector.end(), [](const ::escargot::ESValue& a, const ::escargot::ESValue& b) -> bool {
            if (a.isEmpty() || a.isUndefined())
                return false;
            if (b.isEmpty() || b.isUndefined())
                return true;
            ::escargot::ESString* vala = a.toString();
            ::escargot::ESString* valb = b.toString();
            return vala->string() < valb->string();
        });
    } else {
        // TODO non fast mode sort
    }
}

template <typename Comp>
ALWAYS_INLINE void ESObject::sort(const Comp& c)
{
    if (isESArrayObject()) {
        std::sort(asESArrayObject()->m_vector.begin(), asESArrayObject()->m_vector.end(), c);
    } else {
        // TODO non fast mode sort
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

}

#endif
