#ifndef __Escargot__
#define __Escargot__

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <string>
#include <cstring>
#include <cassert>
#include <functional>
#include <algorithm>
#include <cmath>
#include <csetjmp>
#include <limits>
#include <locale>
#include <clocale>
#include <cwchar>

#include <time.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>

#define RAPIDJSON_PARSE_DEFAULT_FLAGS kParseFullPrecisionFlag
#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/memorystream.h>
#include <rapidjson/internal/dtoa.h>
#include <rapidjson/internal/strtod.h>

#include <re2/re2.h>

#include <gc_cpp.h>
#include <gc_allocator.h>

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

  pointer_free_allocator() throw() {}
  pointer_free_allocator(const pointer_free_allocator&) throw() {}
# if !(GC_NO_MEMBER_TEMPLATES || 0 < _MSC_VER && _MSC_VER <= 1200)
  // MSVC++ 6.0 do not support member templates
  template <class GC_Tp1> pointer_free_allocator
          (const pointer_free_allocator<GC_Tp1>&) throw() {}
# endif
  ~pointer_free_allocator() throw() {}

  pointer address(reference GC_x) const { return &GC_x; }
  const_pointer address(const_reference GC_x) const { return &GC_x; }

  // GC_n is permitted to be 0.  The C++ standard says nothing about what
  // the return value is when GC_n == 0.
  GC_Tp* allocate(size_type GC_n, const void* = 0) {
    return new(PointerFreeGC) GC_Tp[GC_n];
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

#if defined(NDEBUG)
#define ASSERT(assertion) ((void)0)
#define ASSERT_NOT_REACHED() ((void)0)
#else
#define ASSERT(assertion) assert(assertion);
#define ASSERT_NOT_REACHED() do { assert(false); } while (0)
#endif

#define RELEASE_ASSERT(assertion) assert(assertion);
#define RELEASE_ASSERT_NOT_REACHED() do { assert(false); } while (0)

#if !defined(WARN_UNUSED_RETURN) && COMPILER(GCC)
#define WARN_UNUSED_RETURN __attribute__((__warn_unused_result__))
#endif

#if !defined(WARN_UNUSED_RETURN)
#define WARN_UNUSED_RETURN
#endif

#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || \
    defined(__BIG_ENDIAN__) || \
    defined(__ARMEB__) || \
    defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || \
    defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
#define ESCARGOT_BIG_ENDIAN
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || \
    defined(__LITTLE_ENDIAN__) || \
    defined(__ARMEL__) || \
    defined(__THUMBEL__) || \
    defined(__AARCH64EL__) || \
    defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
#define ESCARGOT_LITTLE_ENDIAN
#else
#error "I don't know what architecture this is!"
#endif

#include "util/RefCounted.h"
#include "util/RefPtr.h"
#include "util/OwnPtr.h"

#include "runtime/InternalAtomicString.h"
#include "runtime/NullableString.h"
#include "runtime/ESValue.h"
//#include "runtime/InternalString.h"
#include "runtime/StaticStrings.h"

#endif
