#ifndef ESValueInlines_h
#define ESValueInlines_h

namespace escargot {

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


inline bool ESValue::isSmi() const
{
    return HAS_SMI_TAG(this);
}

inline bool ESValue::isHeapObject() const
{
    return HAS_OBJECT_TAG(this);
}

inline bool ESValue::isJSSlot() const
{
    return (HAS_OBJECT_TAG(this)) && ((reinterpret_cast<const HeapObject*>(this))->type() & HeapObject::Type::JSSlot);
}

inline Smi* ESValue::toSmi() const
{
    ASSERT(isSmi());
    return static_cast<Smi*>(const_cast<ESValue*>(this));
    /* TODO
    else if (object->IsHeapPNumber()) {
        double value = Handle<HeapPNumber>::cast(object)->value();
        int int_value = FastD2I(value);
        if (value == FastI2D(int_value) && Smi::IsValid(int_value)) {
            return handle(Smi::fromInt(int_value), isolate);
        }
    }
    return new Smi();
    */
}

inline HeapObject* ESValue::toHeapObject() const
{
    ASSERT(isHeapObject());
    return static_cast<HeapObject*>(const_cast<ESValue*>(this));
}

inline JSSlot* ESValue::toJSSlot()
{
    ASSERT(isJSSlot());
    return reinterpret_cast<JSSlot*>(this);
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
            if (o->isJSObject()) {
                if (o->isJSDate()) {
                    return PNumber::create(o->toJSDate()->getTimeAsMilisec());
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
        if (o->isPNumber()) {
            return this;
        } else if (o->isESUndefined()) {
            return esNaN;
        } else if (o->isESNull()) {
            return Smi::fromInt(0);
        } else if (o->isPBoolean()) {
            return Smi::fromInt(o->toPBoolean()->get());
        } else if (o->isJSString()) {
            ASSERT(false); //TODO
        } else if (o->isJSObject()) {
            if (o->isJSDate()) {
                return PNumber::create(o->toJSDate()->getTimeAsMilisec());
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
        if (o->isPNumber()) {
            double d = o->toPNumber()->get();
            long long int posInt = d<0?-1:1 * std::floor(std::abs(d));
            long long int int32bit = posInt % 0x100000000;
            int res;
            if (int32bit >= 0x80000000)
                res = int32bit - 0x100000000;
            else
                res = int32bit;
            if (res >= 0x40000000)
                ret = PNumber::create(res);
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

inline PString* ESValue::toString()
{
    return PString::create(toESString());
}

inline ESValue* ESValue::ensureValue()
{
    if(isJSSlot()) {
        return toJSSlot()->value();
    }
    return this;
}

}

#endif
