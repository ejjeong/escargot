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

#ifndef __Escargot__
#define __Escargot__

#include <cstdlib>
#include <cstdio>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <cstring>
#include <sstream>
#include <cassert>
#include <functional>
#include <algorithm>
#include <cmath>
#include <csetjmp>
#include <limits>
#include <locale>
#include <clocale>
#include <cwchar>
#include <climits>

// icu
#include <unicode/locid.h>
#include <unicode/datefmt.h>
// end of icu

#if defined(ENABLE_ESJIT) && !defined(NDEBUG)
#include <iostream>
#endif

#ifdef ANDROID
#include <sys/resource.h>
#endif

#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>

#define RAPIDJSON_PARSE_DEFAULT_FLAGS kParseFullPrecisionFlag
#define RAPIDJSON_ERROR_CHARTYPE char
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/internal/dtoa.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>
#include <rapidjson/error/en.h>

// #define PROFILE_MASSIF

#ifndef PROFILE_MASSIF

#include <gc_cpp.h>
#include <gc_allocator.h>

#else

#include <gc.h>

void* GC_malloc_hook(size_t siz);
void* GC_malloc_atomic_hook(size_t siz);
void GC_free_hook(void* address);

#undef GC_MALLOC
#define GC_MALLOC(X) GC_malloc_hook(X)

#undef GC_MALLOC_ATOMIC
#define GC_MALLOC_ATOMIC(X) GC_malloc_atomic_hook(X)

#undef GC_FREE
#define GC_FREE(X) GC_free_hook(X)

#undef GC_REGISTER_FINALIZER_NO_ORDER
#define GC_REGISTER_FINALIZER_NO_ORDER(p, f, d, of, od) GC_register_finalizer_no_order(p, f, d, of, od)

#include <gc_cpp.h>
#include <gc_allocator.h>

#endif

template <class GC_Tp>
class pointer_free_allocator {
public:
    typedef size_t     size_type;
    typedef ptrdiff_t  difference_type;
    typedef GC_Tp*       pointer;
    typedef const GC_Tp* const_pointer;
    typedef GC_Tp&       reference;
    typedef const GC_Tp& const_reference;
    typedef GC_Tp        value_type;

    template <class GC_Tp1> struct rebind {
        typedef pointer_free_allocator<GC_Tp1> other;
    };

    pointer_free_allocator() throw() { }
    pointer_free_allocator(const pointer_free_allocator&) throw() { }
# if !(GC_NO_MEMBER_TEMPLATES || 0 < _MSC_VER && _MSC_VER <= 1200)
    // MSVC++ 6.0 do not support member templates
    template <class GC_Tp1> pointer_free_allocator
    (const pointer_free_allocator<GC_Tp1>&) throw() { }
# endif
    ~pointer_free_allocator() throw() { }

    pointer address(reference GC_x) const { return &GC_x; }
    const_pointer address(const_reference GC_x) const { return &GC_x; }

    // GC_n is permitted to be 0. The C++ standard says nothing about what
    // the return value is when GC_n == 0.
    GC_Tp* allocate(size_type GC_n, const void* = 0)
    {
        return (GC_Tp *)GC_MALLOC_ATOMIC(sizeof(GC_Tp) * GC_n);
    }

    // __p is not permitted to be a null pointer.
    void deallocate(pointer __p, size_type GC_ATTR_UNUSED GC_n)
    { GC_FREE(__p); }

    size_type max_size() const throw()
    { return size_t(-1) / sizeof(GC_Tp); }

    void construct(pointer __p, const GC_Tp& __val) { new(__p) GC_Tp(__val); }
    void destroy(pointer __p) { __p->~GC_Tp(); }
};

template<>
class pointer_free_allocator<void> {
    typedef size_t      size_type;
    typedef ptrdiff_t   difference_type;
    typedef void*       pointer;
    typedef const void* const_pointer;
    typedef void        value_type;

    template <class GC_Tp1> struct rebind {
        typedef pointer_free_allocator<GC_Tp1> other;
    };
};


template <class GC_T1, class GC_T2>
inline bool operator==(const pointer_free_allocator<GC_T1>&, const pointer_free_allocator<GC_T2>&)
{
    return true;
}

template <class GC_T1, class GC_T2>
inline bool operator!=(const pointer_free_allocator<GC_T1>&, const pointer_free_allocator<GC_T2>&)
{
    return false;
}

template <class GC_Tp>
class gc_malloc_allocator {
public:
    typedef size_t     size_type;
    typedef ptrdiff_t  difference_type;
    typedef GC_Tp*       pointer;
    typedef const GC_Tp* const_pointer;
    typedef GC_Tp&       reference;
    typedef const GC_Tp& const_reference;
    typedef GC_Tp        value_type;

    template <class GC_Tp1> struct rebind {
        typedef gc_malloc_allocator<GC_Tp1> other;
    };

