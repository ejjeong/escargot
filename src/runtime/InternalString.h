#ifndef InternalString_h
#define InternalString_h

namespace escargot {

typedef std::wstring InternalStringStd;
class InternalStringData : public gc_cleanup, public InternalStringStd {
public:
    InternalStringData()
    {
        m_hashData.m_isHashInited =  false;
    }
    InternalStringData(const wchar_t* str)
        : InternalStringStd(str)
    {
        m_hashData.m_isHashInited =  false;
    }
    InternalStringData(InternalStringStd&& src)
        : InternalStringStd(std::move(src))
    {
        m_hashData.m_isHashInited =  false;
    }

    InternalStringData(const InternalStringData& src) = delete;
    void operator = (const InternalStringData& src) = delete;

    ALWAYS_INLINE size_t hashValue() const
    {
        initHash();
        return m_hashData.m_hashData;
    }

    ALWAYS_INLINE void initHash() const
    {
        if(!m_hashData.m_isHashInited) {
            m_hashData.m_isHashInited = true;
            std::hash<InternalStringStd> hashFn;
            m_hashData.m_hashData = hashFn((InternalStringStd &)*this);
        }
    }
protected:
#pragma pack(push, 1)
#ifdef ESCARGOT_64
    mutable struct {
        size_t m_hashData:63;
        bool m_isHashInited:1;
    } m_hashData;
#else
    mutable struct {
        size_t m_hashData:31;
        bool m_isHashInited:1;
    } m_hashData;
#endif
#pragma pack(pop)
};


extern InternalStringData emptyStringData;

//http://egloos.zum.com/profrog/v/1177107
ALWAYS_INLINE size_t utf8ToUtf16(char* UTF8, wchar_t& uc)
{
    size_t tRequiredSize = 0;

    uc = 0x0000;

    // ASCII byte
    if( 0 == (UTF8[0] & 0x80) )
    {
       uc = UTF8[0];
       tRequiredSize = 1;
    }
    else // Start byte for 2byte
    if( 0xC0 == (UTF8[0] & 0xE0) &&
           0x80 == (UTF8[1] & 0xC0) )
    {
       uc += (UTF8[0] & 0x1F) << 6;
       uc += (UTF8[1] & 0x3F) << 0;
       tRequiredSize = 2;
    }
    else // Start byte for 3byte
    if( 0xE0 == (UTF8[0] & 0xE0) &&
           0x80 == (UTF8[1] & 0xC0) &&
           0x80 == (UTF8[2] & 0xC0) )
    {
       uc += (UTF8[0] & 0x1F) << 12;
       uc += (UTF8[1] & 0x3F) << 6;
       uc += (UTF8[2] & 0x3F) << 0;
       tRequiredSize = 3;
    }
    else
    {
        // Invalid case
        tRequiredSize = 1;
        //RELEASE_ASSERT_NOT_REACHED();
    }

    return tRequiredSize;
}


ALWAYS_INLINE NullableString toNullableUtf8(std::wstring m_string)
{
    unsigned strLength = m_string.length();
    const wchar_t* pt = m_string.data();
    char buffer [MB_CUR_MAX];
    memset(buffer, 0, MB_CUR_MAX);

    char* string = new char[strLength * MB_CUR_MAX + 1];

    int idx = 0;
    for (unsigned i = 0; i < strLength; i++) {
        int length = std::wctomb(buffer,*pt);
        if (length<1) {
            string[idx++] = '\0';
        } else {
            strncpy(string+idx, buffer, length);
//            memcpy(string+idx, buffer, length);
            idx += length;
        }
        pt++;
    }
    return NullableString(string, idx);
}

ALWAYS_INLINE const char * utf16ToUtf8(const wchar_t *t)
{
    unsigned strLength = 0;
    const wchar_t* pt = t;
    char buffer [MB_CUR_MAX];
    while(*pt) {
        int length = std::wctomb(buffer,*pt);
        if (length<1)
            break;
        strLength += length;
        pt++;
    }

    char* result = (char *)GC_malloc_atomic(strLength + 1);
    pt = t;
    unsigned currentPosition = 0;

    while(*pt) {
        int length = std::wctomb(buffer,*pt);
        if (length<1)
            break;
        memcpy(&result[currentPosition],buffer,length);
        currentPosition += length;
        pt++;
    }
    /*
        unsigned strLength = m_string->length();
        const wchar_t* pt = data();
        char buffer [MB_CUR_MAX];

        char* result = (char *)GC_malloc_atomic(strLength + 1);
        pt = data();
        unsigned currentPosition = 0;

        unsigned index = 0;
        for (unsigned i = 0; i< strLength; i++) {
            int length = std::wctomb(buffer,*pt);
            if (length<1) {
                length = 1;
                *buffer = 0;
              }
            memcpy(&result[currentPosition],buffer,length);
            currentPosition += length;
            pt++;
        }
        */
    result[strLength] = 0;

    return result;
}

class InternalString {
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
        size_t tl = wcslen(buf);
        allocString(tl);
        m_string->append(&buf[0], &buf[tl]);
    }

