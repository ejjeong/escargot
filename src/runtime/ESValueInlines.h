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
            return strings->nonAtomicNumbers[num];
        return ESString::create(num);
    } else if(isNumber()) {
        double d = asNumber();
        if (std::isnan(d))
            return strings->NaN;
        if (std::isinf(d)) {
            if(std::signbit(d))
                return strings->NegativeInfinity;
            else
                return strings->Infinity;
        }
        //convert -0.0 into 0.0
        //in c++, d = -0.0, d == 0.0 is true
        if (d == 0.0)
            d = 0;

        return ESString::create(d);
    } else if(isUndefined()) {
        return strings->undefined;
    } else if(isNull()) {
        return strings->null;
    } else if(isBoolean()) {
        if(asBoolean())
            return strings->stringTrue;
        else
            return strings->stringFalse;
    } else {
        return toPrimitive(PreferString).toString();
    }
}

inline ESObject* ESValue::toObject() const
{
    ESFunctionObject* function;
    ESObject* receiver;
    if (isESPointer() && asESPointer()->isESObject()) {
       return asESPointer()->asESObject();
    } else if (isNumber()) {
        function = ESVMInstance::currentInstance()->globalObject()->number();
        receiver = ESNumberObject::create(toNumber());
    } else if (isBoolean()) {
        function = ESVMInstance::currentInstance()->globalObject()->boolean();
        ESValue ret;
        if (toBoolean())
            ret = ESValue(ESValue::ESTrueTag::ESTrue);
        else
            ret = ESValue(ESValue::ESFalseTag::ESFalse);
        receiver = ESBooleanObject::create(ret);
    } else if (isESString()) {
        function = ESVMInstance::currentInstance()->globalObject()->string();
        receiver = ESStringObject::create(asESPointer()->asESString());
    } else if(isNull()){
        throw ESValue(TypeError::create(ESString::create(u"cannot convert null into object")));
    } else if(isUndefined()){
        throw ESValue(TypeError::create(ESString::create(u"cannot convert undefined into object")));
    } else {
        RELEASE_ASSERT_NOT_REACHED();
    }
    receiver->setConstructor(function);
    receiver->set__proto__(function->protoType());
    return receiver;
}

inline ESObject* ESValue::toFunctionReceiverObject() const
{
    if(isUndefinedOrNull()) {
        return ESVMInstance::currentInstance()->globalObject();
    } else {
        return toObject();
    }
}


