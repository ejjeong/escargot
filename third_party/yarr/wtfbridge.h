/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*-
 * vim: set ts=8 sts=4 et sw=4 tw=99:
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef yarr_wtfbridge_h
#define yarr_wtfbridge_h

/*
 * WTF compatibility layer. This file provides various type and data
 * definitions for use by Yarr.
 */

#include <stdio.h>
#include <stdarg.h>
#include "Escargot.h"
#define WTF_OS_UNIX 1
#define JS_HOWMANY(x,y) (((x)+(y)-1)/(y))
#define JS_ROUNDUP(x,y) (JS_HOWMANY(x,y)*(y))
template <class T>
struct AlignmentTestStruct
{
    char c;
    T t;
};

/* This macro determines the alignment requirements of a type. */
#define JS_ALIGNMENT_OF(t_) \
  (sizeof(AlignmentTestStruct<t_>) - sizeof(t_))

//#include "jsstr.h"
//#include "jsprvtd.h"
//#include "vm/String.h"
//#include "assembler/wtf/Platform.h"
//#include "assembler/jit/ExecutableAllocator.h"
//#include "js/TemplateLib.h"

template <typename T, typename U> struct IsSameType {
    static const bool value = false;
};

template <typename T> struct IsSameType<T, T> {
    static const bool value = true;
};

#define CRASH RELEASE_ASSERT_NOT_REACHED
#include "CheckedArithmetic.h"

/*
 * Basic type definitions.
 */

#define JS_EXPORT_PRIVATE
#define WTF_MAKE_FAST_ALLOCATED
#define NO_RETURN_DUE_TO_ASSERT
typedef escargot::u16string String;
typedef escargot::u16string UString;
typedef char16_t UChar;
typedef char LChar;

#include "PageAllocation.h"

enum TextCaseSensitivity {
    TextCaseSensitive,
    TextCaseInsensitive
};


namespace JSC { namespace Yarr {
class Unicode {
  public:
    //TODO use ICU!
    static UChar toUpper(UChar c) { return toupper(c); }
    static UChar toLower(UChar c) { return tolower(c); }
};

/*
 * Do-nothing smart pointer classes. These have a compatible interface
 * with the smart pointers used by Yarr, but they don't actually do
 * reference counting.
 */
template<typename T>
class RefCounted {
};

template<typename T>
class RefPtr {
    T *ptr;
  public:
    RefPtr(T *p) { ptr = p; }
    operator bool() const { return ptr != NULL; }
    const T *operator ->() const { return ptr; }
    T *get() { return ptr; }
};

template<typename T>
class PassRefPtr {
    T *ptr;
  public:
    PassRefPtr(T *p) { ptr = p; }
    operator T*() { return ptr; }
};

template<typename T>
class PassOwnPtr {
    T *ptr;
  public:
    PassOwnPtr(T *p) { ptr = p; }

    T *get() { return ptr; }
};

template<typename T>
class OwnPtr {
    T *ptr;
  public:
    OwnPtr() : ptr(NULL) { }
    OwnPtr(PassOwnPtr<T> p) : ptr(p.get()) { }

    ~OwnPtr() {
    }

    OwnPtr<T> &operator=(PassOwnPtr<T> p) {
        ptr = p.get();
        return *this;
    }

    T *operator ->() { return ptr; }

    T *get() { return ptr; }

    T *release() {
        T *result = ptr;
        ptr = NULL;
        return result;
    }
};

template<typename T>
PassRefPtr<T> adoptRef(T *p) { return PassRefPtr<T>(p); }

template<typename T>
PassOwnPtr<T> adoptPtr(T *p) { return PassOwnPtr<T>(p); }

template<typename T>
class Ref {
    T &val;
  public:
    Ref(T &val) : val(val) { }
    operator T&() const { return val; }
};

template<typename T, size_t N = 0>
class Vector {
  public:
    std::vector<T, gc_allocator<T> > impl;
  public:
    Vector() {}

    Vector(const Vector &v) {
        append(v);
    }

    size_t size() const {
        return impl.size();
    }

    T &operator[](size_t i) {
        return impl[i];
    }

    const T &operator[](size_t i) const {
        return impl[i];
    }

    T &at(size_t i) {
        return impl[i];
    }

    const T *begin() const {
        return impl.begin();
    }

    T &last() {
        return impl.back();
    }

    bool isEmpty() const {
        return impl.empty();
    }

    template <typename U>
    void append(const U &u) {
        impl.push_back(static_cast<T>(u));
    }

    template <size_t M>
    void append(const Vector<T,M> &v) {
        impl.append(v.impl);
    }

    void insert(size_t i, const T& t) {
        impl.insert(impl.begin() + i, t);
    }

    void remove(size_t i) {
        impl.erase(impl.begin() + i);
    }

    void clear() {
        return impl.clear();
    }

    void shrink(size_t newLength) {
        ASSERT(newLength <= impl.size());
        impl.resize(newLength);
    }

    void shrinkToFit()
    {

    }

    void swap(Vector &other) {
        impl.swap(other.impl);
    }

    void deleteAllValues() {
        RELEASE_ASSERT_NOT_REACHED();
    }

    void reserve(size_t capacity) {
        impl.reserve(capacity);
    }
};


template <typename T, size_t N>
inline void
deleteAllValues(Vector<T, N> &v) {
    v.deleteAllValues();
}

static inline void
dataLog(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    va_end(ap);
}

#if ENABLE_YARR_JIT

/*
 * Minimal JSGlobalData. This used by Yarr to get the allocator.
 */
class JSGlobalData {
  public:
    ExecutableAllocator *regexAllocator;

    JSGlobalData(ExecutableAllocator *regexAllocator)
     : regexAllocator(regexAllocator) { }
};

#endif

 /*
  * Do-nothing version of a macro used by WTF to avoid unused
  * parameter warnings.
  */
#define UNUSED_PARAM(e)

} /* namespace Yarr */

} /* namespace JSC */

namespace WTF {
/*
 * Sentinel value used in Yarr.
 */
const size_t notFound = size_t(-1);

}

#define JS_EXPORT_PRIVATE

#endif /* yarr_wtfbridge_h */
