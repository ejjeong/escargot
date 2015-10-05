#ifndef ESValueInlines_h
#define ESValueInlines_h

namespace escargot {

/*
const int kApiPointerSize = sizeof(void*);
const int kApiIntSize = sizeof(int);
const int kApiInt64Size = sizeof(int64_t);

// Tag information for HeapObject.
const int kHeapObjectTag = 0;
const int kHeapObjectTagSize = 2;
const intptr_t kHeapObjectTagMask = (1 << kHeapObjectTagSize) - 1;

// Tag information for Smi.
const int kSmiTag = 1;
const int kSmiTagSize = 1;
const intptr_t kSmiTagMask = (1 << kSmiTagSize) - 1;

template <size_t ptr_size> struct SmiTagging;

template<int kSmiShiftSize>
ALWAYS_INLINE ESValue* IntToSmiT(int value) {
    int smi_shift_bits = kSmiTagSize + kSmiShiftSize;
    uintptr_t tagged_value =
        (static_cast<uintptr_t>(value) << smi_shift_bits) | kSmiTag;
    return reinterpret_cast<ESValue*>(tagged_value);
}

// Smi constants for 32-bit systems.
template <> struct SmiTagging<4> {
  enum { kSmiShiftSize = 0, kSmiValueSize = 31 };
  static int SmiShiftSize() { return kSmiShiftSize; }
  static int SmiValueSize() { return kSmiValueSize; }
  ALWAYS_INLINE static int SmiToInt(const ESValue* value) {
    int shift_bits = kSmiTagSize + kSmiShiftSize;
    // Throw away top 32 bits and shift down (requires >> to be sign extending).
    return static_cast<int>(reinterpret_cast<intptr_t>(value) >> shift_bits);
  }
  ALWAYS_INLINE static ESValue* IntToSmi(int value) {
    return IntToSmiT<kSmiShiftSize>(value);
  }
  ALWAYS_INLINE static bool IsValidSmi(intptr_t value) {
    // To be representable as an tagged small integer, the two
    // most-significant bits of 'value' must be either 00 or 11 due to
    // sign-extension. To check this we add 01 to the two
    // most-significant bits, and check if the most-significant bit is 0
    //
    // CAUTION: The original code below:
    // bool result = ((value + 0x40000000) & 0x80000000) == 0;
    // may lead to incorrect results according to the C language spec, and
    // in fact doesn't work correctly with gcc4.1.1 in some cases: The
    // compiler may produce undefined results in case of signed integer
    // overflow. The computation must be done w/ unsigned ints.
    return static_cast<uintptr_t>(value + 0x40000000U) < 0x80000000U;
  }
};

// Smi constants for 64-bit systems.
template <> struct SmiTagging<8> {
  enum { kSmiShiftSize = 31, kSmiValueSize = 32 };
  static int SmiShiftSize() { return kSmiShiftSize; }
  static int SmiValueSize() { return kSmiValueSize; }
  ALWAYS_INLINE static int SmiToInt(const ESValue* value) {
    int shift_bits = kSmiTagSize + kSmiShiftSize;
    // Shift down and throw away top 32 bits.
    return static_cast<int>(reinterpret_cast<intptr_t>(value) >> shift_bits);
  }
  ALWAYS_INLINE static ESValue* IntToSmi(int value) {
    return IntToSmiT<kSmiShiftSize>(value);
  }
  ALWAYS_INLINE static bool IsValidSmi(intptr_t value) {
    // To be representable as a long smi, the value must be a 32-bit integer.
    return (value == static_cast<int32_t>(value));
  }
};

typedef SmiTagging<kApiPointerSize> PlatformSmiTagging;
const int kSmiShiftSize = PlatformSmiTagging::kSmiShiftSize;
const int kSmiValueSize = PlatformSmiTagging::kSmiValueSize;

#define HAS_SMI_TAG(value) \
    ((reinterpret_cast<intptr_t>(value) & kSmiTagMask) == kSmiTag)
#define HAS_OBJECT_TAG(value) \
    ((reinterpret_cast<intptr_t>(value) & kHeapObjectTagMask) == kHeapObjectTag)

*/

/*
inline bool ESValue::isSmi() const
{
    return HAS_SMI_TAG(this);
}

inline bool ESValue::isHeapObject() const
{
    return HAS_OBJECT_TAG(this);
}
*/

/*
inline Smi* ESValue::toSmi() const
{
    ASSERT(isSmi());
    return static_cast<Smi*>(const_cast<ESValue*>(this));
    // TODO
    // else if (object->IsHeapESNumber()) {
    //     double value = Handle<HeapESNumber>::cast(object)->value();
    //     int int_value = FastD2I(value);
    //     if (value == FastI2D(int_value) && Smi::IsValid(int_value)) {
    //         return handle(Smi::fromInt(int_value), isolate);
    //     }
    // }
    // return new Smi();
}

inline HeapObject* ESValue::toHeapObject() const
{
    ASSERT(isHeapObject());
    return static_cast<HeapObject*>(const_cast<ESValue*>(this));
}

inline ESSlot* ESValue::toESSlot()
{
    ASSERT(isESSlot());
    return reinterpret_cast<ESSlot*>(this);
}


inline int Smi::value()
{
    int shift_bits = kSmiTagSize + kSmiShiftSize;
    return static_cast<int>(reinterpret_cast<intptr_t>(this) >> shift_bits);
}

inline Smi* Smi::fromInt(int value)
{
    //TODO DCHECK(Smi::IsValid(value));
    int smi_shift_bits = kSmiTagSize + kSmiShiftSize;
    uintptr_t tagged_value =
        (static_cast<uintptr_t>(value) << smi_shift_bits) | kSmiTag;
    return reinterpret_cast<Smi*>(tagged_value);
}

inline Smi* Smi::fromIntptr(intptr_t value)
{
    //TODO DCHECK(Smi::IsValid(value));
    int smi_shift_bits = kSmiTagSize + kSmiShiftSize;
    return reinterpret_cast<Smi*>((value << smi_shift_bits) | kSmiTag);
}

// http://www.ecma-international.org/ecma-262/5.1/#sec-9.1
inline ESValue* ESValue::toPrimitive(PrimitiveTypeHint hint)
{
    ESValue* ret = this;
    if(LIKELY(isSmi())) {
    } else {
        HeapObject* o = toHeapObject();
        // Primitive type: the result equals the input argument (no conversion).
        if (!o->isPrimitive()) {
            if (o->isESObject()) {
                if (o->isESDateObject()) {
                    return ESNumber::create(o->toESDateObject()->getTimeAsMilisec());
                } else {
                    ASSERT(false); // TODO
                }
            } else {
                ASSERT(false); // TODO
            }
        }
    }
    return ret;
}

// http://www.ecma-international.org/ecma-262/5.1/#sec-9.3
inline ESValue* ESValue::toNumber()
{
    // TODO
    ESValue* ret = this;
    if(LIKELY(isSmi())) {
    } else {
        HeapObject* o = toHeapObject();
        if (o->isESNumber()) {
            return this;
        } else if (o->isESUndefined()) {
            return esNaN;
        } else if (o->isESNull()) {
            return Smi::fromInt(0);
        } else if (o->isESBoolean()) {
            return Smi::fromInt(o->toESBoolean()->get());
        } else if (o->isESStringObject()) {
            ASSERT(false); //TODO
        } else if (o->isESObject()) {
            if (o->isESDateObject()) {
                return ESNumber::create(o->toESDateObject()->getTimeAsMilisec());
              }
            return this->toPrimitive()->toNumber();
        } else {
            ASSERT(false); // TODO
        }
    }
    return ret;
}

// http://www.ecma-international.org/ecma-262/5.1/#sec-9.5
inline ESValue* ESValue::toInt32()
{
    // TODO
    ESValue* ret = this->toNumber();
    if(LIKELY(isSmi())) {
    } else {
        HeapObject* o = this->toHeapObject();
        if (o->isESNumber()) {
            double d = o->toESNumber()->get();
            long long int posInt = d<0?-1:1 * std::floor(std::abs(d));
            long long int int32bit = posInt % 0x100000000;
            int res;
            if (int32bit >= 0x80000000)
                res = int32bit - 0x100000000;
            else
                res = int32bit;
            if (res >= 0x40000000)
                ret = ESNumber::create(res);
            else
                ret = Smi::fromInt(res);
        } else {
            ASSERT(false); // TODO
        }
    }
    return ret;
}

// http://www.ecma-international.org/ecma-262/5.1/#sec-9.4
inline ESValue* ESValue::toInteger()
{
    // TODO
    ESValue* ret = this->toNumber();
    if(LIKELY(isSmi())) {
    } else {
        ASSERT(false); // TODO
    }
    return ret;
}

inline ESString* ESValue::toString()
{
    return ESString::create(toInternalString());
}
*/

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
        if (std::isinf(d) == 1)
            return strings->Infinity;
        if (std::isinf(d) == -1)
            return strings->NegativeInfinity;