    gc_malloc_allocator() throw() { }
    gc_malloc_allocator(const gc_malloc_allocator&) throw() { }
# if !(GC_NO_MEMBER_TEMPLATES || 0 < _MSC_VER && _MSC_VER <= 1200)
    // MSVC++ 6.0 do not support member templates
    template <class GC_Tp1> gc_malloc_allocator
    (const gc_malloc_allocator<GC_Tp1>&) throw() { }
# endif
    ~gc_malloc_allocator() throw() { }

    pointer address(reference GC_x) const { return &GC_x; }
    const_pointer address(const_reference GC_x) const { return &GC_x; }

    // GC_n is permitted to be 0. The C++ standard says nothing about what
    // the return value is when GC_n == 0.
    GC_Tp* allocate(size_type GC_n, const void* = 0)
    {
        return (GC_Tp *)GC_MALLOC(sizeof(GC_Tp) * GC_n);
    }

    // __p is not permitted to be a null pointer.
    void deallocate(pointer __p, size_type GC_ATTR_UNUSED GC_n)
    { GC_FREE(__p); }

    size_type max_size() const throw()
    { return size_t(-1) / sizeof(GC_Tp); }

    void construct(pointer __p, const GC_Tp& __val) { new(__p) GC_Tp(__val); }
    void destroy(pointer __p) { __p->~GC_Tp(); }
};

template<>
class gc_malloc_allocator<void> {
    typedef size_t      size_type;
    typedef ptrdiff_t   difference_type;
    typedef void*       pointer;
    typedef const void* const_pointer;
    typedef void        value_type;

    template <class GC_Tp1> struct rebind {
        typedef gc_malloc_allocator<GC_Tp1> other;
    };
};


template <class GC_T1, class GC_T2>
inline bool operator==(const gc_malloc_allocator<GC_T1>&, const gc_malloc_allocator<GC_T2>&)
{
    return true;
}

template <class GC_T1, class GC_T2>
inline bool operator!=(const gc_malloc_allocator<GC_T1>&, const gc_malloc_allocator<GC_T2>&)
{
    return false;
}



/* COMPILER() - the compiler being used to build the project */
#define COMPILER(FEATURE) (defined COMPILER_##FEATURE  && COMPILER_##FEATURE)


/* COMPILER(MSVC) - Microsoft Visual C++ */
#if defined(_MSC_VER)
#define COMPILER_MSVC 1

/* Specific compiler features */
#if !COMPILER(CLANG) && _MSC_VER >= 1600
#define COMPILER_SUPPORTS_CXX_NULLPTR 1
#endif

#if COMPILER(CLANG)
/* Keep strong enums turned off when building with clang-cl: We cannot yet build all of Blink without fallback to cl.exe, and strong enums are exposed at ABI boundaries. */
#undef COMPILER_SUPPORTS_CXX_STRONG_ENUMS
#else
#define COMPILER_SUPPORTS_CXX_OVERRIDE_CONTROL 1
#define COMPILER_QUIRK_FINAL_IS_CALLED_SEALED 1
#endif

#endif

/* COMPILER(GCC) - GNU Compiler Collection */
#if defined(__GNUC__)
#define COMPILER_GCC 1
#define GCC_VERSION (__GNUC__ * 10000 + __GNUC_MINOR__ * 100 + __GNUC_PATCHLEVEL__)
#define GCC_VERSION_AT_LEAST(major, minor, patch) (GCC_VERSION >= (major * 10000 + minor * 100 + patch))
#else
/* Define this for !GCC compilers, just so we can write things like GCC_VERSION_AT_LEAST(4, 1, 0). */
#define GCC_VERSION_AT_LEAST(major, minor, patch) 0
#endif


/* ALWAYS_INLINE */
#ifndef ALWAYS_INLINE
#if COMPILER(GCC) && defined(NDEBUG) && !COMPILER(MINGW)
#define ALWAYS_INLINE inline __attribute__((__always_inline__))
#elif COMPILER(MSVC) && defined(NDEBUG)
#define ALWAYS_INLINE __forceinline
#else
#define ALWAYS_INLINE inline
#endif
#endif

#ifdef ESCARGOT_SMALL_CONFIG
#undef ALWAYS_INLINE
#define ALWAYS_INLINE inline
#endif

/* NEVER_INLINE */
#ifndef NEVER_INLINE
#if COMPILER(GCC)
#define NEVER_INLINE __attribute__((__noinline__))
#else
#define NEVER_INLINE
#endif
#endif


/* UNLIKELY */
#ifndef UNLIKELY
#if COMPILER(GCC)
#define UNLIKELY(x) __builtin_expect((x), 0)
#else
#define UNLIKELY(x) (x)
#endif
#endif


/* LIKELY */
#ifndef LIKELY
#if COMPILER(GCC)
#define LIKELY(x) __builtin_expect((x), 1)
#else
#define LIKELY(x) (x)
#endif
#endif


/* NO_RETURN */
#ifndef NO_RETURN
#if COMPILER(GCC)
#define NO_RETURN __attribute((__noreturn__))
#elif COMPILER(MSVC)
#define NO_RETURN __declspec(noreturn)
#else
#define NO_RETURN
#endif
#endif