inline ESValue ESValue::toPrimitive(PrimitiveTypeHint preferredType) const
{
    if (UNLIKELY(!isPrimitive())) {
        ESObject* obj = asESPointer()->asESObject();
        if (preferredType == PrimitiveTypeHint::PreferString) {
            ESValue toString = obj->get(ESValue(strings->toString), true);
            if(toString.isESPointer() && toString.asESPointer()->isESFunctionObject()) {
                ESValue str = ESFunctionObject::call(ESVMInstance::currentInstance(), toString, obj, NULL, 0, false);
                if(str.isPrimitive())
                    return str;
            }

            ESValue valueOf = obj->get(ESValue(strings->valueOf), true);
            if(valueOf.isESPointer() && valueOf.asESPointer()->isESFunctionObject()) {
                ESValue val = ESFunctionObject::call(ESVMInstance::currentInstance(), valueOf, obj, NULL, 0, false);
                if(val.isPrimitive())
                    return val;
            }
        } else { // preferNumber
            ESValue valueOf = obj->get(ESValue(strings->valueOf), true);
            if(valueOf.isESPointer() && valueOf.asESPointer()->isESFunctionObject()) {
                ESValue val = ESFunctionObject::call(ESVMInstance::currentInstance(), valueOf, obj, NULL, 0, false);
                if(val.isPrimitive())
                    return val;
            }

            ESValue toString = obj->get(ESValue(strings->toString), true);
            if(toString.isESPointer() && toString.asESPointer()->isESFunctionObject()) {
                ESValue str = ESFunctionObject::call(ESVMInstance::currentInstance(), toString, obj, NULL, 0, false);
                if(str.isPrimitive())
                    return str;
            }
        }
        throw ESValue(TypeError::create(strings->emptyESString));
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
                data = o->asESStringObject()->getStringData();
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

inline void ESSlot::setValue(const ::escargot::ESValue& value, ::escargot::ESObject* object)
{
    if(UNLIKELY(!m_flags.m_isWritable)) {
        return ;
    }
    if(LIKELY(m_flags.m_isDataProperty)) {
            m_data = value;
    } else {
        ESAccessorData* data = (ESAccessorData*)m_data.asESPointer();
        data->setValue(object, value);
    }
}

inline ESValue ESSlot::value(::escargot::ESObject* object) const
{
    if(LIKELY(m_flags.m_isDataProperty)) {
        return m_data;
    } else {
        ESAccessorData* data = (ESAccessorData*)m_data.asESPointer();
        return data->value(object);
    }
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

ALWAYS_INLINE ESValue ESAccessorData::value(::escargot::ESObject* obj)
{
    if(m_getter) {
        ASSERT(!m_jsGetter);
        return m_getter(obj);
    }
    if(m_jsGetter) {
        ASSERT(!m_getter);
        return ESFunctionObject::call(ESVMInstance::currentInstance(), m_jsGetter, obj, NULL, 0, false);
    }
    return ESValue();
}

ALWAYS_INLINE void ESAccessorData::setValue(::escargot::ESObject* obj, const ESValue& value)
{
    if(m_setter) {
        ASSERT(!m_jsSetter);
        m_setter(obj, value);
    }
    if(m_jsSetter) {
        ASSERT(!m_setter);
        ESValue arg[] = {value};
        ESFunctionObject::call(ESVMInstance::currentInstance(), m_jsSetter,
            obj ,arg, 1, false);
    }
}

ALWAYS_INLINE size_t ESHiddenClass::defineProperty(ESObject* obj, ESString* name, bool isData, bool isWritable, bool isEnumerable, bool isConfigurable)
{
    size_t idx = findProperty(name);
    if(idx == SIZE_MAX) {
        ESHiddenClass* cls;
        auto iter = m_transitionData.find(name);
        size_t pid = m_propertyInfo.size();
        char flag = assembleHidenClassPropertyInfoFlags(isData, isWritable, isEnumerable, isConfigurable);
        if(iter == m_transitionData.end()) {
            ESHiddenClass** vec = new(GC) ESHiddenClass*[16];
            memset(vec, 0, sizeof (ESHiddenClass *) * 16);
            cls = new ESHiddenClass;
            cls->m_propertyInfo.insert(m_propertyInfo.begin(), m_propertyInfo.end());
            cls->m_propertyInfo.insert(std::make_pair(name, pid));
            cls->m_propertyFlagInfo.assign(m_propertyFlagInfo.begin(), m_propertyFlagInfo.end());
            cls->m_propertyFlagInfo.push_back(ESHiddenClassPropertyInfo(isData, isWritable, isEnumerable, isConfigurable));

            m_transitionData.insert(std::make_pair(name, vec));

            vec[(size_t)flag] = cls;
        } else {
            if(iter->second[(size_t)flag]) {
                cls = iter->second[(size_t)flag];
            } else {
                cls = new ESHiddenClass;
                cls->m_propertyInfo.insert(m_propertyInfo.begin(), m_propertyInfo.end());
                cls->m_propertyInfo.insert(std::make_pair(name, pid));
                cls->m_propertyFlagInfo.assign(m_propertyFlagInfo.begin(), m_propertyFlagInfo.end());
                cls->m_propertyFlagInfo.push_back(ESHiddenClassPropertyInfo(isData, isWritable, isEnumerable, isConfigurable));

                iter->second[(size_t)flag] = cls;
            }
        }
        obj->m_hiddenClass = cls;
        ASSERT(obj->m_hiddenClassData.size() == pid);
        //obj->m_hiddenClassData.push_back(ESValue());
        obj->m_hiddenClassData.resize(pid+1);
        ASSERT(cls->m_propertyFlagInfo.size() == obj->m_hiddenClassData.size());
        ASSERT(cls->m_propertyFlagInfo.size() == cls->m_propertyInfo.size());
        ASSERT(cls->m_propertyFlagInfo.size() - 1 == pid);
        return pid;
    } else {
        return idx;
    }
}

ALWAYS_INLINE ESValue ESHiddenClass::readWithoutCheck(ESObject* obj, size_t idx)
{
    if(LIKELY(m_propertyFlagInfo[idx].m_isDataProperty)) {
        return obj->m_hiddenClassData[idx];
    } else {
        ESAccessorData* data = (ESAccessorData *)obj->m_hiddenClassData[idx].asESPointer();
        return data->value(obj);
    }
}

ALWAYS_INLINE ESValue ESHiddenClass::read(ESObject* obj, ESString* name)
{
    return read(obj, findProperty(name));
}

ALWAYS_INLINE ESValue ESHiddenClass::read(ESObject* obj, size_t idx)
{
    if(idx < m_propertyFlagInfo.size()) {
        if(LIKELY(m_propertyFlagInfo[idx].m_isDataProperty)) {
            return obj->m_hiddenClassData[idx];
        } else {
            ESAccessorData* data = (ESAccessorData *)obj->m_hiddenClassData[idx].asESPointer();
            return data->value(obj);
        }
    } else {
        return ESValue();
    }
}


inline void ESObject::definePropertyOrThrow(escargot::ESValue key, bool isWritable, bool isEnumerable, bool isConfigurable, const ESValue& initalValue)
{
    if(isESArrayObject()) {
        int i;
        if (key.isInt32()) {
            i = key.asInt32();
            int len = asESArrayObject()->length();
            if (i == len && asESArrayObject()->isFastmode()) {
                asESArrayObject()->setLength(len+1);
            }
            else if (i >= len) {
                if (asESArrayObject()->shouldConvertToSlowMode(i))
                    asESArrayObject()->convertToSlowMode();
                asESArrayObject()->setLength(i+1);
            }
        }
    }

    if(UNLIKELY(m_map != NULL)){
        escargot::ESString* str = key.toString();
        auto iter = m_map->find(str);
        if(iter != m_map->end()) {
            m_map->erase(iter);
        }
        m_flags.m_propertyIndex++;
        m_map->insert(std::make_pair(str, ::escargot::ESSlot(initalValue, m_flags.m_propertyIndex, isWritable, isEnumerable, isConfigurable)));
    } else {
        size_t ret = m_hiddenClass->defineProperty(this, key.toString(), true, isWritable, isEnumerable, isConfigurable);
        if((ret != SIZE_MAX) && (
                (isWritable != m_hiddenClass->m_propertyFlagInfo[ret].m_isWritable)
                || (isConfigurable != m_hiddenClass->m_propertyFlagInfo[ret].m_isConfigurable)
                || (isEnumerable != m_hiddenClass->m_propertyFlagInfo[ret].m_isEnumerable)
                || (true != m_hiddenClass->m_propertyFlagInfo[ret].m_isDataProperty))
                ) {
            convertIntoMapMode();
            definePropertyOrThrow(key, isWritable, isEnumerable, isConfigurable, initalValue);
            return ;
        }

        if(UNLIKELY(m_hiddenClass->m_propertyInfo.size() >= ESHiddenClass::ESHiddenClassSizeLimit)) {
            convertIntoMapMode();
            definePropertyOrThrow(key, isWritable, isEnumerable, isConfigurable, initalValue);
            return ;
        }
        ASSERT(m_hiddenClass->m_propertyFlagInfo[ret].m_isDataProperty);
        m_hiddenClassData[ret] = initalValue;
    }
}

inline bool ESObject::deletePropety(const ESValue& key)
{
    if(isESArrayObject()) {
        if (key.isInt32()) {
            int i = key.toInt32();
            if(i < asESArrayObject()->length()) {
                asESArrayObject()->m_vector[i] = ESValue(ESValue::ESEmptyValue);
                return true;
            }
        }
    }

    if(m_hiddenClass) {
        //TODO
        convertIntoMapMode();
    }

    ASSERT(m_map);
    escargot::ESString* str = key.toString();
    auto iter = m_map->find(str);
    if(iter != m_map->end() && iter->second.isConfigurable()) {
        m_map->erase(iter);
        return true;
    }
    return false;
}

inline void ESObject::propertyFlags(const ESValue& key, bool& exists, bool& isDataProperty, bool& isWritable, bool& isEnumerable, bool& isConfigurable)
{
    if (isESArrayObject() && asESArrayObject()->isFastmode() && key.isInt32()) {
        //TODO
        RELEASE_ASSERT_NOT_REACHED();
    }
    if(UNLIKELY(m_map != NULL)){
        auto iter = m_map->find(key.toString());
        if(iter == m_map->end()) {
            exists = false;
            return ;
        }
        exists = true;
        isDataProperty = iter->second.isDataProperty();
        isWritable = iter->second.isWritable();
        isConfigurable = iter->second.isConfigurable();
        isEnumerable = iter->second.isEnumerable();
    } else {
        size_t idx = m_hiddenClass->findProperty(key.toString());
        if(idx == SIZE_MAX) {
            if (__proto__().isUndefined()) {
                exists = false;
                return;
            }
            ESObject* proto = __proto__().asESPointer()->asESObject();
            proto->propertyFlags(key, exists, isDataProperty, isWritable, isEnumerable, isConfigurable);
        } else {
            exists = true;
            isDataProperty = m_hiddenClass->m_propertyFlagInfo[idx].m_isDataProperty;
            isWritable = m_hiddenClass->m_propertyFlagInfo[idx].m_isWritable;
            isConfigurable = m_hiddenClass->m_propertyFlagInfo[idx].m_isConfigurable;
            isEnumerable = m_hiddenClass->m_propertyFlagInfo[idx].m_isEnumerable;
        }
    }
}

inline ESAccessorData* ESObject::accessorData(const ESValue& key)
{
    if (isESArrayObject() && asESArrayObject()->isFastmode() && key.isInt32()) {
        RELEASE_ASSERT_NOT_REACHED();
    }
    if(UNLIKELY(m_map != NULL)){
        auto iter = m_map->find(key.toString());
        ASSERT(iter != m_map->end());
        ASSERT(!iter->second.isDataProperty());
        return iter->second.accessorData();
    } else {
        size_t idx = m_hiddenClass->findProperty(key.toString());
        ASSERT(idx != SIZE_MAX);
        ASSERT(!m_hiddenClass->m_propertyFlagInfo[idx].m_isDataProperty);
        ESAccessorData* data = ((ESAccessorData *)m_hiddenClassData[idx].asESPointer());
        return data;
    }
}


ALWAYS_INLINE bool ESObject::hasOwnProperty(escargot::ESValue key)
{
    if((isESArrayObject() && asESArrayObject()->isFastmode()) || isESTypedArrayObject()) {
        size_t idx = SIZE_MAX;
        if(key.isInt32() && key.asInt32() >= 0) {
            idx = key.asInt32();
        } else {
            key = key.toString();
            idx = key.asESString()->tryToUseAsIndex();
        }
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
    if(UNLIKELY(m_map != NULL)){
        auto iter = m_map->find(key.toString());
        if(iter == m_map->end())
            return false;
        return true;
    } else {
        return m_hiddenClass->findProperty(key.toString()) != SIZE_MAX;
    }
}

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-get-o-p
ALWAYS_INLINE ESValue ESObject::get(escargot::ESValue key, bool searchPrototype)
{
    if ((isESArrayObject() && asESArrayObject()->isFastmode()) || isESTypedArrayObject()) {
        if(key.isInt32()) {
            int idx = key.asInt32();
            if(LIKELY(idx >= 0)) {
                if(UNLIKELY(isESTypedArrayObject())) {
                    return asESTypedArrayObjectWrapper()->get(idx);
                } else if(LIKELY(idx < asESArrayObject()->length())) {
                    ESValue e = asESArrayObject()->m_vector[idx];
                    if(UNLIKELY(e == ESValue(ESValue::ESEmptyValue)))
                        return ESValue();
                    else
                        return e;
                }
            }
        } else {
            escargot::ESString* str = key.toString();
            size_t number = str->tryToUseAsIndex();
            if(number != SIZE_MAX) {
                if(LIKELY(isESArrayObject())) {
                    if(LIKELY((int)number < asESArrayObject()->length())) {
                        ESValue e = asESArrayObject()->m_vector[number];
                        if(UNLIKELY(e.isEmpty()))
                            return ESValue();
                        else
                            return e;
                    }
                } else {
                    return asESTypedArrayObjectWrapper()->get(number);
                }
            }
            key = str;
        }
    }

    ESValue v = find(key, searchPrototype, this);
    if(v.isEmpty())
        return ESValue();
    return v;
}

ALWAYS_INLINE ESValue* ESObject::addressOfProperty(escargot::ESValue key)
{
    if(isESArrayObject()) {
        int i;
        if (key.isInt32()) {
            i = key.asInt32();
            int len = asESArrayObject()->length();
            if (asESArrayObject()->isFastmode() && i >= 0 && i < len)
                return &asESArrayObject()->m_vector[i];
            else {
                //TODO
                RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }

    if(UNLIKELY(m_map != NULL)){
        escargot::ESString* str = key.toString();
        auto iter = m_map->find(str);
        if(iter == m_map->end()) {
            return NULL;
        }
        ASSERT(iter->second.isDataProperty());
        return iter->second.data();
    } else {
        size_t ret = m_hiddenClass->findProperty(key.toString());
        if(ret == SIZE_MAX)
            return NULL;
        ASSERT(m_hiddenClass->m_propertyFlagInfo[ret].m_isDataProperty);
        if(m_hiddenClass->m_propertyFlagInfo[ret].m_isDataProperty) {
            return &m_hiddenClassData[ret];
        } else {
            return NULL;
        }
    }
}

ALWAYS_INLINE const int32_t ESObject::length()
{
    if (LIKELY(isESArrayObject()))
        return asESArrayObject()->length();
    else
        return get(strings->length, true).toInteger();
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
        set(strings->length, ESValue(len - 1));
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

inline escargot::ESValue ESObject::find(const escargot::ESValue& key, bool searchPrototype, escargot::ESObject* realObj)
{
    if(realObj == nullptr)
        realObj = this;

    if (isESArrayObject() && asESArrayObject()->isFastmode() && key.isInt32()) {
        int idx = key.asInt32();
        if(LIKELY(idx >= 0 && idx < asESArrayObject()->length()))
            return asESArrayObject()->m_vector[idx];
    }
    if(UNLIKELY(m_map != NULL)){
        escargot::ESString* str = key.toString();
        auto iter = m_map->find(str);
        if(iter == m_map->end()) {
            if(searchPrototype) {
                if(UNLIKELY(this->m___proto__.isUndefined())) {
                    return ESValue(ESValue::ESEmptyValue);
                }
                ESObject* target = this->m___proto__.asESPointer()->asESObject();
                while(true) {
                    escargot::ESValue s = target->find(str, false, realObj);
                    if (!s.isEmpty()) {
                        return s;
                    }
                    ESValue proto = target->__proto__();
                    if (proto.isESPointer() && proto.asESPointer()->isESObject()) {
                        target = proto.asESPointer()->asESObject();
                    } else {
                        return ESValue(ESValue::ESEmptyValue);
                    }
                }
            } else
                return ESValue(ESValue::ESEmptyValue);
        }
        return iter->second.value(this);
    } else {
        escargot::ESString* str = key.toString();
        size_t idx = m_hiddenClass->findProperty(str);
        if(idx == SIZE_MAX) {
            if(searchPrototype) {
                if(UNLIKELY(this->m___proto__.isUndefined())) {
                    return ESValue(ESValue::ESEmptyValue);
                }
                ESObject* target = this->m___proto__.asESPointer()->asESObject();
                while(true) {
                    escargot::ESValue s = target->find(str, false, realObj);
                    if (!s.isEmpty()) {
                        return s;
                    }
                    ESValue proto = target->__proto__();
                    if (proto.isESPointer() && proto.asESPointer()->isESObject()) {
                        target = proto.asESPointer()->asESObject();
                    } else {
                        return ESValue(ESValue::ESEmptyValue);
                    }
                }
            } else {
                return ESValue(ESValue::ESEmptyValue);
            }
        } else {
            if(LIKELY(m_hiddenClass->m_propertyFlagInfo[idx].m_isDataProperty))
                return m_hiddenClassData[idx];
            else {
                ESAccessorData* data = ((ESAccessorData *)m_hiddenClassData[idx].asESPointer());
                return data->value(realObj);
            }
        }
    }
}

//NOTE rooted ESSlot has short life time.
ALWAYS_INLINE escargot::ESValue ESObject::findOnlyPrototype(const escargot::ESValue& key)
{
    if(UNLIKELY(this->m___proto__.isUndefined())) {
        return ESValue(ESValue::ESEmptyValue);
    }
    ESObject* target = this->m___proto__.asESPointer()->asESObject();
    while(true) {
        escargot::ESValue s = target->find(key, false, this);
        if (!s.isEmpty()) {
            return s;
        }
        ESValue proto = target->__proto__();
        if (proto.isESPointer() && proto.asESPointer()->isESObject()) {
            target = proto.asESPointer()->asESObject();
        } else {
            break;
        }
    }

    return ESValue(ESValue::ESEmptyValue);
}

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-set-o-p-v-throw
ALWAYS_INLINE void ESObject::set(escargot::ESValue key, const ESValue& val, bool shouldThrowException)
{
    if ((isESArrayObject() && asESArrayObject()->isFastmode()) || isESTypedArrayObject()) {
        if(key.isInt32()) {
            int idx = key.asInt32();
            if(UNLIKELY(isESTypedArrayObject())) {
                asESTypedArrayObjectWrapper()->set(idx, val);
                return ;
            } else if(idx >= 0) {
                int len = asESArrayObject()->length();
                if (idx < len) {
                    asESArrayObject()->m_vector[idx] = val;
                    return ;
                } else {
                    if (asESArrayObject()->shouldConvertToSlowMode(idx)) {
                        asESArrayObject()->convertToSlowMode();
                        asESArrayObject()->setLength(idx + 1);
                    } else {
                        asESArrayObject()->setLength(idx + 1);
                        asESArrayObject()->m_vector[idx] = val;
                        return ;
                    }
                }
            }

        } else {
            escargot::ESString* str = key.toString();
            size_t number = str->tryToUseAsIndex();
            if(number != SIZE_MAX) {
                if(LIKELY(isESArrayObject())) {
                    if(LIKELY((int)number < asESArrayObject()->length())) {
                        asESArrayObject()->m_vector[number] = val;
                        return ;
                    }
                } else {
                    asESTypedArrayObjectWrapper()->set(number, val);
                    return ;
                }
            }
            key = str;
        }
    }

    if(UNLIKELY(m_map != NULL)){
        //TODO Assert: IsPropertyKey(P) is true.
        //TODO Assert: Type(Throw) is ESBoolean.
        //TODO shouldThrowException
        escargot::ESString* str = key.toString();
        auto iter = m_map->find(str);
        if(iter == m_map->end()) {
            //TODO set flags
            m_map->insert(std::make_pair(str, escargot::ESSlot(val, m_flags.m_propertyIndex, true, true, true)));
        } else {
            if(LIKELY(iter->second.isWritable()))
                iter->second.setValue(val, this);
        }
    } else {
        escargot::ESString* keyStr = key.toString();
        size_t ret = m_hiddenClass->findProperty(keyStr);
        if(ret == SIZE_MAX) {
            //define
            if(UNLIKELY(m_hiddenClass->m_propertyInfo.size() >= ESHiddenClass::ESHiddenClassSizeLimit)) {
                convertIntoMapMode();
                auto iter = m_map->find(keyStr);
                ASSERT(iter == m_map->end());
                m_map->insert(std::make_pair(keyStr, escargot::ESSlot(val, m_flags.m_propertyIndex, true, true, true)));
                return ;
            } else {
                ret = m_hiddenClass->defineProperty(this, keyStr, true, true, true, true);
            }
        }

        if(LIKELY(m_hiddenClass->m_propertyFlagInfo[ret].m_isWritable)) {
            if(LIKELY(m_hiddenClass->m_propertyFlagInfo[ret].m_isDataProperty)) {
                m_hiddenClassData[ret] = val;
            } else {
                ESAccessorData * d = (ESAccessorData *)m_hiddenClassData[ret].asESPointer();
                d->setValue(this, val);
            }
        }
    }
}

ALWAYS_INLINE size_t ESObject::keyCount()
{
    size_t siz = 0;
    if (isESArrayObject() && asESArrayObject()->isFastmode()) {
        siz += asESArrayObject()->length();
    }
    if(m_map)
        siz += m_map->size();
    else
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

    std::vector<std::pair<escargot::ESString*, size_t> > keys;

    if(UNLIKELY(m_map != NULL)){
        auto iter = m_map->begin();
        while(iter != m_map->end()) {
            if(iter->second.isEnumerable()) {
                keys.push_back(std::make_pair(iter->first, iter->second.propertyIndex()));
            }
            iter++;
        }
    } else {
        auto iter = m_hiddenClass->m_propertyInfo.begin();
        while(iter != m_hiddenClass->m_propertyInfo.end()) {
            size_t idx = iter->second;
            if(m_hiddenClass->m_propertyFlagInfo[idx].m_isEnumerable) {
                keys.push_back(std::make_pair(iter->first, idx));
            }
            iter++;
        }
    }

    std::sort(keys.begin(), keys.end(), [](const std::pair<escargot::ESString*, size_t>& a, const std::pair<escargot::ESString*, size_t>& b) -> bool {
        return a.second < b.second;
    });

    for(size_t i = 0; i < keys.size() ; i ++) {
        t(keys[i].first);
    }
}

ALWAYS_INLINE ESValue ESObject::readHiddenClass(size_t idx)
{
    ASSERT(!m_map);
    if(LIKELY(m_hiddenClass->m_propertyFlagInfo[idx].m_isDataProperty))
        return m_hiddenClassData[idx];
    else {
        return ((ESAccessorData *)m_hiddenClassData[idx].asESPointer())->value(this);
    }
}

ALWAYS_INLINE void ESObject::writeHiddenClass(size_t idx, const ESValue& value)
{
    ASSERT(!m_map);
    if(LIKELY(m_hiddenClass->m_propertyFlagInfo[idx].m_isWritable)) {
        if(LIKELY(m_hiddenClass->m_propertyFlagInfo[idx].m_isDataProperty))
            m_hiddenClassData[idx] = value;
        else {
            ((ESAccessorData *)m_hiddenClassData[idx].asESPointer())->setValue(this, value);
        }
    }
}

ALWAYS_INLINE void ESObject::appendHiddenClassItem(escargot::ESString* keyStr, const ESValue& value)
{
    ASSERT(!m_map);
    ASSERT(m_hiddenClass->findProperty(keyStr) == SIZE_MAX);
    if(UNLIKELY(m_hiddenClass->m_propertyInfo.size() >= ESHiddenClass::ESHiddenClassSizeLimit)) {
        convertIntoMapMode();
        auto iter = m_map->find(keyStr);
        ASSERT(iter == m_map->end());
        m_map->insert(std::make_pair(keyStr, escargot::ESSlot(value, m_flags.m_propertyIndex, true, true, true)));
    } else {
        bool exists, isDataProperty, isWritable, isEnumerable, isConfigurable;
        propertyFlags(keyStr, exists, isDataProperty, isWritable, isEnumerable, isConfigurable);

        if (exists && isWritable) {
            size_t ret = m_hiddenClass->defineProperty(this, keyStr, true, isWritable, isEnumerable, isConfigurable);
            m_hiddenClassData[ret] = value;
        } else if (!exists) {
            size_t ret = m_hiddenClass->defineProperty(this, keyStr, true, true, true, true);
            m_hiddenClassData[ret] = value;
        }
    }
}


}

#endif
