#include "Escargot.h"
#include "bytecode/ByteCode.h"

namespace escargot {

NEVER_INLINE EnumerateObjectData* executeEnumerateObject(ESObject* obj)
{
    EnumerateObjectData* data = new EnumerateObjectData();
    data->m_object = obj;
    data->m_keys.reserve(obj->keyCount());
    ESObject* target = obj;
    std::unordered_set<ESString*, std::hash<ESString*>, std::equal_to<ESString*>, gc_allocator<ESString *> > keyStringSet;
    target->enumeration([&data, &keyStringSet](ESValue key) {
        data->m_keys.push_back(key);
        keyStringSet.insert(key.toString());
    });
    ESValue proto = target->__proto__();
    while(proto.isESPointer() && proto.asESPointer()->isESObject()) {
        target = proto.asESPointer()->asESObject();
        target->enumeration([&data, &keyStringSet](ESValue key) {
            ESString* str = key.toString();
            if(keyStringSet.find(str) != keyStringSet.end()) {
                data->m_keys.push_back(key);
                keyStringSet.insert(str);
            }
        });
        proto = target->__proto__();
    }

    return data;
}

}
