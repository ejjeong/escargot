#ifndef ESValue_h
#define ESValue_h

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
};

class JSArray : public JSObject {
};

class LexicalEnvironment;
class AST;
class JSFunction : public JSObject {
public:
    JSFunction(LexicalEnvironment* , AST* );
protected:
    LexicalEnvironment* m_outerEnvironment;
    AST* m_body;
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
