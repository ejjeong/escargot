#ifndef ESVMInstance_h
#define ESVMInstance_h

#include "runtime/GlobalObject.h"
#include "runtime/InternalAtomicString.h"

namespace WTF {
class BumpPointerAllocator;
}

#ifdef ENABLE_ESJIT
namespace nanojit {
class Config;
}
#endif

namespace escargot {

class ExecutionContext;
class GlobalObject;
class ESVMInstance;
class CodeBlock;
class OpcodeTable;
class Try;
class ScriptParser;
class ESIdentifierVector;
struct ESSimpleAllocatorMemoryFragment;

#ifndef ANDROID
extern __thread ESVMInstance* currentInstance;
#else
extern ESVMInstance* currentInstance;
#endif

typedef std::unordered_map<std::pair<const char *, size_t>, ESString *,
    std::hash<std::pair<const char *, size_t> >, std::equal_to<std::pair<const char *, size_t> >,
    gc_allocator<std::pair<const std::pair<const char *, size_t>, ESString *> > > InternalAtomicStringMap;

class ESVMInstance : public gc_cleanup {
#ifdef ENABLE_ESJIT
    friend ESValue interpret(ESVMInstance* instance, CodeBlock* codeBlock, size_t programCounter, unsigned maxStackPos);
#else
    friend ESValue interpret(ESVMInstance* instance, CodeBlock* codeBlock, size_t programCounter, ESValue* stackStorage, ESValueVector* heapStorage);
#endif
    friend NEVER_INLINE void tryOperation(ESVMInstance* instance, CodeBlock* codeBlock, char* codeBuffer, ExecutionContext* ec, size_t programCounter, Try* code, ESValue* stackStorage, ESValueVector* heapStorage);
    friend NEVER_INLINE void tryOperationThrowCase(const ESValue& err, LexicalEnvironment* oldEnv, ExecutionContext* backupedEC, ESVMInstance* instance, CodeBlock* codeBlock, char* codeBuffer, ExecutionContext* ec, size_t programCounter, Try* code, ESValue* stackStorage, ESValueVector* heapStorage);
    friend class ESFunctionObject;
    friend class ExpressionStatementNode;
    friend class TryStatementNode;
    friend class ESSimpleAllocator;
public:
    ESVMInstance();
    ~ESVMInstance();
    ESValue evaluate(ESString* source, bool isForGlobalScope = true);

    ALWAYS_INLINE ExecutionContext* currentExecutionContext() { return m_currentExecutionContext; }
    ALWAYS_INLINE ExecutionContext* globalExecutionContext() { return m_globalExecutionContext; }
    ALWAYS_INLINE GlobalObject* globalObject() { return m_globalObject; }

    void enter();
    void exit();
    ALWAYS_INLINE static ESVMInstance* currentInstance()
    {
        return escargot::currentInstance;
    }

    ALWAYS_INLINE OpcodeTable* opcodeTable() { return m_opcodeTable; }
    ALWAYS_INLINE std::unordered_map<void *, unsigned char>& opcodeResverseTable() { return m_opcodeReverseTable; }
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
        m_identifierCacheInvalidationCheckCount++;
        if (UNLIKELY(m_identifierCacheInvalidationCheckCount == std::numeric_limits<unsigned>::max())) {
            m_identifierCacheInvalidationCheckCount = 0;
        }
    }

    ALWAYS_INLINE ESPropertyAccessorData* object__proto__AccessorData() { return &m_object__proto__AccessorData; }
    ALWAYS_INLINE ESPropertyAccessorData* functionPrototypeAccessorData() { return &m_functionPrototypeAccessorData; }
    ALWAYS_INLINE ESPropertyAccessorData* arrayLengthAccessorData() { return &m_arrayLengthAccessorData; }
    ALWAYS_INLINE ESPropertyAccessorData* stringObjectLengthAccessorData() { return &m_stringObjectLengthAccessorData; }

    ALWAYS_INLINE ESFunctionObject* globalFunctionPrototype() { return m_globalFunctionPrototype; }
    ALWAYS_INLINE void setGlobalFunctionPrototype(ESFunctionObject* o) { m_globalFunctionPrototype = o; }

    ALWAYS_INLINE WTF::BumpPointerAllocator* bumpPointerAllocator() { return m_bumpPointerAllocator; };

    long timezoneOffset();
    const tm* computeLocalTime(const timespec& ts);

    ALWAYS_INLINE ESHiddenClass* initialHiddenClassForObject()
    {
        return &m_initialHiddenClassForObject;
    }

    ALWAYS_INLINE ESHiddenClass* initialHiddenClassForFunctionObject()
    {
        return m_initialHiddenClassForFunctionObject;
    }

