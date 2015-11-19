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

void InternalAtomicString::init(ESVMInstance* instance, const char16_t* src, size_t len)
{
    ASSERT(instance);
    auto iter = instance->m_atomicStringMap.find(std::make_pair(src, len));
    if (iter == instance->m_atomicStringMap.end()) {
        u16string s(src, &src[len]);
        ESString* newData = ESString::create(std::move(s));
        instance->m_atomicStringMap.insert(std::make_pair(std::make_pair(newData->data(), len), newData));
        m_string = newData;
    } else {
        m_string = iter->second;
    }
}

}
