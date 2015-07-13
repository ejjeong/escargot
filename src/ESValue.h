#ifndef ESValue_h
#define ESValue_h

class ESValue {
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
    JSFunction(LexicalEnvironment*, AST*);
    LexicalEnvironment* outerEnvironment;
    AST* body;
    enum ThisBindingStatus {
        lexical, initialized, uninitialized
    };
    ThisBindingStatus thisBindingStatus;
    //JSObject functionObject;
    //HomeObject
    ////JSObject newTarget
    //BindThisValue(V);
    //GetThisBinding();
};

#endif