    ALWAYS_INLINE ESHiddenClass* initialHiddenClassForFunctionObjectWithoutPrototype()
    {
        return m_initialHiddenClassForFunctionObjectWithoutPrototype;
    }

    ALWAYS_INLINE ESHiddenClass* initialHiddenClassForPrototypeObject()
    {
        return m_initialHiddenClassForPrototypeObject;
    }

    ALWAYS_INLINE ESHiddenClass* initialHiddenClassForArrayObject()
    {
        return m_initialHiddenClassForArrayObject;
    }

    ALWAYS_INLINE ScriptParser* scriptParser()
    {
        return m_scriptParser;
    }

    ALWAYS_INLINE std::unordered_map<double, ESString*, std::hash<double>, std::equal_to<double>, gc_allocator<std::pair<const double, ESString* > > >* dtoaCache()
    {
        return &m_dtoaCache;
    }

    // Function for debug
    static void printValue(ESValue val);
    ALWAYS_INLINE unsigned long tickCount()
    {
        struct timespec timespec;
        clock_gettime(CLOCK_MONOTONIC, &timespec);
        return (unsigned long)(timespec.tv_sec * 1000000L + timespec.tv_nsec / 1000);
    }

    NEVER_INLINE void throwError(const ESValue& error)
    {
        ASSERT(m_error.isEmpty());
        m_error = error;
        std::jmp_buf* tryPosition = m_tryPositions.back();
        m_tryPositions.pop_back();
        std::longjmp(*tryPosition, 1);
        RELEASE_ASSERT_NOT_REACHED();
    }

    std::jmp_buf& registerTryPos(std::jmp_buf* arg)
    {
        ASSERT(m_error.isEmpty());
        m_tryPositions.push_back(arg);
        return *arg;
    }

    void unregisterTryPos(std::jmp_buf* arg)
    {
        ASSERT(m_error.isEmpty());
        ASSERT(arg == m_tryPositions.back());
        m_tryPositions.pop_back();
    }
    
    ESValue getCatchedError()
    {
        ESValue error = m_error;
#ifndef NDEBUG
        m_error = ESValue(ESValue::ESEmptyValueTag::ESEmptyValue);
#endif
        return error;
    }

#ifdef ENABLE_ESJIT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
    static size_t offsetOfCurrentExecutionContext() { return offsetof(ESVMInstance, m_currentExecutionContext); }
    static size_t offsetOfIdentifierCacheInvalidationCheckCount() { return offsetof(ESVMInstance, m_identifierCacheInvalidationCheckCount); }
#pragma GCC diagnostic pop
    nanojit::Config* getJITConfig() { return m_JITConfig; }
#endif

#ifndef NDEBUG
    bool m_dumpByteCode;
    bool m_dumpExecuteByteCode;
    bool m_verboseJIT;
    bool m_reportUnsupportedOpcode;
    bool m_reportCompiledFunction;
    bool m_reportOSRExitedFunction;
    bool m_useVerboseWriter;
    size_t m_compiledFunctions;
    size_t m_osrExitedFunctions;
#endif
    bool m_useExprFilter;
    bool m_useCseFilter;
    size_t m_jitThreshold;
    size_t m_osrExitThreshold;
    bool m_profile;

protected:
    ScriptParser* m_scriptParser;
    ExecutionContext* m_globalExecutionContext;
    ExecutionContext* m_currentExecutionContext;
    GlobalObject* m_globalObject;

    OpcodeTable* m_opcodeTable;
    std::unordered_map<void *, unsigned char> m_opcodeReverseTable;


    friend class InternalAtomicString;
    friend class InternalAtomicStringData;
    InternalAtomicStringMap m_atomicStringMap;

    Strings m_strings;
    unsigned m_identifierCacheInvalidationCheckCount;

    ESHiddenClass m_initialHiddenClassForObject;
    ESHiddenClass* m_initialHiddenClassForFunctionObject;
    ESHiddenClass* m_initialHiddenClassForFunctionObjectWithoutPrototype;
    ESHiddenClass* m_initialHiddenClassForPrototypeObject;
    ESHiddenClass* m_initialHiddenClassForArrayObject;

    ESPropertyAccessorData m_object__proto__AccessorData;
    ESPropertyAccessorData m_functionPrototypeAccessorData;
    ESPropertyAccessorData m_arrayLengthAccessorData;
    ESPropertyAccessorData m_stringObjectLengthAccessorData;

    ESFunctionObject* m_globalFunctionPrototype;

    ESValue m_lastExpressionStatementValue;

    WTF::BumpPointerAllocator* m_bumpPointerAllocator;

    timespec m_cachedTimeOrigin;
    long m_gmtoff;
    tm m_time;

    std::vector<std::jmp_buf*, pointer_free_allocator<std::jmp_buf*> > m_tryPositions;
    ESValue m_error;

