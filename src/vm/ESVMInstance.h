#ifndef ESVMInstance_h
#define ESVMInstance_h

#include "runtime/GlobalObject.h"
#include "runtime/InternalAtomicString.h"

namespace escargot {

class ExecutionContext;
class GlobalObject;
class ESVMInstance;

extern __thread ESVMInstance* currentInstance;

typedef std::unordered_map<std::wstring, InternalAtomicStringData *,
        std::hash<std::wstring>,std::equal_to<std::wstring> > InternalAtomicStringMap;

class ESVMInstance : public gc {
    friend class ESFunctionObject;
public:
    ESVMInstance();
    ESValue evaluate(const std::string& source);

    ALWAYS_INLINE ExecutionContext* currentExecutionContext() { return m_currentExecutionContext; }
    GlobalObject* globalObject() { return m_globalObject; }

    void enter();
    void exit();
    static ESVMInstance* currentInstance()
    {
        return escargot::currentInstance;
    }

    ALWAYS_INLINE Strings& strings() { return m_strings; }

    template <typename F>
    void runOnGlobalContext(const F& f)
    {
        ExecutionContext* ctx = m_currentExecutionContext;
        m_currentExecutionContext = m_globalExecutionContext;
        f();
        m_currentExecutionContext = ctx;
    }

    template <typename F>
    ESValue runOnEvalContext(const F& f, bool isDirectCall);

    size_t identifierCacheInvalidationCheckCount()
    {
        return m_identifierCacheInvalidationCheckCount;
    }

    void invalidateIdentifierCacheCheckCount()
    {
        m_identifierCacheInvalidationCheckCount ++;
        if(UNLIKELY(m_identifierCacheInvalidationCheckCount == SIZE_MAX)) {
            m_identifierCacheInvalidationCheckCount = 0;
        }
    }

    ESAccessorData* object__proto__AccessorData() { return &m_object__proto__AccessorData; }
    ESAccessorData* functionPrototypeAccessorData() { return &m_functionPrototypeAccessorData; }
    ESAccessorData* arrayLengthAccessorData() { return &m_arrayLengthAccessorData; }
    ESAccessorData* stringLengthAccessorData() { return &m_stringLengthAccessorData; }

protected:
    ExecutionContext* m_globalExecutionContext;
    ExecutionContext* m_currentExecutionContext;
    GlobalObject* m_globalObject;

    friend class InternalAtomicString;
    friend class InternalAtomicStringData;
    InternalAtomicStringMap m_atomicStringMap;

    Strings m_strings;
    size_t m_identifierCacheInvalidationCheckCount;

    ESAccessorData m_object__proto__AccessorData;
    ESAccessorData m_functionPrototypeAccessorData;
    ESAccessorData m_arrayLengthAccessorData;
    ESAccessorData m_stringLengthAccessorData;
};

}

#include "runtime/ExecutionContext.h"

namespace escargot {

template <typename F>
ESValue ESVMInstance::runOnEvalContext(const F& f, bool isDirectCall)
{
    ExecutionContext* ctx = m_currentExecutionContext;
    ESValue ret;
    if (!ctx || !isDirectCall) {
        m_currentExecutionContext = m_globalExecutionContext;
        ret = f();
    } else {
        ExecutionContext* caller = ctx->callerContext();
        ExecutionContext ec(caller->environment(), caller->needsActivation(), false, caller);
        m_currentExecutionContext = &ec;
        //m_currentExecutionContext = m_currentExecutionContext->callerContext();
        ret = f();
    }
    m_currentExecutionContext = ctx;
    return ret;
}

}

#endif
