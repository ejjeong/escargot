#ifndef ESValue_h
#define ESValue_h
#include <unordered_map>

namespace escargot {

class ESValue {
    void* operator new(size_t, void* p) = delete;
    void* operator new[](size_t, void* p) = delete;
    void* operator new(size_t size) = delete;
    void* operator new[](size_t size) = delete;
};

class HeapObject {
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
    ESValue getValue(std::wstring key);
    void setValue(std::wstring key, ESValue val, bool throw_flag);

protected:
    std::unordered_map<std::wstring, ESValue> m_map;
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
