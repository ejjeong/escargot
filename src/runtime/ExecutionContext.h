#ifndef ExecutionContext_h
#define ExecutionContext_h

#include "ESValue.h"

namespace escargot {

class ReferenceError {
public:
    ReferenceError()
    {
        m_identifier = L"";
    }
    ReferenceError(const InternalString& identifier)
    {
        m_identifier = identifier;
    }

    const InternalString& identifier() { return m_identifier; }

protected:
    InternalString m_identifier;
};

class TypeError {

};

class LexicalEnvironment;
class ExecutionContext : public gc {
public:
    ExecutionContext(LexicalEnvironment* varEnv);
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
    ESSlot* resolveBinding(const InternalAtomicString& name);

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

    ALWAYS_INLINE const InternalAtomicString& lastUsedPropertyNameInMemberExpressionNode()
    {
        return m_lastUsedPropertyNameInMemberExpressionNode;
    }

    ALWAYS_INLINE ESValue* lastUsedPropertyValueInMemberExpressionNode()
    {
        return m_lastUsedPropertyValueInMemberExpressionNode;
    }

    ALWAYS_INLINE void setLastESObjectMetInMemberExpressionNode(ESObject* obj, const InternalAtomicString& name, ESValue* value)
    {
        m_lastESObjectMetInMemberExpressionNode = obj;
        m_lastUsedPropertyNameInMemberExpressionNode = name;
        m_lastUsedPropertyValueInMemberExpressionNode = value;
    }

    void doReturn(ESValue* returnValue)
    {
        m_returnValue = returnValue;
        std::longjmp(m_returnPosition,1);
    }

    std::jmp_buf& returnPosition() { return m_returnPosition; }
    ESValue* returnValue()
    {
        return m_returnValue;
    }

private:
    ESFunctionObject* m_function;
    LexicalEnvironment* m_lexicalEnvironment;
    LexicalEnvironment* m_variableEnvironment;
    ESObject* m_lastESObjectMetInMemberExpressionNode;
    InternalAtomicString m_lastUsedPropertyNameInMemberExpressionNode;
    ESValue* m_lastUsedPropertyValueInMemberExpressionNode;
    ESValue* m_returnValue;
    std::jmp_buf m_returnPosition;
};

}

#endif
