#include "Escargot.h"
#include "InternalAtomicString.h"

#include "vm/ESVMInstance.h"

namespace escargot {

template<class T> size_t strlenT(T* arg)
{
    size_t i = 0;
    while (arg[i] != 0)
        ++i;
    return i;
}

InternalAtomicString::InternalAtomicString(const char16_t* src)
{
    size_t len = strlenT(src);
    init(ESVMInstance::currentInstance(), src, len);
}

InternalAtomicString::InternalAtomicString(const char16_t* src, size_t len)
{
    init(ESVMInstance::currentInstance(), src, len);
}

InternalAtomicString::InternalAtomicString(ESVMInstance* instance,  const char16_t* src)
{
    size_t len = strlenT(src);
    init(instance, src, len);
}

InternalAtomicString::InternalAtomicString(ESVMInstance* instance, const char16_t* src, size_t len)
{
    init(instance, src, len);
}

InternalAtomicString::InternalAtomicString(const char* src)
{
    size_t len = strlenT(src);
    init(ESVMInstance::currentInstance(), src, len);
}

InternalAtomicString::InternalAtomicString(const char* src, size_t len)
{
    init(ESVMInstance::currentInstance(), src, len);
}

InternalAtomicString::InternalAtomicString(ESVMInstance* instance,  const char* src)
{
    size_t len = strlenT(src);
    init(instance, src, len);
}

InternalAtomicString::InternalAtomicString(ESVMInstance* instance, const char* src, size_t len)
{
    init(instance, src, len);
}


void InternalAtomicString::init(ESVMInstance* instance, const char* src, size_t len)
{
    ASSERT(instance);
    auto iter = instance->m_atomicStringMap.find(std::make_pair(src, len));
    if (iter == instance->m_atomicStringMap.end()) {
        ASCIIString s(src, &src[len]);
        ESString* newData = ESString::create(std::move(s));
        instance->m_atomicStringMap.insert(std::make_pair(std::make_pair(newData->asciiData(), len), newData));
        m_string = newData;
    } else {
        m_string = iter->second;
    }
}


void InternalAtomicString::init(ESVMInstance* instance, const char16_t* src, size_t u16len)
{
    ASSERT(instance);
    if (isAllASCII(src, u16len)) {
        char* abuf;
        ALLOCA_WRAPPER(instance, abuf, char*, u16len, true);
        for (unsigned i = 0 ; i < u16len ; i ++) {
            abuf[i] = src[i];
        }
        init(instance, abuf, u16len);
        return;
    }
    size_t siz;
    const char* buf = utf16ToUtf8(src, u16len, &siz);
    siz--;
    auto iter = instance->m_atomicStringMap.find(std::make_pair(buf, siz));
    if (iter == instance->m_atomicStringMap.end()) {
        UTF16String s(src, &src[u16len]);
        ESString* newData = ESString::create(std::move(s));
        instance->m_atomicStringMap.insert(std::make_pair(std::make_pair(buf, siz), newData));
        m_string = newData;
    } else {
        m_string = iter->second;
    }
}

}
