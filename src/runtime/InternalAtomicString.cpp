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

template <typename T>
static void toUTF16(const char* src, size_t len, T fn)
{
    char16_t* out = (char16_t *)alloca(len * sizeof(char16_t));
    for (unsigned i = 0; i < len ; i ++) {
        out[i] = src[i];
    }
    fn(out, len);
}

InternalAtomicString::InternalAtomicString(const char* src)
{
    size_t len = strlenT(src);
    toUTF16(src, len, [&](char16_t* buf, unsigned len) {
        init(ESVMInstance::currentInstance(), buf, len);
    });

}

InternalAtomicString::InternalAtomicString(const char* src, size_t len)
{
    toUTF16(src, len, [&](char16_t* buf, unsigned len) {
        init(ESVMInstance::currentInstance(), buf, len);
    });
}

InternalAtomicString::InternalAtomicString(ESVMInstance* instance,  const char* src)
{
    size_t len = strlenT(src);
    toUTF16(src, len, [&](char16_t* buf, unsigned len) {
        init(instance, buf, len);
    });
}

InternalAtomicString::InternalAtomicString(ESVMInstance* instance, const char* src, size_t len)
{
    toUTF16(src, len, [&](char16_t* buf, unsigned len) {
        init(instance, buf, len);
    });
}

void InternalAtomicString::init(ESVMInstance* instance, const char16_t* src, size_t len)
{
    ASSERT(instance);
    auto iter = instance->m_atomicStringMap.find(std::make_pair(src, len));
    if (iter == instance->m_atomicStringMap.end()) {
        u16string s(src, &src[len]);
        ESString* newData = ESString::create(std::move(s));
        instance->m_atomicStringMap.insert(std::make_pair(std::make_pair(newData->utf16Data(), len), newData));
        m_string = newData;
    } else {
        m_string = iter->second;
    }
}

}
