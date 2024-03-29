/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

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
class JobQueue;
struct ESSimpleAllocatorMemoryFragment;

#ifndef ANDROID
extern __thread ESVMInstance* currentInstance;
#else
extern ESVMInstance* currentInstance;
#endif

typedef std::unordered_map<std::pair<const char *, size_t>, ESString *,
    std::hash<std::pair<const char *, size_t> >, std::equal_to<std::pair<const char *, size_t> >,
    gc_allocator<std::pair<const std::pair<const char *, size_t>, ESString *> > > InternalAtomicStringMap;

class ESVMInstance : public gc {
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
    ESValue evaluate(ESString* source);
    ESValue evaluateEval(ESString* source, bool isDirectCall, CodeBlock* outerCodeBlock);

    ALWAYS_INLINE ExecutionContext* currentExecutionContext() { return m_currentExecutionContext; }
    ALWAYS_INLINE ExecutionContext* globalExecutionContext() { return m_globalExecutionContext; }
    ALWAYS_INLINE GlobalObject* globalObject() { return m_globalObject; }
#ifdef USE_ES6_FEATURE
    ALWAYS_INLINE JobQueue* jobQueue() { return m_jobQueue; }
#endif

    ALWAYS_INLINE void enter()
    {
        ASSERT(!escargot::currentInstance);
        escargot::currentInstance = this;
        escargot::strings = &m_strings;
        char dummy;
        m_stackStart = &dummy;
    }

    void exit();
    ALWAYS_INLINE static ESVMInstance* currentInstance()
    {
        return escargot::currentInstance;
    }

    ALWAYS_INLINE OpcodeTable* opcodeTable() { return m_opcodeTable; }
    ALWAYS_INLINE std::unordered_map<void *, unsigned char>& opcodeResverseTable() { return m_opcodeReverseTable; }
    ALWAYS_INLINE Strings& strings() { return m_strings; }

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
    ALWAYS_INLINE ESPropertyAccessorData* regexpAccessorData(size_t index) { ASSERT(index < 5); return &m_regexpAccessorData[index]; }

    ALWAYS_INLINE ESFunctionObject* globalFunctionPrototype() { return m_globalFunctionPrototype; }
    ALWAYS_INLINE void setGlobalFunctionPrototype(ESFunctionObject* o) { m_globalFunctionPrototype = o; }

    ALWAYS_INLINE ESPropertyAccessorData* throwerAccessorData() { return &m_throwerAccessorData; }

    ALWAYS_INLINE WTF::BumpPointerAllocator* bumpPointerAllocator() { return m_bumpPointerAllocator; };

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

    ALWAYS_INLINE ESHiddenClass* initialHiddenClassForRegExpObject()
    {
        return m_initialHiddenClassForRegExpObject;
    }

    ALWAYS_INLINE ScriptParser* scriptParser()
    {
        return m_scriptParser;
    }

    ALWAYS_INLINE std::unordered_map<double, ESString*, std::hash<double>, std::equal_to<double>, gc_allocator<std::pair<const double, ESString* > > >* dtoaCache()
    {
        return &m_dtoaCache;
    }

    ALWAYS_INLINE std::unordered_map<ESRegExpObject::RegExpCacheKey, ESRegExpObject::RegExpCacheEntry,
        std::hash<ESRegExpObject::RegExpCacheKey>, std::equal_to<ESRegExpObject::RegExpCacheKey>,
        gc_allocator<std::pair<ESRegExpObject::RegExpCacheKey, ESRegExpObject::RegExpCacheEntry> > >* regexpCache()
    {
        return &m_regexpCache;
    }

    // Function for debug
    static void printValue(ESValue val, bool newLine = true);
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

    NEVER_INLINE void throwError(ESErrorObject::Code code, const char* templateString)
    {
        throwError(ESErrorObject::create(ESString::create(templateString), code));
    }

    NEVER_INLINE void throwError(ESErrorObject::Code code, const char* templateString, ESString* replacer)
    {
        ESString* errorMessage;

        size_t len1 = strlen(templateString);
        size_t len2 = replacer->length();
        if (replacer->isASCIIString()) {
            char buf[len1 + len2 + 1];
            snprintf(buf, sizeof(buf), templateString, replacer->asciiData());
            errorMessage = ESString::create(buf);
        } else {
            char16_t buf[len1 + 1];
            for (size_t i = 0; i < len1; i++) {
                buf[i] = templateString[i];
            }
            UTF16String str(buf);
            size_t idx;
            if ((idx = str.find(u"%s")) != SIZE_MAX) {
                str.replace(str.begin() + idx, str.begin() + idx + 2, replacer->utf16Data());
            }
            errorMessage = ESString::create(str);
        }

        throwError(ESErrorObject::create(errorMessage, code));
    }

