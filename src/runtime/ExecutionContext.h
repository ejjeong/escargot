#ifndef ExecutionContext_h
#define ExecutionContext_h

#include "ESValue.h"
#include "bytecode/ByteCode.h"

namespace escargot {

struct jmpbuf_wrapper {
    std::jmp_buf m_buffer;
};

class LexicalEnvironment;
class ExecutionContext : public gc {
public:
    ExecutionContext(LexicalEnvironment* varEnv, bool needsActivation, bool isNewExpression,
        ExecutionContext* callerContext,
        ESValue* arguments = NULL, size_t argumentsCount = 0,
        ESValue* cachedDeclarativeEnvironmentRecord = NULL
        );
    ALWAYS_INLINE LexicalEnvironment* environment()
    {
        //TODO
        return m_variableEnvironment;
    }

    ALWAYS_INLINE void setEnvironment(LexicalEnvironment* env)
    {
        //TODO
        ASSERT(env);
        m_variableEnvironment = env;
    }

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvebinding
    ESValue* resolveBinding(const InternalAtomicString& atomicName);

    ESValue* resolveArgumentsObjectBinding();

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvethisbinding
    ESValue resolveThisBinding();
    ESObject* resolveThisBindingToObject()
    {
        return resolveThisBinding().toObject();
    }

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-getthisenvironment
    LexicalEnvironment* getThisEnvironment();

    ALWAYS_INLINE bool needsActivation() { return m_needsActivation; } //child & parent AST has eval, with, catch
    ALWAYS_INLINE bool isNewExpression() { return m_isNewExpression; }
    ESValue* arguments() { return m_arguments; }
    size_t argumentCount() { return m_argumentCount; }
    ESValue readArgument(size_t idx)
    {
        if(idx < argumentCount()) {
            return m_arguments[idx];
        } else {
            return ESValue();
        }
    }

    ESValue* cachedDeclarativeEnvironmentRecordESValue()
    {
        return m_cachedDeclarativeEnvironmentRecord;
    }

    bool isStrictMode() { return m_isStrict; }
    void setStrictMode(bool s) { m_isStrict = s; }

    ExecutionContext* callerContext() { return m_callerContext; }

#ifdef ENABLE_ESJIT
    bool inOSRExit() { return m_inOSRExit; }
    char* getBp() { return m_stackBuf; }
    void setBp(char* bp) { m_stackBuf = bp; }
    unsigned getStackPos() { return m_stackPos; }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
    static size_t offsetOfArguments() { return offsetof(ExecutionContext, m_arguments); }
    static size_t offsetofInOSRExit() { return offsetof(ExecutionContext, m_inOSRExit); }
    static size_t offsetofStackBuf() { return offsetof(ExecutionContext, m_stackBuf); }
    static size_t offsetofStackPos() { return offsetof(ExecutionContext, m_stackPos); }
    static size_t offsetofcachedDeclarativeEnvironmentRecordESValue()
    {
        return offsetof(ExecutionContext, m_cachedDeclarativeEnvironmentRecord);
    }
    static size_t offsetOfEnvironment() { return offsetof(ExecutionContext, m_variableEnvironment); }
#pragma GCC diagnostic pop
#endif

    ESValue& tryOrCatchBodyResult() { return m_tryOrCatchBodyResult; }
    private:
    bool m_needsActivation;
    bool m_isNewExpression;
    bool m_isStrict;

    ExecutionContext* m_callerContext;

    ESValue* m_arguments;
    size_t m_argumentCount;

    LexicalEnvironment* m_lexicalEnvironment;
    LexicalEnvironment* m_variableEnvironment;

    ESValue* m_cachedDeclarativeEnvironmentRecord;
    //instance->currentExecutionContext()->environment()->record()->toDeclarativeEnvironmentRecord()

    ESValue m_tryOrCatchBodyResult;
#ifdef ENABLE_ESJIT
    bool m_inOSRExit;
    unsigned m_stackPos;
    char* m_stackBuf;
#endif
};

}

#endif

