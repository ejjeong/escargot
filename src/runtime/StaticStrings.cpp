#include "Escargot.h"
#include "vm/ESVMInstance.h"

namespace escargot {

void Strings::initStaticStrings(ESVMInstance* instance)
{
    null = ESAtomicString(instance, L"null");
    undefined = ESAtomicString(instance, L"undefined");
    prototype = ESAtomicString(instance, L"prototype");
    constructor = ESAtomicString(instance, L"constructor");
    name = ESAtomicString(instance, L"name");
    arguments = ESAtomicString(instance, L"arguments");
    length = ESAtomicString(instance, L"length");
    __proto__ = ESAtomicString(instance, L"__proto__");

    for(unsigned i = 0; i < ESCARGOT_STRINGS_NUMBERS_MAX ; i ++) {
        numbers[i] = ESAtomicString(instance, ESString((int)i).data());
    }

    String = ESAtomicString(instance, L"String");
    Number = ESAtomicString(instance, L"Number");
    Object = ESAtomicString(instance, L"Object");
    ReferenceError = ESAtomicString(instance, L"ReferenceError");
    Array = ESAtomicString(instance, L"Array");
    Function = ESAtomicString(instance, L"Function");
    Empty = ESAtomicString(instance, L"Empty");
    Date = ESAtomicString(instance, L"Date");
}

Strings* strings;

}
