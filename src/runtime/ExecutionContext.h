#ifndef ExecutionContext_h
#define ExecutionContext_h

#include "ESValue.h"

namespace escargot {

struct jmpbuf_wrapper {
    std::jmp_buf m_buffer;
};

class LexicalEnvironment;
class ExecutionContext : public gc {
public:
    ExecutionContext(LexicalEnvironment* varEnv, bool needsActivation, bool isNewExpression,
            ExecutionContext* callerContext, ESValue* arguments = NULL, size_t argumentsCount = 0,
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
        m_variableEnvironment = env;
    }

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvebinding
    ESSlotAccessor resolveBinding(const InternalAtomicString& atomicName, ESString* name);

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvethisbinding
    ESObject* resolveThisBinding();

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-getthisenvironment
    LexicalEnvironment* getThisEnvironment();

    ALWAYS_INLINE void resetLastESObjectMetInMemberExpressionNode()
    {
        m_lastESObjectMetInMemberExpressionNode = NULL;
    }

    ALWAYS_INLINE ESObject* lastESObjectMetInMemberExpressionNode()
    {
        return m_lastESObjectMetInMemberExpressionNode;
    }

    ALWAYS_INLINE void setLastESObjectMetInMemberExpressionNode(ESObject* obj)
    {
        m_lastESObjectMetInMemberExpressionNode = obj;
    }

    void doReturn(const ESValue& returnValue)
    {
        m_returnValue = returnValue;
        std::longjmp(m_returnPosition,1);
    }

    void setReturnValue(const ESValue& returnValue)
    {
        m_returnValue = returnValue;
    }

    std::jmp_buf& returnPosition() { return m_returnPosition; }

    void doBreak()
    {
        longjmp(m_breakPositions.back().m_buffer, 1);
    }

    void doContinue()
    {
        longjmp(m_continuePositions.back().m_buffer, 1);
    }

    void pushContinuePosition(jmpbuf_wrapper& pos)
    {
        m_continuePositions.push_back(pos);
    }

    void popContinuePosition()
    {
        m_continuePositions.pop_back();
    }

    template <typename T>
    void setJumpPositionAndExecute(const T& fn) {
        jmpbuf_wrapper newone;
        int r = setjmp(newone.m_buffer);
        if (r != 1) {
            m_breakPositions.push_back(newone);
            fn();
        } else {
            m_continuePositions.pop_back();
        }
        m_breakPositions.pop_back();
    }

    ESValue returnValue()
    {
        return m_returnValue;
    }

    ALWAYS_INLINE bool needsActivation() { return m_needsActivation; } //child & parent AST has eval, with, catch
    ALWAYS_INLINE bool isNewExpression() { return m_isNewExpression; }
    ExecutionContext* callerContext() { return m_callerContext; }
    ESValue* arguments() { return m_arguments; }
    size_t argumentCount() { return m_argumentCount; }

    ESValue* cachedDeclarativeEnvironmentRecordESValue()
    {
        return m_cachedDeclarativeEnvironmentRecord;
    }

private:
    ESFunctionObject* m_function;

    bool m_needsActivation;
    bool m_isNewExpression;

    ExecutionContext* m_callerContext;

    ESValue* m_arguments;
    size_t m_argumentCount;

    LexicalEnvironment* m_lexicalEnvironment;
    LexicalEnvironment* m_variableEnvironment;

    ESValue* m_cachedDeclarativeEnvironmentRecord;
    //instance->currentExecutionContext()->environment()->record()->toDeclarativeEnvironmentRecord()

    ESObject* m_lastESObjectMetInMemberExpressionNode;

    ESValue m_returnValue;
    std::jmp_buf m_returnPosition;

    std::vector<jmpbuf_wrapper> m_breakPositions;
    std::vector<jmpbuf_wrapper> m_continuePositions;
};

}

#endif
