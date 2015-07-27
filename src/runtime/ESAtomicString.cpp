#include "Escargot.h"
#include "ESAtomicString.h"

#include "vm/ESVMInstance.h"

namespace escargot {

ESAtomicStringData emptyAtomicString;

ESAtomicStringData::ESAtomicStringData()
{
    m_instance = NULL;
    initHash();
}
ESAtomicStringData::ESAtomicStringData(ESVMInstance* instance, const wchar_t* str)
    : ESStringStd(str)
{
    m_instance = instance;
    initHash();
}

ESAtomicStringData::~ESAtomicStringData()
{
    if(m_instance) {
        auto iter = m_instance->m_atomicStringMap.find(*this);
        ASSERT(iter != m_instance->m_atomicStringMap.end());
        m_instance->m_atomicStringMap.erase(iter);
    }
}

ESAtomicString::ESAtomicString(const std::wstring& src)
    : ESAtomicString(ESVMInstance::currentInstance(), src)
{
}
ESAtomicString::ESAtomicString(const wchar_t* src)
    : ESAtomicString(ESVMInstance::currentInstance(), std::wstring(src))
{
}

ESAtomicString::ESAtomicString(ESVMInstance* instance, const std::wstring& src)
{
    ASSERT(instance);
    auto iter = instance->m_atomicStringMap.find(src);
    if(iter == instance->m_atomicStringMap.end()) {
        ESAtomicStringData* newData = new ESAtomicStringData(instance, src.data());
        instance->m_atomicStringMap[src] = newData;
        m_string = newData;
    } else {
        m_string = iter->second;
    }
}

}
