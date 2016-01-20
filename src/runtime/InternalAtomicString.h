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
template<> struct hash<std::pair<const char *, size_t> > {
    size_t operator()(std::pair<const char *, size_t> const &x) const
    {
        size_t length = x.second;
        size_t hash = static_cast<size_t>(0xc70f6907UL);
        const char* cptr = reinterpret_cast<const char*>(x.first);
        for (; length; --length)
            hash = (hash * 131) + *cptr++;
        return hash;
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
