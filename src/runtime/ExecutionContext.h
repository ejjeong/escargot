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
    ReferenceError(const ESString& identifier)
    {
        m_identifier = identifier;
    }

    const ESString& identifier() { return m_identifier; }

protected:
    ESString m_identifier;
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

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvebinding
    JSSlot* resolveBinding(const ESString& name);

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-resolvethisbinding
    JSObject* resolveThisBinding();

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-getthisenvironment
    LexicalEnvironment* getThisEnvironment();

    ALWAYS_INLINE void resetLastJSObjectMetInMemberExpressionNode()
    {
        m_lastJSObjectMetInMemberExpressionNode = NULL;
    }

    ALWAYS_INLINE JSObject* lastJSObjectMetInMemberExpressionNode()
    {
        return m_lastJSObjectMetInMemberExpressionNode;
    }

    ALWAYS_INLINE const ESString& lastUsedPropertyNameInMemberExpressionNode()
    {
        return m_lastUsedPropertyNameInMemberExpressionNode;
    }

    ALWAYS_INLINE ESValue* lastUsedPropertyValueInMemberExpressionNode()
    {
        return m_lastUsedPropertyValueInMemberExpressionNode;
    }

    ALWAYS_INLINE void setLastJSObjectMetInMemberExpressionNode(JSObject* obj, const ESString& name, ESValue* value)
    {
        m_lastJSObjectMetInMemberExpressionNode = obj;
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
    JSFunction* m_function;
    LexicalEnvironment* m_lexicalEnvironment;
    LexicalEnvironment* m_variableEnvironment;
    JSObject* m_lastJSObjectMetInMemberExpressionNode;
    ESString m_lastUsedPropertyNameInMemberExpressionNode;
    ESValue* m_lastUsedPropertyValueInMemberExpressionNode;
    ESValue* m_returnValue;
    std::jmp_buf m_returnPosition;
};

}

#endif
