#ifndef InternalString_h
#define InternalString_h

namespace escargot {

typedef std::wstring InternalStringStd;
class InternalStringData : public gc_cleanup, public InternalStringStd {
public:
    InternalStringData()
    {
        initHash();
    }
    InternalStringData(const wchar_t* str)
        : InternalStringStd(str)
    {
        initHash();
    }
    InternalStringData(InternalStringStd&& src)
    {
        InternalStringStd::operator =(src);
        initHash();
    }

    InternalStringData(const InternalStringData& src) = delete;
    void operator = (const InternalStringData& src) = delete;

    ALWAYS_INLINE size_t hashValue() const
    {
        return m_hashData;
    }

    ALWAYS_INLINE void initHash()
    {
        std::hash<InternalStringStd> hashFn;
        m_hashData = hashFn((InternalStringStd &)*this);
    }
protected:
    size_t m_hashData;
};


extern InternalStringData emptyStringData;

class InternalString : public gc {
public:
    ALWAYS_INLINE InternalString()
    {
        m_string = &emptyStringData;
    }

    ALWAYS_INLINE InternalString(InternalStringData* data)
    {
        m_string = data;
    }

    explicit InternalString(int number)
    {
        m_string = new InternalStringData(std::move(std::to_wstring(number)));
    }

    explicit InternalString(double number)
    {
        //FIXME
        wchar_t buf[512];
        std::swprintf(buf, 511, L"%.26g", number);
        allocString(wcslen(buf));
        wcscpy((wchar_t *)m_string->data(), buf);
        m_string->initHash();
    }

    InternalString(const char* s)
    {
        m_string = NULL;

        std::mbstate_t state = std::mbstate_t();
        int len = std::mbsrtowcs(NULL, &s, 0, &state);
        allocString(len);
        std::mbsrtowcs((wchar_t *)m_string->data(), &s, m_string->size(), &state);
        m_string->initHash();
    }

    ALWAYS_INLINE InternalString(const wchar_t* s)
    {
        m_string = new InternalStringData(s);
    }


    ALWAYS_INLINE InternalString(const InternalString& s)
    {
        m_string = s.m_string;
    }

    ALWAYS_INLINE friend bool operator == (const InternalString& a,const InternalString& b);

    ALWAYS_INLINE const wchar_t* data() const
    {
        return m_string->data();
    }

    ALWAYS_INLINE const InternalStringData* string() const
    {
        return m_string;
    }

    ALWAYS_INLINE int length() const
    {
        return m_string->length();
    }

    void append(const InternalString& src)
    {
        if(m_string == &emptyStringData) {
            m_string = src.m_string;
        } else if(src.m_string != &emptyStringData) {
            m_string->append(src.m_string->begin(), src.m_string->end());
            m_string->initHash();
        }
    }

#ifndef NDEBUG
    void show() const
    {
        wprintf(L"%ls\n",data());
    }
#endif

protected:

    void allocString(size_t stringLength)
    {
        m_string = new InternalStringData();
        m_string->resize(stringLength);
    }

    InternalStringData* m_string;
};

ALWAYS_INLINE bool operator == (const InternalString& a, const InternalString& b)
{
    if(a.m_string->hashValue() == b.m_string->hashValue()) {
        return *a.m_string == *b.m_string;
    }
    return false;
}

typedef std::vector<InternalString,gc_allocator<InternalString> > InternalStringVector;

}

namespace std
{
template<> struct hash<::escargot::InternalString>
{
    size_t operator()(escargot::InternalString const &x) const
    {
        return x.string()->hashValue();
    }
};

template<> struct equal_to<::escargot::InternalString>
{
    bool operator()(escargot::InternalString const &a, escargot::InternalString const &b) const
    {
        return *a.string() == *b.string();
    }
};

template<> struct less<::escargot::InternalString>
{
    bool operator()(escargot::InternalString const &a, escargot::InternalString const &b) const
    {
        return *a.string() < *b.string();
    }
};

}

#endif
