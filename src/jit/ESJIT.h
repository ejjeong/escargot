#ifndef ESJIT_h
#define ESJIT_h

#ifdef ENABLE_ESJIT

#include "runtime/ESValue.h"
#include "stdarg.h"

namespace escargot {

class CodeBlock;
class ESVMInstance;

namespace ESJIT {

class ESGraph;

typedef ESValueInDouble (*JITFunction)(ESVMInstance*);

struct ESJITAllocatorMemoryFragment {
    void* m_buffer;
    size_t m_currentUsage;
    size_t m_totalSize;
};

// TODO implmenet multi-thread support
class ESJITAllocator {
public:
    inline static void* alloc(size_t size)
    {
        size_t currentFragmentRemain;

        if (UNLIKELY(!m_allocatedMemorys.size())) {
            currentFragmentRemain = 0;
        } else {
            currentFragmentRemain = m_allocatedMemorys.back().m_totalSize - m_allocatedMemorys.back().m_currentUsage;
        }

        if (currentFragmentRemain < size) {
            allocSlow();
            currentFragmentRemain = s_fragmentBufferSize;
        }

        ASSERT(currentFragmentRemain >= size);
        void* buf = &((char *)m_allocatedMemorys.back().m_buffer)[m_allocatedMemorys.back().m_currentUsage];
        m_allocatedMemorys.back().m_currentUsage += size;
        return buf;
    }
    static void allocSlow();
    static void freeAll();
private:
    static const unsigned s_fragmentBufferSize = 10240;
    static std::vector<ESJITAllocatorMemoryFragment> m_allocatedMemorys;
};

template<class T>
class CustomAllocator {
    public:
    typedef size_t     size_type;
    typedef ptrdiff_t  difference_type;
    typedef T*       pointer;
    typedef const T* const_pointer;
    typedef T&       reference;
    typedef const T& const_reference;
    typedef T        value_type;

    template <class T2> struct rebind {
        typedef CustomAllocator<T2> other;
    };

    CustomAllocator() { }
    CustomAllocator(const CustomAllocator&) throw() { }
    template <class T2> CustomAllocator(const CustomAllocator<T2>&) throw() { }
    ~CustomAllocator() throw() { }

    pointer address(reference GC_x) const { return &GC_x; }
    const_pointer address(const_reference GC_x) const { return &GC_x; }

    T* allocate(size_type GC_n, const void* = 0)
    {
        return static_cast<T*>(ESJITAllocator::alloc(GC_n * sizeof(T)));
    }

    void deallocate(pointer __p, size_type GC_ATTR_UNUSED GC_n) { }

    size_type max_size() const throw() { return size_t(-1) / sizeof(T); }

    void construct(pointer __p, const T& __val) { new(__p) T(__val); }
    void destroy(pointer __p) { __p->~T(); }
};

template<>
class CustomAllocator<void> {
    typedef size_t      size_type;
    typedef ptrdiff_t   difference_type;
    typedef void*       pointer;
    typedef const void* const_pointer;
    typedef void        value_type;

    template <class T2> struct rebind {
        typedef CustomAllocator<T2> other;
    };
};

template <class T1, class T2>
inline bool operator==(const CustomAllocator<T1>&, const CustomAllocator<T2>&)
{
    return true;
}

template <class T1, class T2>
inline bool operator!=(const CustomAllocator<T1>&, const CustomAllocator<T2>&)
{
    return false;
}

class ESJITAlloc {
    public:
    inline void* operator new( size_t size )
    {
        return ESJITAllocator::alloc(size);
    }
    inline void* operator new( size_t size, void *p )
    {
        RELEASE_ASSERT_NOT_REACHED();
    }
    inline void operator delete( void* obj )
    {

    }
    inline void* operator new[]( size_t size )
    {
        return ESJITAllocator::alloc(size);
    }
    inline void* operator new[]( size_t size, void *p )
    {
        RELEASE_ASSERT_NOT_REACHED();
    }
    inline void operator delete[]( void*, void* )
    {

    }
};

class ESJITCompiler {
public:
    ESJITCompiler(CodeBlock* codeBlock)
        : m_codeBlock(codeBlock), m_graph(nullptr), m_native(nullptr) { }

    bool compile(ESVMInstance* instance);
    void finalize();

    CodeBlock* codeBlock() { return m_codeBlock; }
    ESGraph* ir() { return m_graph; }
    JITFunction native() { return m_native; }

private:
    CodeBlock* m_codeBlock;
    ESGraph* m_graph;
    JITFunction m_native;
};

JITFunction JITCompile(CodeBlock* codeBlock, ESVMInstance* instance);

void logVerboseJIT(const char* fmt...);

#ifndef LOG_VJ
#ifndef NDEBUG
#define LOG_VJ(fmt, ...) ::escargot::ESJIT::logVerboseJIT(fmt, __VA_ARGS__)
#else
#define LOG_VJ(fmt, ...)
#endif
#endif

}

}
#endif
#endif
