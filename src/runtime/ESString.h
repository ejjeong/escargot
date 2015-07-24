#ifndef ESString_h
#define ESString_h

namespace escargot {

typedef std::wstring ESStringStd;
class ESStringData : public gc_cleanup, public ESStringStd {
public:
    ESStringData() { }
    ESStringData(const wchar_t* str)
        : ESStringStd(str) { }
    ESStringData(ESStringStd&& src)
    {
        ESStringStd::operator =(src);
    }

    ESStringData(const ESStringData& src) = delete;
    void operator = (const ESStringData& src) = delete;
};


class ESString : public gc {
public:
    ESString()
    {
        m_string = NULL;
        m_hashData.m_hashValue = m_hashData.m_isHashInited = false;
    }

    explicit ESString(int number)
    {
        m_string = new ESStringData(std::move(std::to_wstring(number)));
        m_hashData.m_hashValue = m_hashData.m_isHashInited = false;
    }

    explicit ESString(double number)
    {
        //FIXME
        wchar_t buf[512];
        std::swprintf(buf, 511, L"%g", number);
        allocString(wcslen(buf));
        wcscpy((wchar_t *)m_string->data(), buf);
        m_hashData.m_hashValue = m_hashData.m_isHashInited = false;
    }

    ESString(const char* s)
    {
        m_string = NULL;
        m_hashData.m_hashValue = m_hashData.m_isHashInited = false;

        std::mbstate_t state = std::mbstate_t();
        int len = std::mbsrtowcs(NULL, &s, 0, &state);
        allocString(len);
        std::mbsrtowcs((wchar_t *)m_string->data(), &s, m_string->size(), &state);
    }

    ESString(const wchar_t* s)
    {
        m_string = new ESStringData(s);
        m_hashData.m_hashValue = m_hashData.m_isHashInited = false;
    }


    ESString(const ESString& s)
    {
        m_hashData = s.m_hashData;
        m_string = s.m_string;
    }

    ALWAYS_INLINE friend bool operator == (const ESString& a,const ESString& b);

    const wchar_t* data() const
    {
        if(m_string) {
            return m_string->data();
        }
        return NULL;
    }

    size_t hashValue() const
    {
        initHash();
        return m_hashData.m_hashValue;
    }

    void append(const ESString& src)
    {
        if(!m_string) {
            m_string = src.m_string;
            m_hashData = src.m_hashData;
        } else if(src.m_string) {
            m_string->append(src.m_string->begin(), src.m_string->end());
            invalidationHash();
        }
    }

    ALWAYS_INLINE void initHash() const
    {
        if(m_string && !m_hashData.m_isHashInited) {
            std::hash<std::wstring> hashFn;
            m_hashData.m_hashValue = hashFn((std::wstring &)*m_string);
            m_hashData.m_isHashInited = true;
        }
    }

#ifndef NDEBUG
    void show() const
    {
        wprintf(L"%ls\n",data());
    }
#endif

protected:

    ALWAYS_INLINE void invalidationHash() const
    {
        m_hashData.m_isHashInited = false;
        m_hashData.m_hashValue = 0;
    }

    void allocString(size_t stringLength)
    {
        m_string = new ESStringData();
        m_string->resize(stringLength);

        invalidationHash();
    }

#pragma pack(push, 1)
    mutable struct {
        bool m_isHashInited:1;
        size_t m_hashValue:31;
    } m_hashData;
#pragma pack(pop)

    ESStringData* m_string;
};

ALWAYS_INLINE bool operator == (const ESString& a,const ESString& b)
{
    a.initHash();
    b.initHash();

    if(a.m_hashData.m_hashValue == b.m_hashData.m_hashValue) {
        if(a.m_string && b.m_string) {
            if(*a.m_string == *b.m_string) {
                return true;
            }
            return false;
        } else if (!a.m_string && !b.m_string){
            return a == b;
        }
        return false;
    } else {
        return false;
    }
}

typedef std::vector<ESString,gc_allocator<ESString>> ESStringVector;

}

namespace std
{
template<> struct hash<::escargot::ESString>
{
    size_t operator()(escargot::ESString const &x) const
    {
        return x.hashValue();
    }
};
}

#endif