        if (d == -0.0)
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
        ASSERT(asESPointer()->isESObject());
        ESObject* obj = asESPointer()->asESObject();
        ESValue ret = ESFunctionObject::call(ESVMInstance::currentInstance(), obj->get(ESValue(strings->toString), true), obj, NULL, 0, false);
        ASSERT(ret.isESString());
        return ret.asESString();
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
    if (UNLIKELY(isESPointer() && asESPointer()->isESObject())) {
        if (preferredType == PrimitiveTypeHint::PreferString || !asESPointer()->asESObject()->hasValueOf()) {
            return ESValue(toString());
        } else { // preferNumber
            return ESValue(asESPointer()->asESObject()->valueOf());
        }
    } else {
        return *this;
    }
    RELEASE_ASSERT_NOT_REACHED();
    return ESValue();
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
        if(d == -0.0)
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
    if(UNLIKELY(isnan(num))) {
        return 0;
    } else if(UNLIKELY(num == -0.0)) {
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
    return isUndefined() || isNull() || isNumber() || isESString() || isBoolean();
}

inline double ESValue::toDouble(ESValue value)
{
    return bitwise_cast<double>(value.u.asInt64);
}

inline ESValue ESValue::fromDouble(double value)
{
    ESValue val;
    val.u.asInt64 = bitwise_cast<uint64_t>(value);
    return val;
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


//DO NOT USE THIS FUNCTION
//NOTE rooted ESSlot has short life time.
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
            //TODO(what if property is non-configurable)
            ASSERT(iter->second.isConfigurable());
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
        }

