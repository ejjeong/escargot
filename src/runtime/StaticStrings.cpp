#include "Escargot.h"
#include "vm/ESVMInstance.h"

namespace escargot {

void Strings::initStaticStrings(ESVMInstance* instance)
{
    emptyESString = ESString::create(u"");
    emptyAtomicString = InternalAtomicString(instance, u"");
    for(unsigned i = 0; i < ESCARGOT_ASCII_TABLE_MAX ; i ++) {
        asciiTable[i] = ESString::create((char16_t)i);
    }
    null = ESString::create(u"null");
    undefined =  ESString::create(u"undefined");
    prototype =  ESString::create(u"prototype");
    constructor =  ESString::create(u"constructor");
    name =  ESString::create(u"name");
    length =  ESString::create(u"length");
    atomicLength = InternalAtomicString(instance, u"length");
    atomicName =  InternalAtomicString(instance, u"name");
    arguments =  ESString::create(u"arguments");
    atomicArguments =  InternalAtomicString(instance, u"arguments");
    __proto__ =  ESString::create(u"__proto__");

    for(unsigned i = 0; i < ESCARGOT_STRINGS_NUMBERS_MAX ; i ++) {
        numbers[i] = InternalAtomicString(instance, ESString::create((int)i)->data());
        nonAtomicNumbers[i] = ESString::create((int)i);
    }

    String =  ESString::create(u"String");
    Number =  ESString::create(u"Number");
    NaN =  ESString::create(u"NaN");
    Infinity =  ESString::create(u"Infinity");
    NEGATIVE_INFINITY =  ESString::create(u"NEGATIVE_INFINITY");
    POSITIVE_INFINITY =  ESString::create(u"POSITIVE_INFINITY");
    MAX_VALUE =  ESString::create(u"MAX_VALUE");
    MIN_VALUE =  ESString::create(u"MIN_VALUE");
    eval =  ESString::create(u"eval");
    atomicEval = InternalAtomicString(instance, eval->data());
    Boolean =  ESString::create(u"Boolean");
    Object =  ESString::create(u"Object");
    Array =  ESString::create(u"Array");
    Error =  ESString::create(u"Error");
    message =  ESString::create(u"message");
    ReferenceError =  ESString::create(u"ReferenceError");
    TypeError =  ESString::create(u"TypeError");
    RangeError =  ESString::create(u"RangeError");
    SyntaxError =  ESString::create(u"SyntaxError");
    concat = ESString::create(u"concat");
    indexOf = ESString::create(u"indexOf");
    join = ESString::create(u"join");
    push = ESString::create(u"push");
    pop = ESString::create(u"pop");
    slice = ESString::create(u"slice");
    splice = ESString::create(u"splice");
    shift = ESString::create(u"shift");
    sort = ESString::create(u"sort");
    Function =  ESString::create(u"Function");
    Empty =  ESString::create(u"Empty");
    Date =  ESString::create(u"Date");
    getDate =  ESString::create(u"getDate");
    getDay =  ESString::create(u"getDay");
    getFullYear =  ESString::create(u"getFullYear");
    getHours =  ESString::create(u"getHours");
    getMinutes =  ESString::create(u"getMinutes");
    getMonth =  ESString::create(u"getMonth");
    getSeconds =  ESString::create(u"getSeconds");
    getTime =  ESString::create(u"getTime");
    getTimezoneOffset =  ESString::create(u"getTimezoneOffset");
    setTime =  ESString::create(u"setTime");
    Math =  ESString::create(u"Math");
    PI =  ESString::create(u"PI");
    E =  ESString::create(u"E");
    abs =  ESString::create(u"abs");
    cos =  ESString::create(u"cos");
    ceil = ESString::create(u"ceil");
    max =  ESString::create(u"max");
    min =  ESString::create(u"min");
    floor =  ESString::create(u"floor");
    pow =  ESString::create(u"pow");
    random =  ESString::create(u"random");
    round =  ESString::create(u"round");
    sin =  ESString::create(u"sin");
    sqrt =  ESString::create(u"sqrt");
    log =  ESString::create(u"log");
    toString =  ESString::create(u"toString");
    boolean = ESString::create(u"boolean");
    number = ESString::create(u"number");
    toFixed =  ESString::create(u"toFixed");
    toPrecision =  ESString::create(u"toPrecision");
    string = ESString::create(u"string");
    object = ESString::create(u"object");
    function = ESString::create(u"function");
    stringTrue = ESString::create(u"true");
    stringFalse = ESString::create(u"false");
    RegExp = ESString::create(u"RegExp");
    source = ESString::create(u"source");
    valueOf = ESString::create(u"valueOf");
    test = ESString::create(u"test");
    exec = ESString::create(u"exec");
    input = ESString::create(u"input");
    index = ESString::create(u"index");
    Int8Array = ESString::create(u"Int8Array");
    Int16Array = ESString::create(u"Int16Array");
    Int32Array = ESString::create(u"Int32Array");
    Uint8Array = ESString::create(u"Uint8Array");
    Uint16Array = ESString::create(u"Uint16Array");
    Uint32Array = ESString::create(u"Uint32Array");
    Uint8ClampedArray = ESString::create(u"Uint8ClampedArray");
    Float32Array = ESString::create(u"Float32Array");
    Float64Array = ESString::create(u"Float64Array");
}

Strings* strings;

}
