#ifndef WTF_Adpdpter
#define WTF_Adpdpter

#define JS_EXPORT_PRIVATE
#define WTF_MAKE_FAST_ALLOCATED
typedef escargot::u16string String;
typedef escargot::u16string UString;
typedef char16_t UChar;
typedef char LChar;

#include "PageAllocation.h"

enum TextCaseSensitivity {
    TextCaseSensitive,
    TextCaseInsensitive
};


namespace JSC { namespace Yarr {
class Unicode {
  public:
    //TODO use ICU!
    static UChar toUpper(UChar c) { return toupper(c); }
    static UChar toLower(UChar c) { return tolower(c); }
};

/*
 * Do-nothing smart pointer classes. These have a compatible interface
 * with the smart pointers used by Yarr, but they don't actually do
 * reference counting.
 */
template<typename T>
class RefCounted {
};

template<typename T>
class RefPtr {
    T *ptr;
  public:
    RefPtr(T *p) { ptr = p; }
    operator bool() const { return ptr != NULL; }
    const T *operator ->() const { return ptr; }
    T *get() { return ptr; }
};

template<typename T>
class PassRefPtr {
    T *ptr;
  public:
    PassRefPtr(T *p) { ptr = p; }
    operator T*() { return ptr; }
};

template<typename T>
class PassOwnPtr {
    T *ptr;
  public:
    PassOwnPtr(T *p) { ptr = p; }

    T *get() { return ptr; }
};

template<typename T>
class OwnPtr {
    T *ptr;
  public:
    OwnPtr() : ptr(NULL) { }
    OwnPtr(PassOwnPtr<T> p) : ptr(p.get()) { }

    ~OwnPtr() {
        if (ptr)
            js_delete(ptr);
    }

    OwnPtr<T> &operator=(PassOwnPtr<T> p) {
        ptr = p.get();
        return *this;
    }

    T *operator ->() { return ptr; }

    T *get() { return ptr; }

    T *release() {
        T *result = ptr;
        ptr = NULL;
        return result;
    }
};

template<typename T>
PassRefPtr<T> adoptRef(T *p) { return PassRefPtr<T>(p); }

template<typename T>
PassOwnPtr<T> adoptPtr(T *p) { return PassOwnPtr<T>(p); }

template<typename T>
class Ref {
    T &val;
  public:
    Ref(T &val) : val(val) { }
    operator T&() const { return val; }
};

template<typename T, size_t N = 0>
class Vector {
  public:
    std::vector<T, gc_allocator<T> > impl;
  public:
    Vector() {}

    Vector(const Vector &v) {
        append(v);
    }

    size_t size() const {
        return impl.length();
    }

    T &operator[](size_t i) {
        return impl[i];
    }

    const T &operator[](size_t i) const {
        return impl[i];
    }

    T &at(size_t i) {
        return impl[i];
    }

    const T *begin() const {
        return impl.begin();
    }

    T &last() {
        return impl.back();
    }

    bool isEmpty() const {
        return impl.empty();
    }

    template <typename U>
    void append(const U &u) {
        impl.append(static_cast<T>(u));
    }

    template <size_t M>
    void append(const Vector<T,M> &v) {
        impl.append(v.impl);
    }

    void insert(size_t i, const T& t) {
        impl.insert(&impl[i], t);
    }

    void remove(size_t i) {
        impl.erase(&impl[i]);
    }

    void clear() {
        return impl.clear();
    }

    void shrink(size_t newLength) {
        ASSERT(newLength <= impl.length());
        impl.resize(newLength);
    }

    void shrinkToFit()
    {

    }

    void swap(Vector &other) {
        impl.swap(other.impl);
    }

    void deleteAllValues() {
        RELEASE_ASSERT_NOT_REACHED();
    }

