#ifndef ExecutionContext_h
#define ExecutionContext_h

namespace escargot {

class JSFunction;
class JSObjectSlot;

class LexicalEnvironment;
class ExecutionContext : public gc_cleanup {
public:
    ExecutionContext(LexicalEnvironment* lexEnv, LexicalEnvironment* varEnv);
    LexicalEnvironment* variableEnvironment();

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvebinding
    JSObjectSlot* resolveBinding(const ESString& name);
    //ResolveBinding(name, [env])
    //GetThisEnvironment()

private:
    JSFunction* m_function;
    LexicalEnvironment* m_lexicalEnvironment;
    LexicalEnvironment* m_variableEnvironment;
};

}

#endif
