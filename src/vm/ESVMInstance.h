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
class CodeBlock;
class OpcodeTable;

extern __thread ESVMInstance* currentInstance;

typedef std::unordered_map<u16string, InternalAtomicStringData *,
        std::hash<u16string>, std::equal_to<u16string>,
        gc_allocator<std::pair<const u16string, InternalAtomicStringData *>> > InternalAtomicStringMap;

class ESVMInstance : public gc_cleanup {
    friend ESValue interpret(ESVMInstance* instance, CodeBlock* codeBlock, size_t programCounter);
    friend class ESFunctionObject;
    friend class ExpressionStatementNode;
    friend class TryStatementNode;
public:
    ESVMInstance();
    ~ESVMInstance();
    ESValue evaluate(u16string& source);

    ALWAYS_INLINE ExecutionContext* currentExecutionContext() { return m_currentExecutionContext; }
    ALWAYS_INLINE ExecutionContext* globalExecutionContext() { return m_globalExecutionContext; }
    ALWAYS_INLINE GlobalObject* globalObject() { return m_globalObject; }

    void enter();
    void exit();
    ALWAYS_INLINE static ESVMInstance* currentInstance()
    {
        return escargot::currentInstance;
    }

    ALWAYS_INLINE OpcodeTable* opcodeTable() { return m_table; }
    ALWAYS_INLINE Strings& strings() { return m_strings; }

    template <typename F>
    void runOnGlobalContext(const F& f)
    {
        ExecutionContext* ctx = m_currentExecutionContext;
        m_currentExecutionContext = m_globalExecutionContext;
        invalidateIdentifierCacheCheckCount();
        f();
        m_currentExecutionContext = ctx;
    }

    template <typename F>
    ESValue runOnEvalContext(const F& f, bool isDirectCall);

    ALWAYS_INLINE const unsigned& identifierCacheInvalidationCheckCount()
    {
        return m_identifierCacheInvalidationCheckCount;
    }

    ALWAYS_INLINE void invalidateIdentifierCacheCheckCount()
    {
        m_identifierCacheInvalidationCheckCount ++;
        if(UNLIKELY(m_identifierCacheInvalidationCheckCount == std::numeric_limits<unsigned>::max())) {
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

    ALWAYS_INLINE ESHiddenClass* initialHiddenClassForArrayObject()
    {
        return &m_initialHiddenClassForArrayObject;
    }

    //Function for debug
    static void printValue(ESValue val);
    ALWAYS_INLINE unsigned long tickCount()
    {
        struct timespec timespec;
        clock_gettime(CLOCK_MONOTONIC,&timespec);
        return (unsigned long)(timespec.tv_sec * 1000000L + timespec.tv_nsec/1000);
    }

#ifndef NDEBUG
    bool m_dumpByteCode;
    bool m_dumpExecuteByteCode;
#endif

protected:
    ExecutionContext* m_globalExecutionContext;
    ExecutionContext* m_currentExecutionContext;
    GlobalObject* m_globalObject;

    OpcodeTable* m_table;

    friend class InternalAtomicString;
    friend class InternalAtomicStringData;
    InternalAtomicStringMap m_atomicStringMap;

    Strings m_strings;
    unsigned m_identifierCacheInvalidationCheckCount;

    ESHiddenClass m_initialHiddenClassForObject;
    ESHiddenClass m_initialHiddenClassForFunction;
    ESHiddenClass m_initialHiddenClassForArrayObject;

    ESAccessorData m_object__proto__AccessorData;
    ESAccessorData m_functionPrototypeAccessorData;
    ESAccessorData m_arrayLengthAccessorData;
    ESAccessorData m_stringObjectLengthAccessorData;

    ESFunctionObject* m_globalFunctionPrototype;

    ESValue m_lastExpressionStatementValue;

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
        ret = f();
    }
    m_currentExecutionContext = ctx;
    invalidateIdentifierCacheCheckCount();
    return ret;
}

}

#endif
