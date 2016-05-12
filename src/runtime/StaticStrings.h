#ifndef StaticStrings_h
#define StaticStrings_h

namespace escargot {

class ESVMInstance;
class ESString;

#define FOR_EACH_STATIC_STRING(F) \
    F(null) \
    F(undefined) \
    F(prototype) \
    F(constructor) \
    F(name) \
    F(arguments) \
    F(caller) \
    F(callee) \
    F(length) \
    F(__proto__) \
    F(value) \
    F(writable) \
    F(enumerable) \
    F(configurable) \
    F(get) \
    F(set) \
    F(String) \
    F(Number) \
    F(NaN) \
    F(Infinity) \
    F(NEGATIVE_INFINITY) \
    F(POSITIVE_INFINITY) \
    F(MAX_VALUE) \
    F(MIN_VALUE) \
    F(eval) \
    F(Object) \
    F(GlobalObject) \
    F(Boolean) \
    F(Error) \
    F(ReferenceError) \
    F(TypeError) \
    F(RangeError) \
    F(SyntaxError) \
    F(URIError) \
    F(EvalError) \
    F(message) \
    F(valueOf) \
    F(Array) \
    F(isArray) \
    F(concat) \
    F(forEach) \
    F(indexOf) \
    F(lastIndexOf) \
    F(join) \
    F(push) \
    F(pop) \
    F(slice) \
    F(splice) \
    F(shift) \
    F(sort) \
    F(Function) \
    F(Empty) \
    F(Date) \
    F(getDate) \
    F(getDay) \
    F(getFullYear) \
    F(getHours) \
    F(getMilliseconds) \
    F(getMinutes) \
    F(getMonth) \
    F(getSeconds) \
    F(getTime) \
    F(getTimezoneOffset) \
    F(getUTCDate) \
    F(getUTCDay) \
    F(getUTCFullYear) \
    F(getUTCHours) \
    F(getUTCMilliseconds) \
    F(getUTCMinutes) \
    F(getUTCMonth) \
    F(getUTCSeconds) \
    F(setDate) \
    F(setFullYear) \
    F(setHours) \
    F(setMilliseconds) \
    F(setMinutes) \
    F(setMonth) \
    F(setSeconds) \
    F(setTime) \
    F(setUTCDate) \
    F(setUTCFullYear) \
    F(setUTCHours) \
    F(setUTCMilliseconds) \
    F(setUTCMinutes) \
    F(setUTCMonth) \
    F(setUTCSeconds) \
    F(toDateString) \
    F(toISOString) \
    F(toLocaleDateString) \
    F(toLocaleTimeString) \
    F(toTimeString) \
    F(toUTCString) \
    F(getYear) \
    F(setYear) \
    F(toGMTString) \
    F(Math) \
    F(PI) \
    F(E) \
    F(abs) \
    F(cos) \
    F(ceil) \
    F(max) \
    F(min) \
    F(floor) \
    F(pow) \
    F(random) \
    F(round) \
    F(sin) \
    F(sqrt) \
    F(tan) \
    F(log) \
    F(toString) \
    F(toLocaleString) \
    F(boolean) \
    F(number) \
    F(toFixed) \
    F(toPrecision) \
    F(string) \
    F(object) \
    F(function) \
    F(RegExp) \
    F(source) \
    F(lastIndex) \
    F(test) \
    F(exec) \
    F(input) \
    F(index) \
    F(compile) \
    F(byteLength) \
    F(subarray) \
    F(buffer) \
    F(JSON) \
    F(parse) \
    F(stringify) \
    F(toJSON) \
    F(getPrototypeOf) \
    F(isPrototypeOf) \
    F(propertyIsEnumerable) \
    F(ignoreCase) \
    F(global) \
    F(multiline) \
    F(implements) \
    F(interface) \
    F(package) \
    F(yield) \
    F(let) \
    F(LN10) \
    F(LN2) \
    F(LOG10E) \
    F(LOG2E) \
    F(MAX_SAFE_INTEGER) \
    F(MIN_SAFE_INTEGER) \
    F(SQRT1_2) \
    F(SQRT2) \
    F(UTC) \
    F(acos) \
    F(acosh) \
    F(anonymous) \
    F(apply) \
    F(asin) \
    F(asinh) \
    F(atan) \
    F(atan2) \
    F(atanh) \
    F(bind) \
    F(call) \
    F(cbrt) \
    F(charAt) \
    F(charCodeAt) \
    F(create) \
    F(dbgBreak) \
    F(decodeURI) \
    F(decodeURIComponent) \
    F(defineProperties) \
    F(defineProperty) \
    F(encodeURI) \
    F(encodeURIComponent) \
    F(escape) \
    F(every) \
    F(exp) \
    F(fill) \
    F(filter) \
    F(find) \
    F(findIndex) \
    F(freeze) \
    F(fromCharCode) \
    F(gc) \
    F(gcHeapSize) \
    F(getOwnPropertyDescriptor) \
    F(getOwnPropertyNames) \
    F(hasOwnProperty) \
    F(imul) \
    F(isExtensible) \
    F(isFinite) \
    F(isFrozen) \
    F(isNaN) \
    F(isSealed) \
    F(keys) \
    F(load) \
    F(localeCompare) \
    F(map) \
    F(match) \
    F(now) \
    F(parseFloat) \
    F(parseInt) \
    F(preventExtensions) \
    F(print) \
    F(read) \
    F(append) \
    F(reduce) \
    F(reduceRight) \
    F(replace) \
    F(reverse) \
    F(run) \
    F(seal) \
    F(search) \
    F(some) \
    F(split) \
    F(startsWith) \
    F(substr) \
    F(substring) \
    F(toExponential) \
    F(toLocaleLowerCase) \
    F(toLocaleUpperCase) \
    F(toLowerCase) \
    F(toUpperCase) \
    F(trim) \
    F(unescape) \
    F(unshift) \
    FOR_EACH_ES6_STRING(F)

#ifdef USE_ES6_FEATURE
#define FOR_EACH_ES6_STRING(F) \
    F(TypedArray) \
    F(Int8Array) \
    F(Int16Array) \
    F(Int32Array) \
    F(Uint8Array) \
    F(Uint16Array) \
    F(Uint32Array) \
    F(Uint8ClampedArray) \
    F(Float32Array) \
    F(Float64Array) \
    F(ArrayBuffer)
#else
#define FOR_EACH_ES6_STRING(F)
#endif

class Strings {
public:
    InternalAtomicString emptyString;
    InternalAtomicString NegativeInfinity;
    InternalAtomicString stringTrue;
    InternalAtomicString stringFalse;
    InternalAtomicString stringPublic;
    InternalAtomicString stringProtected;
    InternalAtomicString stringPrivate;
    InternalAtomicString stringStatic;
    InternalAtomicString defaultRegExpString;

#define ESCARGOT_ASCII_TABLE_MAX 128
    InternalAtomicString asciiTable[ESCARGOT_ASCII_TABLE_MAX];

#define ESCARGOT_STRINGS_NUMBERS_MAX 128
    InternalAtomicString numbers[ESCARGOT_STRINGS_NUMBERS_MAX];

#define DECLARE_STATIC_STRING(name) InternalAtomicString name;
    FOR_EACH_STATIC_STRING(DECLARE_STATIC_STRING);
#undef DECLARE_STATIC_STRING

    void initStaticStrings(ESVMInstance* instance);
};

extern Strings* strings;

}

#endif
