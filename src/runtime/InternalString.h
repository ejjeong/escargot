#ifndef InternalString_h
#define InternalString_h

namespace escargot {

typedef std::wstring InternalStringStd;
class InternalStringData : public gc_cleanup, public InternalStringStd {
public:
    InternalStringData()
    {
        m_isHashInited =  false;
    }
    InternalStringData(const wchar_t* str)
        : InternalStringStd(str)
    {
        m_isHashInited =  false;
    }
    InternalStringData(InternalStringStd&& src)
    {
        InternalStringStd::operator =(src);
        m_isHashInited =  false;
    }

    InternalStringData(const InternalStringData& src) = delete;
    void operator = (const InternalStringData& src) = delete;

    ALWAYS_INLINE size_t hashValue() const
    {
        initHash();
        return m_hashData;
    }

    ALWAYS_INLINE void initHash() const
    {
        if(!m_isHashInited) {
            m_isHashInited = true;
            std::hash<InternalStringStd> hashFn;
            m_hashData = hashFn((InternalStringStd &)*this);
        }
    }
protected:
    mutable size_t m_hashData;
    mutable bool m_isHashInited;
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
        m_string = new(PointerFreeGC) InternalStringData(std::move(std::to_wstring(number)));
    }

    explicit InternalString(double number)
    {
        wchar_t buf[512];
        char chbuf[50];
        char* end = rapidjson::internal::dtoa(number, chbuf);
        int i = 0;
        for (char* p = chbuf; p != end; ++p)
            buf[i++] = (wchar_t) *p;
        buf[i] = L'\0';
        //std::swprintf(buf, 511, L"%.17lg", number);
        allocString(wcslen(buf));
        wcscpy((wchar_t *)m_string->data(), buf);
    }

    explicit InternalString(wchar_t c)
    {
        m_string = new InternalStringData(InternalStringStd({c}));
    }

    InternalString(const char* s)
    {
        std::mbstate_t state = std::mbstate_t();
        int len = std::mbsrtowcs(NULL, &s, 0, &state);
        allocString(len);
        std::mbsrtowcs((wchar_t *)m_string->data(), &s, m_string->size(), &state);
    }

    ALWAYS_INLINE InternalString(const wchar_t* s)
    {
        m_string = new(PointerFreeGC) InternalStringData(s);
    }

    ALWAYS_INLINE InternalString(InternalStringStd&& s)
    {
        m_string = new(PointerFreeGC) InternalStringData(std::move(s));
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
            m_string = new InternalStringData(src.m_string->data());
        } else if(src.m_string != &emptyStringData) {
            m_string->append(src.m_string->begin(), src.m_string->end());
        }
    }

    std::string toStdString() const
    {
        const wchar_t* pt = data();
        std::string ret;
        char buffer [MB_CUR_MAX];
        memset(buffer, 0, MB_CUR_MAX);
        while(*pt) {
            int length = std::wctomb(buffer,*pt);
            if (length<1)
                break;
            ret.append(buffer);
            pt++;
        }
        return ret;
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
        m_string = new(PointerFreeGC) InternalStringData();
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
