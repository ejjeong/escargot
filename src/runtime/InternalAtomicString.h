#ifndef InternalAtomicString_h
#define InternalAtomicString_h

namespace escargot {

class ESVMInstance;

class InternalAtomicString {
protected:
    void init(ESVMInstance* instance, const char* src, size_t len);
    void init(ESVMInstance* instance, const char16_t* src, size_t len);
public:
    ALWAYS_INLINE InternalAtomicString()
    {
        m_string = NULL;
    }

    ALWAYS_INLINE InternalAtomicString(const InternalAtomicString& src)
    {
        m_string = src.m_string;
    }

    InternalAtomicString(const char16_t* src);
    InternalAtomicString(const char16_t* src, size_t len);
    InternalAtomicString(ESVMInstance* instance,  const char16_t* src);
    InternalAtomicString(ESVMInstance* instance,  const char16_t* src, size_t len);

    InternalAtomicString(const char* src);
    InternalAtomicString(const char* src, size_t len);
    InternalAtomicString(ESVMInstance* instance,  const char* src);
    InternalAtomicString(ESVMInstance* instance,  const char* src, size_t len);

    ALWAYS_INLINE ESString* string() const
    {
        return m_string;
    }

    operator ESString*() const
    {
        return string();
    }

    ALWAYS_INLINE friend bool operator == (const InternalAtomicString& a, const InternalAtomicString& b);
    ALWAYS_INLINE friend bool operator != (const InternalAtomicString& a, const InternalAtomicString& b);

#ifdef ENABLE_ESJIT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
    static size_t offsetOfString() { return offsetof(InternalAtomicString, m_string); }
#pragma GCC diagnostic pop
#endif

    void clear()
    {
        m_string = NULL;
    }
protected:
    ESString* m_string;
};

ALWAYS_INLINE bool operator == (const InternalAtomicString& a, const InternalAtomicString& b)
{
    return a.string() == b.string();
}

ALWAYS_INLINE bool operator != (const InternalAtomicString& a, const InternalAtomicString& b)
{
    return !operator==(a, b);
}

typedef std::vector<InternalAtomicString, gc_allocator<InternalAtomicString> > InternalAtomicStringVector;

}

namespace std {
template<> struct hash<escargot::InternalAtomicString> {
    size_t operator()(escargot::InternalAtomicString const &x) const
    {
        return x.string()->hashValue();
    }
};

template<> struct equal_to<escargot::InternalAtomicString> {
    bool operator()(escargot::InternalAtomicString const &a, escargot::InternalAtomicString const &b) const
    {
        return a.string() == b.string();
    }
};

}

namespace std {
template<> struct hash<escargot::u16string> {
    size_t operator()(escargot::u16string const &x) const
    {
        std::hash<std::basic_string<char16_t> > hashFn;
        return hashFn((const std::basic_string<char16_t> &)x);
    }
};

template<> struct equal_to<escargot::u16string> {
    bool operator()(escargot::u16string const &a, escargot::u16string const &b) const
    {
        return a == b;
    }
};

}

// this hash function is borrow from https://github.com/gcc-mirror/gcc/blob/master/libstdc%2B%2B-v3/libsupc%2B%2B/hash_bytes.cc

inline std::size_t unaligned_load(const char* p)
{
    std::size_t result;
    memcpy(&result, p, sizeof(result));
    return result;
}

#if defined(ESCARGOT_32)
// Implementation of Murmur hash for 32-bit size_t.
inline size_t hashBytes(const void* ptr, size_t len, size_t seed)
{
    const size_t m = 0x5bd1e995;
    size_t hash = seed ^ len;
    const char* buf = static_cast<const char*>(ptr);

    // Mix 4 bytes at a time into the hash.
    while (len >= 4) {
        size_t k = unaligned_load(buf);
        k *= m;
        k ^= k >> 24;
        k *= m;
        hash *= m;
        hash ^= k;
        buf += 4;
        len -= 4;
    }

    // Handle the last few bytes of the input array.
    switch (len) {
    case 3:
        hash ^= static_cast<unsigned char>(buf[2]) << 16;
    case 2:
        hash ^= static_cast<unsigned char>(buf[1]) << 8;
    case 1:
        hash ^= static_cast<unsigned char>(buf[0]);
        hash *= m;
    };

    // Do a few final mixes of the hash.
    hash ^= hash >> 13;
    hash *= m;
    hash ^= hash >> 15;
    return hash;
}

#elif defined(ESCARGOT_64)

// Loads n bytes, where 1 <= n < 8.
inline std::size_t load_bytes(const char* p, int n)
{
    std::size_t result = 0;
    --n;
    do {
        result = (result << 8) + static_cast<unsigned char>(p[n]);
    } while (--n >= 0);
    return result;
}

inline std::size_t shift_mix(std::size_t v)
{
    return v ^ (v >> 47);
}

// Implementation of Murmur hash for 64-bit size_t.
inline size_t hashBytes(const void* ptr, size_t len, size_t seed)
{
    static const size_t mul = (((size_t) 0xc6a4a793UL) << 32UL)
    + (size_t) 0x5bd1e995UL;
    const char* const buf = static_cast<const char*>(ptr);

    // Remove the bytes not divisible by the sizeof(size_t). This
    // allows the main loop to process the data as 64-bit integers.
    const int len_aligned = len & ~0x7;
    const char* const end = buf + len_aligned;
    size_t hash = seed ^ (len * mul);
    for (const char* p = buf; p != end; p += 8) {
        const size_t data = shift_mix(unaligned_load(p) * mul) * mul;
        hash ^= data;
        hash *= mul;
    }
    if ((len & 0x7) != 0) {
        const size_t data = load_bytes(end, len & 0x7);
        hash ^= data;
        hash *= mul;
    }
    hash = shift_mix(hash) * mul;
    hash = shift_mix(hash);
    return hash;
}

#else

// Dummy hash implementation for unusual sizeof(size_t).
inline size_t hashBytes(const void* ptr, size_t len, size_t seed)
{
    size_t hash = seed;
    const char* cptr = reinterpret_cast<const char*>(ptr);
    for (; len; --len)
    hash = (hash * 131) + *cptr++;
    return hash;
}


#endif

namespace std {
template<> struct hash<std::pair<const char *, size_t> > {
    size_t operator()(std::pair<const char *, size_t> const &x) const
    {
        size_t seed = static_cast<size_t>(0xc70f6907UL);
        return hashBytes(x.first, x.second * sizeof(char), seed);
    }
};

template<> struct equal_to<std::pair<const char *, size_t> > {
    bool operator()(std::pair<const char *, size_t> const &a, std::pair<const char *, size_t> const &b) const
    {
        if (a.second == b.second) {
            return memcmp(a.first, b.first, sizeof(char) * a.second) == 0;
        }
        return false;
    }
};

}

#endif
