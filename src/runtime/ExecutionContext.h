#ifndef ExecutionContext_h
#define ExecutionContext_h

#include "ESValue.h"
#include "bytecode/ByteCode.h"

namespace escargot {

class LexicalEnvironment;
class ExecutionContext : public gc {
public:
    ALWAYS_INLINE ExecutionContext(LexicalEnvironment* varEnv, bool isNewExpression, bool isStrictMode,
        ESValue* arguments = NULL, size_t argumentsCount = 0)
            : m_thisValue(ESValue::ESForceUninitialized)
            , m_tryOrCatchBodyResultOrArgumentsInfo(ESValue::ESForceUninitialized)
    {
        ASSERT(varEnv);
        m_data.m_isNewExpression = isNewExpression;
        m_data.m_isStrict = isStrictMode;
        m_environment = varEnv;
        m_tryOrCatchBodyResultOrArgumentsInfo = (ESPointer *)arguments;
        m_data.m_argumentCount = argumentsCount;
#ifdef ENABLE_ESJIT
        m_inOSRExit = false;
        m_executeNextByteCode = false;
#endif
    }

    ALWAYS_INLINE LexicalEnvironment* environment()
    {
        // TODO
        return m_environment;
    }

    ALWAYS_INLINE void setEnvironment(LexicalEnvironment* env)
    {
        // TODO
        ASSERT(env);
        m_environment = env;
    }

    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvebinding
    ESValue* resolveBinding(const InternalAtomicString& atomicName);
    ESValue* resolveBinding(const InternalAtomicString& atomicName, LexicalEnvironment*& env);

    ESValue* resolveArgumentsObjectBinding();

    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvethisbinding
    void setThisBinding(const ESValue& v)
    {
        m_thisValue = v;
    }

    ESValue resolveThisBinding()
    {
        return m_thisValue;
    }

    ESObject* resolveThisBindingToObject()
    {
        return m_thisValue.toObject();
    }

    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-getthisenvironment
    LexicalEnvironment* getThisEnvironment();

    ALWAYS_INLINE bool isNewExpression() { return m_data.m_isNewExpression; }

    // NOTE this argument information is for nativeFunctions. do not use this interpreter of jit
    ESValue* arguments() { return (ESValue *)m_tryOrCatchBodyResultOrArgumentsInfo.asESPointer(); }
    size_t argumentCount() { return m_data.m_argumentCount; }
    ESValue readArgument(size_t idx)
    {
        if (idx < argumentCount()) {
            return arguments()[idx];
        } else {
            return ESValue();
        }
    }

    bool isStrictMode() { return m_data.m_isStrict; }

#ifdef ENABLE_ESJIT
    bool inOSRExit() { return m_inOSRExit; }
    bool executeNextByteCode() { return m_executeNextByteCode; }
    char* getBp() { return m_stackBuf; }
    void setBp(char* bp) { m_stackBuf = bp; }
    unsigned getStackPos() { return m_stackPos; }
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
    static size_t offsetOfArguments() { return offsetof(ExecutionContext, m_arguments); }
    static size_t offsetofInOSRExit() { return offsetof(ExecutionContext, m_inOSRExit); }
    static size_t offsetofExecuteNextByteCode() { return offsetof(ExecutionContext, m_executeNextByteCode); }
    static size_t offsetofStackBuf() { return offsetof(ExecutionContext, m_stackBuf); }
    static size_t offsetofStackPos() { return offsetof(ExecutionContext, m_stackPos); }
    static size_t offsetOfEnvironment() { return offsetof(ExecutionContext, m_environment); }
#pragma GCC diagnostic pop
#endif

    ESValue& tryOrCatchBodyResult() { return m_tryOrCatchBodyResultOrArgumentsInfo; }
private:
    struct {
        bool m_isNewExpression;
        bool m_isStrict;
        uint16_t m_argumentCount;
    } m_data;

    // TODO
    // LexicalEnvironment* m_lexicalEnvironment;
    // LexicalEnvironment* m_variableEnvironment;
    LexicalEnvironment* m_environment;

    ESValue m_thisValue;
    ESValue m_tryOrCatchBodyResultOrArgumentsInfo;
#ifdef ENABLE_ESJIT
    bool m_inOSRExit;
    bool m_executeNextByteCode;
    unsigned m_stackPos;
    char* m_stackBuf;
#endif
};

}

#endif