    explicit InternalString(wchar_t c)
    {
        m_string = new InternalStringData(InternalStringStd({c}));
    }

    InternalString(const char* s)
    {
        std::mbstate_t state = std::mbstate_t();
        int len = std::mbsrtowcs(NULL, &s, 0, &state);
        allocString(0);
        m_string->resize(len);
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
    ALWAYS_INLINE friend bool operator < (const InternalString& a,const InternalString& b);
    ALWAYS_INLINE friend bool operator > (const InternalString& a,const InternalString& b);
    ALWAYS_INLINE friend bool operator <= (const InternalString& a,const InternalString& b);
    ALWAYS_INLINE friend bool operator >= (const InternalString& a,const InternalString& b);

    ALWAYS_INLINE const wchar_t* data() const
    {
        return m_string->data();
    }

    ALWAYS_INLINE const char* utf8Data() const
    {
        return utf16ToUtf8(data());
    }



    ALWAYS_INLINE const InternalStringData* string() const
    {
        return m_string;
    }

    ALWAYS_INLINE unsigned length() const
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
        wprintf(L"start\n");
        unsigned strLength = m_string->length();
        const wchar_t* pt = data();
        std::string ret;
        char buffer [MB_CUR_MAX];
        memset(buffer, 0, MB_CUR_MAX);

        for (unsigned i = 0; i < strLength; i++) {
            int length = std::wctomb(buffer,*pt);
            if (length<1) {
                buffer[0] = '\0';
                ret.push_back('\0');
            } else {
                ret.append(buffer);
              }
            wprintf(L"ret.len = %d\n",ret.length());
            pt++;
         }
//
//        while(true) {
//            int length = std::wctomb(buffer,*pt);
//            if (length<1) {
//                *buffer = 0;
//             }
//            ret.append(buffer);
//            if (ret.length() >= strLength)
//                break;
//            pt++;
//        }
        return ret;
    }

#ifndef NDEBUG
    void show() const
    {
        wprintf(L"%ls\n",data());
    }
#endif

    void allocString(size_t stringLength)
    {
        m_string = new(PointerFreeGC) InternalStringData();
        m_string->reserve(stringLength);
    }
protected:
    InternalStringData* m_string;
};

ALWAYS_INLINE InternalString utf8ToUtf16(const char *s, int length)
{
    wchar_t* wstr = (wchar_t *)GC_malloc_atomic(length * 6 + 1);
    char* pt8 = (char*) s;
    //wprintf(L"start length:%d\n", length);
    int wlen = 0;
    int idx = 0;
    int decodeLength = 0;
    while (decodeLength < length) {
        wchar_t wc;
        wlen = utf8ToUtf16(pt8,wc);
        wstr[idx++] = wc;
        pt8 += wlen;
        decodeLength += wlen;
    }
    return InternalString(std::wstring(wstr, idx));
}

ALWAYS_INLINE bool operator == (const InternalString& a, const InternalString& b)
{
    if(a.m_string->hashValue() == b.m_string->hashValue()) {
        return *a.m_string == *b.m_string;
    }
    return false;
}

ALWAYS_INLINE bool operator < (const InternalString& a, const InternalString& b)
{
    return *a.m_string < *b.m_string;
}

ALWAYS_INLINE bool operator > (const InternalString& a, const InternalString& b)
{
    return *a.m_string > *b.m_string;
}

ALWAYS_INLINE bool operator <= (const InternalString& a, const InternalString& b)
{
    return *a.m_string <= *b.m_string;
}

ALWAYS_INLINE bool operator >= (const InternalString& a, const InternalString& b)
{
    return *a.m_string >= *b.m_string;
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
