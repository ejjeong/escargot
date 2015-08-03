#include "Escargot.h"
#include "vm/ESVMInstance.h"

namespace escargot {

void Strings::initStaticStrings(ESVMInstance* instance)
{
    null = InternalAtomicString(instance, L"null");
    undefined = InternalAtomicString(instance, L"undefined");
    prototype = InternalAtomicString(instance, L"prototype");
    constructor = InternalAtomicString(instance, L"constructor");
    name = InternalAtomicString(instance, L"name");
    arguments = InternalAtomicString(instance, L"arguments");
    length = InternalAtomicString(instance, L"length");
    __proto__ = InternalAtomicString(instance, L"__proto__");

    for(unsigned i = 0; i < ESCARGOT_STRINGS_NUMBERS_MAX ; i ++) {
        numbers[i] = InternalAtomicString(instance, InternalString((int)i).data());
    }

    String = InternalAtomicString(instance, L"String");
    Number = InternalAtomicString(instance, L"Number");
    Object = InternalAtomicString(instance, L"Object");
    ReferenceError = InternalAtomicString(instance, L"ReferenceError");
    Array = InternalAtomicString(instance, L"Array");
    Function = InternalAtomicString(instance, L"Function");
    Empty = InternalAtomicString(instance, L"Empty");
    Date = InternalAtomicString(instance, L"Date");
    getTime = InternalAtomicString(instance, L"getTime");
}

Strings* strings;

}
