#ifndef ESVMInstance_h
#define ESVMInstance_h

#include "runtime/GlobalObject.h"
#include "runtime/InternalAtomicString.h"

namespace WTF {
class BumpPointerAllocator;
}

namespace escargot {

class ExecutionContext;
class GlobalObject;
class ESVMInstance;

extern __thread ESVMInstance* currentInstance;

typedef std::unordered_map<u16string, InternalAtomicStringData *,
        std::hash<u16string>, std::equal_to<u16string>,
        gc_allocator<std::pair<const u16string, InternalAtomicStringData *>> > InternalAtomicStringMap;

class ESVMInstance : public gc_cleanup {
    friend class ESFunctionObject;
public:
    ESVMInstance();
    ~ESVMInstance();
    ESValue evaluate(u16string& source);

    ALWAYS_INLINE ExecutionContext* currentExecutionContext() { return m_currentExecutionContext; }
    ALWAYS_INLINE GlobalObject* globalObject() { return m_globalObject; }

    void enter();
    void exit();
    ALWAYS_INLINE static ESVMInstance* currentInstance()
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

    ALWAYS_INLINE size_t identifierCacheInvalidationCheckCount()
    {
        return m_identifierCacheInvalidationCheckCount;
    }

    ALWAYS_INLINE void invalidateIdentifierCacheCheckCount()
    {
        m_identifierCacheInvalidationCheckCount ++;
        if(UNLIKELY(m_identifierCacheInvalidationCheckCount == SIZE_MAX)) {
            m_identifierCacheInvalidationCheckCount = 0;
        }
    }

    ALWAYS_INLINE ESAccessorData* object__proto__AccessorData() { return &m_object__proto__AccessorData; }
    ALWAYS_INLINE ESAccessorData* functionPrototypeAccessorData() { return &m_functionPrototypeAccessorData; }
    ALWAYS_INLINE ESAccessorData* arrayLengthAccessorData() { return &m_arrayLengthAccessorData; }
    ALWAYS_INLINE ESAccessorData* stringObjectLengthAccessorData() { return &m_stringObjectLengthAccessorData; }

    ALWAYS_INLINE ESFunctionObject* globalFunctionPrototype() { return m_globalFunctionPrototype; }
    ALWAYS_INLINE void setGlobalFunctionPrototype(ESFunctionObject* o) { m_globalFunctionPrototype = o; }

    ALWAYS_INLINE WTF::BumpPointerAllocator* bumpPointerAllocator() { return m_bumpPointerAllocator; };

    int timezoneOffset();
    const tm* computeLocalTime(const timespec& ts);

    ALWAYS_INLINE ESHiddenClass* initialHiddenClassForObject()
    {
        return &m_initialHiddenClassForObject;
    }

    ALWAYS_INLINE ESHiddenClass* initialHiddenClassForFunction()
    {
        return &m_initialHiddenClassForFunction;
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

    ESHiddenClass m_initialHiddenClassForObject;
    ESHiddenClass m_initialHiddenClassForFunction;

    ESAccessorData m_object__proto__AccessorData;
    ESAccessorData m_functionPrototypeAccessorData;
    ESAccessorData m_arrayLengthAccessorData;
    ESAccessorData m_stringObjectLengthAccessorData;

    ESFunctionObject* m_globalFunctionPrototype;

    WTF::BumpPointerAllocator* m_bumpPointerAllocator;

    timespec m_cachedTimeOrigin;
    tm* m_cachedTime;
    tm m_time;
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
