#include "Escargot.h"
#include "vm/ESVMInstance.h"

namespace escargot {

void Strings::initStaticStrings(ESVMInstance* instance)
{
    emptyString = InternalAtomicString(instance, u"");
    for (unsigned i = 0; i < ESCARGOT_ASCII_TABLE_MAX ; i ++) {
        ESString* str = ESString::create((char16_t)i);
        asciiTable[i] = InternalAtomicString(instance, str->string());
    }
    null = InternalAtomicString(instance, u"null");
    undefined =  InternalAtomicString(instance, u"undefined");
    prototype =  InternalAtomicString(instance, u"prototype");
    constructor =  InternalAtomicString(instance, u"constructor");
    name =  InternalAtomicString(instance, u"name");
    length =  InternalAtomicString(instance, u"length");
    arguments =  InternalAtomicString(instance, u"arguments");
    __proto__ =  InternalAtomicString(instance, u"__proto__");

    for (unsigned i = 0; i < ESCARGOT_STRINGS_NUMBERS_MAX ; i ++) {
        ESString* str = ESString::create((int)i);
        numbers[i] = InternalAtomicString(str->data());
    }

    String =  InternalAtomicString(instance, u"String");
    Number =  InternalAtomicString(instance, u"Number");
    NaN =  InternalAtomicString(instance, u"NaN");
    Infinity =  InternalAtomicString(instance, u"Infinity");
    NegativeInfinity =  InternalAtomicString(instance, u"-Infinity");
    NEGATIVE_INFINITY =  InternalAtomicString(instance, u"NEGATIVE_INFINITY");
    POSITIVE_INFINITY =  InternalAtomicString(instance, u"POSITIVE_INFINITY");
    MAX_VALUE =  InternalAtomicString(instance, u"MAX_VALUE");
    MIN_VALUE =  InternalAtomicString(instance, u"MIN_VALUE");
    eval =  InternalAtomicString(instance, u"eval");
    Boolean =  InternalAtomicString(instance, u"Boolean");
    Object =  InternalAtomicString(instance, u"Object");
    Array =  InternalAtomicString(instance, u"Array");
    Error =  InternalAtomicString(instance, u"Error");
    message =  InternalAtomicString(instance, u"message");
    ReferenceError =  InternalAtomicString(instance, u"ReferenceError");
    TypeError =  InternalAtomicString(instance, u"TypeError");
    RangeError =  InternalAtomicString(instance, u"RangeError");
    SyntaxError =  InternalAtomicString(instance, u"SyntaxError");
    forEach =  InternalAtomicString(instance, u"forEach");
    isArray = InternalAtomicString(instance, u"isArray");
    concat = InternalAtomicString(instance, u"concat");
    indexOf = InternalAtomicString(instance, u"indexOf");
    lastIndexOf = InternalAtomicString(instance, u"lastIndexOf");
    join = InternalAtomicString(instance, u"join");
    push = InternalAtomicString(instance, u"push");
    pop = InternalAtomicString(instance, u"pop");
    slice = InternalAtomicString(instance, u"slice");
    splice = InternalAtomicString(instance, u"splice");
    shift = InternalAtomicString(instance, u"shift");
    sort = InternalAtomicString(instance, u"sort");
    Function =  InternalAtomicString(instance, u"Function");
    Empty =  InternalAtomicString(instance, u"Empty");
    Date =  InternalAtomicString(instance, u"Date");
    getDate =  InternalAtomicString(instance, u"getDate");
    getDay =  InternalAtomicString(instance, u"getDay");
    getFullYear =  InternalAtomicString(instance, u"getFullYear");
    getHours =  InternalAtomicString(instance, u"getHours");
    getMinutes =  InternalAtomicString(instance, u"getMinutes");
    getMonth =  InternalAtomicString(instance, u"getMonth");
    getSeconds =  InternalAtomicString(instance, u"getSeconds");
    getTime =  InternalAtomicString(instance, u"getTime");
    getTimezoneOffset =  InternalAtomicString(instance, u"getTimezoneOffset");
    setTime =  InternalAtomicString(instance, u"setTime");
    Math =  InternalAtomicString(instance, u"Math");
    PI =  InternalAtomicString(instance, u"PI");
    E =  InternalAtomicString(instance, u"E");
    abs =  InternalAtomicString(instance, u"abs");
    cos =  InternalAtomicString(instance, u"cos");
    ceil = InternalAtomicString(instance, u"ceil");
    max =  InternalAtomicString(instance, u"max");
    min =  InternalAtomicString(instance, u"min");
    floor =  InternalAtomicString(instance, u"floor");
    pow =  InternalAtomicString(instance, u"pow");
    random =  InternalAtomicString(instance, u"random");
    round =  InternalAtomicString(instance, u"round");
    sin =  InternalAtomicString(instance, u"sin");
    sqrt =  InternalAtomicString(instance, u"sqrt");
    tan =  InternalAtomicString(instance, u"tan");
    log =  InternalAtomicString(instance, u"log");
    toString =  InternalAtomicString(instance, u"toString");
    toLocaleString =  InternalAtomicString(instance, u"toLocaleString");
    boolean = InternalAtomicString(instance, u"boolean");
    number = InternalAtomicString(instance, u"number");
    toFixed =  InternalAtomicString(instance, u"toFixed");
    toPrecision =  InternalAtomicString(instance, u"toPrecision");
    string = InternalAtomicString(instance, u"string");
    object = InternalAtomicString(instance, u"object");
    function = InternalAtomicString(instance, u"function");
    stringTrue = InternalAtomicString(instance, u"true");
    stringFalse = InternalAtomicString(instance, u"false");
    RegExp = InternalAtomicString(instance, u"RegExp");
    source = InternalAtomicString(instance, u"source");
    valueOf = InternalAtomicString(instance, u"valueOf");
    test = InternalAtomicString(instance, u"test");
    exec = InternalAtomicString(instance, u"exec");
    input = InternalAtomicString(instance, u"input");
    index = InternalAtomicString(instance, u"index");
    Int8Array = InternalAtomicString(instance, u"Int8Array");
    Int16Array = InternalAtomicString(instance, u"Int16Array");
    Int32Array = InternalAtomicString(instance, u"Int32Array");
    Uint8Array = InternalAtomicString(instance, u"Uint8Array");
    Uint16Array = InternalAtomicString(instance, u"Uint16Array");
    Uint32Array = InternalAtomicString(instance, u"Uint32Array");
    Uint8ClampedArray = InternalAtomicString(instance, u"Uint8ClampedArray");
    Float32Array = InternalAtomicString(instance, u"Float32Array");
    Float64Array = InternalAtomicString(instance, u"Float64Array");
    ArrayBuffer = InternalAtomicString(instance, u"ArrayBuffer");
    byteLength = InternalAtomicString(instance, u"byteLength");
    subarray = InternalAtomicString(instance, u"subarray");
    set = InternalAtomicString(instance, u"set");
    buffer = InternalAtomicString(instance, u"buffer");
    JSON = InternalAtomicString(instance, u"JSON");
    parse = InternalAtomicString(instance, u"parse");
    stringify = InternalAtomicString(instance, u"stringify");
    toJSON = InternalAtomicString(instance, u"toJSON");
    getPrototypeOf = InternalAtomicString(instance, u"getPrototypeOf");
    isPrototypeOf = InternalAtomicString(instance, u"isPrototypeOf");
}

Strings* strings;

}
