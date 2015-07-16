#ifndef ESString_h
#define ESString_h

namespace escargot {

//borrow concept from coffeemix/runtime/gc_helper.h
typedef std::basic_string<wchar_t, std::char_traits<wchar_t>, gc_allocator<wchar_t> > ESStringStd;
class ESString : public gc_cleanup {
public:
    ESString()
    {
        m_string = NULL;
        m_hashValue = m_isHashInited = false;
    }

    explicit ESString(int number)
    {
        //FIXME
        std::wstring ws = std::to_wstring(number);
        allocString(ws.size());
        wcscpy((wchar_t *)m_string->data(), ws.data());
        m_hashValue = m_isHashInited = false;
    }

    ESString(const char* s)
    {
        m_string = NULL;
        m_hashValue = m_isHashInited = false;

        std::mbstate_t state = std::mbstate_t();
        int len = std::mbsrtowcs(NULL, &s, 0, &state);
        allocString(len);
        std::mbsrtowcs((wchar_t *)m_string->data(), &s, m_string->size(), &state);
    }

    ESString(const wchar_t* s)
    {
        m_string = new ESStringStd(s);
        m_hashValue = m_isHashInited = false;
    }


    ESString(const ESString& s)
    {
        m_hashValue = s.m_hashValue;
        m_isHashInited = s.m_isHashInited;
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
        return m_hashValue;
    }

#ifndef NDEBUG
    void show() const
    {
        wprintf(L"%ls\n",data());
    }
#endif

protected:

    ALWAYS_INLINE void initHash() const
    {
        if(m_string && !m_isHashInited) {
            std::hash<std::wstring> hashFn;
            m_hashValue = hashFn((std::wstring &)*m_string);
            m_isHashInited = true;
        }
    }

    ALWAYS_INLINE void invalidationHash() const
    {
        m_isHashInited = false;
        m_hashValue = 0;
    }

    void allocString(size_t stringLength)
    {
        m_string = new ESStringStd();
        m_string->resize(stringLength);

        invalidationHash();
    }

    mutable size_t m_hashValue;
    mutable bool m_isHashInited;

    ESStringStd* m_string;
};

ALWAYS_INLINE bool operator == (const ESString& a,const ESString& b)
{
    a.initHash();
    b.initHash();

    if(a.m_hashValue == b.m_hashValue) {
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
