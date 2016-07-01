/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "Escargot.h"
#include "vm/ESVMInstance.h"

namespace escargot {

void Strings::initStaticStrings(ESVMInstance* instance)
{
    emptyString = InternalAtomicString(instance, "");
    NegativeInfinity = InternalAtomicString(instance, "-Infinity");
    stringTrue = InternalAtomicString(instance, "true");
    stringFalse = InternalAtomicString(instance, "false");
    stringPublic = InternalAtomicString(instance, "public");
    stringProtected = InternalAtomicString(instance, "protected");
    stringPrivate = InternalAtomicString(instance, "private");
    stringStatic = InternalAtomicString(instance, "static");
    defaultRegExpString = InternalAtomicString(instance, "(?:)");

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
