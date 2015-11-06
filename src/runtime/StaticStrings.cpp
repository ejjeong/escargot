#include "Escargot.h"
#include "vm/ESVMInstance.h"

namespace escargot {

void Strings::initStaticStrings(ESVMInstance* instance)
{
    emptyString = InternalAtomicString(instance, u"");
    NegativeInfinity = InternalAtomicString(instance, u"-Infinity");
    stringTrue = InternalAtomicString(instance, u"true");
    stringFalse = InternalAtomicString(instance, u"false");

    for (unsigned i = 0; i < ESCARGOT_ASCII_TABLE_MAX ; i ++) {
        ESString* str = ESString::create((char16_t)i);
        asciiTable[i] = InternalAtomicString(instance, str->string());
    }

    for (unsigned i = 0; i < ESCARGOT_STRINGS_NUMBERS_MAX ; i ++) {
        ESString* str = ESString::create((int)i);
        numbers[i] = InternalAtomicString(str->data());
    }

#define INIT_STATIC_STRING(name) name = InternalAtomicString(instance, u"" #name);
    FOR_EACH_STATIC_STRING(INIT_STATIC_STRING)
#undef INIT_STATIC_STRING
}

Strings* strings;

}