    bool reserve(size_t capacity) {
        return impl.reserve(capacity);
    }
};


template <typename T, size_t N>
inline void
deleteAllValues(Vector<T, N> &v) {
    v.deleteAllValues();
}

#include <limits>
#include <stdint.h>
#include <type_traits>

/* Checked<T>
 *
 * This class provides a mechanism to perform overflow-safe integer arithmetic
 * without having to manually ensure that you have all the required bounds checks
 * directly in your code.
 *
 * There are two modes of operation:
 *  - The default is Checked<T, CrashOnOverflow>, and crashes at the point
 *    and overflow has occurred.
 *  - The alternative is Checked<T, RecordOverflow>, which uses an additional
 *    byte of storage to track whether an overflow has occurred, subsequent
 *    unchecked operations will crash if an overflow has occured
 *
 * It is possible to provide a custom overflow handler, in which case you need
 * to support these functions:
 *  - void overflowed();
 *    This function is called when an operation has produced an overflow.
 *  - bool hasOverflowed();
 *    This function must return true if overflowed() has been called on an
 *    instance and false if it has not.
 *  - void clearOverflow();
 *    Used to reset overflow tracking when a value is being overwritten with
 *    a new value.
 *
 * Checked<T> works for all integer types, with the following caveats:
 *  - Mixing signedness of operands is only supported for types narrower than
 *    64bits.
 *  - It does have a performance impact, so tight loops may want to be careful
 *    when using it.
 *
 */

enum class CheckedState {
    DidOverflow,
    DidNotOverflow
};

class CrashOnOverflow {
public:
    static void overflowed()
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    void clearOverflow() { }

public:
    bool hasOverflowed() const { return false; }
};

class RecordOverflow {
protected:
    RecordOverflow()
        : m_overflowed(false)
    {
    }

    void overflowed()
    {
        m_overflowed = true;
    }

