#ifndef ExecutionContext_h
#define ExecutionContext_h

class JSFunction;
class LexicalEnvironment;
class ExecutionContext {
public:
    ExecutionContext(LexicalEnvironment* lexEnv, LexicalEnvironment* varEnv);
    LexicalEnvironment* currentEnvironment();
    //ResolveBinding(name, [env])
    //GetThisEnvironment()

private:
    JSFunction* function;
    LexicalEnvironment* lexicalEnv;
    LexicalEnvironment* variableEnv;
};

#endif
