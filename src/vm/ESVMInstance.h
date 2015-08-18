#ifndef ESVMInstance_h
#define ESVMInstance_h

#include "runtime/GlobalObject.h"
#include "runtime/InternalAtomicString.h"

namespace escargot {

class ExecutionContext;
class GlobalObject;
class ESVMInstance;

extern __thread ESVMInstance* currentInstance;

typedef std::unordered_map<u16string, InternalAtomicStringData *,
        std::hash<u16string>,std::equal_to<u16string> > InternalAtomicStringMap;

class ESVMInstance : public gc_cleanup {
    friend class ESFunctionObject;
public:
    ESVMInstance();
    ~ESVMInstance();
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
    ESAccessorData* stringObjectLengthAccessorData() { return &m_stringObjectLengthAccessorData; }

    ESFunctionObject* globalFunctionPrototype() { return m_globalFunctionPrototype; }
    void setGlobalFunctionPrototype(ESFunctionObject* o) { m_globalFunctionPrototype = o; }

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
    ESAccessorData m_stringObjectLengthAccessorData;

    ESFunctionObject* m_globalFunctionPrototype;
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
        ExecutionContext ec(caller->environment(), true, false, caller);
        m_currentExecutionContext = &ec;
        //m_currentExecutionContext = m_currentExecutionContext->callerContext();
        ret = f();
    }
    m_currentExecutionContext = ctx;
    return ret;
}

}

#endif