    void clearOverflow()
    {
        m_overflowed = false;
    }

public:
    bool hasOverflowed() const { return m_overflowed; }

private:
    unsigned char m_overflowed;
};

template <typename T, class OverflowHandler = CrashOnOverflow> class Checked;
template <typename T> struct RemoveChecked;
template <typename T> struct RemoveChecked<Checked<T>>;

template <typename Target, typename Source, bool targetSigned = std::numeric_limits<Target>::is_signed, bool sourceSigned = std::numeric_limits<Source>::is_signed> struct BoundsChecker;
template <typename Target, typename Source> struct BoundsChecker<Target, Source, false, false> {
    static bool inBounds(Source value)
    {
        // Same signedness so implicit type conversion will always increase precision
        // to widest type
        return value <= std::numeric_limits<Target>::max();
    }
};

template <typename Target, typename Source> struct BoundsChecker<Target, Source, true, true> {
    static bool inBounds(Source value)
    {
        // Same signedness so implicit type conversion will always increase precision
        // to widest type
        return std::numeric_limits<Target>::min() <= value && value <= std::numeric_limits<Target>::max();
    }
};

template <typename Target, typename Source> struct BoundsChecker<Target, Source, false, true> {
    static bool inBounds(Source value)
    {
        // Target is unsigned so any value less than zero is clearly unsafe
        if (value < 0)
            return false;
        // If our (unsigned) Target is the same or greater width we can
        // convert value to type Target without losing precision
        if (sizeof(Target) >= sizeof(Source))
            return static_cast<Target>(value) <= std::numeric_limits<Target>::max();
        // The signed Source type has greater precision than the target so
        // max(Target) -> Source will widen.
        return value <= static_cast<Source>(std::numeric_limits<Target>::max());
    }
};

template <typename Target, typename Source> struct BoundsChecker<Target, Source, true, false> {
    static bool inBounds(Source value)
    {
        // Signed target with an unsigned source
        if (sizeof(Target) <= sizeof(Source))
            return value <= static_cast<Source>(std::numeric_limits<Target>::max());
        // Target is Wider than Source so we're guaranteed to fit any value in
        // unsigned Source
        return true;
    }
};

template <typename Target, typename Source, bool CanElide = std::is_same<Target, Source>::value || (sizeof(Target) > sizeof(Source)) > struct BoundsCheckElider;
template <typename Target, typename Source> struct BoundsCheckElider<Target, Source, true> {
    static bool inBounds(Source) { return true; }
};
template <typename Target, typename Source> struct BoundsCheckElider<Target, Source, false> : public BoundsChecker<Target, Source> {
};

template <typename Target, typename Source> static inline bool isInBounds(Source value)
{
    return BoundsCheckElider<Target, Source>::inBounds(value);
}

template <typename T> struct RemoveChecked {
    typedef T CleanType;
    static const CleanType DefaultValue = 0;
};

template <typename T> struct RemoveChecked<Checked<T, CrashOnOverflow>> {
    typedef typename RemoveChecked<T>::CleanType CleanType;
    static const CleanType DefaultValue = 0;
};

template <typename T> struct RemoveChecked<Checked<T, RecordOverflow>> {
    typedef typename RemoveChecked<T>::CleanType CleanType;
    static const CleanType DefaultValue = 0;
};

// The ResultBase and SignednessSelector are used to workaround typeof not being
// available in MSVC
template <typename U, typename V, bool uIsBigger = (sizeof(U) > sizeof(V)), bool sameSize = (sizeof(U) == sizeof(V))> struct ResultBase;
template <typename U, typename V> struct ResultBase<U, V, true, false> {
    typedef U ResultType;
};

template <typename U, typename V> struct ResultBase<U, V, false, false> {
    typedef V ResultType;
};

template <typename U> struct ResultBase<U, U, false, true> {
    typedef U ResultType;
};

template <typename U, typename V, bool uIsSigned = std::numeric_limits<U>::is_signed, bool vIsSigned = std::numeric_limits<V>::is_signed> struct SignednessSelector;
template <typename U, typename V> struct SignednessSelector<U, V, true, true> {
    typedef U ResultType;
};

template <typename U, typename V> struct SignednessSelector<U, V, false, false> {
    typedef U ResultType;
};

template <typename U, typename V> struct SignednessSelector<U, V, true, false> {
    typedef V ResultType;
};

template <typename U, typename V> struct SignednessSelector<U, V, false, true> {
    typedef U ResultType;
};

template <typename U, typename V> struct ResultBase<U, V, false, true> {
    typedef typename SignednessSelector<U, V>::ResultType ResultType;
};

template <typename U, typename V> struct Result : ResultBase<typename RemoveChecked<U>::CleanType, typename RemoveChecked<V>::CleanType> {
};

template <typename LHS, typename RHS, typename ResultType = typename Result<LHS, RHS>::ResultType,
    bool lhsSigned = std::numeric_limits<LHS>::is_signed, bool rhsSigned = std::numeric_limits<RHS>::is_signed> struct ArithmeticOperations;

template <typename LHS, typename RHS, typename ResultType> struct ArithmeticOperations<LHS, RHS, ResultType, true, true> {
    // LHS and RHS are signed types

    // Helper function
    static inline bool signsMatch(LHS lhs, RHS rhs)
    {
        return (lhs ^ rhs) >= 0;
    }

    static inline bool add(LHS lhs, RHS rhs, ResultType& result) WARN_UNUSED_RETURN
    {
        if (signsMatch(lhs, rhs)) {
            if (lhs >= 0) {
                if ((std::numeric_limits<ResultType>::max() - rhs) < lhs)
                    return false;
            } else {
                ResultType temp = lhs - std::numeric_limits<ResultType>::min();
                if (rhs < -temp)
                    return false;
            }
        } // if the signs do not match this operation can't overflow
        result = lhs + rhs;
        return true;
    }

    static inline bool sub(LHS lhs, RHS rhs, ResultType& result) WARN_UNUSED_RETURN
    {
        if (!signsMatch(lhs, rhs)) {
            if (lhs >= 0) {
                if (lhs > std::numeric_limits<ResultType>::max() + rhs)
                    return false;
            } else {
                if (rhs > std::numeric_limits<ResultType>::max() + lhs)
                    return false;
            }
        } // if the signs match this operation can't overflow
        result = lhs - rhs;
        return true;
    }

    static inline bool multiply(LHS lhs, RHS rhs, ResultType& result) WARN_UNUSED_RETURN
    {
        if (signsMatch(lhs, rhs)) {
            if (lhs >= 0) {
                if (lhs && (std::numeric_limits<ResultType>::max() / lhs) < rhs)
                    return false;
            } else {
                if (static_cast<ResultType>(lhs) == std::numeric_limits<ResultType>::min() || static_cast<ResultType>(rhs) == std::numeric_limits<ResultType>::min())
                    return false;
                if ((std::numeric_limits<ResultType>::max() / -lhs) < -rhs)
                    return false;
            }
        } else {
            if (lhs < 0) {
                if (rhs && lhs < (std::numeric_limits<ResultType>::min() / rhs))
                    return false;
            } else {
                if (lhs && rhs < (std::numeric_limits<ResultType>::min() / lhs))
                    return false;
            }
        }
        result = lhs * rhs;
        return true;
    }

    static inline bool equals(LHS lhs, RHS rhs) { return lhs == rhs; }

};

template <typename LHS, typename RHS, typename ResultType> struct ArithmeticOperations<LHS, RHS, ResultType, false, false> {
    // LHS and RHS are unsigned types so bounds checks are nice and easy
    static inline bool add(LHS lhs, RHS rhs, ResultType& result) WARN_UNUSED_RETURN
    {
        ResultType temp = lhs + rhs;
        if (temp < lhs)
            return false;
        result = temp;
        return true;
    }

    static inline bool sub(LHS lhs, RHS rhs, ResultType& result) WARN_UNUSED_RETURN
    {
        ResultType temp = lhs - rhs;
        if (temp > lhs)
            return false;
        result = temp;
        return true;
    }

    static inline bool multiply(LHS lhs, RHS rhs, ResultType& result) WARN_UNUSED_RETURN
    {
        if (!lhs || !rhs) {
            result = 0;
            return true;
        }
        if (std::numeric_limits<ResultType>::max() / lhs < rhs)
            return false;
        result = lhs * rhs;
        return true;
    }

    static inline bool equals(LHS lhs, RHS rhs) { return lhs == rhs; }

};

template <typename ResultType> struct ArithmeticOperations<int, unsigned, ResultType, true, false> {
    static inline bool add(int64_t lhs, int64_t rhs, ResultType& result)
    {
        int64_t temp = lhs + rhs;
        if (temp < std::numeric_limits<ResultType>::min())
            return false;
        if (temp > std::numeric_limits<ResultType>::max())
            return false;
        result = static_cast<ResultType>(temp);
        return true;
    }

    static inline bool sub(int64_t lhs, int64_t rhs, ResultType& result)
    {
        int64_t temp = lhs - rhs;
        if (temp < std::numeric_limits<ResultType>::min())
            return false;
        if (temp > std::numeric_limits<ResultType>::max())
            return false;
        result = static_cast<ResultType>(temp);
        return true;
    }

    static inline bool multiply(int64_t lhs, int64_t rhs, ResultType& result)
    {
        int64_t temp = lhs * rhs;
        if (temp < std::numeric_limits<ResultType>::min())
            return false;
        if (temp > std::numeric_limits<ResultType>::max())
            return false;
        result = static_cast<ResultType>(temp);
        return true;
    }

    static inline bool equals(int lhs, unsigned rhs)
    {
        return static_cast<int64_t>(lhs) == static_cast<int64_t>(rhs);
    }
};

template <typename ResultType> struct ArithmeticOperations<unsigned, int, ResultType, false, true> {
    static inline bool add(int64_t lhs, int64_t rhs, ResultType& result)
    {
        return ArithmeticOperations<int, unsigned, ResultType>::add(rhs, lhs, result);
    }

    static inline bool sub(int64_t lhs, int64_t rhs, ResultType& result)
    {
        return ArithmeticOperations<int, unsigned, ResultType>::sub(lhs, rhs, result);
    }

    static inline bool multiply(int64_t lhs, int64_t rhs, ResultType& result)
    {
        return ArithmeticOperations<int, unsigned, ResultType>::multiply(rhs, lhs, result);
    }

    static inline bool equals(unsigned lhs, int rhs)
    {
        return ArithmeticOperations<int, unsigned, ResultType>::equals(rhs, lhs);
    }
};

template <typename U, typename V, typename R> static inline bool safeAdd(U lhs, V rhs, R& result)
{
    return ArithmeticOperations<U, V, R>::add(lhs, rhs, result);
}

template <typename U, typename V, typename R> static inline bool safeSub(U lhs, V rhs, R& result)
{
    return ArithmeticOperations<U, V, R>::sub(lhs, rhs, result);
}

template <typename U, typename V, typename R> static inline bool safeMultiply(U lhs, V rhs, R& result)
{
    return ArithmeticOperations<U, V, R>::multiply(lhs, rhs, result);
}

template <typename U, typename V> static inline bool safeEquals(U lhs, V rhs)
{
    return ArithmeticOperations<U, V>::equals(lhs, rhs);
}

enum ResultOverflowedTag { ResultOverflowed };

template <typename T, class OverflowHandler> class Checked : public OverflowHandler {
public:
    template <typename _T, class _OverflowHandler> friend class Checked;
    Checked()
        : m_value(0)
    {
    }

    Checked(ResultOverflowedTag)
        : m_value(0)
    {
        this->overflowed();
    }

    template <typename U> Checked(U value)
    {
        if (!isInBounds<T>(value))
            this->overflowed();
        m_value = static_cast<T>(value);
    }

    template <typename V> Checked(const Checked<T, V>& rhs)
        : m_value(rhs.m_value)
    {
        if (rhs.hasOverflowed())
            this->overflowed();
    }

    template <typename U> Checked(const Checked<U, OverflowHandler>& rhs)
        : OverflowHandler(rhs)
    {
        if (!isInBounds<T>(rhs.m_value))
            this->overflowed();
        m_value = static_cast<T>(rhs.m_value);
    }

    template <typename U, typename V> Checked(const Checked<U, V>& rhs)
    {
        if (rhs.hasOverflowed())
            this->overflowed();
        if (!isInBounds<T>(rhs.m_value))
            this->overflowed();
        m_value = static_cast<T>(rhs.m_value);
    }

    const Checked& operator=(Checked rhs)
    {
        this->clearOverflow();
        if (rhs.hasOverflowed())
            this->overflowed();
        m_value = static_cast<T>(rhs.m_value);
        return *this;
    }

    template <typename U> const Checked& operator=(U value)
    {
        return *this = Checked(value);
    }

    template <typename U, typename V> const Checked& operator=(const Checked<U, V>& rhs)
    {
        return *this = Checked(rhs);
    }

    // prefix
    const Checked& operator++()
    {
        if (m_value == std::numeric_limits<T>::max())
            this->overflowed();
        m_value++;
        return *this;
    }

    const Checked& operator--()
    {
        if (m_value == std::numeric_limits<T>::min())
            this->overflowed();
        m_value--;
        return *this;
    }

    // postfix operators
    const Checked operator++(int)
    {
        if (m_value == std::numeric_limits<T>::max())
            this->overflowed();
        return Checked(m_value++);
    }

    const Checked operator--(int)
    {
        if (m_value == std::numeric_limits<T>::min())
            this->overflowed();
        return Checked(m_value--);
    }

    // Boolean operators
    bool operator!() const
    {
        if (this->hasOverflowed())
            RELEASE_ASSERT_NOT_REACHED();
        return !m_value;
    }

    typedef void* (Checked::*UnspecifiedBoolType);
    operator UnspecifiedBoolType*() const
    {
        if (this->hasOverflowed())
            RELEASE_ASSERT_NOT_REACHED();
        return (m_value) ? reinterpret_cast<UnspecifiedBoolType*>(1) : 0;
    }

    // Value accessors. unsafeGet() will crash if there's been an overflow.
    T unsafeGet() const
    {
        if (this->hasOverflowed())
            RELEASE_ASSERT_NOT_REACHED();
        return m_value;
    }

    inline CheckedState safeGet(T& value) const WARN_UNUSED_RETURN
    {
        value = m_value;
        if (this->hasOverflowed())
            return CheckedState::DidOverflow;
        return CheckedState::DidNotOverflow;
    }

    // Mutating assignment
    template <typename U> const Checked operator+=(U rhs)
    {
        if (!safeAdd(m_value, rhs, m_value))
            this->overflowed();
        return *this;
    }

    template <typename U> const Checked operator-=(U rhs)
    {
        if (!safeSub(m_value, rhs, m_value))
            this->overflowed();
        return *this;
    }

    template <typename U> const Checked operator*=(U rhs)
    {
        if (!safeMultiply(m_value, rhs, m_value))
            this->overflowed();
        return *this;
    }

    const Checked operator*=(double rhs)
    {
        double result = rhs * m_value;
        // Handle +/- infinity and NaN
        if (!(std::numeric_limits<T>::min() <= result && std::numeric_limits<T>::max() >= result))
            this->overflowed();
        m_value = (T)result;
        return *this;
    }

    const Checked operator*=(float rhs)
    {
        return *this *= (double)rhs;
    }

    template <typename U, typename V> const Checked operator+=(Checked<U, V> rhs)
    {
        if (rhs.hasOverflowed())
            this->overflowed();
        return *this += rhs.m_value;
    }

    template <typename U, typename V> const Checked operator-=(Checked<U, V> rhs)
    {
        if (rhs.hasOverflowed())
            this->overflowed();
        return *this -= rhs.m_value;
    }

    template <typename U, typename V> const Checked operator*=(Checked<U, V> rhs)
    {
        if (rhs.hasOverflowed())
            this->overflowed();
        return *this *= rhs.m_value;
    }

    // Equality comparisons
    template <typename V> bool operator==(Checked<T, V> rhs)
    {
        return unsafeGet() == rhs.unsafeGet();
    }

    template <typename U> bool operator==(U rhs)
    {
        if (this->hasOverflowed())
            this->overflowed();
        return safeEquals(m_value, rhs);
    }

    template <typename U, typename V> const Checked operator==(Checked<U, V> rhs)
    {
        return unsafeGet() == Checked(rhs.unsafeGet());
    }

    template <typename U> bool operator!=(U rhs)
    {
        return !(*this == rhs);
    }

private:
    // Disallow implicit conversion of floating point to integer types
    Checked(float);
    Checked(double);
    void operator=(float);
    void operator=(double);
    void operator+=(float);
    void operator+=(double);
    void operator-=(float);
    void operator-=(double);
    T m_value;
};

template <typename U, typename V, typename OverflowHandler> static inline Checked<typename Result<U, V>::ResultType, OverflowHandler> operator+(Checked<U, OverflowHandler> lhs, Checked<V, OverflowHandler> rhs)
{
    U x = 0;
    V y = 0;
    bool overflowed = lhs.safeGet(x) == CheckedState::DidOverflow || rhs.safeGet(y) == CheckedState::DidOverflow;
    typename Result<U, V>::ResultType result = 0;
    overflowed |= !safeAdd(x, y, result);
    if (overflowed)
        return ResultOverflowed;
    return result;
}

template <typename U, typename V, typename OverflowHandler> static inline Checked<typename Result<U, V>::ResultType, OverflowHandler> operator-(Checked<U, OverflowHandler> lhs, Checked<V, OverflowHandler> rhs)
{
    U x = 0;
    V y = 0;
    bool overflowed = lhs.safeGet(x) == CheckedState::DidOverflow || rhs.safeGet(y) == CheckedState::DidOverflow;
    typename Result<U, V>::ResultType result = 0;
    overflowed |= !safeSub(x, y, result);
    if (overflowed)
        return ResultOverflowed;
    return result;
}

template <typename U, typename V, typename OverflowHandler> static inline Checked<typename Result<U, V>::ResultType, OverflowHandler> operator*(Checked<U, OverflowHandler> lhs, Checked<V, OverflowHandler> rhs)
{
    U x = 0;
    V y = 0;
    bool overflowed = lhs.safeGet(x) == CheckedState::DidOverflow || rhs.safeGet(y) == CheckedState::DidOverflow;
    typename Result<U, V>::ResultType result = 0;
    overflowed |= !safeMultiply(x, y, result);
    if (overflowed)
        return ResultOverflowed;
    return result;
}

template <typename U, typename V, typename OverflowHandler> static inline Checked<typename Result<U, V>::ResultType, OverflowHandler> operator+(Checked<U, OverflowHandler> lhs, V rhs)
{
    return lhs + Checked<V, OverflowHandler>(rhs);
}

template <typename U, typename V, typename OverflowHandler> static inline Checked<typename Result<U, V>::ResultType, OverflowHandler> operator-(Checked<U, OverflowHandler> lhs, V rhs)
{
    return lhs - Checked<V, OverflowHandler>(rhs);
}

template <typename U, typename V, typename OverflowHandler> static inline Checked<typename Result<U, V>::ResultType, OverflowHandler> operator*(Checked<U, OverflowHandler> lhs, V rhs)
{
    return lhs * Checked<V, OverflowHandler>(rhs);
}

template <typename U, typename V, typename OverflowHandler> static inline Checked<typename Result<U, V>::ResultType, OverflowHandler> operator+(U lhs, Checked<V, OverflowHandler> rhs)
{
    return Checked<U, OverflowHandler>(lhs) + rhs;
}

template <typename U, typename V, typename OverflowHandler> static inline Checked<typename Result<U, V>::ResultType, OverflowHandler> operator-(U lhs, Checked<V, OverflowHandler> rhs)
{
    return Checked<U, OverflowHandler>(lhs) - rhs;
}

template <typename U, typename V, typename OverflowHandler> static inline Checked<typename Result<U, V>::ResultType, OverflowHandler> operator*(U lhs, Checked<V, OverflowHandler> rhs)
{
    return Checked<U, OverflowHandler>(lhs) * rhs;
}

// Convenience typedefs.
typedef Checked<int8_t, RecordOverflow> CheckedInt8;
typedef Checked<uint8_t, RecordOverflow> CheckedUint8;
typedef Checked<int16_t, RecordOverflow> CheckedInt16;
typedef Checked<uint16_t, RecordOverflow> CheckedUint16;
typedef Checked<int32_t, RecordOverflow> CheckedInt32;
typedef Checked<uint32_t, RecordOverflow> CheckedUint32;
typedef Checked<int64_t, RecordOverflow> CheckedInt64;
typedef Checked<uint64_t, RecordOverflow> CheckedUint64;
typedef Checked<size_t, RecordOverflow> CheckedSize;

template<typename T, typename U>
Checked<T, RecordOverflow> checkedSum(U value)
{
    return Checked<T, RecordOverflow>(value);
}
template<typename T, typename U, typename... Args>
Checked<T, RecordOverflow> checkedSum(U value, Args... args)
{
    return Checked<T, RecordOverflow>(value) + checkedSum<T>(args...);
}

// Sometimes, you just want to check if some math would overflow - the code to do the math is
// already in place, and you want to guard it.

template<typename T, typename... Args> bool sumOverflows(Args... args)
{
    return checkedSum<T>(args...).hasOverflowed();
}



#if WTF_CPU_SPARC
#define MINIMUM_BUMP_POOL_SIZE 0x2000
#elif WTF_CPU_IA64
#define MINIMUM_BUMP_POOL_SIZE 0x4000
#else
#define MINIMUM_BUMP_POOL_SIZE 0x1000
#endif

class BumpPointerPool {
public:
    // ensureCapacity will check whether the current pool has capacity to
    // allocate 'size' bytes of memory  If it does not, it will attempt to
    // allocate a new pool (which will be added to this one in a chain).
    //
    // If allocation fails (out of memory) this method will return null.
    // If the return value is non-null, then callers should update any
    // references they have to this current (possibly full) BumpPointerPool
    // to instead point to the newly returned BumpPointerPool.
    BumpPointerPool* ensureCapacity(size_t size)
    {
        void* allocationEnd = static_cast<char*>(m_current) + size;
        ASSERT(allocationEnd > m_current); // check for overflow
        if (allocationEnd <= static_cast<void*>(this))
            return this;
        return ensureCapacityCrossPool(this, size);
    }

