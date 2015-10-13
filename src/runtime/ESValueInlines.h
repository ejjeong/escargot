#ifndef ESValueInlines_h
#define ESValueInlines_h

namespace escargot {


// The fast double-to-(unsigned-)int conversion routine does not guarantee
// rounding towards zero.
// The result is unspecified if x is infinite or NaN, or if the rounded
// integer value is outside the range of type int.
inline int FastD2I(double x) {
  return static_cast<int32_t>(x);
}


inline double FastI2D(int x) {
  // There is no rounding involved in converting an integer to a
  // double, so this code should compile to a few instructions without
  // any FPU pipeline stalls.
  return static_cast<double>(x);
}

//==============================================================================
//===common architecture========================================================
//==============================================================================

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

inline ESString* ESValue::toString() const
{
    if(isESString()) {
        return asESString();
    } else if(isInt32()) {
        int num = asInt32();
        if(num >= 0 && num < ESCARGOT_STRINGS_NUMBERS_MAX)
            return strings->numbers[num].string();
        return ESString::create(num);
    } else if(isNumber()) {
        double d = asNumber();
        if (std::isnan(d))
            return strings->NaN.string();
        if (std::isinf(d)) {
            if(std::signbit(d))
                return strings->NegativeInfinity.string();
            else
                return strings->Infinity.string();
        }
        //convert -0.0 into 0.0
        //in c++, d = -0.0, d == 0.0 is true
        if (d == 0.0)
            d = 0;

        return ESString::create(d);
    } else if(isUndefined()) {
        return strings->undefined.string();
    } else if(isNull()) {
        return strings->null.string();
    } else if(isBoolean()) {
        if(asBoolean())
            return strings->stringTrue.string();
        else
            return strings->stringFalse.string();
    } else {
        return toPrimitive(PreferString).toString();
    }
}

inline ESObject* ESValue::toObject() const
{
    ESFunctionObject* function;
    ESObject* object;
    if (isESPointer() && asESPointer()->isESObject()) {
       return asESPointer()->asESObject();
    } else if (isNumber()) {
        object = ESNumberObject::create(toNumber());
    } else if (isBoolean()) {
        object = ESBooleanObject::create(toBoolean());
    } else if (isESString()) {
        object = ESStringObject::create(asESPointer()->asESString());
    } else if(isNull()){
        throw ESValue(TypeError::create(ESString::create(u"cannot convert null into object")));
    } else if(isUndefined()){
        throw ESValue(TypeError::create(ESString::create(u"cannot convert undefined into object")));
    } else {
        RELEASE_ASSERT_NOT_REACHED();
    }
    return object;
}

inline ESValue ESValue::toPrimitive(PrimitiveTypeHint preferredType) const
{
    if (UNLIKELY(!isPrimitive())) {
        ESObject* obj = asESPointer()->asESObject();
        if (preferredType == PrimitiveTypeHint::PreferString) {
            ESValue toString = obj->get(ESValue(strings->toString.string()));
            if(toString.isESPointer() && toString.asESPointer()->isESFunctionObject()) {
                ESValue str = ESFunctionObject::call(ESVMInstance::currentInstance(), toString, obj, NULL, 0, false);
                if(str.isPrimitive())
                    return str;
            }

            ESValue valueOf = obj->get(ESValue(strings->valueOf.string()));
            if(valueOf.isESPointer() && valueOf.asESPointer()->isESFunctionObject()) {
                ESValue val = ESFunctionObject::call(ESVMInstance::currentInstance(), valueOf, obj, NULL, 0, false);
                if(val.isPrimitive())
                    return val;
            }
        } else { // preferNumber
            ESValue valueOf = obj->get(ESValue(strings->valueOf.string()));
            if(valueOf.isESPointer() && valueOf.asESPointer()->isESFunctionObject()) {
                ESValue val = ESFunctionObject::call(ESVMInstance::currentInstance(), valueOf, obj, NULL, 0, false);
                if(val.isPrimitive())
                    return val;
            }

            ESValue toString = obj->get(ESValue(strings->toString.string()));
            if(toString.isESPointer() && toString.asESPointer()->isESFunctionObject()) {
                ESValue str = ESFunctionObject::call(ESVMInstance::currentInstance(), toString, obj, NULL, 0, false);
                if(str.isPrimitive())
                    return str;
            }
        }
        throw ESValue(TypeError::create());
    } else {
        return *this;
    }
}

inline double ESValue::toNumber() const
{
    //http://www.ecma-international.org/ecma-262/6.0/#sec-tonumber
#ifdef ESCARGOT_64
    auto n = u.asInt64 & TagTypeNumber;
    if (LIKELY(n)) {
        if(n == TagTypeNumber) {
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
    else if (isESPointer()) {
        ESPointer* o = asESPointer();
        if (o->isESString() || o->isESStringObject()) {
            double val;
            ESString* data;
            if(LIKELY(o->isESString())) {
                data = o->asESString();
            } else {
                data = o->asESStringObject()->stringData();
            }

            //A StringNumericLiteral that is empty or contains only white space is converted to +0.
            if(data->length() == 0)
                return 0;

            data->wcharData([&val, &data](const wchar_t* buf, unsigned size){
                wchar_t* end;
                val = wcstod(buf, &end);
                if(end != buf + size) {
                    const u16string& str = data->string();
                    bool isOnlyWhiteSpace = true;
                    for(unsigned i = 0; i < str.length() ; i ++) {
                        //FIXME we shold not use isspace function. implement javascript isspace function.
                        if(!isspace(str[i])) {
                            isOnlyWhiteSpace = false;
                            break;
                        }
                    }
                    if(isOnlyWhiteSpace) {
                        val = 0;
                    } else
                        val = std::numeric_limits<double>::quiet_NaN();
                }
            });
            return val;
        } else if (o->isESObject())
            return toPrimitive().toNumber();
        else if (o->isESDateObject())
            return o->asESDateObject()->getTimeAsMilisec();
    }
    RELEASE_ASSERT_NOT_REACHED();
}

inline bool ESValue::toBoolean() const
{
    if (*this == ESValue(true))
        return true;

    if (*this == ESValue(false))
        return false;

    if (isInt32())
        return asInt32();

    if (isDouble()) {
        double d = asDouble();
        if(isnan(d))
            return false;
        if(d == 0.0)
            return false;
        return true;
    }

    if (isUndefinedOrNull())
        return false;

    ASSERT(isESPointer());
    if(asESPointer()->isESString())
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
    // TODO check +0, -0, +inf, -inf
    return d < 0 ? -1 : 1 * std::floor(std::abs(d));
}

// http://www.ecma-international.org/ecma-262/5.1/#sec-9.5
inline int32_t ESValue::toInt32() const
{
    //consume fast case
    if (LIKELY(isInt32()))
        return asInt32();

    //Let number be the result of calling ToNumber on the input argument.
    double num = toNumber();

    //If number is NaN, +0, −0, +∞, or −∞, return +0.
    if(UNLIKELY(isnan(num) || num == 0.0 || isinf(0.0))) {
        return 0;
    }

    //Let posInt be sign(number) * floor(abs(number)).
    long long int posInt = (num < 0 ? -1 : 1) * std::floor(std::abs(num));
    //Let int32bit be posInt modulo 232; that is, a finite integer value k of Number type with positive sign
    //and less than 232 in magnitude such that the mathematical difference of posInt and k is mathematically an integer multiple of 232.
    long long int int32bit = posInt % 0x100000000;
    int res;
    //If int32bit is greater than or equal to 231, return int32bit − 232, otherwise return int32bit.
    if (int32bit >= 0x80000000)
        res = int32bit - 0x100000000;
    else
        res = int32bit;
    return res;
}

inline uint32_t ESValue::toUint32() const
{
    //consume fast case
    if (LIKELY(isInt32()))
        return asInt32();

    //Let number be the result of calling ToNumber on the input argument.
    double num = toNumber();

    //If number is NaN, +0, −0, +∞, or −∞, return +0.
    if(UNLIKELY(isnan(num) || num == 0.0 || isinf(0.0))) {
        return 0;
    }

    //Let posInt be sign(number) × floor(abs(number)).
    long long int posInt = (num < 0 ? -1 : 1) * std::floor(std::abs(num));

    //Let int32bit be posInt modulo 232; that is, a finite integer value k of Number type with positive sign and
    //less than 232 in magnitude such that the mathematical difference of posInt and k is mathematically an integer multiple of 232.
    long long int int32bit = posInt % 0x100000000;
    return int32bit;
}

inline bool ESValue::isObject() const
{
    return isESPointer() && asESPointer()->isESObject();
}

inline double ESValue::toLength() const
{
    return toInteger(); // TODO
}

inline bool ESValue::isPrimitive() const
{
    //return isUndefined() || isNull() || isNumber() || isESString() || isBoolean();
    return !isESPointer() || asESPointer()->isESString();
}

inline size_t ESValue::toIndex() const
{
    size_t idx = SIZE_MAX;
    int32_t i;
    if(isInt32() && (i = asInt32()) >= 0) {
        return i;
    } else {
        ESString* key = toString();
        return key->tryToUseAsIndex();
    }
}

inline double ESValue::toRawDouble(ESValue value)
{
    return bitwise_cast<double>(value.u.asInt64);
}

inline ESValue ESValue::fromRawDouble(double value)
{
    ESValue val;
    val.u.asInt64 = bitwise_cast<uint64_t>(value);
    return val;
}


// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-abstract-equality-comparison
inline bool ESValue::abstractEqualsTo(const ESValue& val)
{
    if(isInt32() && val.isInt32()) {
        if(u.asInt64 == val.u.asInt64)
            return true;
        return false;
    } else if (isNumber() && val.isNumber()) {
        double a = asNumber();
        double b = val.asNumber();

        if (std::isnan(a) || std::isnan(b))
            return false;
        else if(a == b)
            return true;

        return false;
    } else {
        if (isUndefined() && val.isUndefined()) return true;
        if (isNull() && val.isNull()) return true;
        if (isNull() && val.isUndefined()) return true;
        else if (isUndefined() && val.isNull()) return true;
        //If Type(x) is Number and Type(y) is String,
        else if (isNumber() && val.isESString()) {
            //return the result of the comparison x == ToNumber(y).
            return asNumber() == val.toNumber();
        }
        //If Type(x) is String and Type(y) is Number,
        else if (isESString() && val.isNumber()) {
            //return the result of the comparison ToNumber(x) == y.
            return val.asNumber() == toNumber();
        }
        //If Type(x) is Boolean, return the result of the comparison ToNumber(x) == y.
        else if (isBoolean()) {
            //return the result of the comparison ToNumber(x) == y.
            ESValue x(toNumber());
            return x.abstractEqualsTo(val);
        }
        //If Type(y) is Boolean, return the result of the comparison x == ToNumber(y).
        else if (val.isBoolean()) {
            //return the result of the comparison ToNumber(x) == y.
            return abstractEqualsTo(ESValue(val.toNumber()));
        }
        //If Type(x) is either String, Number, or Symbol and Type(y) is Object, then
        else if ((isESString() || isNumber()) && val.isObject()) {
            return abstractEqualsTo(val.toPrimitive());
        }
        //If Type(x) is Object and Type(y) is either String, Number, or Symbol, then
        else if (isObject() && (val.isESString() || val.isNumber())) {
            return toPrimitive().abstractEqualsTo(val);
        }

        if (isESPointer() && val.isESPointer()) {
            ESPointer* o = asESPointer();
            ESPointer* comp = val.asESPointer();

            if(o->isESString() && comp->isESString())
                return *o->asESString() == *comp->asESString();
            return equalsTo(val);
        }
    }
    return false;
}

inline bool ESValue::equalsTo(const ESValue& val)
{
    if(isUndefined())
        return val.isUndefined();

    if(isNull())
        return val.isNull();

    if(isBoolean())
        return val.isBoolean() && asBoolean() == val.asBoolean();

    if(isNumber()) {
        if(!val.isNumber())
            return false;
        double a = asNumber();
        double b = val.asNumber();
        if (std::isnan(a) || std::isnan(b))
            return false;
        //we can pass [If x is +0 and y is −0, return true. If x is −0 and y is +0, return true.]
        //because
        //double a = -0.0;
        //double b = 0.0;
        //a == b; is true
        return a == b;
    }

    if(isESPointer()) {
        ESPointer* o = asESPointer();
        if (!val.isESPointer())
            return false;
        ESPointer* o2 = val.asESPointer();
        if(o->isESString()) {
            if(!o2->isESString())
                return false;
            return o->asESString()->string() == o2->asESString()->string();
        }
        if(o == o2)
            return o == o2;
    }
    return false;
}

//==============================================================================
//===32-bit architecture========================================================
//==============================================================================

#if ESCARGOT_32

inline ESValue::ESValue()
{
    //u.asBits.tag = EmptyValueTag;
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

inline ESValue::ESValue(ESPointer* ptr)
{
    if (ptr)
        u.asBits.tag = CellTag;
    else
        u.asBits.tag = EmptyValueTag;
    u.asBits.payload = reinterpret_cast<int32_t>(ptr);
}

inline ESValue::ESValue(const ESPointer* ptr)
{
    if (ptr)
        u.asBits.tag = CellTag;
    else
        u.asBits.tag = EmptyValueTag;
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
    return u.ptr == other.u.ptr;
}

inline bool ESValue::operator!=(const ESValue& other) const
{
    return u.ptr != other.u.ptr;
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
    return tag() == ESValue::ESTrueTag::ESTrue;
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

inline bool ESValue::isNumber() const
{
    return isInt32() || isDouble();
}

inline bool ESValue::isESPointer() const
{
    return tag() == CellTag;
}

inline bool ESValue::isUndefined() const
{
    return tag() == ESUndefinedTag::ESUndefined;
}

inline bool ESValue::isNull() const
{
    return tag() == ESNullTag::ESNull;
}

inline bool ESValue::isBoolean() const
{
    return tag() == ESValue::ESFalseTag::ESFalse || tag() == ESValue::ESTrueTag::ESTrue;
}

ALWAYS_INLINE JSCell* ESValue::asESPointer() const
{
    ASSERT(isESPointer());
    return reinterpret_cast<ESPointer*>(u.asBits.payload);
}

//==============================================================================
//===64-bit architecture========================================================
//==============================================================================

#else

inline ESValue::ESValue()
{
    //u.asInt64 = ValueEmpty;
    u.asInt64 = ValueUndefined;
}

inline ESValue::ESValue(ESForceUninitializedTag)
{
}

inline ESValue::ESValue(ESNullTag)
{
    u.asInt64 = ValueNull;
}

inline ESValue::ESValue(ESUndefinedTag)
{
    u.asInt64 = ValueUndefined;
}

inline ESValue::ESValue(ESEmptyValueTag)
{
    u.asInt64 = ValueEmpty;
}

inline ESValue::ESValue(ESTrueTag)
{
    u.asInt64 = ValueTrue;
}

inline ESValue::ESValue(ESFalseTag)
{
    u.asInt64 = ValueFalse;
}

inline ESValue::ESValue(bool b)
{
    u.asInt64 = (TagBitTypeOther | TagBitBool | b);
}

inline ESValue::ESValue(ESPointer* ptr)
{
    u.ptr = ptr;
}

inline ESValue::ESValue(const ESPointer* ptr)
{
    u.ptr = const_cast<ESPointer*>(ptr);
}

inline int64_t reinterpretDoubleToInt64(double value)
{
    return bitwise_cast<int64_t>(value);
}
inline double reinterpretInt64ToDouble(int64_t value)
{
    return bitwise_cast<double>(value);
}

ALWAYS_INLINE ESValue::ESValue(EncodeAsDoubleTag, double d)
{
    u.asInt64 = reinterpretDoubleToInt64(d) + DoubleEncodeOffset;
}

inline ESValue::ESValue(int i)
{
    u.asInt64 = TagTypeNumber | static_cast<uint32_t>(i);
}

inline bool ESValue::operator==(const ESValue& other) const
{
    return u.asInt64 == other.u.asInt64;
}

inline bool ESValue::operator!=(const ESValue& other) const
{
    return u.asInt64 != other.u.asInt64;
}

inline bool ESValue::isInt32() const
{
    //return (u.asInt64 & TagTypeNumber) == TagTypeNumber;
    ASSERT(sizeof (short) == 2);
    unsigned short* firstByte = (unsigned short *)&u.asInt64;
#ifdef ESCARGOT_LITTLE_ENDIAN
    return firstByte[3] == 0xffff;
#else
    return *firstByte == 0xffff;
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

inline bool ESValue::isEmpty() const
{
    return u.asInt64 == ValueEmpty;
}

inline bool ESValue::isNumber() const
{
    //return u.asInt64 & TagTypeNumber;
    ASSERT(sizeof (short) == 2);
    unsigned short* firstByte = (unsigned short *)&u.asInt64;
#ifdef ESCARGOT_LITTLE_ENDIAN
    return firstByte[3];
#else
    return firstByte[0];
#endif
}

inline bool ESValue::isESString() const
{
    //CHECK should we consider isESStringObject in this point?
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

ALWAYS_INLINE ESValue ESPropertyAccessorData::value(::escargot::ESObject* obj)
{
    if(m_nativeGetter) {
        ASSERT(!m_jsGetter);
        return m_nativeGetter(obj);
    }
    if(m_jsGetter) {
        ASSERT(!m_nativeGetter);
        return ESFunctionObject::call(ESVMInstance::currentInstance(), m_jsGetter, obj, NULL, 0, false);
    }
    return ESValue();
}

ALWAYS_INLINE void ESPropertyAccessorData::setValue(::escargot::ESObject* obj, const ESValue& value)
{
    if(m_nativeSetter) {
        ASSERT(!m_jsSetter);
        m_nativeSetter(obj, value);
    }
    if(m_jsSetter) {
        ASSERT(!m_nativeSetter);
        ESValue arg[] = {value};
        ESFunctionObject::call(ESVMInstance::currentInstance(), m_jsSetter,
            obj ,arg, 1, false);
    }
}

ALWAYS_INLINE ESHiddenClass* ESHiddenClass::removeProperty(size_t idx)
{
    ASSERT(idx != SIZE_MAX);
    //can not delete __proto__
    ASSERT(idx != 0);
    ASSERT(m_propertyInfo[idx].m_flags.m_isConfigurable);

    if(m_flags.m_isVectorMode) {
        ESHiddenClass* ret = ESVMInstance::currentInstance()->initialHiddenClassForObject();
        ASSERT(*m_propertyInfo[0].m_name == *strings->__proto__.string());
        for(unsigned i = 1 ; i < m_propertyInfo.size() ; i ++) {
            if(idx != i) {
                size_t d;
                ret = ret->defineProperty(m_propertyInfo[i].m_name, m_propertyInfo[i].m_flags.m_isDataProperty
                        , m_propertyInfo[i].m_flags.m_isWritable, m_propertyInfo[i].m_flags.m_isEnumerable, m_propertyInfo[i].m_flags.m_isConfigurable);
            }
        }

        return ret;
    } else {
        m_propertyInfo.erase(m_propertyInfo.begin() + idx);

        auto iter = m_propertyIndexHashMapInfo.begin();
        auto deleteIdx = m_propertyIndexHashMapInfo.end();

        //TODO update m_flags.m_has~~series
        while(iter != m_propertyIndexHashMapInfo.end()) {
            if(iter->second == idx) {
                deleteIdx = iter;
            } else if(iter->second > idx) {
                iter->second = iter->second - 1;
            }
            iter ++;
        }
        ASSERT(deleteIdx != m_propertyIndexHashMapInfo.end());
        m_propertyIndexHashMapInfo.erase(deleteIdx);

        if(m_propertyInfo.size() < ESHiddenClassVectorModeSizeLimit / 2) {
            //convert into vector mode
            ESHiddenClass* ret = ESVMInstance::currentInstance()->initialHiddenClassForObject();
            for(unsigned i = 1 ; i < m_propertyInfo.size() ; i ++) {
                if(idx != i) {
                    size_t d;
                    ret = ret->defineProperty(m_propertyInfo[i].m_name, m_propertyInfo[i].m_flags.m_isDataProperty
                            , m_propertyInfo[i].m_flags.m_isWritable, m_propertyInfo[i].m_flags.m_isEnumerable, m_propertyInfo[i].m_flags.m_isConfigurable);
                }
            }
            return ret;
        }
        return this;
    }
}

ALWAYS_INLINE ESHiddenClass* ESHiddenClass::defineProperty(ESString* name, bool isData, bool isWritable, bool isEnumerable, bool isConfigurable)
{
    ASSERT(findProperty(name) == SIZE_MAX);
    if(m_propertyInfo.size() > ESHiddenClassVectorModeSizeLimit) {
        ESHiddenClass* cls = new ESHiddenClass;
        cls->m_flags.m_isVectorMode = false;
        for(unsigned i = 0 ; i < m_propertyInfo.size() ; i ++) {
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
    if(iter == m_transitionData.end()) {
        ESHiddenClass** vec = new(GC) ESHiddenClass*[16];
        memset(vec, 0, sizeof (ESHiddenClass *) * 16);
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
        if(iter->second[(size_t)flag]) {
            cls = iter->second[(size_t)flag];
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
}

ALWAYS_INLINE ESValue ESHiddenClass::read(ESObject* obj, ESString* name)
{
    return read(obj, findProperty(name));
}

ALWAYS_INLINE ESValue ESHiddenClass::read(ESObject* obj, size_t idx)
{
    if(LIKELY(m_propertyInfo[idx].m_flags.m_isDataProperty)) {
        return obj->m_hiddenClassData[idx];
    } else {
        ESPropertyAccessorData* data = (ESPropertyAccessorData *)obj->m_hiddenClassData[idx].asESPointer();
        return data->value(obj);
    }
}

ALWAYS_INLINE void ESHiddenClass::write(ESObject* obj, ESString* name, const ESValue& val)
{
    write(obj, findProperty(name), val);
}

ALWAYS_INLINE void ESHiddenClass::write(ESObject* obj, size_t idx, const ESValue& val)
{
    if(UNLIKELY(!m_propertyInfo[idx].m_flags.m_isWritable)) {
        return ;
    }
    if(LIKELY(m_propertyInfo[idx].m_flags.m_isDataProperty)) {
        obj->m_hiddenClassData[idx] = val;
    } else {
        ESPropertyAccessorData* data = (ESPropertyAccessorData *)obj->m_hiddenClassData[idx].asESPointer();
        data->setValue(obj, val);
    }
}


inline void ESObject::defineDataProperty(const escargot::ESValue& key, bool isWritable, bool isEnumerable, bool isConfigurable, const ESValue& initalValue)
{
    if(isESArrayObject() && asESArrayObject()->isFastmode()) {
        size_t i = key.toIndex();
        if (i != SIZE_MAX) {
            if(isWritable && isEnumerable && isEnumerable) {
                int len = asESArrayObject()->length();
                if (i == len) {
                    asESArrayObject()->setLength(len+1);
                }
                else if (i >= len) {
                    if (asESArrayObject()->shouldConvertToSlowMode(i))
                        asESArrayObject()->convertToSlowMode();
                    asESArrayObject()->setLength(i+1);
                }
                return ;
            } else {
                asESArrayObject()->convertToSlowMode();
            }
        }
    }
    if(isESTypedArrayObject()) {
        size_t i = key.toIndex();
        if (i != SIZE_MAX) {
            throw ESValue(TypeError::create(ESString::create("cannot redefine property")));
        }
    }

    if(UNLIKELY(m_flags.m_isGlobalObject))
        ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();

    size_t oldIdx = m_hiddenClass->findProperty(key.toString());
    if(oldIdx == SIZE_MAX) {
        m_hiddenClass = m_hiddenClass->defineProperty(key.toString(), true, isWritable, isEnumerable, isConfigurable);
        m_hiddenClassData.push_back(initalValue);
    } else {
        if(!m_hiddenClass->m_propertyInfo[oldIdx].m_flags.m_isConfigurable) {
            throw ESValue(TypeError::create(ESString::create("cannot redefine property")));
        }
        m_hiddenClass = m_hiddenClass->removeProperty(oldIdx);
        m_hiddenClassData.erase(m_hiddenClassData.begin() + oldIdx);
        m_hiddenClass = m_hiddenClass->defineProperty(key.toString(), true, isWritable, isEnumerable, isConfigurable);
        m_hiddenClassData.push_back(initalValue);
    }
}

inline void ESObject::defineAccessorProperty(const escargot::ESValue& key,ESPropertyAccessorData* data, bool isWritable, bool isEnumerable, bool isConfigurable)
{
    if(isESArrayObject() && asESArrayObject()->isFastmode()) {
        size_t i = key.toIndex();
        if (i != SIZE_MAX) {
            asESArrayObject()->convertToSlowMode();
        }
    }
    if(isESTypedArrayObject()) {
        size_t i = key.toIndex();
        if (i != SIZE_MAX) {
            throw ESValue(TypeError::create(ESString::create("cannot redefine property")));
        }
    }

    size_t oldIdx = m_hiddenClass->findProperty(key.toString());

    if(oldIdx == SIZE_MAX) {
        m_hiddenClass = m_hiddenClass->defineProperty(key.toString(), false, isWritable, isEnumerable, isConfigurable);
        m_hiddenClassData.push_back((ESPointer *)data);
    } else {
        if(!m_hiddenClass->m_propertyInfo[oldIdx].m_flags.m_isConfigurable) {
            throw ESValue(TypeError::create(ESString::create("cannot redefine property")));
        }
        m_hiddenClass = m_hiddenClass->removeProperty(oldIdx);
        m_hiddenClassData.erase(m_hiddenClassData.begin() + oldIdx);
        m_hiddenClass = m_hiddenClass->defineProperty(key.toString(), false, isWritable, isEnumerable, isConfigurable);
        m_hiddenClassData.push_back((ESPointer *)data);
    }
}

inline bool ESObject::deletePropety(const ESValue& key)
{
    if(isESArrayObject() && asESArrayObject()->isFastmode()) {
        size_t i = key.toIndex();
        if (i != SIZE_MAX) {
            if(i < asESArrayObject()->length()) {
                asESArrayObject()->m_vector[i] = ESValue(ESValue::ESEmptyValue);
                return true;
            }
            return false;
        }
    }
    if(isESTypedArrayObject()) {
        size_t i = key.toIndex();
        if (i != SIZE_MAX) {
            return false;
        }
    }

    size_t idx = m_hiddenClass->findProperty(key.toString());
    if(idx == SIZE_MAX)
        return false;

    m_hiddenClass = m_hiddenClass->removeProperty(idx);
    if(UNLIKELY(m_flags.m_isGlobalObject))
        ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
    return true;
}

ALWAYS_INLINE bool ESObject::hasOwnProperty(const escargot::ESValue& key)
{
    if((isESArrayObject() && asESArrayObject()->isFastmode()) || isESTypedArrayObject()) {
        size_t idx = key.toIndex();
        if(idx != SIZE_MAX) {
            if(LIKELY(isESArrayObject())) {
                if((int)idx < asESArrayObject()->length() && asESArrayObject()->m_vector[idx] != ESValue(ESValue::ESEmptyValue)) {
                    return true;
                } else {
                    return false;
                }
            } else {
                if((int)idx < asESTypedArrayObjectWrapper()->length()) {
                    return true;
                } else {
                    return false;
                }
            }
        } else {
        }
    }
    return m_hiddenClass->findProperty(key.toString()) != SIZE_MAX;
}

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-get-o-p
ALWAYS_INLINE ESValue ESObject::get(escargot::ESValue key)
{
    ESObject* target = this;
    escargot::ESString* keyString = NULL;
    while(true) {
        if(target->isESArrayObject() && target->asESArrayObject()->isFastmode()) {
            size_t idx = key.toIndex();
            if(idx != SIZE_MAX) {
                if(LIKELY((int)idx < target->asESArrayObject()->length())) {
                    ESValue e = target->asESArrayObject()->m_vector[idx];
                    if(LIKELY(!e.isEmpty()))
                        return e;
                }
            }
        }
        if(target->isESTypedArrayObject()) {
            size_t idx = key.toIndex();
            if(idx != SIZE_MAX) {
                return target->asESTypedArrayObjectWrapper()->get(idx);
            }
        }

        if(!keyString) {
            keyString = key.toString();
        }
        size_t t = target->m_hiddenClass->findProperty(keyString);
        if(t != SIZE_MAX) {
            return target->m_hiddenClass->read(target, t);
        }
        if (target->m___proto__.isESPointer() && target->m___proto__.asESPointer()->isESObject()) {
            target = target->m___proto__.asESPointer()->asESObject();
        } else {
            return ESValue();
        }
    }
}

ALWAYS_INLINE ESValue ESObject::getOwnProperty(escargot::ESValue key)
{
    if(isESArrayObject() && asESArrayObject()->isFastmode()) {
        size_t idx = key.toIndex();
        if(idx != SIZE_MAX) {
            if(LIKELY((int)idx < asESArrayObject()->length())) {
                ESValue e = asESArrayObject()->m_vector[idx];
                if(LIKELY(!e.isEmpty()))
                    return e;
            }
        }
    }
    if(isESTypedArrayObject()) {
        size_t idx = key.toIndex();
        if(idx != SIZE_MAX) {
            return asESTypedArrayObjectWrapper()->get(idx);
        }
    }

    escargot::ESString* keyString = key.toString();
    size_t t = m_hiddenClass->findProperty(keyString);
    if(t != SIZE_MAX) {
        return m_hiddenClass->read(this, t);
    } else {
        return ESValue();
    }
}

ALWAYS_INLINE ESValue* ESObject::addressOfProperty(escargot::ESValue key)
{
    ASSERT(m_flags.m_isGlobalObject);
    size_t ret = m_hiddenClass->findProperty(key.toString());
    if(ret == SIZE_MAX)
        return NULL;
    ASSERT(m_hiddenClass->m_propertyInfo[ret].m_flags.m_isDataProperty);
    return &m_hiddenClassData[ret];
}

ALWAYS_INLINE const int32_t ESObject::length()
{
    if (LIKELY(isESArrayObject()))
        return asESArrayObject()->length();
    else
        return get(strings->length.string()).toInteger();
}
ALWAYS_INLINE ESValue ESObject::pop()
{
    int len = length();
    if (len == 0) return ESValue();
    if (LIKELY(isESArrayObject() && asESArrayObject()->isFastmode()))
        return asESArrayObject()->fastPop();
    else {
        ESValue ret = get(ESValue(len-1));
        deletePropety(ESValue(len-1));
        set(strings->length.string(), ESValue(len - 1));
        return ret;
    }
}
ALWAYS_INLINE void ESObject::eraseValues(int idx, int cnt)
{
    if (LIKELY(isESArrayObject()))
        asESArrayObject()->eraseValues(idx, cnt);
    else {
        for (int k = 0, i = idx; i < length() && k < cnt; i++, k++) {
            set(ESValue(i), get(ESValue(i+cnt)));
        }
        //NOTE: length is set in Array.splice
    }
}

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-set-o-p-v-throw
ALWAYS_INLINE bool ESObject::set(const escargot::ESValue& key, const ESValue& val)
{
    if(isESArrayObject() && asESArrayObject()->isFastmode()) {
        size_t idx = key.toIndex();
        if(idx != SIZE_MAX) {
            if(idx >= asESArrayObject()->length()) {
                if (asESArrayObject()->shouldConvertToSlowMode(idx)) {
                    asESArrayObject()->convertToSlowMode();
                    asESArrayObject()->setLength(idx + 1);
                    goto array_fastmode_fail;
                } else {
                    asESArrayObject()->setLength(idx + 1);
                }
            }

            if(LIKELY(!asESArrayObject()->m_vector[idx].isEmpty())) {
                asESArrayObject()->m_vector[idx] = val;
                return true;
            } else {
                //if hole, check prototypes.
                ESValue target = m___proto__;
                while(true) {
                    if(!target.isObject()) {
                        break;
                    }
                    if(target.asESPointer()->asESObject()->hiddenClass()->hasIndexedReadOnlyProperty()) {
                        size_t t = target.asESPointer()->asESObject()->hiddenClass()->findProperty(key.toString());
                        if(t != SIZE_MAX) {
                            if(!target.asESPointer()->asESObject()->hiddenClass()->m_propertyInfo[t].m_flags.m_isWritable)
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
    if(isESTypedArrayObject()) {
        size_t idx = key.toIndex();
        if(idx != SIZE_MAX) {
            if(idx < asESTypedArrayObjectWrapper()->length())
                asESTypedArrayObjectWrapper()->set(idx, val);
            return true;
        }
    }
    array_fastmode_fail:
    escargot::ESString* keyString = key.toString();
    size_t idx = m_hiddenClass->findProperty(keyString);
    if(idx == SIZE_MAX) {
        ESValue target = m___proto__;
        while(true) {
            if(!target.isObject()) {
                break;
            }
            if(target.asESPointer()->asESObject()->hiddenClass()->hasReadOnlyProperty()) {
                size_t t = target.asESPointer()->asESObject()->hiddenClass()->findProperty(key.toString());
                if(t != SIZE_MAX) {
                    if(!target.asESPointer()->asESObject()->hiddenClass()->m_propertyInfo[t].m_flags.m_isWritable)
                        return false;
                }
            }
            target = target.asESPointer()->asESObject()->__proto__();
        }
        m_hiddenClass = m_hiddenClass->defineProperty(keyString, true, true, true, true);
        m_hiddenClassData.push_back(val);

        if(UNLIKELY(m_flags.m_isGlobalObject))
                ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
    } else {
        m_hiddenClass->write(this, keyString, val);
    }

    return true;
}

ALWAYS_INLINE void ESObject::set(const escargot::ESValue& key, const ESValue& val, bool throwExpetion)
{
    if(!set(key, val) && throwExpetion) {
        throw ESValue(TypeError::create());
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
    siz += m_hiddenClassData.size();
    return siz;
}

template <typename Functor>
ALWAYS_INLINE void ESObject::enumeration(Functor t)
{
    if (isESArrayObject() && asESArrayObject()->isFastmode()) {
        for (int i = 0; i < asESArrayObject()->length(); i++) {
            if(asESArrayObject()->m_vector[i].isEmpty())
                continue;
            t(ESValue(i));
        }
    }

    if (isESTypedArrayObject()) {
        for (int i = 0; i < asESTypedArrayObjectWrapper()->length(); i++) {
            t(ESValue(i));
        }
    }

    auto iter = m_hiddenClass->m_propertyInfo.begin();
    while(iter != m_hiddenClass->m_propertyInfo.end()) {
        if(iter->m_flags.m_isEnumerable) {
            t(ESValue(iter->m_name));
        }
        iter++;
    }
}

template<>
inline ESTypedArrayObject<Int8Adaptor>::ESTypedArrayObject(TypedArrayType arraytype,
                   ESPointer::Type type)
       : ESTypedArrayObjectWrapper(arraytype,
                                   (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->int8ArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Int16Adaptor>::ESTypedArrayObject(TypedArrayType arraytype,
                   ESPointer::Type type)
       : ESTypedArrayObjectWrapper(arraytype,
                                   (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->int16ArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Int32Adaptor>::ESTypedArrayObject(TypedArrayType arraytype,
                   ESPointer::Type type)
       : ESTypedArrayObjectWrapper(arraytype,
                                   (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->int32ArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Uint8Adaptor>::ESTypedArrayObject(TypedArrayType arraytype,
                   ESPointer::Type type)
       : ESTypedArrayObjectWrapper(arraytype,
                                   (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->uint8ArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Uint16Adaptor>::ESTypedArrayObject(TypedArrayType arraytype,
                   ESPointer::Type type)
       : ESTypedArrayObjectWrapper(arraytype,
                                   (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->uint16ArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Uint32Adaptor>::ESTypedArrayObject(TypedArrayType arraytype,
                   ESPointer::Type type)
       : ESTypedArrayObjectWrapper(arraytype,
                                   (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->uint32ArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Uint8ClampedAdaptor>::ESTypedArrayObject(TypedArrayType arraytype,
                   ESPointer::Type type)
       : ESTypedArrayObjectWrapper(arraytype,
                                   (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->uint8ClampedArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Float32Adaptor>::ESTypedArrayObject(TypedArrayType arraytype,
                   ESPointer::Type type)
       : ESTypedArrayObjectWrapper(arraytype,
                                   (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->float32ArrayPrototype())
{
}

template<>
inline ESTypedArrayObject<Float64Adaptor>::ESTypedArrayObject(TypedArrayType arraytype,
                   ESPointer::Type type)
       : ESTypedArrayObjectWrapper(arraytype,
                                   (Type)(Type::ESObject | Type::ESTypedArrayObject), ESVMInstance::currentInstance()->globalObject()->float64ArrayPrototype())
{
}

}

#endif
