#ifndef ESAtomicString_h
#define ESAtomicString_h

#include "ESString.h"

namespace escargot {

class ESVMInstance;

class ESAtomicStringData : public gc_cleanup, public ESStringStd {
    friend class ESAtomicString;
protected:
    ESAtomicStringData(ESVMInstance* instance, const wchar_t* str);

    ESAtomicStringData(const ESAtomicStringData& src) = delete;
    void operator = (const ESAtomicStringData& src) = delete;

    ALWAYS_INLINE void initHash()
    {
        std::hash<ESStringStd> hashFn;
        m_hashData = hashFn((ESStringStd &)*this);
    }
public:
    ESAtomicStringData();
    ~ESAtomicStringData();
    ALWAYS_INLINE size_t hashValue() const
    {
        return m_hashData;
    }

protected:
    size_t m_hashData;
    ESVMInstance* m_instance;
};

extern ESAtomicStringData emptyAtomicString;

class ESAtomicString {
    friend class ESAtomicStringData;
protected:
    ALWAYS_INLINE ESAtomicString(ESAtomicStringData* string)
    {
        m_string = string;
    }
public:
    ALWAYS_INLINE ESAtomicString()
    {
        m_string = &emptyAtomicString;
    }
    ALWAYS_INLINE ESAtomicString(const ESAtomicString& src)
    {
        m_string = src.m_string;
    }
    ESAtomicString(const std::wstring& src);
    ESAtomicString(const wchar_t* src);
    ESAtomicString(ESVMInstance* instance, const std::wstring& src);
    ALWAYS_INLINE const wchar_t* data() const
    {
        return m_string->data();
    }

    ALWAYS_INLINE const ESAtomicStringData* string() const
    {
        return m_string;
    }

    operator ESString() const
    {
        return ESString(m_string->data());
    }

    ALWAYS_INLINE friend bool operator == (const ESAtomicString& a,const ESAtomicString& b);
    ALWAYS_INLINE friend bool operator != (const ESAtomicString& a,const ESAtomicString& b);
protected:
    ESAtomicStringData* m_string;
};

ALWAYS_INLINE bool operator == (const ESAtomicString& a,const ESAtomicString& b)
{
    return a.string() == b.string();
}

ALWAYS_INLINE bool operator != (const ESAtomicString& a,const ESAtomicString& b)
{
    return a.string() != b.string();
}

typedef std::vector<ESAtomicString, gc_allocator<ESAtomicString> > ESAtomicStringVector;

}

namespace std
{
template<> struct hash<::escargot::ESAtomicString>
{
    size_t operator()(escargot::ESAtomicString const &x) const
    {
        return x.string()->hashValue();
    }
};

template<> struct equal_to<::escargot::ESAtomicString>
{
    bool operator()(escargot::ESAtomicString const &a, escargot::ESAtomicString const &b) const
    {
        return a.string() == b.string();
    }
};

}

#endif
