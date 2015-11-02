#include "Escargot.h"
#include "InternalAtomicString.h"

#include "vm/ESVMInstance.h"

namespace escargot {

InternalAtomicString::InternalAtomicString(u16string& src)
    : InternalAtomicString(ESVMInstance::currentInstance(), src)
{
}

InternalAtomicString::InternalAtomicString(const char16_t* src)
{
    u16string s(src);
    init(ESVMInstance::currentInstance(), s);
}

InternalAtomicString::InternalAtomicString(ESVMInstance* instance, const u16string& src)
{
    init(instance, src);
}

void InternalAtomicString::init(ESVMInstance* instance, const u16string& src)
{
    ASSERT(instance);
    auto iter = instance->m_atomicStringMap.find(src);
    if(iter == instance->m_atomicStringMap.end()) {
        ESString* newData = ESString::create(src);
        instance->m_atomicStringMap.insert(std::make_pair(src, newData));
        m_string = newData;
    } else {
        m_string = iter->second;
    }
}

}

