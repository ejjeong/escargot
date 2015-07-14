#ifndef ESValue_h
#define ESValue_h
#include <unordered_map>
#include "ESString.h"

namespace escargot {

class Smi;
class HeapObject;

class ESValue {
public:
    void* operator new(size_t, void* p) = delete;
    void* operator new[](size_t, void* p) = delete;
    void* operator new(size_t size) = delete;
    void* operator new[](size_t size) = delete;

    bool isSmi() const;
    bool isHeapObject() const;
    Smi* toSmi() const;
    HeapObject* toHeapObject() const;
};

class Smi : public ESValue {
public:
    int getValue();
    static inline Smi* fromInt(int value);
    static inline Smi* fromIntptr(intptr_t value);
};

class HeapObject : public ESValue {
};

class Null : public HeapObject {
};

class Undefined : public HeapObject {
};

class Boolean : public HeapObject {
};

class Number : public HeapObject {
};

class String : public HeapObject {
};

class JSObject : public HeapObject {
public:
    struct PropertyDescriptor {
        ESValue m_value;
        bool m_isWritable;
        bool m_isEnumerable;
        bool m_isConfigurable;
    };
    ESValue* getValue(std::wstring key);
    void setValue(const ESString& key, ESValue val, bool throwFlag) {}
    void definePropertyOrThrow(ESString key, PropertyDescriptor pd) {}
    bool hasOwnProperty(const ESString& key) {
        return true;
    }

    //$6.1.7.2 Object Internal Methods and Internal Slots
    bool isExtensible() {
        return true;
    }

    PropertyDescriptor getOwnProperty(const ESString& ekey) {
        std::wstring key = std::wstring(ESString(ekey).data());
        std::unordered_map<std::wstring, PropertyDescriptor>::iterator it = m_map.find(key);
        if(it != m_map.end())
            return it->second;
        //FIXME
        PropertyDescriptor pd_undefined;
        return pd_undefined;
    }

protected:
    std::unordered_map<std::wstring, PropertyDescriptor> m_map;
};

class JSArray : public JSObject {
};

class LexicalEnvironment;
class Node;
class JSFunction : public JSObject {
public:
    JSFunction(LexicalEnvironment* , Node* );
protected:
    LexicalEnvironment* m_outerEnvironment;
    Node* m_body;
    enum ThisBindingStatus {
        lexical, initialized, uninitialized
    };
    ThisBindingStatus m_thisBindingStatus;
    //JSObject functionObject;
    //HomeObject
    ////JSObject newTarget
    //BindThisValue(V);
    //GetThisBinding();
};

}

#endif
