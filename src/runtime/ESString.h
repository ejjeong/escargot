#ifndef ESString_h
#define ESString_h

namespace escargot {

typedef std::wstring ESStringStd;
class ESStringData : public gc_cleanup, public ESStringStd {
public:
    ESStringData()
    {
        initHash();
    }
    ESStringData(const wchar_t* str)
        : ESStringStd(str)
    {
        initHash();
    }
    ESStringData(ESStringStd&& src)
    {
        ESStringStd::operator =(src);
        initHash();
    }

    ESStringData(const ESStringData& src) = delete;
    void operator = (const ESStringData& src) = delete;

    ALWAYS_INLINE size_t hashValue() const
    {
        return m_hashData;
    }

    ALWAYS_INLINE void initHash()
    {
        std::hash<ESStringStd> hashFn;
        m_hashData = hashFn((ESStringStd &)*this);
    }
protected:
    size_t m_hashData;
};


extern ESStringData emptyStringData;

class ESString : public gc {
public:
    ALWAYS_INLINE ESString()
    {
        m_string = &emptyStringData;
    }

    ALWAYS_INLINE ESString(ESStringData* data)
    {
        m_string = data;
    }

    explicit ESString(int number)
    {
        m_string = new ESStringData(std::move(std::to_wstring(number)));
    }

    explicit ESString(double number)
    {
        //FIXME
        wchar_t buf[512];
        std::swprintf(buf, 511, L"%g", number);
        allocString(wcslen(buf));
        wcscpy((wchar_t *)m_string->data(), buf);
        m_string->initHash();
    }

    ESString(const char* s)
    {
        m_string = NULL;

        std::mbstate_t state = std::mbstate_t();
        int len = std::mbsrtowcs(NULL, &s, 0, &state);
        allocString(len);
        std::mbsrtowcs((wchar_t *)m_string->data(), &s, m_string->size(), &state);
        m_string->initHash();
    }

    ALWAYS_INLINE ESString(const wchar_t* s)
    {
        m_string = new ESStringData(s);
    }


    ALWAYS_INLINE ESString(const ESString& s)
    {
        m_string = s.m_string;
    }

    ALWAYS_INLINE friend bool operator == (const ESString& a,const ESString& b);

    ALWAYS_INLINE const wchar_t* data() const
    {
        return m_string->data();
    }

    ALWAYS_INLINE const ESStringData* string() const
    {
        return m_string;
    }

    void append(const ESString& src)
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
        m_string = new ESStringData();
        m_string->resize(stringLength);
    }

    ESStringData* m_string;
};

ALWAYS_INLINE bool operator == (const ESString& a, const ESString& b)
{
    if(a.m_string->hashValue() == b.m_string->hashValue()) {
        return *a.m_string == *b.m_string;
    }
    return false;
}

typedef std::vector<ESString,gc_allocator<ESString> > ESStringVector;

}

namespace std
{
template<> struct hash<::escargot::ESString>
{
    size_t operator()(escargot::ESString const &x) const
    {
        return x.string()->hashValue();
    }
};

template<> struct equal_to<::escargot::ESString>
{
    bool operator()(escargot::ESString const &a, escargot::ESString const &b) const
    {
        return *a.string() == *b.string();
    }
};

template<> struct less<::escargot::ESString>
{
    bool operator()(escargot::ESString const &a, escargot::ESString const &b) const
    {
        return *a.string() < *b.string();
    }
};

}

#endif
