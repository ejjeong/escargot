#include "Escargot.h"
#include "vm/ESVMInstance.h"

namespace escargot {

void Strings::initStaticStrings(ESVMInstance* instance)
{
    for(unsigned i = 0; i < ESCARGOT_ASCII_TABLE_MAX ; i ++) {
        asciiTable[i] = InternalString((wchar_t)i);
        esAsciiTable[i] = ESString::create(asciiTable[i]);
    }
    null = L"null";
    undefined =  L"undefined";
    prototype =  L"prototype";
    constructor =  L"constructor";
    name =  L"name";
    length =  L"length";
    atomicLength = InternalAtomicString(instance, L"length");
    atomicName =  InternalAtomicString(instance, L"name");
    arguments =  L"arguments";
    atomicArguments =  InternalAtomicString(instance, L"arguments");
    __proto__ =  L"__proto__";

    for(unsigned i = 0; i < ESCARGOT_STRINGS_NUMBERS_MAX ; i ++) {
        numbers[i] = InternalAtomicString(instance, InternalString((int)i).data());
        nonAtomicNumbers[i] = InternalString((int)i);
    }

    String =  L"String";
    Number =  L"Number";
    Boolean =  L"Boolean";
    Object =  L"Object";
    ReferenceError =  L"ReferenceError";
    Array =  L"Array";
    concat = L"concat";
    indexOf = L"indexOf";
    join = L"join";
    push = L"push";
    slice = L"slice";
    splice = L"splice";
    sort = L"sort";
    Function =  L"Function";
    Empty =  L"Empty";
    Date =  L"Date";
    getDate =  L"getDate";
    getDay =  L"getDay";
    getFullYear =  L"getFullYear";
    getHours =  L"getHours";
    getMinutes =  L"getMinutes";
    getMonth =  L"getMonth";
    getSeconds =  L"getSeconds";
    getTime =  L"getTime";
    getTimezoneOffset =  L"getTimezoneOffset";
    setTime =  L"setTime";
    Math =  L"Math";
    PI =  L"PI";
    abs =  L"abs";
    cos =  L"cos";
    ceil = L"ceil";
    max =  L"max";
    floor =  L"floor";
    pow =  L"pow";
    random =  L"random";
    round =  L"round";
    sin =  L"sin";
    sqrt =  L"sqrt";
    log =  L"log";
    toString =  L"toString";
    boolean = L"boolean";
    number = L"number";
    string = L"string";
    object = L"object";
    function = L"function";
    stringTrue = L"true";
    stringFalse = L"false";
    RegExp = L"RegExp";
    source = L"source";
    valueOf = L"valueOf";
    test = L"test";
}

Strings* strings;

}
