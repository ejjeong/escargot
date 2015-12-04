#include "Escargot.h"
#include "vm/ESVMInstance.h"

namespace escargot {

void Strings::initStaticStrings(ESVMInstance* instance)
{
    emptyString = InternalAtomicString(instance, "");
    NegativeInfinity = InternalAtomicString(instance, "-Infinity");
    stringTrue = InternalAtomicString(instance, "true");
    stringFalse = InternalAtomicString(instance, "false");

    for (unsigned i = 0; i < ESCARGOT_ASCII_TABLE_MAX ; i ++) {
        ESString* str = ESString::create((char)i);
        asciiTable[i] = InternalAtomicString(instance, str->asciiData(), str->length());
    }

    for (unsigned i = 0; i < ESCARGOT_STRINGS_NUMBERS_MAX ; i ++) {
        ESString* str = ESString::create((int)i);
        numbers[i] = InternalAtomicString(str->asciiData());
    }

#define INIT_STATIC_STRING(name) name = InternalAtomicString(instance, "" #name);
    FOR_EACH_STATIC_STRING(INIT_STATIC_STRING)
#undef INIT_STATIC_STRING
}

Strings* strings;

}
