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
#include <locale>
#include <clocale>
#include <cwchar>

#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>

#include <rapidjson/document.h>
#include <rapidjson/filereadstream.h>
#include <rapidjson/memorystream.h>

#include <gc_cpp.h>
#include <gc_allocator.h>

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

#include "runtime/ESString.h"
#include "runtime/ESAtomicString.h"
#include "runtime/StaticStrings.h"

#endif
