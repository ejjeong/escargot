#ifndef ExecutionContext_h
#define ExecutionContext_h

#include "ESValue.h"

namespace escargot {

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

    ALWAYS_INLINE void resetLastJSObjectMetInMemberExpressionNode()
    {
        m_lastJSObjectMetInMemberExpressionNode = NULL;
        m_lastUsedNameInMemberExpressionNode = undefined;
    }

    ALWAYS_INLINE JSObject* lastJSObjectMetInMemberExpressionNode()
    {
        return m_lastJSObjectMetInMemberExpressionNode;
    }

    ALWAYS_INLINE ESValue* resetLastUsedNameInMemberExpressionNode()
    {
        return m_lastUsedNameInMemberExpressionNode;
    }

    ALWAYS_INLINE void setLastJSObjectMetInMemberExpressionNode(JSObject* obj, ESValue* name)
    {
        m_lastJSObjectMetInMemberExpressionNode = obj;
        m_lastUsedNameInMemberExpressionNode = name;
    }


private:
    JSFunction* m_function;
    LexicalEnvironment* m_lexicalEnvironment;
    LexicalEnvironment* m_variableEnvironment;
    JSObject* m_lastJSObjectMetInMemberExpressionNode;
    ESValue* m_lastUsedNameInMemberExpressionNode;
};

}

#endif
