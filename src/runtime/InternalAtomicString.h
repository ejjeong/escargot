#ifndef InternalAtomicString_h
#define InternalAtomicString_h

namespace escargot {

class ESVMInstance;

class InternalAtomicStringData : public gc_cleanup {
    friend class InternalAtomicString;
protected:
    InternalAtomicStringData(ESVMInstance* instance, const char16_t* str);
    InternalAtomicStringData(const InternalAtomicStringData& src) = delete;
    void operator = (const InternalAtomicStringData& src) = delete;

public:
    ~InternalAtomicStringData();
    ALWAYS_INLINE size_t hashValue() const
    {
        return m_string->stringData()->hashValue();
    }
    ESString* string() const
    {
        return m_string;
    }

protected:
    ESString* m_string;
    ESVMInstance* m_instance;
};

class InternalAtomicString {
    friend class InternalAtomicStringData;
protected:
    ALWAYS_INLINE InternalAtomicString(InternalAtomicStringData* string)
    {
        m_string = string;
    }

    void init(ESVMInstance* instance, const u16string& src);
public:
    ALWAYS_INLINE InternalAtomicString()
    {
        m_string = NULL;
    }

    ALWAYS_INLINE InternalAtomicString(const InternalAtomicString& src)
    {
        m_string = src.m_string;
    }

    InternalAtomicString(const u16string& src);
    InternalAtomicString(const char16_t* src);
    InternalAtomicString(ESVMInstance* instance, const u16string& src);

    ALWAYS_INLINE const char16_t* data() const
    {
        return m_string->string()->data();
    }

    ALWAYS_INLINE ESString* string() const
    {
        return m_string->string();
    }

    ALWAYS_INLINE InternalAtomicStringData* stringData() const
    {
        return m_string;
    }

    ALWAYS_INLINE friend bool operator == (const InternalAtomicString& a,const InternalAtomicString& b);
    ALWAYS_INLINE friend bool operator != (const InternalAtomicString& a,const InternalAtomicString& b);
protected:
    InternalAtomicStringData* m_string;
};

ALWAYS_INLINE bool operator == (const InternalAtomicString& a,const InternalAtomicString& b)
{
    return *a.string()->stringData() == *b.string()->stringData();
}

ALWAYS_INLINE bool operator != (const InternalAtomicString& a,const InternalAtomicString& b)
{
    return !operator==(a,b);
}

typedef std::vector<InternalAtomicString, gc_allocator<InternalAtomicString> > InternalAtomicStringVector;

}

namespace std
{
template<> struct hash<escargot::InternalAtomicString>
{
    size_t operator()(escargot::InternalAtomicString const &x) const
    {
        return x.stringData()->hashValue();
    }
};

template<> struct equal_to<escargot::InternalAtomicString>
{
    bool operator()(escargot::InternalAtomicString const &a, escargot::InternalAtomicString const &b) const
    {
        return a.string() == b.string();
    }
};

}

namespace std
{
template<> struct hash<escargot::u16string>
{
    size_t operator()(escargot::u16string const &x) const
    {
        std::hash<std::basic_string<char16_t> > hashFn;
        return hashFn((const std::basic_string<char16_t> &)x);
    }
};

template<> struct equal_to<escargot::u16string>
{
    bool operator()(escargot::u16string const &a, escargot::u16string const &b) const
    {
        return a == b;
    }
};

}

#endif
