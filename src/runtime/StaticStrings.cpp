#include "Escargot.h"
#include "vm/ESVMInstance.h"

namespace escargot {

void Strings::initStaticStrings(ESVMInstance* instance)
{
    emptyESString = ESString::create(L"");
    for(unsigned i = 0; i < ESCARGOT_ASCII_TABLE_MAX ; i ++) {
        asciiTable[i] = ESString::create((wchar_t)i);
    }
    null = ESString::create(L"null");
    undefined =  ESString::create(L"undefined");
    prototype =  ESString::create(L"prototype");
    constructor =  ESString::create(L"constructor");
    name =  ESString::create(L"name");
    length =  ESString::create(L"length");
    atomicLength = InternalAtomicString(instance, L"length");
    atomicName =  InternalAtomicString(instance, L"name");
    arguments =  ESString::create(L"arguments");
    atomicArguments =  InternalAtomicString(instance, L"arguments");
    __proto__ =  ESString::create(L"__proto__");

    for(unsigned i = 0; i < ESCARGOT_STRINGS_NUMBERS_MAX ; i ++) {
        numbers[i] = InternalAtomicString(instance, ESString::create((int)i)->data());
        nonAtomicNumbers[i] = ESString::create((int)i);
    }

    String =  ESString::create(L"String");
    Number =  ESString::create(L"Number");
    Boolean =  ESString::create(L"Boolean");
    Object =  ESString::create(L"Object");
    Array =  ESString::create(L"Array");
    Error =  ESString::create(L"Error");
    ReferenceError =  ESString::create(L"ReferenceError");
    concat = ESString::create(L"concat");
    indexOf = ESString::create(L"indexOf");
    join = ESString::create(L"join");
    push = ESString::create(L"push");
    slice = ESString::create(L"slice");
    splice = ESString::create(L"splice");
    sort = ESString::create(L"sort");
    Function =  ESString::create(L"Function");
    Empty =  ESString::create(L"Empty");
    Date =  ESString::create(L"Date");
    getDate =  ESString::create(L"getDate");
    getDay =  ESString::create(L"getDay");
    getFullYear =  ESString::create(L"getFullYear");
    getHours =  ESString::create(L"getHours");
    getMinutes =  ESString::create(L"getMinutes");
    getMonth =  ESString::create(L"getMonth");
    getSeconds =  ESString::create(L"getSeconds");
    getTime =  ESString::create(L"getTime");
    getTimezoneOffset =  ESString::create(L"getTimezoneOffset");
    setTime =  ESString::create(L"setTime");
    Math =  ESString::create(L"Math");
    PI =  ESString::create(L"PI");
    abs =  ESString::create(L"abs");
    cos =  ESString::create(L"cos");
    ceil = ESString::create(L"ceil");
    max =  ESString::create(L"max");
    floor =  ESString::create(L"floor");
    pow =  ESString::create(L"pow");
    random =  ESString::create(L"random");
    round =  ESString::create(L"round");
    sin =  ESString::create(L"sin");
    sqrt =  ESString::create(L"sqrt");
    log =  ESString::create(L"log");
    toString =  ESString::create(L"toString");
    boolean = ESString::create(L"boolean");
    number = ESString::create(L"number");
    string = ESString::create(L"string");
    object = ESString::create(L"object");
    function = ESString::create(L"function");
    stringTrue = ESString::create(L"true");
    stringFalse = ESString::create(L"false");
    RegExp = ESString::create(L"RegExp");
    source = ESString::create(L"source");
    valueOf = ESString::create(L"valueOf");
    test = ESString::create(L"test");
}

Strings* strings;

}
