#ifndef ExecutionContext_h
#define ExecutionContext_h

namespace escargot {

class JSFunction;
class LexicalEnvironment;
class ExecutionContext : public gc_cleanup {
public:
    ExecutionContext(LexicalEnvironment* lexEnv, LexicalEnvironment* varEnv);
    LexicalEnvironment* currentEnvironment();
    //ResolveBinding(name, [env])
    //GetThisEnvironment()

private:
    JSFunction* m_function;
    LexicalEnvironment* m_lexicalEnv;
    LexicalEnvironment* m_variableEnv;
};

}

#endif
