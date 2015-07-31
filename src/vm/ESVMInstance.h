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
    void evaluate(const std::string& source);

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
protected:
    ExecutionContext* m_globalExecutionContext;
    ExecutionContext* m_currentExecutionContext;
    GlobalObject* m_globalObject;

    friend class InternalAtomicString;
    friend class InternalAtomicStringData;
    InternalAtomicStringMap m_atomicStringMap;

    Strings m_strings;
    size_t m_identifierCacheInvalidationCheckCount;
};

}

#endif