    // alloc should only be called after calling ensureCapacity; as such
    // alloc will never fail.
    void* alloc(size_t size)
    {
        void* current = m_current;
        void* allocationEnd = static_cast<char*>(current) + size;
        ASSERT(allocationEnd > current); // check for overflow
        ASSERT(allocationEnd <= static_cast<void*>(this));
        m_current = allocationEnd;
        return current;
    }

    // The dealloc method releases memory allocated using alloc.  Memory
    // must be released in a LIFO fashion, e.g. if the client calls alloc
    // four times, returning pointer A, B, C, D, then the only valid order
    // in which these may be deallocaed is D, C, B, A.
    //
    // The client may optionally skip some deallocations.  In the example
    // above, it would be valid to only explicitly dealloc C, A (D being
    // dealloced along with C, B along with A).
    //
    // If pointer was not allocated from this pool (or pools) then dealloc
    // will CRASH().  Callers should update any references they have to
    // this current BumpPointerPool to instead point to the returned
    // BumpPointerPool.
    BumpPointerPool* dealloc(void* position)
    {
        if ((position >= m_start) && (position <= static_cast<void*>(this))) {
            ASSERT(position <= m_current);
            m_current = position;
            return this;
        }
        return deallocCrossPool(this, position);
    }

    size_t sizeOfNonHeapData() const
    {
        ASSERT(!m_previous);
        size_t n = 0;
        const BumpPointerPool *curr = this;
        while (curr) {
            n += m_allocation.size();
            curr = curr->m_next;
        }
        return n;
    }

private:
    // Placement operator new, returns the last 'size' bytes of allocation for use as this.
    void* operator new(size_t size, const PageAllocation& allocation)
    {
        ASSERT(size < allocation.size());
        return reinterpret_cast<char*>(reinterpret_cast<intptr_t>(allocation.base()) + allocation.size()) - size;
    }

