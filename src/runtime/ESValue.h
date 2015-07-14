#ifndef ESValue_h
#define ESValue_h
#include <unordered_map>

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
    JSObject();
    ESValue* getValue(std::wstring key);
    void setValue(std::wstring key, ESValue* val, bool throw_flag);

protected:
    std::unordered_map<std::wstring, ESValue*> m_map;
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
