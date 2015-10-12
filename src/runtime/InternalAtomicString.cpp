#include "Escargot.h"
#include "InternalAtomicString.h"

#include "vm/ESVMInstance.h"

namespace escargot {

InternalAtomicStringData::InternalAtomicStringData(ESVMInstance* instance, const char16_t* str)
{
    m_instance = instance;
    m_string = ESString::create(std::move(u16string(str)));
}

InternalAtomicStringData::~InternalAtomicStringData()
{
    if(m_instance) {
        auto iter = m_instance->m_atomicStringMap.find(m_string->string());
        ASSERT(iter != m_instance->m_atomicStringMap.end());
        m_instance->m_atomicStringMap.erase(iter);
    }
}

InternalAtomicString::InternalAtomicString(const u16string& src)
    : InternalAtomicString(ESVMInstance::currentInstance(), src)
{
}

InternalAtomicString::InternalAtomicString(const char16_t* src)
    : InternalAtomicString(ESVMInstance::currentInstance(), u16string(src))
{
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
        InternalAtomicStringData* newData = new InternalAtomicStringData(instance, src.data());
        instance->m_atomicStringMap.insert(std::make_pair(src, newData));
        m_string = newData;
    } else {
        m_string = iter->second;
    }
}

}