    void throwOOMError()
    {
        // TODO execution must stop
        throwError(RangeError::create(ESString::create("Out of memory")));
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

    bool registerCheckableObject(ESObject* obj)
    {
        if (m_checkedObjects.find(obj) != m_checkedObjects.end()) {
            return true;
        }

        m_checkedObjects.insert(obj);
        return false;
    }

    void unregisterCheckedObject(ESObject* obj)
    {
        m_checkedObjects.erase(obj);
    }

    void unregisterCheckedObjectAll()
    {
        m_checkedObjects.erase(m_checkedObjects.begin(), m_checkedObjects.end());
    }

    void registerOuterFEName(ESString* name)
    {
        m_outerFENames.insert(name);
    }

    void unregisterOuterFEName(ESString* name)
    {
        m_outerFENames.erase(name);
    }

    bool isFEName(ESString* name)
    {
        return m_outerFENames.find(name) != m_outerFENames.end();
    }

    bool isInCatchClause() { return m_catchDepth != 0; }
    void enterCatchClause() { m_catchDepth++; }
    void exitCatchClause() { ASSERT(isInCatchClause()); m_catchDepth--; }

    char* stackStart() { return m_stackStart; }

    ALWAYS_INLINE void stackCheck()
    {
        char dummy;
        if (UNLIKELY(static_cast<size_t>(stackStart() - &dummy) > options::MaxStackDepth))
            throwError(RangeError::create(ESString::create("Maximum call stack size exceeded.")));
    }

    ALWAYS_INLINE void argumentCountCheck(size_t arglen)
    {
        if (UNLIKELY(arglen > options::MaximumArgumentCount))
            throwError(RangeError::create(ESString::create("Maximum number of arguments exceeded.")));
    }

    void nativeHeapAllocated(size_t size, void* ptr)
    {
        m_nativeHeapUsage += size;

        (void)ptr;
        // printf("allocate %zu (current %zu) %p\n", size, m_nativeHeapUsage, ptr);

        if (m_nativeHeapUsage > options::NativeHeapUsageThreshold)
            throwOOMError();
    }
    void nativeHeapDeallocated(size_t size, void* ptr)
    {
        m_nativeHeapUsage -= size;

        (void)ptr;
        // printf("deallocate %zu (current %zu) %p\n", size, m_nativeHeapUsage, ptr);
    }
    size_t nativeHeapUsage() { return m_nativeHeapUsage; }

    icu::Locale& locale()
    {
        return m_locale;
    }
    // The function below will be used by StarFish
    void setlocale(icu::Locale locale)
    {
        m_locale = locale;
    }

    icu::TimeZone* timezone() const
    {
        return m_timezone;
    }
    void setTimezone(icu::TimeZone *tz)
    {
        m_timezone = tz->clone();
    }
    icu::UnicodeString timezoneID() const
    {
        return m_timezoneID;
    }
    // The function below will be used by StarFish
    void setTimezoneID(icu::UnicodeString id)
    {
        m_timezoneID = id;
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
    bool m_debug;
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

#ifdef USE_ES6_FEATURE
    JobQueue* m_jobQueue;
#endif

    OpcodeTable* m_opcodeTable;
    std::unordered_map<void *, unsigned char> m_opcodeReverseTable;

    char* m_stackStart;

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
    ESHiddenClass* m_initialHiddenClassForRegExpObject;

    ESPropertyAccessorData m_object__proto__AccessorData;
    ESPropertyAccessorData m_functionPrototypeAccessorData;
    ESPropertyAccessorData m_arrayLengthAccessorData;
    ESPropertyAccessorData m_stringObjectLengthAccessorData;
    ESPropertyAccessorData m_regexpAccessorData[5];

    ESFunctionObject* m_globalFunctionPrototype;

    ESPropertyAccessorData m_throwerAccessorData;

    ESValue m_lastExpressionStatementValue;

    WTF::BumpPointerAllocator* m_bumpPointerAllocator;

    icu::Locale m_locale;
    icu::TimeZone* m_timezone;
    icu::UnicodeString m_timezoneID;

    std::vector<std::jmp_buf*, pointer_free_allocator<std::jmp_buf*> > m_tryPositions;
    ESValue m_error;

    std::vector<ESSimpleAllocatorMemoryFragment, pointer_free_allocator<ESSimpleAllocatorMemoryFragment> > m_allocatedMemorys;
    static size_t m_nativeHeapUsage;

#ifdef ENABLE_ESJIT
    nanojit::Config* m_JITConfig;
#endif

    std::unordered_map<double, ESString*, std::hash<double>, std::equal_to<double>, gc_allocator<std::pair<const double, ESString* > > > m_dtoaCache;

    std::unordered_map<ESRegExpObject::RegExpCacheKey, ESRegExpObject::RegExpCacheEntry,
        std::hash<ESRegExpObject::RegExpCacheKey>, std::equal_to<ESRegExpObject::RegExpCacheKey>,
        gc_allocator<std::pair<ESRegExpObject::RegExpCacheKey, ESRegExpObject::RegExpCacheEntry> > > m_regexpCache;

    std::unordered_set<ESObject*, std::hash<ESObject*>, std::equal_to<ESObject*>, gc_allocator<ESObject*> > m_checkedObjects;
    std::unordered_set<ESString*, std::hash<ESString*>, std::equal_to<ESString*>, gc_allocator<ESString*> > m_outerFENames;

    size_t m_catchDepth;
public:
    // FIXME This is to make the return type of getByIdOperation() as ESValue* (not ESValue) for performance.
    // The lifetime is very short, so we should be careful.
    // This will also introduce small memory leak.
    // When getByIdOperation() function can have ESValue as its return type, this should be removed.
    ESValue m_temporaryAccessorBindingValueHolder;
};

ALWAYS_INLINE void stackCheck()
{
    ESVMInstance::currentInstance()->stackCheck();
}

ALWAYS_INLINE ESVMInstance* ESVMInstanceCurrentInstance()
{
    return ESVMInstance::currentInstance();
}

#define ALLOCA_WRAPPER(instance, ptr, type, size, atomic) \
    instance->stackCheck(); \
    if (size > options::AllocaOnHeapThreshold) { \
        if (atomic) { \
            ptr = (type)GC_MALLOC_ATOMIC(size); \
        } else { \
            ptr = (type)GC_MALLOC(size); \
        } \
    } else { \
        ptr = (type)alloca(size); \
    }

class StringRecursionChecker {
public:
    StringRecursionChecker(ESObject* thisObject)
        : m_thisObject(thisObject)
        , m_isRecursiveObject(false)
    {
    }

    bool recursionCheck()
    {
        m_isRecursiveObject = ESVMInstance::currentInstance()->registerCheckableObject(m_thisObject);
        return m_isRecursiveObject;
    }

    ~StringRecursionChecker()
    {
        if (m_isRecursiveObject) {
            m_thisObject = nullptr;
            return;
        }

        ESVMInstance::currentInstance()->unregisterCheckedObject(m_thisObject);
        m_thisObject = nullptr;
    }

private:
    ESObject* m_thisObject;
    bool m_isRecursiveObject;
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


template<class T>
class ESNativeHeapUsageCounter {
public:
    typedef size_t     size_type;
    typedef ptrdiff_t  difference_type;
    typedef T*       pointer;
    typedef const T* const_pointer;
    typedef T&       reference;
    typedef const T& const_reference;
    typedef T        value_type;

    template <class T2> struct rebind {
        typedef ESNativeHeapUsageCounter<T2> other;
    };

    ESNativeHeapUsageCounter() { }
    ESNativeHeapUsageCounter(const ESNativeHeapUsageCounter&) throw() { }
    template <class T2> ESNativeHeapUsageCounter(const ESNativeHeapUsageCounter<T2>&) throw() { }
    ~ESNativeHeapUsageCounter() throw() { }

    pointer address(reference GC_x) const { return &GC_x; }
    const_pointer address(const_reference GC_x) const { return &GC_x; }

    T* allocate(size_type GC_n, const void* = 0)
    {
        void* __p = malloc(GC_n * sizeof(T));
        ESVMInstance::currentInstance()->nativeHeapAllocated(GC_n * sizeof(T), __p);
        return static_cast<T*>(__p);
    }

    void deallocate(pointer __p, size_type GC_ATTR_UNUSED GC_n)
    {
        ESVMInstance::currentInstance()->nativeHeapDeallocated(GC_n * sizeof(T), __p);
        free(__p);
    }

    size_type max_size() const throw() { return size_t(-1) / sizeof(T); }

    void construct(pointer __p, const T& __val) { new(__p) T(__val); }
    void destroy(pointer __p) { __p->~T(); }
};

template<>
class ESNativeHeapUsageCounter<void> {
    typedef size_t      size_type;
    typedef ptrdiff_t   difference_type;
    typedef void*       pointer;
    typedef const void* const_pointer;
    typedef void        value_type;

    template <class T2> struct rebind {
        typedef ESNativeHeapUsageCounter<T2> other;
    };
};

template <class T1, class T2>
inline bool operator==(const ESNativeHeapUsageCounter<T1>&, const ESNativeHeapUsageCounter<T2>&)
{
    return true;
}

template <class T1, class T2>
inline bool operator!=(const ESNativeHeapUsageCounter<T1>&, const ESNativeHeapUsageCounter<T2>&)
{
    return false;
}

}

#include "runtime/ExecutionContext.h"

#endif