    BumpPointerPool(const PageAllocation& allocation)
        : m_current(allocation.base())
        , m_start(allocation.base())
        , m_next(0)
        , m_previous(0)
        , m_allocation(allocation)
    {
    }

    static BumpPointerPool* create(size_t minimumCapacity = 0)
    {
        // Add size of BumpPointerPool object, check for overflow.
        minimumCapacity += sizeof(BumpPointerPool);
        if (minimumCapacity < sizeof(BumpPointerPool))
            return 0;

        size_t poolSize = MINIMUM_BUMP_POOL_SIZE;
        while (poolSize < minimumCapacity) {
            poolSize <<= 1;
            // The following if check relies on MINIMUM_BUMP_POOL_SIZE being a power of 2!
            ASSERT(!(MINIMUM_BUMP_POOL_SIZE & (MINIMUM_BUMP_POOL_SIZE - 1)));
            if (!poolSize)
                return 0;
        }

        PageAllocation allocation = PageAllocation::allocate(poolSize);
        if (!!allocation)
            return new(allocation) BumpPointerPool(allocation);
        return 0;
    }

    void shrink()
    {
        ASSERT(!m_previous);
        m_current = m_start;
        while (m_next) {
            BumpPointerPool* nextNext = m_next->m_next;
            m_next->destroy();
            m_next = nextNext;
        }
    }

