#ifndef InternalAtomicString_h
#define InternalAtomicString_h

namespace escargot {

class ESVMInstance;
class ESValue;

class InternalAtomicStringData : public gc_cleanup, public u16string {
    friend class InternalAtomicString;
protected:
    InternalAtomicStringData(ESVMInstance* instance, const char16_t* str);

    InternalAtomicStringData(const InternalAtomicStringData& src) = delete;
    void operator = (const InternalAtomicStringData& src) = delete;

    ALWAYS_INLINE void initHash()
    {
        std::hash<std::u16string> hashFn;
        m_hashData = hashFn((std::u16string &)*this);
    }
public:
    InternalAtomicStringData();
    ~InternalAtomicStringData();
    ALWAYS_INLINE size_t hashValue() const
    {
        return m_hashData;
    }

protected:
    size_t m_hashData;
    ESVMInstance* m_instance;
};

extern InternalAtomicStringData emptyInternalAtomicString;

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
        m_string = &emptyInternalAtomicString;
    }
    ALWAYS_INLINE InternalAtomicString(const InternalAtomicString& src)
    {
        m_string = src.m_string;
    }

    InternalAtomicString(const u16string& src);
    InternalAtomicString(const char16_t* src);
    InternalAtomicString(ESVMInstance* instance, const u16string& src);
    InternalAtomicString(const ESValue* src);

    ALWAYS_INLINE const char16_t* data() const
    {
        return m_string->data();
    }

    ALWAYS_INLINE const InternalAtomicStringData* string() const
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
    return a.string() == b.string();
}

ALWAYS_INLINE bool operator != (const InternalAtomicString& a,const InternalAtomicString& b)
{
    return a.string() != b.string();
}

typedef std::vector<InternalAtomicString, gc_allocator<InternalAtomicString> > InternalAtomicStringVector;

}

namespace std
{
template<> struct hash<::escargot::InternalAtomicString>
{
    size_t operator()(escargot::InternalAtomicString const &x) const
    {
        return x.string()->hashValue();
    }
};

template<> struct equal_to<::escargot::InternalAtomicString>
{
    bool operator()(escargot::InternalAtomicString const &a, escargot::InternalAtomicString const &b) const
    {
        return a.string() == b.string();
    }
};

}
/*
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
*/
#endif
