#ifndef ESAtomicString_h
#define ESAtomicString_h

#include "ESString.h"

namespace escargot {

class ESVMInstance;

class ESAtomicStringData : public gc_cleanup, public ESStringStd {
    friend class ESAtomicString;
protected:
    ESAtomicStringData(ESVMInstance* instance)
    {
        m_instance = instance;
        initHash();
    }
    ESAtomicStringData(ESVMInstance* instance, const wchar_t* str)
        : ESStringStd(str)
    {
        m_instance = instance;
        initHash();
    }

    ~ESAtomicStringData();

    ESAtomicStringData(const ESAtomicStringData& src) = delete;
    void operator = (const ESAtomicStringData& src) = delete;

    ALWAYS_INLINE void initHash()
    {
        std::hash<ESStringStd> hashFn;
        m_hashData = hashFn((ESStringStd &)*this);
    }
public:
    ALWAYS_INLINE size_t hashValue() const
    {
        return m_hashData;
    }

protected:
    size_t m_hashData;
    ESVMInstance* m_instance;
};

class ESAtomicString {
    friend class ESAtomicStringData;
protected:
    ESAtomicString(ESAtomicStringData* string)
    {
        m_string = string;
    }
public:
    static ESAtomicString create(ESVMInstance* instance, const std::wstring& src);
    ALWAYS_INLINE const wchar_t* data() const
    {
        return m_string->data();
    }

    ALWAYS_INLINE const ESAtomicStringData* string() const
    {
        return m_string;
    }
protected:
    ESAtomicStringData* m_string;
};

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