        if(UNLIKELY(m_hiddenClass->m_propertyInfo.size() >= ESHiddenClass::ESHiddenClassSizeLimit)) {
            convertIntoMapMode();
            definePropertyOrThrow(key, isWritable, isEnumerable, isConfigurable, initalValue);
        }
        ASSERT(m_hiddenClass->m_propertyFlagInfo[ret].m_isDataProperty);
        m_hiddenClassData[ret] = initalValue;
    }
}

inline void ESObject::deletePropety(const ESValue& key)
{
    if(isESArrayObject()) {
        if (key.isInt32()) {
            int i = key.toInt32();
            if(i < asESArrayObject()->length()) {
                asESArrayObject()->m_vector[i] = ESValue(ESValue::ESEmptyValue);
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
    }
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
            exists = false;
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
        auto iter = m_map->find(key.toString());
        if(iter == m_map->end()) {
            if(searchPrototype) {
                if(UNLIKELY(this->m___proto__.isUndefined())) {
                    return ESValue(ESValue::ESEmptyValue);
                }
                ESObject* target = this->m___proto__.asESPointer()->asESObject();
                while(true) {
                    escargot::ESValue s = target->find(key, false, realObj);
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
        size_t idx = m_hiddenClass->findProperty(key.toString());
        if(idx == SIZE_MAX) {
            if(searchPrototype) {
                if(UNLIKELY(this->m___proto__.isUndefined())) {
                    return ESValue(ESValue::ESEmptyValue);
                }
                ESObject* target = this->m___proto__.asESPointer()->asESObject();
                while(true) {
                    escargot::ESValue s = target->find(key, false, realObj);
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
ALWAYS_INLINE escargot::ESValue ESObject::findOnlyPrototype(escargot::ESValue key)
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
        size_t ret = m_hiddenClass->defineProperty(this, keyStr, true, true, true, true);
        m_hiddenClassData[ret] = value;
    }
}


}

#endif