#if !COMPILER(GCC)
#include <codecvt>
#endif

#define ESCARGOT_LOG_INFO(...) fprintf(stdout, __VA_ARGS__);
#define ESCARGOT_LOG_ERROR(...) fprintf(stderr, __VA_ARGS__);

#ifdef ANDROID
#include <android/log.h>
#undef ESCARGOT_LOG_ERROR
#define ESCARGOT_LOG_ERROR(...) __android_log_print(ANDROID_LOG_ERROR, "Escargot", __VA_ARGS__);
#endif

#if defined(ESCARGOT_TIZEN) && !defined(ESCARGOT_STANDALONE)
#include <dlog/dlog.h>
#undef ESCARGOT_LOG_INFO
#undef ESCARGOT_LOG_ERROR
#define ESCARGOT_LOG_INFO(...)  dlog_print(DLOG_INFO, "Escargot", __VA_ARGS__);
#define ESCARGOT_LOG_ERROR(...) dlog_print(DLOG_ERROR, "Escargot", __VA_ARGS__);
#endif

#ifndef CRASH
#define CRASH RELEASE_ASSERT_NOT_REACHED
#endif

#if defined(NDEBUG)
#define ASSERT(assertion) ((void)0)
#define ASSERT_NOT_REACHED() ((void)0)
#define ASSERT_STATIC(assertion, reason)
#else
#define ASSERT(assertion) assert(assertion);
#define ASSERT_NOT_REACHED() do { assert(false); } while (0)
#define ASSERT_STATIC(assertion, reason) static_assert(assertion, reason)
#endif

/* COMPILE_ASSERT */
#ifndef COMPILE_ASSERT
#define COMPILE_ASSERT(exp, name) static_assert((exp), #name)
#endif

#define RELEASE_ASSERT(assertion) do { if (!(assertion)) { ESCARGOT_LOG_ERROR("RELEASE_ASSERT at %s (%d)\n", __FILE__, __LINE__); abort(); } } while (0);
#define RELEASE_ASSERT_NOT_REACHED() do { ESCARGOT_LOG_ERROR("RELEASE_ASSERT_NOT_REACHED at %s (%d)\n", __FILE__, __LINE__); abort(); } while (0)

#if !defined(WARN_UNUSED_RETURN) && COMPILER(GCC)
#define WARN_UNUSED_RETURN __attribute__((__warn_unused_result__))
#endif

#if !defined(WARN_UNUSED_RETURN)
#define WARN_UNUSED_RETURN
#endif

#if defined(__BYTE_ORDER__) && __BYTE_ORDER == __BIG_ENDIAN || \
    defined(__BIG_ENDIAN__) || \
    defined(__ARMEB__) || \
    defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || \
    defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
#define ESCARGOT_BIG_ENDIAN
// #pragma message "big endian"
#elif defined(__BYTE_ORDER__) && __BYTE_ORDER == __LITTLE_ENDIAN || \
    defined(__LITTLE_ENDIAN__) || \
    defined(__i386) || \
    defined(_M_IX86) || \
    defined(__ia64) || \
    defined(__ia64__) || \
    defined(_M_IA64) || \
    defined(__ARMEL__) || \
    defined(__THUMBEL__) || \
    defined(__AARCH64EL__) || \
    defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
#define ESCARGOT_LITTLE_ENDIAN
// #pragma message "little endian"
#else
#error "I don't know what architecture this is!"
#endif

namespace escargot {

typedef std::basic_string<char16_t, std::char_traits<char16_t>, gc_allocator<char16_t> > UTF16String;
typedef std::basic_string<char, std::char_traits<char>, gc_allocator<char> > ASCIIString;

namespace options {

static const size_t KB = 1024;
static const size_t MB = 1024 * KB;
static const size_t GB = 1024 * MB;

// TODO these should be changed on various devices
static const size_t CodeCacheThreshold = 1 * KB;
static const size_t LazyByteCodeGenerationThreshold = 1 * MB;
#ifdef ESCARGOT_TIZEN
static const size_t MaxStackDepth = 4.5 * MB;
static const size_t NativeHeapUsageThreshold = 250 * MB;
#else
static const size_t MaxStackDepth = 5 * MB;
static const size_t NativeHeapUsageThreshold = 300 * MB;
#endif
static const size_t AllocaOnHeapThreshold = 32 * MB;
static const size_t MaximumArgumentCount = 65535;
static const size_t MaximumStringLength = (1 * GB) | 1;
static const int64_t MaximumDatePrimitiveValue = 8640000000000000;

}

}

#ifdef ENABLE_ESJIT
namespace escargot { class ESVMInstance; }
#ifdef ESCARGOT_64
typedef uint64_t ESValueInDouble;
#else
typedef double ESValueInDouble;
#endif
extern "C" { typedef ESValueInDouble (*JITFunction)(escargot::ESVMInstance*); }
#endif

#include "CheckedArithmetic.h"
#include "runtime/ESValue.h"

#endif
