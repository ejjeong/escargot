#include "Escargot.h"
#include "InternalAtomicString.h"

#include "vm/ESVMInstance.h"

namespace escargot {

InternalAtomicStringData emptyInternalAtomicString;

InternalAtomicStringData::InternalAtomicStringData()
{
    m_instance = NULL;
    initHash();
}
InternalAtomicStringData::InternalAtomicStringData(ESVMInstance* instance, const wchar_t* str)
    : std::wstring(str)
{
    m_instance = instance;
    initHash();
}

InternalAtomicStringData::~InternalAtomicStringData()
{
    if(m_instance) {
        auto iter = m_instance->m_atomicStringMap.find(*this);
        ASSERT(iter != m_instance->m_atomicStringMap.end());
        m_instance->m_atomicStringMap.erase(iter);
    }
}

InternalAtomicString::InternalAtomicString(const std::wstring& src)
    : InternalAtomicString(ESVMInstance::currentInstance(), src)
{
}

InternalAtomicString::InternalAtomicString(const wchar_t* src)
    : InternalAtomicString(ESVMInstance::currentInstance(), std::wstring(src))
{
}

InternalAtomicString::InternalAtomicString(const ESValue* src)
{
    //wprintf(L"%ls\n", src->toInternalString().data());
    if(src->isInt32()) {
        int val = src->asInt32();
        if(val >= 0 && val < ESCARGOT_STRINGS_NUMBERS_MAX) {
            *this = strings->numbers[val];
        }
    }
    init(ESVMInstance::currentInstance(), src->toString()->data());
}

InternalAtomicString::InternalAtomicString(ESVMInstance* instance, const std::wstring& src)
{
    init(instance, src);
}

void InternalAtomicString::init(ESVMInstance* instance, const std::wstring& src)
{
    ASSERT(instance);
    auto iter = instance->m_atomicStringMap.find(src);
    if(iter == instance->m_atomicStringMap.end()) {
        InternalAtomicStringData* newData = new InternalAtomicStringData(instance, src.data());
        instance->m_atomicStringMap[src] = newData;
        m_string = newData;
    } else {
        m_string = iter->second;
    }
}

}