    void destroy()
    {
        m_allocation.deallocate();
    }

    static BumpPointerPool* ensureCapacityCrossPool(BumpPointerPool* previousPool, size_t size)
    {
        // The pool passed should not have capacity, so we'll start with the next one.
        ASSERT(previousPool);
        ASSERT((static_cast<char*>(previousPool->m_current) + size) > previousPool->m_current); // check for overflow
        ASSERT((static_cast<char*>(previousPool->m_current) + size) > static_cast<void*>(previousPool));
        BumpPointerPool* pool = previousPool->m_next;

        while (true) {
            if (!pool) {
                // We've run to the end; allocate a new pool.
                pool = BumpPointerPool::create(size);
                previousPool->m_next = pool;
                pool->m_previous = previousPool;
                return pool;
            }

            //
            void* current = pool->m_current;
            void* allocationEnd = static_cast<char*>(current) + size;
            ASSERT(allocationEnd > current); // check for overflow
            if (allocationEnd <= static_cast<void*>(pool))
                return pool;
        }
    }

    static BumpPointerPool* deallocCrossPool(BumpPointerPool* pool, void* position)
    {
        // Should only be called if position is not in the current pool.
        ASSERT((position < pool->m_start) || (position > static_cast<void*>(pool)));

        while (true) {
            // Unwind the current pool to the start, move back in the chain to the previous pool.
            pool->m_current = pool->m_start;
            pool = pool->m_previous;

            // position was nowhere in the chain!
            if (!pool)
                CRASH();

            if ((position >= pool->m_start) && (position <= static_cast<void*>(pool))) {
                ASSERT(position <= pool->m_current);
                pool->m_current = position;
                return pool;
            }
        }
    }

