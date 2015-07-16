#ifndef ExecutionContext_h
#define ExecutionContext_h

namespace escargot {

class JSFunction;
class JSObjectSlot;

class LexicalEnvironment;
class ExecutionContext : public gc_cleanup {
public:
    ExecutionContext(LexicalEnvironment* varEnv);
    ALWAYS_INLINE LexicalEnvironment* environment()
    {
        //TODO
        return m_variableEnvironment;
    }

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvebinding
    JSObjectSlot* resolveBinding(const ESString& name);

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-execution-contexts
    //GetThisEnvironment()

private:
    JSFunction* m_function;
    LexicalEnvironment* m_lexicalEnvironment;
    LexicalEnvironment* m_variableEnvironment;
};

}

#endif
