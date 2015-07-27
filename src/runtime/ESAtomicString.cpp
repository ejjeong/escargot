#include "Escargot.h"
#include "ESAtomicString.h"

#include "vm/ESVMInstance.h"

namespace escargot {

ESAtomicStringData::~ESAtomicStringData()
{
    auto iter = m_instance->m_atomicStringMap.find(*this);
    ASSERT(iter != m_instance->m_atomicStringMap.end());
    m_instance->m_atomicStringMap.erase(iter);
}

ESAtomicString ESAtomicString::create(ESVMInstance* instance, const std::wstring& src)
{
    auto iter = instance->m_atomicStringMap.find(src);
    if(iter == instance->m_atomicStringMap.end()) {
        ESAtomicStringData* newData = new ESAtomicStringData(instance, src.data());
        instance->m_atomicStringMap[src] = newData;
        return newData;
    } else {
        return iter->second;
    }
}

}