    void* m_current;
    void* m_start;
    BumpPointerPool* m_next;
    BumpPointerPool* m_previous;
    PageAllocation m_allocation;

    friend class BumpPointerAllocator;
};

// A BumpPointerAllocator manages a set of BumpPointerPool objects, which
// can be used for LIFO (stack like) allocation.
//
// To begin allocating using this class call startAllocator().  The result
// of this method will be null if the initial pool allocation fails, or a
// pointer to a BumpPointerPool object that can be used to perform
// allocations.  Whilst running no memory will be released until
// stopAllocator() is called.  At this point all allocations made through
// this allocator will be reaped, and underlying memory may be freed.
//
// (In practice we will still hold on to the initial pool to allow allocation
// to be quickly restared, but aditional pools will be freed).
//
// This allocator is non-renetrant, it is encumbant on the clients to ensure
// startAllocator() is not called again until stopAllocator() has been called.
class BumpPointerAllocator {
public:
    BumpPointerAllocator()
        : m_head(0)
    {
    }

    ~BumpPointerAllocator()
    {
        if (m_head)
            m_head->destroy();
    }

    BumpPointerPool* startAllocator()
    {
        if (!m_head)
            m_head = BumpPointerPool::create();
        return m_head;
    }

    void stopAllocator()
    {
        if (m_head)
            m_head->shrink();
    }

    size_t sizeOfNonHeapData() const
    {
        return m_head ? m_head->sizeOfNonHeapData() : 0;
    }

private:
    BumpPointerPool* m_head;
};


} }
#endif