    std::vector<ESSimpleAllocatorMemoryFragment, pointer_free_allocator<ESSimpleAllocatorMemoryFragment> > m_allocatedMemorys;
#ifdef ENABLE_ESJIT
    nanojit::Config* m_JITConfig;
#endif

    std::unordered_map<double, ESString*, std::hash<double>, std::equal_to<double>, gc_allocator<std::pair<const double, ESString* > > > m_dtoaCache;
};

struct ESSimpleAllocatorMemoryFragment {
    void* m_buffer;
    size_t m_currentUsage;
    size_t m_totalSize;
};

// TODO implmenet multi-thread support
class ESSimpleAllocator {
public:
    inline static void* alloc(size_t size)
    {
        std::vector<ESSimpleAllocatorMemoryFragment, pointer_free_allocator<ESSimpleAllocatorMemoryFragment> >& allocatedMemorys = ESVMInstance::currentInstance()->m_allocatedMemorys;

        size_t currentFragmentRemain;
        ESSimpleAllocatorMemoryFragment* last = NULL;

        if (UNLIKELY(!allocatedMemorys.size())) {
            currentFragmentRemain = 0;
        } else {
            currentFragmentRemain = allocatedMemorys.back().m_totalSize - allocatedMemorys.back().m_currentUsage;
            last = &allocatedMemorys.back();
        }

        if (currentFragmentRemain < size) {
            if (size > s_fragmentBufferSize) {
                ESSimpleAllocatorMemoryFragment f;
                f.m_buffer = malloc(size);
                f.m_currentUsage = size;
                f.m_totalSize = size;
                allocatedMemorys.push_back(f);
                return f.m_buffer;
            } else {
                allocSlow();
                currentFragmentRemain = s_fragmentBufferSize;
                last = &allocatedMemorys.back();
            }
        }

        ASSERT(currentFragmentRemain >= size);
        void* buf = &((char *)last->m_buffer)[last->m_currentUsage];
        last->m_currentUsage += size;
        return buf;
    }
    static void allocSlow();
    static void freeAll();
private:
    static const unsigned s_fragmentBufferSize = 2048;
};

class ESSimpleAlloc {
public:
    inline void* operator new( size_t size )
    {
        return ESSimpleAllocator::alloc(size);
    }
    inline void* operator new( size_t size, void *p )
    {
        return p;
    }
    inline void operator delete( void* obj )
    {

    }
    inline void* operator new[]( size_t size )
    {
        return ESSimpleAllocator::alloc(size);
    }
    inline void* operator new[]( size_t size, void *p )
    {
        return p;
    }
    inline void operator delete[]( void*, void* )
    {

    }
};

template<class T>
class ESSimpleAllocatorStd {
public:
    typedef size_t     size_type;
    typedef ptrdiff_t  difference_type;
    typedef T*       pointer;
    typedef const T* const_pointer;
    typedef T&       reference;
    typedef const T& const_reference;
    typedef T        value_type;

    template <class T2> struct rebind {
        typedef ESSimpleAllocatorStd<T2> other;
    };

    ESSimpleAllocatorStd() { }
    ESSimpleAllocatorStd(const ESSimpleAllocatorStd&) throw() { }
    template <class T2> ESSimpleAllocatorStd(const ESSimpleAllocatorStd<T2>&) throw() { }
    ~ESSimpleAllocatorStd() throw() { }

    pointer address(reference GC_x) const { return &GC_x; }
    const_pointer address(const_reference GC_x) const { return &GC_x; }

    T* allocate(size_type GC_n, const void* = 0)
    {
        return static_cast<T*>(ESSimpleAllocator::alloc(GC_n * sizeof(T)));
    }

    void deallocate(pointer __p, size_type GC_ATTR_UNUSED GC_n) { }

    size_type max_size() const throw() { return size_t(-1) / sizeof(T); }

    void construct(pointer __p, const T& __val) { new(__p) T(__val); }
    void destroy(pointer __p) { __p->~T(); }
};

template<>
class ESSimpleAllocatorStd<void> {
    typedef size_t      size_type;
    typedef ptrdiff_t   difference_type;
    typedef void*       pointer;
    typedef const void* const_pointer;
    typedef void        value_type;

    template <class T2> struct rebind {
        typedef ESSimpleAllocatorStd<T2> other;
    };
};

template <class T1, class T2>
inline bool operator==(const ESSimpleAllocatorStd<T1>&, const ESSimpleAllocatorStd<T2>&)
{
    return true;
}

template <class T1, class T2>
inline bool operator!=(const ESSimpleAllocatorStd<T1>&, const ESSimpleAllocatorStd<T2>&)
{
    return false;
}


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
