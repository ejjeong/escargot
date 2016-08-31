/*
 *  Copyright (C) 1999-2001 Harri Porten (porten@kde.org)
 *  Copyright (C) 2001 Peter Kelly (pmk@post.com)
 *  Copyright (C) 2003, 2007, 2008, 2012 Apple Inc. All rights reserved.
 *  Copyright (C) 2016 Samsung Electronics Co., Ltd
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 *
 */

#include "Escargot.h"
#include "GlobalObject.h"
#include "ast/AST.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#include "parser/ScriptParser.h"
#include "bytecode/ByteCodeOperations.h"
#include "runtime/JobQueue.h"

#include "parser/esprima.h"

namespace escargot {

GlobalObject::GlobalObject()
    : ESObject(ESPointer::Type::ESObject, ESValue())
    , m_identifierInterceptor(nullptr)
{
    m_flags.m_isGlobalObject = true;
    m_didSomePrototypeObjectDefineIndexedProperty = false;
}

void GlobalObject::finalize()
{
    for (auto block : m_codeBlocks) {
        block->finalize(m_instance, false);
    }
    m_codeBlocks.clear();
}

const char* errorMessage_GlobalObject_ThisUndefinedOrNull = "%s: this value is undefined or null";
const char* errorMessage_GlobalObject_ThisNotObject = "%s: this value is not an object";
const char* errorMessage_GlobalObject_ThisNotRegExpObject = "%s: this value is not a RegExp object";
const char* errorMessage_GlobalObject_ThisNotDateObject = "%s: this value is not a Date object";
const char* errorMessage_GlobalObject_ThisNotFunctionObject = "%s: this value is not a Function object";
const char* errorMessage_GlobalObject_ThisNotBoolean = "%s: this value is not Boolean nor Boolean object";
const char* errorMessage_GlobalObject_ThisNotNumber = "%s: this value is not Number nor Number object";
const char* errorMessage_GlobalObject_ThisNotString = "%s: this value is not String nor String object";
const char* errorMessage_GlobalObject_ThisNotTypedArrayObject = "%s: this value is not a Typed Array object";
const char* errorMessage_GlobalObject_MalformedURI = "%s: malformed URI";
const char* errorMessage_GlobalObject_RangeError = "%s: invalid range";
const char* errorMessage_GlobalObject_FileNotExist = "%s: cannot load file";
const char* errorMessage_GlobalObject_NotExecutable = "%s: cannot run";
const char* errorMessage_GlobalObject_FirstArgumentNotObject = "%s: first argument is not an object";
const char* errorMessage_GlobalObject_SecondArgumentNotObject = "%s: second argument is not an object";
const char* errorMessage_GlobalObject_DescriptorNotObject = "%s: descriptor is not an object";
const char* errorMessage_GlobalObject_ToLoacleStringNotCallable = "%s: toLocaleString is not callable";
const char* errorMessage_GlobalObject_ToISOStringNotCallable = "%s: toISOString is not callable";
const char* errorMessage_GlobalObject_CallbackNotCallable = "%s: callback is not callable";
const char* errorMessage_GlobalObject_InvalidDate = "%s: Invalid Date";
const char* errorMessage_GlobalObject_JAError = "%s: JA error";
const char* errorMessage_GlobalObject_JOError = "%s: JO error";
const char* errorMessage_GlobalObject_RadixInvalidRange = "%s: radix is invalid range";
const char* errorMessage_GlobalObject_NotDefineable = "%s: cannot define property";
const char* errorMessage_GlobalObject_FirstArgumentNotObjectAndNotNull = "%s: first argument is not an object and not null";
const char* errorMessage_GlobalObject_ReduceError = "%s: reduce of empty array with no initial value";
const char* errorMessage_GlobalObject_FirstArgumentNotCallable = "%s: first argument is not callable";
const char* errorMessage_GlobalObject_FirstArgumentNotString = "%s: first argument is not a string";
const char* errorMessage_GlobalObject_FirstArgumentInvalidLength = "%s: first arugment is an invalid length value";
const char* errorMessage_GlobalObject_InvalidArrayBufferOffset = "%s: ArrayBuffer length minus the byteOffset is not a multiple of the element size";
const char* errorMessage_GlobalObject_NotExistNewInArrayBufferConstructor = "%s: Constructor ArrayBuffer requires \'new\'";
const char* errorMessage_GlobalObject_NotExistNewInTypedArrayConstructor = "%s: Constructor TypedArray requires \'new\'";
const char* errorMessage_GlobalObject_InvalidArrayLength = "%s: Invalid array length";

NEVER_INLINE void throwBuiltinError(ESVMInstance* instance, ESErrorObject::Code code,
    const InternalAtomicString& objectName, bool prototoype, const InternalAtomicString& functionName, const char* templateString)
{
    ESStringBuilder replacerBuilder;
    if (objectName != strings->emptyString) {
        replacerBuilder.appendString(objectName.string());
    }
    if (prototoype) {
        replacerBuilder.appendChar('.');
        replacerBuilder.appendString(strings->prototype.string());
    }
    if (functionName != strings->emptyString) {
        replacerBuilder.appendChar('.');
        replacerBuilder.appendString(functionName.string());
    }

    ASSERT(instance);
    instance->throwError(code, templateString, replacerBuilder.finalize());
    RELEASE_ASSERT_NOT_REACHED();
}

UTF16String codePointTo4digitString(int codepoint)
{
    UTF16String ret;
    int d = 16 * 16* 16;
    for (int i = 0; i < 4; ++i) {
        if (codepoint >= d) {
            char16_t c;
            if (codepoint / d < 10) {
                c = (codepoint / d) + u'0';
            } else {
                c = (codepoint / d) - 10 + u'a';
            }
            codepoint %= d;
            ret.append(&c, 1);
        } else {
            ret.append(u"0");
        }
        d >>= 4;
    }

    return ret;
}

ASCIIString char2hex(char dec)
{
    unsigned char dig1 = (dec & 0xF0) >> 4;
    unsigned char dig2 = (dec & 0x0F);
    if (dig1 <= 9)
        dig1 += 48; // 0, 48inascii
    if (10 <= dig1 && dig1 <= 15)
        dig1 += 65 - 10; // a, 97inascii
    if (dig2 <= 9)
        dig2 += 48;
    if (10 <= dig2 && dig2 <= 15)
        dig2 += 65 - 10;

    ASCIIString r;
    char dig1_appended = static_cast<char>(dig1);
    char dig2_appended = static_cast<char>(dig2);
    r.append(&dig1_appended, 1);
    r.append(&dig2_appended, 1);
    return r;
}

ASCIIString char2hex4digit(char16_t dec)
{
    char dig[4];
    ASCIIString r;
    for (int i = 0; i < 4; i++) {
        dig[i] = (dec & (0xF000 >> i * 4)) >> (12 - i * 4);
        if (dig[i] <= 9)
            dig[i] += 48; // 0, 48inascii
        if (10 <= dig[i] && dig[i] <= 15)
            dig[i] += 65 - 10; // a, 97inascii
        r.append(&dig[i], 1);
    }
    return r;
}

char16_t hex2char(char16_t first, char16_t second)
{
    char16_t dig1 = first;
    char16_t dig2 = second;
    if (48 <= dig1 && dig1 <= 57)
        dig1 -= 48;
    if (65 <= dig1 && dig1 <= 70)
        dig1 -= 65 - 10;
    if (97 <= dig1 && dig1 <= 102)
        dig1 -= 97 - 10;
    if (48 <= dig2 && dig2 <= 57)
        dig2 -= 48;
    if (65 <= dig2 && dig2 <= 70)
        dig2 -= 65 - 10;
    if (97 <= dig2 && dig2 <= 102)
        dig2 -= 97 - 10;

    char16_t dec = dig1 << 4;
    dec |= dig2;

    return dec;
}

static int parseDigit(char16_t c, int radix)
{
    int digit = -1;

    if (c >= '0' && c <= '9')
        digit = c - '0';
    else if (c >= 'A' && c <= 'Z')
        digit = c - 'A' + 10;
    else if (c >= 'a' && c <= 'z')
        digit = c - 'a' + 10;

    if (digit >= radix)
        return -1;

    return digit;
}

static const int SizeOfInfinity = 8;

static bool isInfinity(ESString* str, unsigned p, unsigned length)
{
    return (length - p) >= SizeOfInfinity
        && str->charAt(p) == 'I'
        && str->charAt(p + 1) == 'n'
        && str->charAt(p + 2) == 'f'
        && str->charAt(p + 3) == 'i'
        && str->charAt(p + 4) == 'n'
        && str->charAt(p + 5) == 'i'
        && str->charAt(p + 6) == 't'
        && str->charAt(p + 7) == 'y';
}

void GlobalObject::initGlobalObject()
{
    m_instance = ESVMInstance::currentInstance();
    forceNonVectorHiddenClass(true);
    m_objectPrototype = ESObject::create();
    m_objectPrototype->forceNonVectorHiddenClass(true);
    m_objectPrototype->set__proto__(ESValue(ESValue::ESNull));

    set__proto__(m_objectPrototype);
    installFunction();
    installObject();
    installArray();
    installString();
    installError();
    installDate();
    installMath();
    installJSON();
    installNumber();
    installBoolean();
    installRegExp();
#ifdef USE_ES6_FEATURE
    installArrayBuffer();
    installTypedArray();
    installPromise();
#endif

    // Value Properties of the Global Object
    defineDataProperty(strings->Infinity, false, false, false, ESValue(std::numeric_limits<double>::infinity()));
    defineDataProperty(strings->NaN, false, false, false, ESValue(std::numeric_limits<double>::quiet_NaN()));
    defineDataProperty(strings->undefined, false, false, false, ESValue());

    auto gcFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        GC_gcollect();
        return ESValue();
    }, strings->gc.string());
    set(strings->gc.string(), gcFunction);

#ifdef ESCARGOT_STANDALONE
    auto brkFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        printf("dbgBreak\n");
        return ESValue();
    }, strings->dbgBreak.string());
    set(strings->dbgBreak.string(), brkFunction);

    auto printFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        for (size_t i = 0; i < instance->currentExecutionContext()->argumentCount(); i++) {
            if (i != 0)
                printf(" ");
            ESVMInstance::printValue(instance->currentExecutionContext()->arguments()[i], false);
        }
        printf("\n");
        return ESValue();
    }, strings->print.string());
    set(strings->print.string(), printFunction);

    auto gcHeapSizeFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        unsigned size = GC_get_heap_size();
        return ESValue(size);
    }, strings->gcHeapSize.string());
    set(strings->gcHeapSize.string(), gcHeapSizeFunction);

    auto loadFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        if (instance->currentExecutionContext()->argumentCount()) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            escargot::ESString* str = val.toString();
            const char* origPath = str->utf8Data();
            char fileName[1000];
            const char* origPtr = origPath;
            char* ptr = fileName;
            while (true) {
                if (*origPtr == '\\')  {
                    *ptr++ = '/';
                    origPtr++;
                } else
                    *ptr++ = *origPtr++;
                if (!*origPtr)
                    break;
            }
            *ptr = *origPtr;
            FILE* fp = fopen(fileName, "r");

            if (fp) {
                fseek(fp, 0L, SEEK_END);
                size_t sz = ftell(fp);
                fseek(fp, 0L, SEEK_SET);
                ASCIIString str;
                str.reserve(sz+2);
                static char buf[4096];
                while (fgets(buf, sizeof buf, fp) != NULL) {
                    str += buf;
                }
                fclose(fp);
                return instance->evaluate(escargot::ESString::create(std::move(str)));
            }
        }
        throwBuiltinError(instance, ErrorCode::TypeError, strings->GlobalObject, false, strings->load, errorMessage_GlobalObject_FileNotExist);
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->load.string());
    set(strings->load.string(), loadFunction);

    auto runFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        timeval tv;
        gettimeofday(&tv, 0);
        long long int ms = tv.tv_sec * 1000 + tv.tv_usec / 1000;

        if (instance->currentExecutionContext()->argumentCount()) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            escargot::ESString* str = val.toString();
            FILE* fp = fopen(str->utf8Data(), "r");
            if (fp) {
                fseek(fp, 0L, SEEK_END);
                size_t sz = ftell(fp);
                fseek(fp, 0L, SEEK_SET);
                ASCIIString str;
                str.reserve(sz+2);
                static char buf[4096];
                while (fgets(buf, sizeof buf, fp) != NULL) {
                    str += buf;
                }
                fclose(fp);

                instance->evaluate(escargot::ESString::create(std::move(str)));
                gettimeofday(&tv, 0);
                long long int timeSpent = tv.tv_sec * 1000 + tv.tv_usec / 1000 - ms;
                return ESValue(timeSpent);
            }
        }
        throwBuiltinError(instance, ErrorCode::TypeError, strings->GlobalObject, false, strings->run, errorMessage_GlobalObject_NotExecutable);
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->run.string());
    set(strings->run.string(), runFunction);

    auto readFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        if (instance->currentExecutionContext()->argumentCount()) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            escargot::ESString* str = val.toString();
            FILE* fp = fopen(str->utf8Data(), "r");
            if (fp) {
                fseek(fp, 0L, SEEK_END);
                size_t sz = ftell(fp);
                fseek(fp, 0L, SEEK_SET);
                std::string str;
                str.reserve(sz+2);
                static char buf[4096];
                while (fgets(buf, sizeof buf, fp) != NULL) {
                    str += buf;
                }
                fclose(fp);

                escargot::ESString* data = ESString::create(str.data());
                return data;
            }
        }
        throwBuiltinError(instance, ErrorCode::TypeError, strings->GlobalObject, false, strings->read, "%s: cannot read");
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->read.string());
    set(strings->read.string(), readFunction);

    auto appendFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        if (instance->currentExecutionContext()->argumentCount() > 1) {
            ESValue path = instance->currentExecutionContext()->readArgument(0);
            ESValue content = instance->currentExecutionContext()->readArgument(1);
            const char* pathStr = path.toString()->utf8Data();
            const char* contentStr = content.toString()->utf8Data();
            FILE* fp = fopen(pathStr, "a");
            if (fp) {
                fputs(contentStr, fp);
                fclose(fp);
                return ESValue();
            }
        }
        throwBuiltinError(instance, ErrorCode::TypeError, strings->GlobalObject, false, strings->append, "%s: cannot append");
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->append.string());
    set(strings->append.string(), appendFunction);
#endif

#ifndef NDEBUG
    auto debugOnFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESCARGOT_LOG_INFO("debugOn");
        instance->m_debug = true;
        instance->m_dumpExecuteByteCode = true;
        return ESValue(true);
    }, strings->debugOn.string());
    set(strings->debugOn.string(), debugOnFunction);

    auto debugOffFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESCARGOT_LOG_INFO("debugOff");
        instance->m_debug = false;
        instance->m_dumpExecuteByteCode = false;
        return ESValue(false);
    }, strings->debugOff.string());
    set(strings->debugOff.string(), debugOffFunction);
#endif

    // Function Properties of the Global Object
    m_eval = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue argument = instance->currentExecutionContext()->readArgument(0);
        if (!argument.isESString()) {
            return argument;
        }
        return instance->evaluateEval(argument.asESString(), false, nullptr);
    }, strings->eval.string(), 1);
    defineDataProperty(strings->eval, true, false, true, m_eval);

    // $18.2.2
    defineDataProperty(strings->isFinite, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue ret;
        int len = instance->currentExecutionContext()->argumentCount();
        if (len < 1)
            ret = ESValue(ESValue::ESFalseTag::ESFalse);
        else {
            ESValue& argument = instance->currentExecutionContext()->arguments()[0];
            double num = argument.toNumber();
            if (std::isnan(num) || num == std::numeric_limits<double>::infinity() || num == -std::numeric_limits<double>::infinity())
                ret = ESValue(ESValue::ESFalseTag::ESFalse);
            else
                ret = ESValue(ESValue::ESTrueTag::ESTrue);
        }
        return ret;
    }, strings->isFinite.string(), 1));

    // $18.2.3
    defineDataProperty(strings->isNaN, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue argument = instance->currentExecutionContext()->readArgument(0);
        double num = argument.toNumber();
        return ESValue(std::isnan(num));
    }, strings->isNaN.string(), 1));

    // $18.2.4 parseFloat(string)
    defineDataProperty(strings->parseFloat, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // 1. Let inputString be ToString(string).
        ESValue input = instance->currentExecutionContext()->readArgument(0);
        escargot::ESString* s = input.toString();
        size_t strLen = s->length();

        if (strLen == 1) {
            if (isdigit(s->charAt(0)))
                return ESValue(s->charAt(0) - '0');
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }

        // FIXME we should not create string in this place
        // 2, Let trimmedString be a substring of inputString consisting of the leftmost character
        //    that is not a StrWhiteSpaceChar and all characters to the right of that character.
        //    (In other words, remove leading white space.)
        unsigned p = 0;
        unsigned len = s->length();

        for (; p < len; p++) {
            char16_t c = s->charAt(p);
            if (!(esprima::isWhiteSpace(c) || esprima::isLineTerminator(c)))
                break;
        }

        // empty string
        if (p == len)
            return ESValue(std::numeric_limits<double>::quiet_NaN());

        char16_t ch = s->charAt(p);
        // HexIntegerLiteral
        if (len - p > 1 && ch == '0' && toupper(s->charAt(p + 1)) == 'X')
            return ESValue(0);

        // 3. If neither trimmedString nor any prefix of trimmedString satisfies the syntax of
        //    a StrDecimalLiteral (see 9.3.1), return NaN.
        // 4. Let numberString be the longest prefix of trimmedString, which might be trimmedString itself,
        //    that satisfies the syntax of a StrDecimalLiteral.
        // Check the syntax of StrDecimalLiteral
        switch (ch) {
        case 'I':
            if (isInfinity(s, p, len))
                return ESValue(std::numeric_limits<double>::infinity());
            break;
        case '+':
            if (isInfinity(s, p + 1, len))
                return ESValue(std::numeric_limits<double>::infinity());
            break;
        case '-':
            if (isInfinity(s, p + 1, len))
                return ESValue(-std::numeric_limits<double>::infinity());
            break;
        }

        NullableUTF8String u8Str = s->substring(p, len)->toNullableUTF8String();
        double number = atof(u8Str.m_buffer);
        if (number == 0.0 && !std::signbit(number) && !isdigit(ch) && !(len - p >= 1 && ch == '.' && isdigit(s->charAt(p + 1))))
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        if (number == std::numeric_limits<double>::infinity())
            return ESValue(std::numeric_limits<double>::quiet_NaN());

        // 5. Return the Number value for the MV of numberString.
        return ESValue(number);
    }, strings->parseFloat.string(), 1));

    // $18.2.5 parseInt(string, radix)
    defineDataProperty(strings->parseInt, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue ret;
        int len = instance->currentExecutionContext()->argumentCount();

        // 1. Let inputString be ToString(string).
        ESValue input = instance->currentExecutionContext()->readArgument(0);
        escargot::ESString* s = input.toString();

        // 2. Let S be a newly created substring of inputString consisting of the first character that is not a StrWhiteSpaceChar
        //    and all characters following that character. (In other words, remove leading white space.)
        unsigned p = 0;
        unsigned strLen = s->length();

        for (; p < strLen; p++) {
            char16_t c = s->charAt(p);
            if (!(esprima::isWhiteSpace(c) || esprima::isLineTerminator(c)))
                break;
        }

        // 3. Let sign be 1.
        // 4. If S is not empty and the first character of S is a minus sign -, let sign be −1.
        // 5. If S is not empty and the first character of S is a plus sign + or a minus sign -, then remove the first character from S.
        double sign = 1;
        if (p < strLen) {
            if (s->charAt(p) == '+')
                p++;
            else if (s->charAt(p) == '-') {
                sign = -1;
                p++;
            }
        }

        // 6. Let R = ToInt32(radix).
        // 7. Let stripPrefix be true.
        // 8. If R ≠ 0, then
        //    b. If R 16, let stripPrefix be false.
        // 9. Else, R = 0
        //    a. Let R = 10.
        // 10. If stripPrefix is true, then
        //     a. If the length of S is at least 2 and the first two characters of S are either “0x” or “0X”, then remove the first two characters from S and let R = 16.
        // 11. If S contains any character that is not a radix-R digit, then let Z be the substring of S consisting of all characters
        //     before the first such character; otherwise, let Z be S.
        int radix = 0;
        if (len >= 2) {
            radix = instance->currentExecutionContext()->arguments()[1].toInt32();
        }
        if ((radix == 0 || radix == 16) && strLen - p >= 2 && s->charAt(p) == '0' && (s->charAt(p + 1) == 'x' || s->charAt(p + 1) == 'X')) {
            radix = 16;
            p += 2;
        }
        if (radix == 0)
            radix = 10;

        // 8.a If R < 2 or R > 36, then return NaN.
        if (radix < 2 || radix > 36)
            return ESValue(std::numeric_limits<double>::quiet_NaN());

        // 13. Let mathInt be the mathematical integer value that is represented by Z in radix-R notation,
        //     using the letters AZ and az for digits with values 10 through 35. (However, if R is 10 and Z contains more than 20 significant digits,
        //     every significant digit after the 20th may be replaced by a 0 digit, at the option of the implementation;
        //     and if R is not 2, 4, 8, 10, 16, or 32, then mathInt may be an implementation-dependent approximation to the mathematical integer value
        //     that is represented by Z in radix-R notation.)
        // 14. Let number be the Number value for mathInt.
        bool sawDigit = false;
        double number = 0.0;
        while (p < strLen) {
            int digit = parseDigit(s->charAt(p), radix);
            if (digit == -1)
                break;
            sawDigit = true;
            number *= radix;
            number += digit;
            p++;
        }

        // 12. If Z is empty, return NaN.
        if (!sawDigit)
            return ESValue(std::numeric_limits<double>::quiet_NaN());

        // 15. Return sign × number.
        return ESValue(sign * number);
    }, strings->parseInt.string(), 2));

    // $18.2.6.2 decodeURI
    defineDataProperty(strings->decodeURI, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int argLen = instance->currentExecutionContext()->argumentCount();
        if (argLen == 0)
            return ESValue();

        escargot::ESString* stringValue = instance->currentExecutionContext()->arguments()->toString();
        NullableUTF8String componentString = stringValue->toNullableUTF8String();
        int strLen = stringValue->length();

        UTF16String unescaped;
        for (int i = 0; i < strLen; i++) {
            char16_t t = stringValue->charAt(i);
            if (t != '%') {
                unescaped.append(&t, 1);
            } else {
                int start = i;
                if (i+2 >= strLen)
                    throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURI, errorMessage_GlobalObject_MalformedURI);
                char16_t next = stringValue->charAt(i+1);
                char16_t nextnext = stringValue->charAt(i+2);
                if (!((48 <= next && next <= 57) || (65 <= next && next <= 70) || (97 <= next && next <= 102))) // hex digit check
                    throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURI, errorMessage_GlobalObject_MalformedURI);
                if (!((48 <= nextnext && nextnext <= 57) || (65 <= nextnext && nextnext <= 70) || (97 <= nextnext && nextnext <= 102)))
                    throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURI, errorMessage_GlobalObject_MalformedURI);

                // char to hex
                unsigned char b = (((next & 0x10) ? (next & 0xf) : ((next & 0xf) + 9)) << 4)
                    | ((nextnext & 0x10) ? (nextnext & 0xf) : ((nextnext & 0xf) + 9));

                i += 2;

                // most significant bit in b is 0
                if (!(b & 0x80)) {
                    // let C be the character with code unit value B.
                    // if C is not in reservedSet, then let S be the String containing only the character C.
                    // else, C is in reservedSet, Let S be the substring of string from position start to position k included.
                    const char16_t c = b & 0x7f;
                    if ((c == ';' || c == '/' || c == '?' // uriReserved
                        || c == ':' || c == '@' || c == '&'
                        || c == '=' || c == '+' || c == '$'
                        || c == ',')
                        || c == '#') { // special case
                        unescaped.append(1, stringValue->charAt(start));
                        unescaped.append(1, stringValue->charAt(start+1));
                        unescaped.append(1, stringValue->charAt(start+2));
                    } else {
                        unescaped.append(&c, 1);
                    }
                } else { // most significant bit in b is 1
                    unsigned char b_tmp = b;
                    int n = 1;
                    while (n < 5) {
                        b_tmp <<= 1;
                        if ((b_tmp & 0x80) == 0) {
                            break;
                        }
                        n++;
                    }
                    if (n == 1 || n == 5) {
                        throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURI, errorMessage_GlobalObject_MalformedURI);
                    }
                    unsigned char octets[4];
                    octets[0] = b;
                    if (i + (3 * (n - 1)) >= strLen) {
                        throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURI, errorMessage_GlobalObject_MalformedURI);
                    }

                    int j = 1;
                    while (j < n) {
                        i++;
                        if (stringValue->charAt(i) != '%') {
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURI, errorMessage_GlobalObject_MalformedURI);
                        }
                        next = stringValue->charAt(i+1);
                        nextnext = stringValue->charAt(i+2);
                        if (!((48 <= next && next <= 57) || (65 <= next && next <= 70) || (97 <= next && next <= 102))) // hex digit check
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURI, errorMessage_GlobalObject_MalformedURI);
                        if (!((48 <= nextnext && nextnext <= 57) || (65 <= nextnext && nextnext <= 70) || (97 <= nextnext && nextnext <= 102)))
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURI, errorMessage_GlobalObject_MalformedURI);

                        b = (((next & 0x10) ? (next & 0xf) : ((next & 0xf) + 9)) << 4)
                            | ((nextnext & 0x10) ? (nextnext & 0xf) : ((nextnext & 0xf) + 9));

                        if ((b & 0xC0) != 0x80) {
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURI, errorMessage_GlobalObject_MalformedURI);
                        }

                        i += 2;
                        octets[j] = b;
                        j++;
                    }
                    unsigned int v;
                    if (n == 2) {
                        v = (octets[0] & 0x1F) << 6 | (octets[1] & 0x3F);
                        if ((octets[0] == 0xC0) || (octets[0] == 0xC1)) {
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURI, errorMessage_GlobalObject_MalformedURI); // overlong
                        }
                    } else if (n == 3) {
                        v = (octets[0] & 0x0F) << 12 | (octets[1] & 0x3F) << 6 | (octets[2] & 0x3F);
                        if (0xD800 <= v && v <= 0xDFFF) {
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURI, errorMessage_GlobalObject_MalformedURI);
                        }
                        if ((octets[0] == 0xE0) && ((octets[1] < 0xA0) || (octets[1] > 0xBF))) {
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURI, errorMessage_GlobalObject_MalformedURI); // overlong
                        }
                    } else if (n == 4) {
                        v = (octets[0] & 0x07) << 18 | (octets[1] & 0x3F) << 12 | (octets[2] & 0x3F) << 6 | (octets[3] & 0x3F);
                        if ((octets[0] == 0xF0) && ((octets[1] < 0x90) || (octets[1] > 0xBF))) {
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURI, errorMessage_GlobalObject_MalformedURI); // overlong
                        }
                    } else {
                        RELEASE_ASSERT_NOT_REACHED();
                    }
                    if (v >= 0x10000) {
                        const char16_t l = (((v - 0x10000) & 0x3ff) + 0xdc00);
                        const char16_t h = ((((v - 0x10000) >> 10) & 0x3ff) + 0xd800);
                        unescaped.append(&h, 1);
                        unescaped.append(&l, 1);
                    } else {
                        const char16_t l = v & 0xFFFF;
                        unescaped.append(&l, 1);
                    }
                }
            }
        }
        return escargot::ESString::create(std::move(unescaped));
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->decodeURI.string(), 1));

    // $18.2.6.3 decodeURIComponent(encodedURIComponent)
    defineDataProperty(strings->decodeURIComponent, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int argLen = instance->currentExecutionContext()->argumentCount();
        if (argLen == 0)
            return ESValue();

        escargot::ESString* stringValue = instance->currentExecutionContext()->arguments()->toString();
        NullableUTF8String componentString = stringValue->toNullableUTF8String();
        int strLen = stringValue->length();

        UTF16String unescaped;
        for (int i = 0; i < strLen; i++) {
            char16_t t = stringValue->charAt(i);
            if (t != '%') {
                unescaped.append(&t, 1);
            } else {
                // int start = i;
                if (i+2 >= strLen)
                    throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURIComponent, errorMessage_GlobalObject_MalformedURI);
                char16_t next = stringValue->charAt(i+1);
                char16_t nextnext = stringValue->charAt(i+2);
                if (!((48 <= next && next <= 57) || (65 <= next && next <= 70) || (97 <= next && next <= 102))) // hex digit check
                    throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURIComponent, errorMessage_GlobalObject_MalformedURI);
                if (!((48 <= nextnext && nextnext <= 57) || (65 <= nextnext && nextnext <= 70) || (97 <= nextnext && nextnext <= 102)))
                    throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURIComponent, errorMessage_GlobalObject_MalformedURI);

                unsigned char b = (((next & 0x10) ? (next & 0xf) : ((next & 0xf) + 9)) << 4)
                    | ((nextnext & 0x10) ? (nextnext & 0xf) : ((nextnext & 0xf) + 9));

                i += 2;

                if (!(b & 0x80)) {
                    // let C be the character with code unit value B.
                    // if C is not in reservedSet, then let S be the String containing only the character C.
                    // else, C is in reservedSet, Let S be the substring of string from position start to position k included.
                    const char16_t c = b & 0x7f;
                    unescaped.append(&c, 1);
                } else { // most significant bit in b is 1
                    unsigned char b_tmp = b;
                    int n = 1;
                    while (n < 5) {
                        b_tmp <<= 1;
                        if ((b_tmp & 0x80) == 0) {
                            break;
                        }
                        n++;
                    }
                    if (n == 1 || n == 5) {
                        throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURIComponent, errorMessage_GlobalObject_MalformedURI);
                    }
                    unsigned char octets[4];
                    octets[0] = b;
                    if (i + (3 * (n - 1)) >= strLen) {
                        throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURIComponent, errorMessage_GlobalObject_MalformedURI);
                    }

                    int j = 1;
                    while (j < n) {
                        i++;
                        if (stringValue->charAt(i) != '%') {
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURIComponent, errorMessage_GlobalObject_MalformedURI);
                        }
                        next = stringValue->charAt(i+1);
                        nextnext = stringValue->charAt(i+2);
                        if (!((48 <= next && next <= 57) || (65 <= next && next <= 70) || (97 <= next && next <= 102))) // hex digit check
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURIComponent, errorMessage_GlobalObject_MalformedURI);
                        if (!((48 <= nextnext && nextnext <= 57) || (65 <= nextnext && nextnext <= 70) || (97 <= nextnext && nextnext <= 102)))
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURIComponent, errorMessage_GlobalObject_MalformedURI);

                        b = (((next & 0x10) ? (next & 0xf) : ((next & 0xf) + 9)) << 4)
                            | ((nextnext & 0x10) ? (nextnext & 0xf) : ((nextnext & 0xf) + 9));

                        if ((b & 0xC0) != 0x80) {
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURIComponent, errorMessage_GlobalObject_MalformedURI);
                        }

                        i += 2;
                        octets[j] = b;
                        j++;
                    }
                    unsigned int v;
                    if (n == 2) {
                        v = (octets[0] & 0x1F) << 6 | (octets[1] & 0x3F);
                        if ((octets[0] == 0xC0) || (octets[0] == 0xC1)) {
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURIComponent, errorMessage_GlobalObject_MalformedURI); // overlong
                        }
                    } else if (n == 3) {
                        v = (octets[0] & 0x0F) << 12 | (octets[1] & 0x3F) << 6 | (octets[2] & 0x3F);
                        if (0xD800 <= v && v <= 0xDFFF) {
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURIComponent, errorMessage_GlobalObject_MalformedURI);
                        }
                        if ((octets[0] == 0xE0) && ((octets[1] < 0xA0) || (octets[1] > 0xBF))) {
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURIComponent, errorMessage_GlobalObject_MalformedURI); // overlong
                        }
                    } else if (n == 4) {
                        v = (octets[0] & 0x07) << 18 | (octets[1] & 0x3F) << 12 | (octets[2] & 0x3F) << 6 | (octets[3] & 0x3F);
                        if ((octets[0] == 0xF0) && ((octets[1] < 0x90) || (octets[1] > 0xBF))) {
                            throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->decodeURIComponent, errorMessage_GlobalObject_MalformedURI); // overlong
                        }
                    } else {
                        RELEASE_ASSERT_NOT_REACHED();
                    }
                    if (v >= 0x10000) {
                        const char16_t l = (((v - 0x10000) & 0x3ff) + 0xdc00);
                        const char16_t h = ((((v - 0x10000) >> 10) & 0x3ff) + 0xd800);
                        unescaped.append(&h, 1);
                        unescaped.append(&l, 1);
                    } else {
                        const char16_t l = v & 0xFFFF;
                        unescaped.append(&l, 1);
                    }
                }
            }
        }
        return escargot::ESString::create(std::move(unescaped));
    }, strings->decodeURIComponent.string(), 1));

    // $18.2.6.4 encodeURI(uri)
    defineDataProperty(strings->encodeURI, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int argLen = instance->currentExecutionContext()->argumentCount();
        if (argLen == 0)
            return ESValue();

        escargot::ESString* stringValue = instance->currentExecutionContext()->arguments()->toString();
        NullableUTF8String componentString = stringValue->toNullableUTF8String();
        int strLen = stringValue->length();

        ASCIIString escaped;
        for (int i = 0; i < strLen; i++) {
            char16_t t = stringValue->charAt(i);
            if ((48 <= t && t <= 57) // DecimalDigit
                || (65 <= t && t <= 90) // uriAlpha - lower case
                || (97 <= t && t <= 122) // uriAlpha - lower case
                || (t == '-' || t == '_' || t == '.' // uriMark
                || t == '!' || t == '~'
                || t == '*' || t == '\'' || t == '('
                || t == ')')
                || (t == ';' || t == '/' || t == '?' // uriReserved
                || t == ':' || t == '@' || t == '&'
                || t == '=' || t == '+' || t == '$'
                || t == ',')
                || t == '#') { // special case
                escaped.append(1, stringValue->charAt(i));
            } else if (t <= 0x007F) {
                escaped.append("%");
                escaped.append(char2hex(t));
            } else if (0x0080 <= t && t <= 0x07FF) {
                escaped.append("%");
                escaped.append(char2hex(0x00C0 + (t & 0x07C0) / 0x0040));
                escaped.append("%");
                escaped.append(char2hex(0x0080 + (t & 0x003F)));
            } else if ((0x0800 <= t && t <= 0xD7FF)
                || (0xE000 <= t/* && t <= 0xFFFF*/)) {
                escaped.append("%");
                escaped.append(char2hex(0x00E0 + (t & 0xF000) / 0x1000));
                escaped.append("%");
                escaped.append(char2hex(0x0080 + (t & 0x0FC0) / 0x0040));
                escaped.append("%");
                escaped.append(char2hex(0x0080 + (t & 0x003F)));
            } else if (0xD800 <= t && t <= 0xDBFF) {
                if (i + 1 == strLen) {
                    throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->encodeURI, errorMessage_GlobalObject_MalformedURI);
                } else {
                    if (0xDC00 <= stringValue->charAt(i + 1) && stringValue->charAt(i + 1) <= 0xDFFF) {
                        int index = (t - 0xD800) * 0x400 + (stringValue->charAt(i + 1) - 0xDC00) + 0x10000;
                        escaped.append("%");
                        escaped.append(char2hex(0x00F0 + (index & 0x1C0000) / 0x40000));
                        escaped.append("%");
                        escaped.append(char2hex(0x0080 + (index & 0x3F000) / 0x1000));
                        escaped.append("%");
                        escaped.append(char2hex(0x0080 + (index & 0x0FC0) / 0x0040));
                        escaped.append("%");
                        escaped.append(char2hex(0x0080 + (index & 0x003F)));
                        i++;
                    } else {
                        throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->encodeURI, errorMessage_GlobalObject_MalformedURI);
                    }
                }
            } else if (0xDC00 <= t && t <= 0xDFFF) {
                throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->encodeURI, errorMessage_GlobalObject_MalformedURI);
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
        }
        return escargot::ESString::create(std::move(escaped));
    }, strings->encodeURI.string(), 1));

    // $18.2.6.5 encodeURIComponent(uriComponent)
    defineDataProperty(strings->encodeURIComponent, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int argLen = instance->currentExecutionContext()->argumentCount();
        if (argLen == 0)
            return ESValue();

        escargot::ESString* stringValue = instance->currentExecutionContext()->arguments()->toString();
        NullableUTF8String componentString = stringValue->toNullableUTF8String();
        int strLen = stringValue->length();

        ASCIIString escaped;
        for (int i = 0; i < strLen; i++) {
            char16_t t = stringValue->charAt(i);
            if ((48 <= t && t <= 57) // DecimalDigit
                || (65 <= t && t <= 90) // uriAlpha - lower case
                || (97 <= t && t <= 122) // uriAlpha - lower case
                || (t == '-' || t == '_' || t == '.' // uriMark
                || t == '!' || t == '~'
                || t == '*' || t == '\'' || t == '('
                || t == ')'))  {
                escaped.append(1, stringValue->charAt(i));
            } else if (t <= 0x007F) {
                escaped.append("%");
                escaped.append(char2hex(t));
            } else if (0x0080 <= t && t <= 0x07FF) {
                escaped.append("%");
                escaped.append(char2hex(0x00C0 + (t & 0x07C0) / 0x0040));
                escaped.append("%");
                escaped.append(char2hex(0x0080 + (t & 0x003F)));
            } else if ((0x0800 <= t && t <= 0xD7FF)
                || (0xE000 <= t/* && t <= 0xFFFF*/)) {
                escaped.append("%");
                escaped.append(char2hex(0x00E0 + (t & 0xF000) / 0x1000));
                escaped.append("%");
                escaped.append(char2hex(0x0080 + (t & 0x0FC0) / 0x0040));
                escaped.append("%");
                escaped.append(char2hex(0x0080 + (t & 0x003F)));
            } else if (0xD800 <= t && t <= 0xDBFF) {
                if (i + 1 == strLen) {
                    throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->encodeURIComponent, errorMessage_GlobalObject_MalformedURI);
                } else {
                    if (0xDC00 <= stringValue->charAt(i + 1) && stringValue->charAt(i + 1) <= 0xDFFF) {
                        int index = (t - 0xD800) * 0x400 + (stringValue->charAt(i + 1) - 0xDC00) + 0x10000;
                        escaped.append("%");
                        escaped.append(char2hex(0x00F0 + (index & 0x1C0000) / 0x40000));
                        escaped.append("%");
                        escaped.append(char2hex(0x0080 + (index & 0x3F000) / 0x1000));
                        escaped.append("%");
                        escaped.append(char2hex(0x0080 + (index & 0x0FC0) / 0x0040));
                        escaped.append("%");
                        escaped.append(char2hex(0x0080 + (index & 0x003F)));
                        i++;
                    } else {
                        throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->encodeURIComponent, errorMessage_GlobalObject_MalformedURI);
                    }
                }
            } else if (0xDC00 <= t && t <= 0xDFFF) {
                throwBuiltinError(instance, ErrorCode::URIError, strings->GlobalObject, false, strings->encodeURIComponent, errorMessage_GlobalObject_MalformedURI);
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
        }

        return escargot::ESString::create(std::move(escaped));
    }, strings->encodeURIComponent.string(), 1));

    // $B.2.1.1 escape(string)
    defineDataProperty(strings->escape, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESString* str = instance->currentExecutionContext()->readArgument(0).toString();
        size_t length = str->length();
        ASCIIString R = "";
        for (size_t i = 0; i < length; i++) {
            char16_t t = str->charAt(i);
            if ((48 <= t && t <= 57) // DecimalDigit
                || (65 <= t && t <= 90) // uriAlpha - upper case
                || (97 <= t && t <= 122) // uriAlpha - lower case
                || t == '@' || t == '*' || t == '_' || t == '+' || t == '-' || t == '.' || t == '/') {
                R.push_back(t);
            } else if (t < 256) {
                // %xy
                R.append("%");
                R.append(char2hex(t));
            } else {
                // %uwxyz
                R.append("%u");
                R.append(char2hex4digit(t));
            }
        }
        return escargot::ESString::create(std::move(R));
    }, strings->escape.string(), 1));

    // $B.2.1.2 unescape(string)
    defineDataProperty(strings->unescape, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESString* str = instance->currentExecutionContext()->readArgument(0).toString();
        size_t length = str->length();
        UTF16String R;
        bool unescapeValue = false;
        for (size_t i = 0; i < length; i++) {
            char16_t first = str->charAt(i);
            if (first == '%') {
                if (length - i >= 6) {
                    char16_t second = str->charAt(i+1);
                    char16_t third = str->charAt(i+2);
                    if (second == 'u') {
                        char16_t fourth = str->charAt(i+3);
                        char16_t fifth = str->charAt(i+4);
                        char16_t sixth = str->charAt(i+5);

                        // hex dig check
                        if (((48 <= third && third <= 57) || (65 <= third && third <= 70) || (97 <= third && third <= 102))
                            && ((48 <= fourth && fourth <= 57) || (65 <= fourth && fourth <= 70) || (97 <= fourth && fourth <= 102))
                            && ((48 <= fifth && fifth <= 57) || (65 <= fifth && fifth <= 70) || (97 <= fifth && fifth <= 102))
                            && ((48 <= sixth && sixth <= 57) || (65 <= sixth && sixth <= 70) || (97 <= sixth && sixth <= 102))) {
                            char16_t l = hex2char(third, fourth) << 8;
                            l |= hex2char(fifth, sixth);
                            R.append(&l, 1);
                            i += 5;
                            unescapeValue = true;
                        }
                    } else if (((48 <= second && second <= 57) || (65 <= second && second <= 70) || (97 <= second && second <= 102))
                        && ((48 <= third && third <= 57) || (65 <= third && third <= 70) || (97 <= third && third <= 102))) {
                        char16_t l = hex2char(second, third);
                        R.append(&l, 1);
                        i += 2;
                        unescapeValue = true;
                    }
                } else if (length - i >= 3) {
                    char16_t second = str->charAt(i+1);
                    char16_t third = str->charAt(i+2);
                    if (((48 <= second && second <= 57) || (65 <= second && second <= 70) || (97 <= second && second <= 102))
                        && ((48 <= third && third <= 57) || (65 <= third && third <= 70) || (97 <= third && third <= 102))) {
                        char16_t l = hex2char(second, third);
                        R.append(&l, 1);
                        i += 2;
                        unescapeValue = true;
                    }
                }
            }

            if (!unescapeValue) {
                char16_t l = str->charAt(i);
                R.append(&l, 1);
            }
            unescapeValue = false;
        }
        return escargot::ESString::create(std::move(R));
    }, strings->unescape.string(), 1));
}


void GlobalObject::installFunction()
{
    // $19.2.1 Function Constructor
    m_function = ESFunctionObject::create(NULL, [](ESVMInstance* instance) -> ESValue {
        int len = instance->currentExecutionContext()->argumentCount();
        CodeBlock* codeBlock;
        if (len == 0) {
            codeBlock = CodeBlock::create(ExecutableType::FunctionCode);
            ParserContextInformation parserContextInformation;
            ByteCodeGenerateContext context(codeBlock, parserContextInformation);
            codeBlock->pushCode(ReturnFunction(), context, NULL);
            codeBlock->pushCode(End(), context, NULL);
            codeBlock->m_hasCode = true;
        } else {
            ESStringBuilder argBuilder, bodyBuilder;

            argBuilder.appendString(strings->asciiTable[(size_t)'('].string());
            for (int i = 0; i < len-1; i++) {
                argBuilder.appendString(instance->currentExecutionContext()->arguments()[i].toString());
                if (i != len-2) {
                    argBuilder.appendString(strings->asciiTable[(size_t)','].string());
                }
            }
            argBuilder.appendString(strings->asciiTable[(size_t)'\n'].string());
            argBuilder.appendString(strings->asciiTable[(size_t)')'].string());
            escargot::ESString* argSource = argBuilder.finalize();

            bodyBuilder.appendString(strings->asciiTable[(size_t)'{'].string());
            escargot::ESString* rawBody = instance->currentExecutionContext()->arguments()[len-1].toString();
            bodyBuilder.appendString(rawBody);
            bodyBuilder.appendString(strings->asciiTable[(size_t)'\n'].string());
            bodyBuilder.appendString(strings->asciiTable[(size_t)'}'].string());
            escargot::ESString* bodySource = bodyBuilder.finalize();

            ParserContextInformation parserContextInformation;
            codeBlock = instance->scriptParser()->parseSingleFunction(instance, argSource, bodySource, parserContextInformation);
        }
        escargot::ESFunctionObject* function;
        LexicalEnvironment* scope = instance->globalExecutionContext()->environment();
        if (instance->currentExecutionContext()->isNewExpression() && instance->currentExecutionContext()->resolveThisBindingToObject()->isESFunctionObject()) {
            function = instance->currentExecutionContext()->resolveThisBindingToObject()->asESFunctionObject();
            function->initialize(scope, codeBlock);
        } else
            function = ESFunctionObject::create(scope, codeBlock, strings->anonymous.string(), codeBlock->m_argumentCount);
        ESObject* prototype = ESObjectCreate();
        prototype->set__proto__(instance->globalObject()->object()->protoType());
        return function;
    }, strings->Function, 1, true); // $19.2.2.1 Function.length: This is a data property with a value of 1.
    ::escargot::ESFunctionObject* emptyFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        return ESValue();
    }, strings->Function, 1);
    m_function->forceNonVectorHiddenClass(true);
    m_function->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    m_functionPrototype = emptyFunction;
    m_functionPrototype->forceNonVectorHiddenClass(true);
    m_functionPrototype->set__proto__(m_objectPrototype);
    m_functionPrototype->setProtoType(emptyFunction);
    m_function->set__proto__(emptyFunction);
    m_function->setProtoType(emptyFunction);
    m_functionPrototype->defineDataProperty(strings->constructor, true, false, true, m_function);

    ESVMInstance::currentInstance()->setGlobalFunctionPrototype(m_functionPrototype);

    // Function.prototype.toString
    m_functionPrototype->defineDataProperty(strings->toString, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // FIXME
        if (instance->currentExecutionContext()->resolveThisBindingToObject()->isESFunctionObject()) {
            escargot::ESFunctionObject* fn = instance->currentExecutionContext()->resolveThisBindingToObject()->asESFunctionObject();
            ESStringBuilder builder;
            builder.appendString("function ");
            builder.appendString(fn->name());
            builder.appendString("() {");
            if (fn->codeBlock()->m_isBuiltInFunction)
                builder.appendString(" [native code] ");
            builder.appendString("}");
            return builder.finalize();
        }
        throwBuiltinError(instance, ErrorCode::TypeError, strings->Function, true, strings->toString, errorMessage_GlobalObject_ThisNotFunctionObject);
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->toString, 0));

    // $19.2.3.1 Function.prototype.apply(thisArg, argArray)
    m_functionPrototype->defineDataProperty(strings->apply, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        if (!thisValue.isESPointer() || !thisValue.asESPointer()->isESFunctionObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Function, true, strings->apply, errorMessage_GlobalObject_ThisNotFunctionObject);
        auto thisVal = thisValue.asESPointer()->asESFunctionObject();
        ESValue thisArg = instance->currentExecutionContext()->readArgument(0);
        ESValue argArray = instance->currentExecutionContext()->readArgument(1);
        size_t arrlen;
        ESValue* arguments = nullptr;
        if (argArray.isUndefinedOrNull()) {
            // do nothing
            arrlen = 0;
            arguments = nullptr;
        } else if (argArray.isObject()) {
            if (argArray.asESPointer()->isESArrayObject()) {
                escargot::ESArrayObject* argArrayObj = argArray.asESPointer()->asESArrayObject();
                arrlen = argArrayObj->length();
                instance->argumentCountCheck(arrlen);
                ALLOCA_WRAPPER(instance, arguments, ESValue*, sizeof(ESValue) * arrlen, false);
                for (size_t i = 0; i < arrlen; i++) {
                    arguments[i] = argArrayObj->get(i);
                }
            } else {
                escargot::ESObject* obj = argArray.asESPointer()->asESObject();
                arrlen = obj->get(strings->length.string()).toUint32();
                instance->argumentCountCheck(arrlen);
                ALLOCA_WRAPPER(instance, arguments, ESValue*, sizeof(ESValue) * arrlen, false);
                for (size_t i = 0; i < arrlen; i++) {
                    arguments[i] = obj->get(ESValue(i));
                }
            }
        } else {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Function, true, strings->apply, errorMessage_GlobalObject_SecondArgumentNotObject);
        }

        return ESFunctionObject::call(instance, thisVal, thisArg, arguments, arrlen, false);
    }, strings->apply.string(), 2));

    // 19.2.3.2 Function.prototype.bind (thisArg , ...args)
    m_functionPrototype->defineDataProperty(strings->bind, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisVal = instance->currentExecutionContext()->resolveThisBinding();
        if (!thisVal.isESPointer() || !thisVal.asESPointer()->isESFunctionObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Function, true, strings->bind, errorMessage_GlobalObject_ThisNotFunctionObject);
        }
        escargot::ESFunctionObject* boundTargetFunction = thisVal.asESPointer()->asESFunctionObject();
        escargot::ESValue boundThis = instance->currentExecutionContext()->readArgument(0);
        size_t boundArgumentsCount = (instance->currentExecutionContext()->argumentCount() >= 2) ? instance->currentExecutionContext()->argumentCount() - 1 : 0;
        ESValue* boundArguments = instance->currentExecutionContext()->arguments() + 1;

        escargot::ESFunctionObject* function = escargot::ESFunctionObject::createBoundFunction(instance, boundTargetFunction, boundThis, boundArguments, boundArgumentsCount);

        function->defineAccessorProperty(strings->caller.string(), instance->throwerAccessorData(), true, false, false);
        function->defineAccessorProperty(strings->arguments.string(), instance->throwerAccessorData(), true, false, false);
        // NOTE
        // The binded function has only one bytecode what is CallBoundFunction
        // so we should not try JIT for binded function.
        return function;
    }, strings->bind.string(), 1));

    // 19.2.3.3 Function.prototype.call (thisArg , ...args)
    m_functionPrototype->defineDataProperty(strings->call, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        auto thisVal = instance->currentExecutionContext()->resolveThisBinding();
        if (!thisVal.isESPointer() || !thisVal.asESPointer()->isESFunctionObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Function, true, strings->call, errorMessage_GlobalObject_ThisNotFunctionObject);
        size_t arglen = instance->currentExecutionContext()->argumentCount();
        size_t callArgLen = (arglen > 0) ? arglen - 1 : 0;
        ESValue thisArg = instance->currentExecutionContext()->readArgument(0);
        instance->argumentCountCheck(callArgLen);
        ESValue* arguments;
        ALLOCA_WRAPPER(instance, arguments, ESValue*, sizeof(ESValue) * callArgLen, false);
        for (size_t i = 1; i < arglen; i++) {
            arguments[i - 1] = instance->currentExecutionContext()->arguments()[i];
        }

        return ESFunctionObject::call(instance, thisVal, thisArg, arguments, callArgLen, false);
    }, strings->call.string(), 1));

    defineDataProperty(strings->Function, true, false, true, m_function);
}

inline ESValue objectDefineProperties(ESVMInstance* instance, ESValue object, ESValue& properties)
{
    if (!object.isObject())
        throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, false, strings->defineProperty, errorMessage_GlobalObject_FirstArgumentNotObject);
    ESObject* props = properties.toObject();
    std::vector<std::pair<ESValue, PropertyDescriptor> > descriptors;
    props->enumeration([&](ESValue key) {
        bool hasKey = props->hasOwnProperty(key);
        if (hasKey) {
            ESValue propertyDesc = props->get(key);
            if (!propertyDesc.isObject())
                throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, false, strings->defineProperty, errorMessage_GlobalObject_DescriptorNotObject);
            descriptors.push_back(std::make_pair(key, PropertyDescriptor(propertyDesc.asESPointer()->asESObject())));
        }
    });
    for (auto it : descriptors) {
        if (object.toObject()->isESArrayObject())
            object.asESPointer()->asESArrayObject()->defineOwnProperty(it.first, it.second, true);
        else
            object.asESPointer()->asESObject()->defineOwnProperty(it.first, it.second, true);
    }
    return object;
}

void GlobalObject::installObject()
{
    ::escargot::ESFunctionObject* emptyFunction = m_functionPrototype;
    m_object = ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue value = instance->currentExecutionContext()->readArgument(0);
        if (value.isUndefined() || value.isNull()) {
            return ESObjectCreate();
        } else {
            return value.toObject();
        }
        return ESValue();
    }, strings->Object, 1, true);
    m_object->forceNonVectorHiddenClass(true);
    m_object->set__proto__(emptyFunction);
    m_object->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);
    m_object->setProtoType(m_objectPrototype);
    m_objectPrototype->defineDataProperty(strings->constructor, true, false, true, m_object);

    m_objectProtoTypeToString = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        if (thisValue.isUndefined())
            return ESString::createAtomicString("[object Undefined]");
        if (thisValue.isNull())
            return ESString::createAtomicString("[object Null]");
        ESObject* thisVal = thisValue.toObject();
        if (thisVal->isESArrayObject()) {
            return ESString::createAtomicString("[object Array]");
        } else if (thisVal->isESStringObject()) {
            return ESString::createAtomicString("[object String]");
        } else if (thisVal->isESFunctionObject()) {
            return ESString::createAtomicString("[object Function]");
        } else if (thisVal->isESErrorObject()) {
            return ESString::createAtomicString("[object Error]");
        } else if (thisVal->isESBooleanObject()) {
            return ESString::createAtomicString("[object Boolean]");
        } else if (thisVal->isESNumberObject()) {
            return ESString::createAtomicString("[object Number]");
        } else if (thisVal->isESDateObject()) {
            return ESString::createAtomicString("[object Date]");
        } else if (thisVal->isESRegExpObject()) {
            return ESString::createAtomicString("[object RegExp]");
        } else if (thisVal->isESMathObject()) {
            return ESString::createAtomicString("[object Math]");
        } else if (thisVal->isESJSONObject()) {
            return ESString::createAtomicString("[object JSON]");
#ifdef USE_ES6_FEATURE
        } else if (thisVal->isESTypedArrayObject()) {
            ASCIIString ret = "[object ";
            ESValue ta_constructor = thisVal->get(strings->constructor.string());
            // ALWAYS created from new expression
            ASSERT(ta_constructor.isESPointer() && ta_constructor.asESPointer()->isESObject());
            ESValue ta_name = ta_constructor.asESPointer()->asESObject()->get(strings->name.string());
            ret.append(ta_name.toString()->asciiData());
            ret.append("]");
            return ESString::createAtomicString(ret.data());
        } else if (thisVal->isESPromiseObject()) {
            return ESString::createAtomicString("[object Promise]");
#endif
        } else if (thisVal->isESArgumentsObject()) {
            return ESString::createAtomicString("[object Arguments]");
        } else if (thisVal->isGlobalObject()) {
            return ESString::createAtomicString("[object global]");
        }
        return ESString::createAtomicString("[object Object]");
    }, strings->toString, 0);
    // Object.prototype.toString
    m_objectPrototype->defineDataProperty(strings->toString, true, false, true, m_objectProtoTypeToString);

    // $19.1.3.2 Object.prototype.hasOwnProperty(V)
    m_objectPrototype->defineDataProperty(strings->hasOwnProperty, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESString* keyString = instance->currentExecutionContext()->readArgument(0).toPrimitive(ESValue::PrimitiveTypeHint::PreferString).toString();
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObj, Object, hasOwnProperty);
        ESValue ret = ESValue(thisObj->hasOwnProperty(keyString));
        return ret;
    }, strings->hasOwnProperty.string(), 1));

    // $19.1.2.3 Object.defineProperties ( O, P, Attributes )
    m_object->defineDataProperty(strings->defineProperties, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue object = instance->currentExecutionContext()->readArgument(0);
        ESValue properties = instance->currentExecutionContext()->readArgument(1);
        return objectDefineProperties(instance, object, properties);
    }, strings->defineProperties.string(), 2));

    // $19.1.2.4 Object.defineProperty ( O, P, Attributes )
    // http://www.ecma-international.org/ecma-262/6.0/#sec-object.defineproperty
    m_object->defineDataProperty(strings->defineProperty, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue objVal = instance->currentExecutionContext()->readArgument(0);
        if (!objVal.isObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, false, strings->defineProperty, errorMessage_GlobalObject_FirstArgumentNotObject);
        }
        ESObject* obj = objVal.toObject();

        escargot::ESString* name = instance->currentExecutionContext()->readArgument(1).toString();
        ESValue descVal = instance->currentExecutionContext()->readArgument(2);

        if (!descVal.isObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, false, strings->defineProperty, errorMessage_GlobalObject_DescriptorNotObject);
        }
        PropertyDescriptor desc = PropertyDescriptor(descVal.toObject());

        if (obj->isESArrayObject()) {
            obj->asESArrayObject()->defineOwnProperty(name, desc, true);
        } else {
            obj->defineOwnProperty(name, desc, true);
        }

        return objVal;
    }, strings->defineProperty.string(), 3));

    // $19.1.2.2 Object.create ( O [ , Properties ] )
    // http://www.ecma-international.org/ecma-262/6.0/#sec-object.defineproperty
    m_object->defineDataProperty(strings->create, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue proto = instance->currentExecutionContext()->readArgument(0);
        if (!proto.isObject() && !proto.isNull()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, false, strings->create, errorMessage_GlobalObject_FirstArgumentNotObjectAndNotNull);
        }
        ESObject* obj = ESObjectCreate();
        if (proto.isNull())
            obj->set__proto__(ESValue(ESValue::ESNull));
        else
            obj->set__proto__(proto);
        if (!instance->currentExecutionContext()->readArgument(1).isUndefined()) {
            return objectDefineProperties(instance, obj, instance->currentExecutionContext()->arguments()[1]);
        }
        return obj;
    }, strings->create.string(), 2));

    // $19.1.2.5 Object.freeze ( O )
    m_object->defineDataProperty(strings->freeze, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue O = instance->currentExecutionContext()->readArgument(0);
        if (!O.isObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, false, strings->freeze, errorMessage_GlobalObject_FirstArgumentNotObject);
        ESObject* obj = O.toObject();
        obj->forceNonVectorHiddenClass();
        if (obj->isESArrayObject())
            obj->asESArrayObject()->convertToSlowMode();
        std::vector<std::pair<ESValue, ESHiddenClassPropertyInfo*> > writableOrconfigurableProperties;
        obj->enumerationWithNonEnumerable([&](ESValue key, ESHiddenClassPropertyInfo* propertyInfo) {
            propertyInfo->setWritable(false);
            propertyInfo->setConfigurable(false);
        });
        obj->setExtensible(false);
        return O;
    }, strings->freeze.string(), 1));

    // $19.1.2.6 Object.getOwnPropertyDescriptor
    m_object->defineDataProperty(strings->getOwnPropertyDescriptor, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue arg0 = instance->currentExecutionContext()->readArgument(0);
        if (!arg0.isObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, false, strings->getOwnPropertyDescriptor, errorMessage_GlobalObject_FirstArgumentNotObject);
        ESObject* obj = arg0.toObject();

        ESValue arg1 = instance->currentExecutionContext()->readArgument(1);
        escargot::ESString* propertyKey = arg1.toString();

        size_t idx = obj->hiddenClass()->findProperty(propertyKey);
        if (idx != SIZE_MAX)
            return escargot::PropertyDescriptor::fromPropertyDescriptor(obj, propertyKey, idx);
        else {
            if (UNLIKELY(obj->hasPropertyInterceptor())) {
                ESValue v = obj->readKeyForPropertyInterceptor(propertyKey);
                if (!v.isDeleted()) {
                    ESObject* desc = ESObjectCreate();
                    desc->set(strings->value.string(), v);
                    desc->set(strings->writable.string(), ESValue(false));
                    desc->set(strings->enumerable.string(), ESValue(false));
                    desc->set(strings->configurable.string(), ESValue(false));
                    return desc;
                }
            }
            return escargot::PropertyDescriptor::fromPropertyDescriptorForIndexedProperties(obj, arg1.toIndex());
        }
    }, strings->getOwnPropertyDescriptor.string(), 2));

    // $19.1.2.7 Object.getOwnPropertyNames
    m_object->defineDataProperty(strings->getOwnPropertyNames, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue O = instance->currentExecutionContext()->readArgument(0);
        if (!O.isObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, false, strings->getOwnPropertyNames, errorMessage_GlobalObject_FirstArgumentNotObject);
        ESObject* obj = O.toObject();
        escargot::ESArrayObject* nameList = ESArrayObject::create();
        obj->enumerationWithNonEnumerable([&nameList](ESValue key, ESHiddenClassPropertyInfo*) {
            if (key.isESString())
                nameList->push(key);
        });
        return nameList;
    }, strings->getOwnPropertyNames.string(), 1));

    // $19.1.2.9 Object.getPrototypeOf
    m_object->defineDataProperty(strings->getPrototypeOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue O = instance->currentExecutionContext()->readArgument(0);
        if (!O.isObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, false, strings->getPrototypeOf, errorMessage_GlobalObject_FirstArgumentNotObject);
        return O.toObject()->__proto__();
    }, strings->getPrototypeOf, 1));

    // $19.1.2.9 Object.isExtensible( O )
    m_object->defineDataProperty(strings->isExtensible, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue O = instance->currentExecutionContext()->readArgument(0);
        if (!O.isObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, false, strings->isExtensible, errorMessage_GlobalObject_FirstArgumentNotObject);
        return ESValue(O.asESPointer()->asESObject()->isExtensible());
    }, strings->isExtensible.string(), 1));

    // $19.1.2.12 Object.isFrozen ( O )
    m_object->defineDataProperty(strings->isFrozen, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue O = instance->currentExecutionContext()->readArgument(0);
        if (!O.isObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, false, strings->isFrozen, errorMessage_GlobalObject_FirstArgumentNotObject);
        ESObject* obj = O.toObject();
        bool hasWritableConfigurableProperty = false;
        obj->enumerationWithNonEnumerable([&](ESValue key, ESHiddenClassPropertyInfo* propertyInfo) {
            if (propertyInfo->isDataProperty() || propertyInfo->isNativeAccessorProperty()) {
                if (propertyInfo->writable())
                    hasWritableConfigurableProperty = true;
            }
            if (propertyInfo->configurable())
                hasWritableConfigurableProperty = true;
        });
        if (hasWritableConfigurableProperty)
            return ESValue(false);
        if (!obj->isExtensible())
            return ESValue(true);
        return ESValue(false);
        return ESValue(true);
    }, strings->isFrozen.string(), 1));

    // $19.1.2.13 Object.isSealed ( O )
    m_object->defineDataProperty(strings->isSealed, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue O = instance->currentExecutionContext()->readArgument(0);
        if (!O.isObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, false, strings->isSealed, errorMessage_GlobalObject_FirstArgumentNotObject);
        ESObject* obj = O.toObject();
        bool hasConfigurableProperty = false;
        obj->enumerationWithNonEnumerable([&](ESValue key, ESHiddenClassPropertyInfo* propertyInfo) {
            if (propertyInfo->configurable())
                hasConfigurableProperty = true;
        });
        if (hasConfigurableProperty)
            return ESValue(false);
        if (!obj->isExtensible())
            return ESValue(true);
        return ESValue(false);
    }, strings->isSealed.string(), 1));

    // $19.1.2.14 Object.keys ( O )
    m_object->defineDataProperty(strings->keys, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // Let obj be ToObject(O).
        ESValue O = instance->currentExecutionContext()->readArgument(0);
        if (!O.isObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, false, strings->keys, errorMessage_GlobalObject_FirstArgumentNotObject);
        ESObject* obj = O.toObject();
        escargot::ESArrayObject* arr = ESArrayObject::create();
        obj->enumeration([&arr](ESValue key) {
            arr->push(key);
        });
        return arr;
    }, strings->keys.string(), 1));

    // $19.1.2.15 Object.preventExtensions ( O )
    m_object->defineDataProperty(strings->preventExtensions, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue O = instance->currentExecutionContext()->readArgument(0);
        if (!O.isObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, false, strings->preventExtensions, errorMessage_GlobalObject_FirstArgumentNotObject);
        ESObject* obj = O.toObject();
        obj->setExtensible(false);
        return O;
    }, strings->preventExtensions.string(), 1));

    // $19.1.2.17 Object.seal ( O )
    m_object->defineDataProperty(strings->seal, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue O = instance->currentExecutionContext()->readArgument(0);
        if (!O.isObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, false, strings->seal, errorMessage_GlobalObject_FirstArgumentNotObject);
        ESObject* obj = O.toObject();
        obj->forceNonVectorHiddenClass();
        if (obj->isESArrayObject())
            obj->asESArrayObject()->convertToSlowMode();
        std::vector<std::pair<ESValue, ESHiddenClassPropertyInfo*> > configurableProperties;
        obj->enumerationWithNonEnumerable([&](ESValue key, ESHiddenClassPropertyInfo* propertyInfo) {
            propertyInfo->setConfigurable(false);
        });
        obj->setExtensible(false);
        return O;
    }, strings->seal.string(), 1));

    // $19.1.3.7 Object.prototype.valueOf ( )
    m_objectPrototype->defineDataProperty(strings->valueOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(ret, Object, valueOf);
        return ret;
    }, strings->valueOf));

    // $19.1.3.3 Object.prototype.isPrototypeOf ( V )
    m_objectPrototype->defineDataProperty(strings->isPrototypeOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue V = instance->currentExecutionContext()->readArgument(0);
        if (!V.isObject())
            return ESValue(false);
        RESOLVE_THIS_BINDING_TO_OBJECT(O, Object, isPrototypeOf);
        while (true) {
            V = V.asESPointer()->asESObject()->__proto__();
            if (V.isNull())
                return ESValue(false);
            if (V.equalsTo(O))
                return ESValue(true);
        }
    }, strings->isPrototypeOf, 1));

    // $19.1.3.4 Object.prototype.propertyIsEnumerable ( V )
    m_objectPrototype->defineDataProperty(strings->propertyIsEnumerable, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // TODO toPropertyKey
        escargot::ESString* key = instance->currentExecutionContext()->readArgument(0).toString();
        RESOLVE_THIS_BINDING_TO_OBJECT(O, Object, propertyIsEnumerable);
        if (!O->hasOwnProperty(key))
            return ESValue(false);
        if ((O->isESArrayObject() && O->asESArrayObject()->isFastmode())
#ifdef USE_ES6_FEATURE
            || O->isESTypedArrayObject()
#endif
            ) {
            if (*key != *strings->length.string())
            // In fast mode, it was already checked in O->hasOwnProperty.
                return ESValue(true);
        }
        size_t t = O->hiddenClass()->findProperty(key);
        if (O->isESStringObject() && t == SIZE_MAX) { // index value
            return ESValue(true);
        }
        if (O->hiddenClass()->propertyInfo(t).enumerable())
            return ESValue(true);
        return ESValue(false);
    }, strings->propertyIsEnumerable, 1));

    // $19.1.3.5 Object.prototype.toLocaleString
    m_objectPrototype->defineDataProperty(strings->toLocaleString, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisO, Object, toLocaleString);
        ESValue func = thisO->get(strings->toString.string());
        if (!func.isESPointer() || !func.asESPointer()->isESFunctionObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Object, true, strings->toLocaleString, errorMessage_GlobalObject_ToLoacleStringNotCallable);
        return ESFunctionObject::call(instance, func, thisO, NULL, 0, false);
    }, strings->toLocaleString, 0));

    defineDataProperty(strings->Object, true, false, true, m_object);
}

void GlobalObject::installError()
{
    auto errorFn = [](ESVMInstance* instance) -> ESValue {
        if (instance->currentExecutionContext()->isNewExpression()) {
            ESValue message = instance->currentExecutionContext()->readArgument(0);
            if (!message.isUndefined()) {
                instance->currentExecutionContext()->resolveThisBindingToObject()->asESErrorObject()->defineDataProperty(
                    strings->message, true, false, true, message.toString());
            }
            return ESValue();
        } else {
            escargot::ESErrorObject* obj = ESErrorObject::create();
            ESValue message = instance->currentExecutionContext()->readArgument(0);
            if (!message.isUndefined()) {
                obj->set(strings->message, message.toString());
            }
            return obj;
        }
    };
    m_error = ::escargot::ESFunctionObject::create(NULL, errorFn, strings->Error, 1, true);

    m_error->forceNonVectorHiddenClass(true);
    m_error->set__proto__(m_functionPrototype);
    m_error->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    m_errorPrototype = escargot::ESErrorObject::create();
    m_error->setProtoType(m_errorPrototype);
    m_errorPrototype->set__proto__(m_objectPrototype);
    m_errorPrototype->defineDataProperty(strings->constructor, true, false, true, m_error);
    m_errorPrototype->defineDataProperty(strings->message, true, false, true, strings->emptyString.string());
    m_errorPrototype->defineDataProperty(strings->name, true, false, true, strings->Error.string());
    m_errorPrototype->forceNonVectorHiddenClass(true);

    escargot::ESFunctionObject* toString = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue v = instance->currentExecutionContext()->resolveThisBinding();
        if (!v.isObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Error, true, strings->toString, errorMessage_GlobalObject_ThisNotObject);

        ESObject* o = v.toObject();

        StringRecursionChecker checker(o);
        if (checker.recursionCheck()) {
            return ESValue(strings->emptyString.string());
        }

        ESValue name = o->get(ESValue(strings->name));
        escargot::ESString* nameStr;
        if (name.isUndefined()) {
            nameStr = strings->Error.string();
        } else {
            nameStr = name.toString();
        }
        ESValue message = o->get(ESValue(strings->message));
        escargot::ESString* messageStr;
        if (message.isUndefined()) {
            messageStr = strings->emptyString.string();
        } else {
            messageStr = message.toString();
        }

        if (nameStr->length() == 0) {
            return messageStr;
        }

        if (messageStr->length() == 0) {
            return nameStr;
        }

        ESStringBuilder builder;
        builder.appendString(nameStr);
        builder.appendString(": ");
        builder.appendString(messageStr);
        return builder.finalize();
    }, strings->toString, 0);
    m_errorPrototype->defineDataProperty(strings->toString, true, false, true, toString);

    defineDataProperty(strings->Error, true, false, true, m_error);

#define DECLARE_ERROR_FUNCTION(ErrorType) \
    auto errorFn##ErrorType = [](ESVMInstance* instance) -> ESValue { \
        if (instance->currentExecutionContext()->isNewExpression()) { \
            ESValue message = instance->currentExecutionContext()->readArgument(0); \
            if (!message.isUndefined()) { \
                instance->currentExecutionContext()->resolveThisBindingToObject()->asESErrorObject()->defineDataProperty( \
                    strings->message, true, false, true, message.toString()); \
            } \
            return ESValue(); \
        } else { \
            escargot::ESErrorObject* obj = ErrorType::create(); \
            ESValue message = instance->currentExecutionContext()->readArgument(0); \
            if (!message.isUndefined()) { \
                obj->set(strings->message, message.toString()); \
            } \
            return obj; \
        } \
    };

    // ///////////////////////////
    DECLARE_ERROR_FUNCTION(ReferenceError);
    m_referenceError = ::escargot::ESFunctionObject::create(NULL, errorFnReferenceError, strings->ReferenceError, 1, true);
    m_referenceError->set__proto__(m_functionPrototype);
    m_referenceError->forceNonVectorHiddenClass(true);
    m_referenceError->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    m_referenceErrorPrototype = ESErrorObject::create();
    m_referenceErrorPrototype->forceNonVectorHiddenClass(true);

    m_referenceError->setProtoType(m_referenceErrorPrototype);

    m_referenceErrorPrototype->defineDataProperty(strings->constructor, true, false, true, m_referenceError);
    m_referenceErrorPrototype->defineDataProperty(strings->message, true, false, true, ESString::createAtomicString(""));
    m_referenceErrorPrototype->defineDataProperty(strings->name, true, false, true, strings->ReferenceError.string());


    defineDataProperty(strings->ReferenceError, true, false, true, m_referenceError);

    // ///////////////////////////
    DECLARE_ERROR_FUNCTION(TypeError);
    m_typeError = ::escargot::ESFunctionObject::create(NULL, errorFnTypeError, strings->TypeError, 1, true);
    m_typeError->set__proto__(m_functionPrototype);
    m_typeError->forceNonVectorHiddenClass(true);
    m_typeError->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    m_typeErrorPrototype = ESErrorObject::create();
    m_typeErrorPrototype->forceNonVectorHiddenClass(true);

    m_typeError->setProtoType(m_typeErrorPrototype);

    m_typeErrorPrototype->defineDataProperty(strings->constructor, true, false, true, m_typeError);
    m_typeErrorPrototype->defineDataProperty(strings->message, true, false, true, ESString::createAtomicString(""));
    m_typeErrorPrototype->defineDataProperty(strings->name, true, false, true, strings->TypeError.string());


    defineDataProperty(strings->TypeError, true, false, true, m_typeError);

    // ///////////////////////////
    DECLARE_ERROR_FUNCTION(RangeError);
    m_rangeError = ::escargot::ESFunctionObject::create(NULL, errorFnRangeError, strings->RangeError, 1, true);
    m_rangeError->set__proto__(m_functionPrototype);
    m_rangeError->forceNonVectorHiddenClass(true);
    m_rangeError->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    m_rangeErrorPrototype = ESErrorObject::create();
    m_rangeErrorPrototype->forceNonVectorHiddenClass(true);

    m_rangeError->setProtoType(m_rangeErrorPrototype);

    m_rangeErrorPrototype->defineDataProperty(strings->constructor, true, false, true, m_rangeError);
    m_rangeErrorPrototype->defineDataProperty(strings->message, true, false, true, ESString::createAtomicString(""));
    m_rangeErrorPrototype->defineDataProperty(strings->name, true, false, true, strings->RangeError.string());

    defineDataProperty(strings->RangeError, true, false, true, m_rangeError);

    // ///////////////////////////
    DECLARE_ERROR_FUNCTION(SyntaxError);
    m_syntaxError = ::escargot::ESFunctionObject::create(NULL, errorFnSyntaxError, strings->SyntaxError, 1, true);
    m_syntaxError->set__proto__(m_functionPrototype);
    m_syntaxError->forceNonVectorHiddenClass(true);
    m_syntaxError->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    m_syntaxErrorPrototype = ESErrorObject::create();
    m_syntaxErrorPrototype->forceNonVectorHiddenClass(true);

    m_syntaxError->setProtoType(m_syntaxErrorPrototype);

    m_syntaxErrorPrototype->defineDataProperty(strings->constructor, true, false, true, m_syntaxError);
    m_syntaxErrorPrototype->defineDataProperty(strings->message, true, false, true, ESString::createAtomicString(""));
    m_syntaxErrorPrototype->defineDataProperty(strings->name, true, false, true, strings->SyntaxError.string());

    defineDataProperty(strings->SyntaxError, true, false, true, m_syntaxError);

    // ///////////////////////////
    DECLARE_ERROR_FUNCTION(URIError);
    m_uriError = ::escargot::ESFunctionObject::create(NULL, errorFnURIError, strings->URIError, 1, true);
    m_uriError->set__proto__(m_functionPrototype);
    m_uriError->forceNonVectorHiddenClass(true);
    m_uriError->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    m_uriErrorPrototype = ESErrorObject::create();
    m_uriErrorPrototype->forceNonVectorHiddenClass(true);

    m_uriError->setProtoType(m_uriErrorPrototype);

    m_uriErrorPrototype->defineDataProperty(strings->constructor, true, false, true, m_uriError);
    m_uriErrorPrototype->defineDataProperty(strings->message, true, false, true, ESString::createAtomicString(""));
    m_uriErrorPrototype->defineDataProperty(strings->name, true, false, true, strings->URIError.string());

    defineDataProperty(strings->URIError, true, false, true, m_uriError);

    // ///////////////////////////
    DECLARE_ERROR_FUNCTION(EvalError);
    m_evalError = ::escargot::ESFunctionObject::create(NULL, errorFnEvalError, strings->EvalError, 1, true);
    m_evalError->set__proto__(m_functionPrototype);
    m_evalError->forceNonVectorHiddenClass(true);
    m_evalError->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    m_evalErrorPrototype = ESErrorObject::create();
    m_evalErrorPrototype->forceNonVectorHiddenClass(true);

    m_evalError->setProtoType(m_evalErrorPrototype);

    m_evalErrorPrototype->defineDataProperty(strings->constructor, true, false, true, m_evalError);
    m_evalErrorPrototype->defineDataProperty(strings->message, true, false, true, ESString::createAtomicString(""));
    m_evalErrorPrototype->defineDataProperty(strings->name, true, false, true, strings->EvalError.string());

    defineDataProperty(strings->EvalError, true, false, true, m_evalError);
}

void GlobalObject::installArray()
{
    m_arrayPrototype = ESArrayObject::create(0);
    m_arrayPrototype->convertToSlowMode();
    m_arrayPrototype->set__proto__(m_objectPrototype);
    m_arrayPrototype->forceNonVectorHiddenClass(true);

    // $22.1.1 Array Constructor
    m_array = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int len = instance->currentExecutionContext()->argumentCount();
        bool interpretArgumentsAsElements = false;
        unsigned int size = 0;
        if (len > 1) {
            size = len;
            interpretArgumentsAsElements = true;
        } else if (len == 1) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            if (val.isNumber()) {
                if (val.equalsTo(ESValue(val.toUint32()))) {
                    size = val.toNumber();
                } else {
                    throwBuiltinError(instance, ErrorCode::RangeError, strings->Array, false, strings->emptyString, errorMessage_GlobalObject_InvalidArrayLength);
                }
            } else {
                size = 1;
                interpretArgumentsAsElements = true;
            }
        }
        escargot::ESArrayObject* array;
        if (instance->currentExecutionContext()->isNewExpression() && instance->currentExecutionContext()->resolveThisBindingToObject()->isESArrayObject()) {
            array = instance->currentExecutionContext()->resolveThisBindingToObject()->asESArrayObject();
            array->setLength(size);
        } else
            array = ESArrayObject::create(size);
        if (interpretArgumentsAsElements) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            if (len > 1 || !val.isInt32()) {
                for (int idx = 0; idx < len; idx++) {
                    array->defineDataProperty(ESValue(idx), true, true, true, val);
                    val = instance->currentExecutionContext()->arguments()[idx + 1];
                }
            }
        } else {
        }
        return array;
    }, strings->Array, 1, true);
    m_array->forceNonVectorHiddenClass(true);
    m_array->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);
    m_arrayPrototype->defineDataProperty(strings->constructor, true, false, true, m_array);

    // $22.1.2.2 Array.isArray(arg)
    m_array->ESObject::defineDataProperty(strings->isArray, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue arg = instance->currentExecutionContext()->readArgument(0);
        if (arg.isESPointer() && arg.asESPointer()->isESArrayObject())
            return ESValue(ESValue::ESTrueTag::ESTrue);
        return ESValue(ESValue::ESFalseTag::ESFalse);
    }, strings->isArray, 1));

    // $22.1.3.1 Array.prototype.concat(...arguments)
    m_arrayPrototype->ESObject::defineDataProperty(strings->concat, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        size_t arglen = instance->currentExecutionContext()->argumentCount();
        RESOLVE_THIS_BINDING_TO_OBJECT(thisBinded, Array, concat);
        escargot::ESArrayObject* ret = ESArrayObject::create(0);
        unsigned n = 0;
        for (size_t i = 0; i < arglen + 1; i++) {
            ESValue argi;
            if (i == 0) {
                argi = thisBinded;
            } else {
                argi = instance->currentExecutionContext()->readArgument(i - 1);
            }
            if (argi.isESPointer() && argi.asESPointer()->isESArrayObject()) {
                double curIndex = 0;
                escargot::ESArrayObject* arr = argi.asESPointer()->asESArrayObject();
                uint32_t len = arr->length();

                while (curIndex < len) {
                    if (arr->hasProperty(ESValue(curIndex))) {
                        ret->defineDataProperty(ESValue(n + curIndex), true, true, true, arr->get(curIndex));
                        curIndex++;
                    } else {
                        curIndex = ESArrayObject::nextIndexForward(arr, curIndex, len, false);
                    }
                }

                if (n > ESValue::ESInvalidIndexValue - len) {
                    throwBuiltinError(instance, ErrorCode::RangeError, strings->Array, true, strings->concat, errorMessage_GlobalObject_RangeError);
                }

                n += len;
                ret->setLength(n);
            } else {
                if (n > ESValue::ESInvalidIndexValue - 1) {
                    throwBuiltinError(instance, ErrorCode::RangeError, strings->Array, true, strings->concat, errorMessage_GlobalObject_RangeError);
                }

                ret->defineDataProperty(ESValue(n++), true, true, true, argi);
            }
        }
        return ret;
    }, strings->concat, 1));

    // $22.1.3.5 Array.prototype.every
    m_arrayPrototype->ESObject::defineDataProperty(strings->every, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {

        // Let O be the result of calling ToObject passing the this value as the argument.
        RESOLVE_THIS_BINDING_TO_OBJECT(O, Array, every);

        // Let lenValue be the result of calling the [[Get]] internal method of O with the argument "length".
        // Let len be ToUint32(lenValue).
        uint32_t len = O->length();

        // If IsCallable(callbackfn) is false, throw a TypeError exception.
        ESValue callbackfn = instance->currentExecutionContext()->readArgument(0);
        if (!callbackfn.isESPointer() || !callbackfn.asESPointer()->isESFunctionObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Array, true, strings->every, errorMessage_GlobalObject_CallbackNotCallable);
        }

        // If thisArg was supplied, let T be thisArg; else let T be undefined.
        ESValue T = instance->currentExecutionContext()->readArgument(1);

        // Let k be 0.
        uint32_t k = 0;

        while (k < len) {
            // Let Pk be ToString(k).
            ESValue pk(k);

            // Let kPresent be the result of calling the [[HasProperty]] internal method of O with argument Pk.
            bool kPresent = O->hasProperty(pk);

            // If kPresent is true, then
            if (kPresent) {
                // Let kValue be the result of calling the [[Get]] internal method of O with argument Pk.
                ESValue kValue = O->get(pk);
                // Let testResult be the result of calling the [[Call]] internal method of callbackfn with T as the this value and argument list containing kValue, k, and O.
                ESValue args[] = {kValue, ESValue(k), O};
                ESValue testResult = ESFunctionObject::call(instance, callbackfn, T, args, 3, false);

                if (!testResult.toBoolean()) {
                    return ESValue(false);
                }

                // Increae k by 1.
                k++;
            } else {
                k = ESArrayObject::nextIndexForward(O, k, len, false);
            }

        }
        return ESValue(true);
    }, strings->every.string(), 1));

#ifdef USE_ES6_FEATURE
    // $22.1.3.6 Array.prototype.fill
    m_arrayPrototype->ESObject::defineDataProperty(strings->fill, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->fill.string(), 1));
#endif

    // $22.1.3.7 Array.prototype.filter
    m_arrayPrototype->ESObject::defineDataProperty(strings->filter, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // Array.prototype.filter ( callbackfn [ , thisArg ] )

        // Let O be the result of calling ToObject passing the this value as the argument.
        RESOLVE_THIS_BINDING_TO_OBJECT(O, Array, filter);

        // Let lenValue be the result of calling the [[Get]] internal method of O with the argument "length".
        // Let len be ToUint32(lenValue).
        uint32_t len = O->length();

        // If IsCallable(callbackfn) is false, throw a TypeError exception.
        ESValue callbackfn = instance->currentExecutionContext()->readArgument(0);
        if (!callbackfn.isESPointer() || !callbackfn.asESPointer()->isESFunctionObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Array, true, strings->filter, errorMessage_GlobalObject_CallbackNotCallable);
        }

        // If thisArg was supplied, let T be thisArg; else let T be undefined.
        ESValue T = instance->currentExecutionContext()->readArgument(1);

        // Let A be a new array created as if by the expression new Array() where Array is the standard built-in constructor with that name.
        escargot::ESArrayObject* A = escargot::ESArrayObject::create(0);
        // Let k be 0.
        uint32_t k = 0;
        // Let to be 0.
        uint32_t to = 0;

        while (k < len) {
            // Let Pk be ToString(k).
            ESValue pk(k);

            // Let kPresent be the result of calling the [[HasProperty]] internal method of O with argument Pk.
            bool kPresent = O->hasProperty(pk);

            // If kPresent is true, then
            if (kPresent) {
                // Let kValue be the result of calling the [[Get]] internal method of O with argument Pk.
                ESValue kValue = O->get(pk);
                // Let selected be the result of calling the [[Call]] internal method of callbackfn with T as the this value and argument list containing kValue, k, and O.
                ESValue args[] = {kValue, ESValue(k), O};
                ESValue selected = ESFunctionObject::call(instance, callbackfn, T, args, 3, false);

                if (selected.toBoolean()) {
                    A->defineDataProperty(ESValue(to), true, true, true, kValue);
                    // Increase to by 1.
                    to++;
                }

                // Increase k by 1.
                k++;
            } else {
                k = ESArrayObject::nextIndexForward(O, k, len, false);
            }
        }

        return A;
    }, strings->filter.string(), 1));

#ifdef USE_ES6_FEATURE
    // $22.1.3.8 Array.prototype.find
    m_arrayPrototype->ESObject::defineDataProperty(strings->find, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->find.string(), 1));

    // $22.1.3.9 Array.prototype.findIndex
    m_arrayPrototype->ESObject::defineDataProperty(strings->findIndex, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->findIndex.string(), 1));
#endif

    // $22.1.3.10 Array.prototype.forEach()
    m_arrayPrototype->ESObject::defineDataProperty(strings->forEach, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // Let O be ToObject(this value).
        RESOLVE_THIS_BINDING_TO_OBJECT(O, Array, forEach);

        // Let len be ToLength(Get(O, "length")).
        uint32_t len = O->length();

        ESValue callbackfn = instance->currentExecutionContext()->readArgument(0);

        // If IsCallable(callbackfn) is false, throw a TypeError exception.
        if (!callbackfn.isESPointer() || !callbackfn.asESPointer()->isESFunctionObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Array, true, strings->forEach, errorMessage_GlobalObject_CallbackNotCallable);
        }

        // If thisArg was supplied, let T be thisArg; else let T be undefined.
        ESValue T = instance->currentExecutionContext()->readArgument(1);

        // Let k be 0.
        size_t k = 0;
        while (k < len) {
            // Let Pk be ToString(k).
            ESValue pk(k);
            // Let kPresent be HasProperty(O, Pk).
            bool kPresent = O->hasProperty(pk);
            if (kPresent) {
                ESValue kValue = O->get(pk);
                ESValue arguments[3] = {kValue, pk, O};
                ESFunctionObject::call(instance, callbackfn, T, arguments, 3, false);
                k++;
            } else {
                k = ESArrayObject::nextIndexForward(O, k, len, false);
            }
        }
        return ESValue();
    }, strings->forEach, 1));

    // $22.1.3.11 Array.prototype.indexOf()
    m_arrayPrototype->ESObject::defineDataProperty(strings->indexOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisBinded, Array, indexOf);
        uint32_t len = thisBinded->length();
        if (len == 0)
            return ESValue(-1);
        else {
            uint32_t k = 0;
            double n = 0;
            ESValue searchElement = instance->currentExecutionContext()->readArgument(0);
            if (instance->currentExecutionContext()->argumentCount() >= 2) {
                ESValue fromIndex = instance->currentExecutionContext()->readArgument(1);
                n = fromIndex.toInteger();
            }

            if (n >= len) {
                return ESValue(-1);
            } else if (n >= 0) {
                k = n;
            } else {
                int tmpk = len - n * (-1);
                if (tmpk < 0)
                    k = 0;
                else
                    k = tmpk;
            }

            while (k < len) {
                bool kPresent = thisBinded->hasProperty(ESValue(k));
                if (kPresent) {
                    ESValue elementK = thisBinded->get(ESValue(k));
                    if (searchElement.equalsTo(elementK)) {
                        return ESValue(k);
                    }
                    k++;
                } else {
                    k = ESArrayObject::nextIndexForward(thisBinded, k, len, false);
                }
            }

            return ESValue(-1);
        }
    }, strings->indexOf, 1));

    // $22.1.3.12 Array.prototype.join(separator)
    m_arrayPrototype->ESObject::defineDataProperty(strings->join, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisBinded, Array, join);
        uint32_t len = thisBinded->length();
        ESValue separator = instance->currentExecutionContext()->readArgument(0);
        size_t lenMax = ESString::maxLength();
        escargot::ESString* sep;

        if (separator.isUndefined()) {
            sep = strings->asciiTable[(size_t)','].string();
        } else {
            sep = separator.toString();
        }

        StringRecursionChecker checker(thisBinded);
        if (checker.recursionCheck()) {
            return ESValue(strings->emptyString.string());
        }

        ESStringBuilder builder;
        double prevIndex = 0;
        double curIndex = 0;
        while (curIndex < len) {
            if (curIndex != 0) {
                if (sep->length() > 0) {
                    if (static_cast<double>(builder.contentLength()) >
                        static_cast<double>(lenMax - (curIndex - prevIndex - 1) * sep->length())) {
                        instance->throwOOMError();
                    }
                    while (curIndex - prevIndex > 1) {
                        builder.appendString(sep);
                        prevIndex++;
                    }
                    builder.appendString(sep);
                }
            }
            ESValue elem = thisBinded->get(ESValue(curIndex));

            if (!elem.isUndefinedOrNull()) {
                builder.appendString(elem.toString());
            }
            prevIndex = curIndex;
            if (elem.isUndefined()) {
                curIndex = ESArrayObject::nextIndexForward(thisBinded, prevIndex, len, true);
            } else {
                curIndex++;
            }
        }
        if (sep->length() > 0) {
            if (static_cast<double>(builder.contentLength()) >
                static_cast<double>(lenMax - (curIndex - prevIndex - 1) * sep->length())) {
                instance->throwOOMError();
            }
            while (curIndex - prevIndex > 1) {
                builder.appendString(sep);
                prevIndex++;
            }
        }
        return builder.finalize();
    }, strings->join, 1));

#ifdef USE_ES6_FEATURE
    // $22.1.3.13 Array.prototype.keys ( )
    m_arrayPrototype->ESObject::defineDataProperty(strings->keys, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->keys.string(), 0));
#endif

    // $22.1.3.14 Array.prototype.lastIndexOf(searchElement [,fromIndex])
    m_arrayPrototype->ESObject::defineDataProperty(strings->lastIndexOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisBinded, Array, lastIndexOf);
        uint32_t len = thisBinded->length();
        if (len == 0) {
            return ESValue(-1);
        } else {
            double n = 0;
            double k = 0;
            ESValue searchElement = instance->currentExecutionContext()->readArgument(0);
            if (instance->currentExecutionContext()->argumentCount() >= 2) {
                ESValue fromIndex = instance->currentExecutionContext()->readArgument(1);
                n = fromIndex.toInteger();
            } else {
                n = len - 1;
            }

            if (n >= 0) {
                k = (n > len - 1) ? len - 1 : n;
            } else {
                k = len + n;
            }

            while (k >= 0) {
                bool kPresent = thisBinded->hasProperty(ESValue(k));
                if (kPresent) {
                    ESValue elementK = thisBinded->get(ESValue(k));
                    if (searchElement.equalsTo(elementK)) {
                        return ESValue(k);
                    }
                    k--;
                } else {
                    k = ESArrayObject::nextIndexBackward(thisBinded, k, -1, false);
                }
            }

            return ESValue(-1);
        }
    }, strings->lastIndexOf.string(), 1));

    // $22.1.3.15 Array.prototype.map(callbackfn[, thisArg])
    m_arrayPrototype->ESObject::defineDataProperty(strings->map, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {

        // Let O be the result of calling ToObject passing the this value as the argument.
        RESOLVE_THIS_BINDING_TO_OBJECT(O, Array, map);

        // Let lenValue be the result of calling the [[Get]] internal method of O with the argument "length".
        // Let len be ToUint32(lenValue).
        uint32_t len = O->length();

        // If IsCallable(callbackfn) is false, throw a TypeError exception.
        ESValue callbackfn = instance->currentExecutionContext()->readArgument(0);
        if (!callbackfn.isESPointer() || !callbackfn.asESPointer()->isESFunctionObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Array, true, strings->map, errorMessage_GlobalObject_CallbackNotCallable);
        }

        // If thisArg was supplied, let T be thisArg; else let T be undefined.
        ESValue T = instance->currentExecutionContext()->readArgument(1);

        // Let A be a new array created as if by the expression new Array() where Array is the standard built-in constructor with that name.
        escargot::ESArrayObject* A = escargot::ESArrayObject::create(len);
        // Let k be 0.
        uint32_t k = 0;

        while (k < len) {
            // Let Pk be ToString(k).
            ESValue pk(k);

            // Let kPresent be the result of calling the [[HasProperty]] internal method of O with argument Pk.
            bool kPresent = O->hasProperty(pk);

            // If kPresent is true, then
            if (kPresent) {
                // Let kValue be the result of calling the [[Get]] internal method of O with argument Pk.
                ESValue kValue = O->get(pk);
                // Let mappedValue be the result of calling the [[Call]] internal method of callbackfn with T as the this value and argument list containing kValue, k, and O.
                ESValue args[] = {kValue, ESValue(k), O};
                ESValue mappedValue = ESFunctionObject::call(instance, callbackfn, T, args, 3, false);

                A->defineDataProperty(pk, true, true, true, mappedValue);
                k++;
            } else {
                k = ESArrayObject::nextIndexForward(O, k, len, false);
            }
        }

        return A;
    }, strings->map.string(), 1));

    // $22.1.3.16 Array.prototype.pop ( )
    m_arrayPrototype->ESObject::defineDataProperty(strings->pop, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisBinded, Array, pop);
        uint32_t len = thisBinded->length();
        if (len == 0) {
            thisBinded->set(strings->length.string(), ESValue(0), true);
            return ESValue();
        }

        ESValue idx = ESValue(len - 1);
        ESValue ret = thisBinded->get(idx);
        thisBinded->deletePropertyWithException(idx);
        thisBinded->set(strings->length.string(), idx, true);
        return ret;
    }, strings->pop, 0));

    // $22.1.3.17 Array.prototype.push(item)
    m_arrayPrototype->ESObject::defineDataProperty(strings->push, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisBinded, Array, push);
        uint32_t argc = instance->currentExecutionContext()->argumentCount();
        uint32_t len = thisBinded->length();

        for (uint32_t i = 0; i < argc; i++) {
            ESValue& val = instance->currentExecutionContext()->arguments()[i];
            thisBinded->set(ESValue((double(len) + i)), val, true);
        }

        if (thisBinded->isESArrayObject() && len > ESValue::ESInvalidIndexValue - argc) {
            throwBuiltinError(instance, ErrorCode::RangeError, strings->Array, true, strings->push, errorMessage_GlobalObject_RangeError);
        }
        ESValue newLen = ESValue(double(len) + argc);
        thisBinded->set(strings->length.string(), newLen, true);
        return newLen;

    }, strings->push, 1));

    // $22.1.3.18 Array.prototype.reduce
    m_arrayPrototype->ESObject::defineDataProperty(strings->reduce, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(O, Array, reduce); // 1
        uint32_t len = O->get(strings->length.string()).toUint32(); // 2-3
        ESValue callbackfn = instance->currentExecutionContext()->readArgument(0);
        size_t argc = instance->currentExecutionContext()->argumentCount();
        ESValue initialValue = ESValue(ESValue::ESEmptyValue);
        if (argc > 1) {
            initialValue = instance->currentExecutionContext()->readArgument(1);
        }

        if (!callbackfn.isESPointer() || !callbackfn.asESPointer()->isESFunctionObject()) // 4
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Array, true, strings->reduce, errorMessage_GlobalObject_CallbackNotCallable);

        if (len == 0 && (initialValue.isUndefined() || initialValue.isEmpty())) // 5
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Array, true, strings->reduce, errorMessage_GlobalObject_ReduceError);

        size_t k = 0; // 6
        ESValue accumulator;
        if (!initialValue.isEmpty()) { // 7
            accumulator = initialValue;
        } else { // 8
            bool kPresent = false; // 8.a
            while (kPresent == false && k < len) { // 8.b
                ESValue Pk = ESValue(k); // 8.b.i
                kPresent = O->hasProperty(Pk); // 8.b.ii
                if (kPresent)
                    accumulator = O->get(Pk); // 8.b.iii.1
                k++; // 8.b.iv
            }
            if (kPresent == false)
                throwBuiltinError(instance, ErrorCode::TypeError, strings->Array, true, strings->reduce, errorMessage_GlobalObject_ReduceError);
        }
        while (k < len) { // 9
            ESValue Pk = ESValue(k); // 9.a
            bool kPresent = O->hasProperty(Pk); // 9.b
            if (kPresent) { // 9.c
                ESValue kValue = O->get(Pk); // 9.c.i
                const int fnargc = 4;
                ESValue* fnargs = (ESValue *)alloca(sizeof(ESValue) * fnargc);
                fnargs[0] = accumulator;
                fnargs[1] = kValue;
                fnargs[2] = ESValue(k);
                fnargs[3] = O;
                accumulator = ESFunctionObject::call(ESVMInstance::currentInstance(), callbackfn, ESValue(), fnargs, fnargc, false);
                k++;
            } else {
                k = ESArrayObject::nextIndexForward(O, k, len, false);
            }
        }
        return accumulator;
    }, strings->reduce.string(), 1));

    // $22.1.3.19 Array.prototype.reduceRight
    m_arrayPrototype->ESObject::defineDataProperty(strings->reduceRight, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(O, Array, reduceRight); // 1
        uint32_t len = O->get(strings->length.string()).toUint32(); // 2-3
        int argc = instance->currentExecutionContext()->argumentCount();
        ESValue callbackfn = instance->currentExecutionContext()->readArgument(0);
        ESValue initialValue = ESValue(ESValue::ESEmptyValue);
        if (argc > 1) {
            initialValue = instance->currentExecutionContext()->readArgument(1);
        }
        if (!callbackfn.isESPointer() || !callbackfn.asESPointer()->isESFunctionObject()) // 4
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Array, true, strings->reduce, errorMessage_GlobalObject_CallbackNotCallable);

        if (len == 0 && (initialValue.isUndefined() || initialValue.isEmpty())) // 5
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Array, true, strings->reduceRight, errorMessage_GlobalObject_ReduceError);
        int k = len - 1; // 6
        ESValue accumulator;
        if (!initialValue.isEmpty()) { // 7
            accumulator = initialValue;
        } else { // 8
            bool kPresent = false; // 8.a
            while (kPresent == false && k >= 0) { // 8.b
                ESValue Pk = ESValue(k); // 8.b.i
                kPresent = O->hasProperty(Pk); // 8.b.ii
                if (kPresent)
                    accumulator = O->get(Pk); // 8.b.iii.1
                k--; // 8.b.iv
            }
            if (kPresent == false)
                throwBuiltinError(instance, ErrorCode::TypeError, strings->Array, true, strings->reduceRight, errorMessage_GlobalObject_ReduceError);
        }
        while (k >= 0) { // 9
            ESValue Pk = ESValue(k); // 9.a
            bool kPresent = O->hasProperty(Pk); // 9.b
            if (kPresent) { // 9.c
                ESValue kValue = O->get(Pk); // 9.c.i
                const int fnargc = 4;
                ESValue* fnargs;
                ALLOCA_WRAPPER(instance, fnargs, ESValue*, sizeof(ESValue) * fnargc, false);
                fnargs[0] = accumulator;
                fnargs[1] = kValue;
                fnargs[2] = ESValue(k);
                fnargs[3] = O;
                accumulator = ESFunctionObject::call(ESVMInstance::currentInstance(), callbackfn, ESValue(), fnargs, fnargc, false);
                k--;
            } else {
                k = ESArrayObject::nextIndexBackward(O, k, -1, false);
            }
        }
        return accumulator;

    }, strings->reduceRight.string(), 1));

    // $22.1.3.20 Array.prototype.reverse()
    m_arrayPrototype->ESObject::defineDataProperty(strings->reverse, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(O, Array, reverse);
        unsigned len = O->length();
        unsigned middle = std::floor(len / 2);
        unsigned lower = 0;
        while (middle > lower) {
            unsigned upper = len - lower - 1;
            ESValue upperP = ESValue(upper);
            ESValue lowerP = ESValue(lower);

            bool lowerExists = O->hasProperty(lowerP);
            ESValue lowerValue;
            if (lowerExists) {
                lowerValue = O->get(lowerP);
            }
            bool upperExists = O->hasProperty(upperP);
            ESValue upperValue;
            if (upperExists) {
                upperValue = O->get(upperP);
            }
            if (lowerExists && upperExists) {
                O->set(lowerP, upperValue, true);
                O->set(upperP, lowerValue, true);
            } else if (!lowerExists && upperExists) {
                O->set(lowerP, upperValue, true);
                O->deletePropertyWithException(upperP);
            } else if (lowerExists && !upperExists) {
                O->deletePropertyWithException(lowerP);
                O->set(upperP, lowerValue, true);
            } else {
                unsigned nextLower = ESArrayObject::nextIndexForward(O, lower, middle, false);
                unsigned nextUpper = ESArrayObject::nextIndexBackward(O, upper, middle, false);
                unsigned x = middle - nextLower;
                unsigned y = nextUpper - middle;
                unsigned lowerCandidate;
                if (x > y) {
                    lowerCandidate = nextLower;
                } else {
                    lowerCandidate = len - nextUpper - 1;
                }
                if (lower == lowerCandidate)
                    break;
                lower = lowerCandidate;
                continue;
            }
            lower++;
        }

        return O;
    }, strings->reverse.string(), 0));

    // $22.1.3.21 Array.prototype.shift ( )
    m_arrayPrototype->ESObject::defineDataProperty(strings->shift, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(O, Array, shift); // 1
        uint32_t arrlen = O->get(strings->length.string()).toUint32(); // 3
        if (arrlen == 0) { // 5
            O->set(strings->length.string(), ESValue(0), true);
            return ESValue();
        }
        ESValue first = O->get(ESValue(0)); // 6
        O->relocateIndexesForward(1, arrlen, -1);
        O->deletePropertyWithException(ESValue(arrlen - 1)); // 10
        O->set(strings->length, ESValue(arrlen - 1), true); // 12
        return first;
    }, strings->shift, 0));

    // $22.1.3.22 Array.prototype.slice(start, end)
    m_arrayPrototype->ESObject::defineDataProperty(strings->slice, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisBinded, Array, slice)
        uint32_t arrlen = thisBinded->length();
        double relativeStart = instance->currentExecutionContext()->readArgument(0).toInteger(), relativeEnd;
        uint32_t k, finalEnd;
        if (relativeStart < 0) {
            k = (arrlen + relativeStart > 0) ? arrlen + relativeStart : 0;
        } else {
            k = (relativeStart < arrlen) ? relativeStart : arrlen;
        }
        ESValue end = instance->currentExecutionContext()->readArgument(1);
        if (end.isUndefined()) {
            relativeEnd = arrlen;
        } else {
            relativeEnd = end.toInteger();
        }
        if (relativeEnd < 0) {
            finalEnd = (arrlen + relativeEnd > 0) ? arrlen + relativeEnd : 0;
        } else {
            finalEnd = (relativeEnd < arrlen) ? relativeEnd : arrlen;
        }
        uint32_t n = 0;
        escargot::ESArrayObject* ret = ESArrayObject::create();
        while (k < finalEnd) {
            bool kPresent = thisBinded->hasProperty(ESValue(k));
            if (kPresent) {
                ESValue kValue = thisBinded->get(ESValue(k));
                ret->defineDataProperty(ESValue(n), true, true, true, kValue);
                k++;
                n++;
            } else {
                uint32_t tmp = ESArrayObject::nextIndexForward(thisBinded, k, arrlen, false);
                n += tmp - k;
                k = tmp;
            }
        }
        return ret;
    }, strings->slice, 2));

    // $22.1.3.23 Array.prototype.some
    m_arrayPrototype->ESObject::defineDataProperty(strings->some, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {

        // Let O be the result of calling ToObject passing the this value as the argument.
        RESOLVE_THIS_BINDING_TO_OBJECT(O, Array, some);

        // Let lenValue be the result of calling the [[Get]] internal method of O with the argument "length".
        // Let len be ToUint32(lenValue).
        uint32_t len = O->length();

        // If IsCallable(callbackfn) is false, throw a TypeError exception.
        ESValue callbackfn = instance->currentExecutionContext()->readArgument(0);
        if (!callbackfn.isESPointer() || !callbackfn.asESPointer()->isESFunctionObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Array, true, strings->some, errorMessage_GlobalObject_CallbackNotCallable);
        }

        // If thisArg was supplied, let T be thisArg; else let T be undefined.
        ESValue T = instance->currentExecutionContext()->readArgument(1);

        // Let k be 0.
        uint32_t k = 0;

        while (k < len) {
            // Let Pk be ToString(k).
            ESValue pk(k);

            // Let kPresent be the result of calling the [[HasProperty]] internal method of O with argument Pk.
            bool kPresent = O->hasProperty(pk);

            // If kPresent is true, then
            if (kPresent) {
                // Let kValue be the result of calling the [[Get]] internal method of O with argument Pk.
                ESValue kValue = O->get(pk);
                // Let testResult be the result of calling the [[Call]] internal method of callbackfn with T as the this value and argument list containing kValue, k, and O.
                ESValue args[] = {kValue, ESValue(k), O};
                ESValue testResult = ESFunctionObject::call(instance, callbackfn, T, args, 3, false);

                if (testResult.toBoolean()) {
                    return ESValue(true);
                }

                k++;
            } else {
                k = ESArrayObject::nextIndexForward(O, k, len, false);
            }
        }
        return ESValue(false);
    }, strings->some.string(), 1));

    // $22.1.3.24 Array.prototype.sort(comparefn)
    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-array.prototype.sort
    m_arrayPrototype->ESObject::defineDataProperty(strings->sort, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        RESOLVE_THIS_BINDING_TO_OBJECT(thisO, Array, sort);
        ESValue cmpfn = instance->currentExecutionContext()->readArgument(0);
        if (!cmpfn.isUndefined()) {
            if (!(cmpfn.isESPointer() && cmpfn.asESPointer()->isESFunctionObject())) {
                throwBuiltinError(instance, ErrorCode::TypeError, strings->Array, true, strings->sort, errorMessage_GlobalObject_FirstArgumentNotCallable);
            }
        }
        bool defaultSort = (arglen == 0)
            || cmpfn.isUndefined();

        thisO->sort([defaultSort, &cmpfn, &instance, &thisO] (
            const ::escargot::ESValue& a,
            const ::escargot::ESValue& b) -> bool {
            if (a.isEmpty() && b.isUndefined())
                return false;
            if (a.isUndefined() && b.isEmpty())
                return true;
            if (a.isEmpty() || a.isUndefined())
                return false;
            if (b.isEmpty() || b.isUndefined())
                return true;
            ESValue arg[2] = { a, b };
            if (defaultSort) {
                ::escargot::ESString* vala = a.toString();
                ::escargot::ESString* valb = b.toString();
                return *vala < *valb;
            } else {
                ESValue ret = ESFunctionObject::call(
                    instance, cmpfn, ESValue(), arg, 2, false);
                return (ret.toNumber() < 0);
            } });
        return thisO;
    }, strings->sort, 1));

    // $22.1.3.25 Array.prototype.splice(start, deleteCount, ...items)
    m_arrayPrototype->ESObject::defineDataProperty(strings->splice, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        size_t arglen = instance->currentExecutionContext()->argumentCount();
        RESOLVE_THIS_BINDING_TO_OBJECT(thisBinded, Array, splice);
        size_t arrlen = thisBinded->length();
        double relativeStart = instance->currentExecutionContext()->readArgument(0).toInteger();
        size_t start;
        size_t deleteCnt = 0, insertCnt = 0;
        double k;

        if (relativeStart < 0)
            start = arrlen+relativeStart > 0 ? arrlen+relativeStart : 0;
        else
            start = relativeStart > arrlen ? arrlen : relativeStart;

        insertCnt = (arglen > 2)? arglen - 2 : 0;
        double dc = instance->currentExecutionContext()->readArgument(1).toInteger();
        if (dc < 0)
            dc = 0;
        deleteCnt = dc > (arrlen-start) ? arrlen-start : dc;

        escargot::ESArrayObject* ret = ESArrayObject::create(0);

        k = start;
        while (k < static_cast<double>(static_cast<double>(deleteCnt) + start)) {
            ESValue from = ESValue(k);

            if (thisBinded->hasProperty(from)) {
                ret->defineDataProperty(ESValue(k - start), true, true, true, thisBinded->get(from));
                k++;
            } else {
                k = ESArrayObject::nextIndexForward(thisBinded, k, arrlen, false);
            }
        }


        size_t leftInsert = insertCnt;
        if (insertCnt < deleteCnt) {
            thisBinded->relocateIndexesForward(static_cast<double>(start) + deleteCnt, static_cast<double>(arrlen), insertCnt- deleteCnt);

            k = arrlen - 1;
            while (k > static_cast<double>(static_cast<double>(arrlen) - deleteCnt + insertCnt - 1)) {
                if (thisBinded->hasProperty(ESValue(k))) {
                    thisBinded->deletePropertyWithException(ESValue(k));
                    k--;
                } else {
                    k = ESArrayObject::nextIndexBackward(thisBinded, k, -1, false);
                }
            }
        } else if (insertCnt > deleteCnt) {
            thisBinded->relocateIndexesBackward(static_cast<double>(arrlen) - 1, static_cast<double>(start) + deleteCnt - 1, insertCnt - deleteCnt);
        }
        k = start;
        size_t argIdx = 2;
        if (arglen > 2) {
            while (leftInsert > 0) {
                thisBinded->set(ESValue(k), instance->currentExecutionContext()->readArgument(argIdx), true);
                leftInsert--;
                argIdx++;
                k++;
            }
        }

        if (thisBinded->isESArrayObject() && arrlen - deleteCnt > ESValue::ESInvalidIndexValue - insertCnt) {
            throwBuiltinError(instance, ErrorCode::RangeError, strings->Array, true, strings->splice, errorMessage_GlobalObject_RangeError);
        }
        thisBinded->set(strings->length, ESValue(double(arrlen) - deleteCnt + insertCnt), true);
        return ret;
    }, strings->splice, 2));

    // $22.1.3.26 Array.prototype.toLocaleString()
    m_arrayPrototype->ESObject::defineDataProperty(strings->toLocaleString, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(array, Array, toLocaleString);
        size_t len = array->get(strings->length.string()).toUint32();
        escargot::ESString* separator = strings->asciiTable[(size_t)','].string();
        if (len == 0)
            return ESValue(strings->emptyString.string());

        StringRecursionChecker checker(array);
        if (checker.recursionCheck()) {
            return ESValue(strings->emptyString.string());
        }

        escargot::ESString* R;
        ESValue firstElement = array->get(ESValue(0));
        if (firstElement.isUndefinedOrNull())
            R = strings->emptyString.string();
        else {
            ESObject* elementObj = firstElement.toObject();
            ESValue func = elementObj->get(strings->toLocaleString.string());
            if (!func.isESPointer() || !func.asESPointer()->isESFunctionObject())
                throwBuiltinError(instance, ErrorCode::TypeError, strings->Array, true, strings->toLocaleString, errorMessage_GlobalObject_ToLoacleStringNotCallable);
            R = ESFunctionObject::call(instance, func, elementObj, NULL, 0, false).toString();
        }

        size_t k = 1;
        escargot::ESString* S;
        while (k < len) {
            S = ESString::concatTwoStrings(R, separator);
            ESValue nextElement = array->get(ESValue(k));
            if (nextElement.isUndefinedOrNull())
                R = strings->emptyString.string();
            else {
                ESObject* elementObj = nextElement.toObject();
                ESValue func = elementObj->get(strings->toLocaleString.string());
                if (!func.isESPointer() || !func.asESPointer()->isESFunctionObject())
                    throwBuiltinError(instance, ErrorCode::TypeError, strings->Array, true, strings->toLocaleString, errorMessage_GlobalObject_ToLoacleStringNotCallable);
                R = ESFunctionObject::call(instance, func, elementObj, NULL, 0, false).toString();
            }
            R = ESString::concatTwoStrings(S, R);
            k++;
        }

        return ESValue(R);
    }, strings->toLocaleString, 0));

    // $22.1.3.27 Array.prototype.toString()
    m_arrayPrototype->ESObject::defineDataProperty(strings->toString, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisBinded, Array, toString);
        ESValue toString = thisBinded->get(strings->join.string());
        if (!toString.isESPointer() || !toString.asESPointer()->isESFunctionObject()) {
            toString = instance->globalObject()->m_objectProtoTypeToString;
        }
        return ESFunctionObject::call(instance, toString, thisBinded, NULL, 0, false);
    }, strings->toString, 0));

    // $22.1.3.28 Array.prototype.unshift(...items)
    m_arrayPrototype->ESObject::defineDataProperty(strings->unshift, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(O, Array, unshift);
        const uint32_t len = O->get(strings->length.string()).toUint32();
        size_t argCount = instance->currentExecutionContext()->argumentCount();
        O->relocateIndexesBackward(static_cast<double>(len) - 1, -1, argCount);

        ESValue* items = instance->currentExecutionContext()->arguments();
        for (size_t j = 0; j < argCount; j++) {
            O->set(ESValue(j), *(items+j), true);
        }

        if (O->isESArrayObject() && len > ESValue::ESInvalidIndexValue - argCount) {
            throwBuiltinError(instance, ErrorCode::RangeError, strings->Array, true, strings->unshift, errorMessage_GlobalObject_RangeError);
        }
        O->set(strings->length.string(), ESValue(static_cast<double>(len) + argCount), true);
        return ESValue(static_cast<double>(len) + argCount);
    }, strings->unshift.string(), 1));

    m_arrayPrototype->ESObject::set(strings->length, ESValue(0));
    m_arrayPrototype->set__proto__(m_objectPrototype);

    m_array->setProtoType(m_arrayPrototype);

    defineDataProperty(strings->Array, true, false, true, m_array);
}

void GlobalObject::installString()
{
    m_string = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        if (instance->currentExecutionContext()->isNewExpression()) {
            // called as constructor
            ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
            escargot::ESStringObject* stringObject = thisObject->asESStringObject();
            if (instance->currentExecutionContext()->argumentCount() == 0) {
                stringObject->setStringData(strings->emptyString.string());
            } else {
                ESValue value = instance->currentExecutionContext()->readArgument(0);
                stringObject->setStringData(value.toString());
            }
            return stringObject;
        } else {
            // called as function
            if (instance->currentExecutionContext()->argumentCount() == 0)
                return strings->emptyString.string();
            ESValue value = instance->currentExecutionContext()->arguments()[0];
            return value.toString();
        }
        return ESValue();
    }, strings->String, 1, true);
    m_string->forceNonVectorHiddenClass(true);
    m_string->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    m_stringPrototype = ESStringObject::create();
    m_stringPrototype->forceNonVectorHiddenClass(true);

    m_stringPrototype->set__proto__(m_objectPrototype);
    m_stringPrototype->defineDataProperty(strings->constructor, true, false, true, m_string);
    m_stringPrototype->defineDataProperty(strings->toString, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        if (instance->currentExecutionContext()->resolveThisBinding().isObject()) {
            if (instance->currentExecutionContext()->resolveThisBindingToObject()->isESStringObject()) {
                return instance->currentExecutionContext()->resolveThisBindingToObject()->asESStringObject()->stringData();
            }
        }
        if (instance->currentExecutionContext()->resolveThisBinding().isESString())
            return instance->currentExecutionContext()->resolveThisBinding().toString();
        throwBuiltinError(instance, ErrorCode::TypeError, strings->String, true, strings->toString, errorMessage_GlobalObject_ThisNotString);
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->toString, 0));

    m_string->set__proto__(m_functionPrototype); // empty Function
    m_string->setProtoType(m_stringPrototype);

    defineDataProperty(strings->String, true, false, true, m_string);

    // $21.1.2.1 String.fromCharCode(...codeUnits)
    m_string->defineDataProperty(strings->fromCharCode, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int length = instance->currentExecutionContext()->argumentCount();
        if (length == 1) {
            char16_t c = instance->currentExecutionContext()->arguments()[0].toUint32() & 0xFFFF;
            if (c < ESCARGOT_ASCII_TABLE_MAX)
                return strings->asciiTable[c].string();
            return ESString::create(c);
        } else {
            UTF16String elements;
            elements.resize(length);
            char16_t* data = const_cast<char16_t *>(elements.data());
            for (int i = 0; i < length ; i ++) {
                data[i] = {(char16_t)instance->currentExecutionContext()->arguments()[i].toInteger()};
            }
            return ESString::createASCIIStringIfNeeded(std::move(elements));
        }
        return ESValue();
    }, strings->fromCharCode.string(), 1));

    // $21.1.3.1 String.prototype.charAt(pos)
    m_stringPrototype->defineDataProperty(strings->charAt, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(str, String, charAt);
        int position;
        if (instance->currentExecutionContext()->argumentCount() == 0) {
            position = 0;
        } else if (instance->currentExecutionContext()->argumentCount() > 0) {
            position = instance->currentExecutionContext()->arguments()[0].toInteger();
        } else {
            return ESValue(strings->emptyString.string());
        }

        if (LIKELY(0 <= position && position < (int)str->length())) {
            char16_t c = str->charAt(position);
            if (LIKELY(c < ESCARGOT_ASCII_TABLE_MAX)) {
                return strings->asciiTable[c].string();
            } else {
                return ESString::create(c);
            }
        } else {
            return strings->emptyString.string();
        }
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->charAt.string(), 1));

    // $21.1.3.2 String.prototype.charCodeAt(pos)
    m_stringPrototype->defineDataProperty(strings->charCodeAt, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(str, String, charCodeAt);
        int position = instance->currentExecutionContext()->readArgument(0).toInteger();
        ESValue ret;
        if (position < 0 || position >= (int)str->length())
            ret = ESValue(std::numeric_limits<double>::quiet_NaN());
        else
            ret = ESValue(str->charAt(position));
        return ret;
    }, strings->charCodeAt.string(), 1));

    // $21.1.3.4 String.prototype.concat(...args)
    m_stringPrototype->defineDataProperty(strings->concat, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(ret, String, concat);
        int argCount = instance->currentExecutionContext()->argumentCount();
        for (int i = 0; i < argCount; i++) {
            escargot::ESString* arg = instance->currentExecutionContext()->arguments()[i].toString();
            ret = ESString::concatTwoStrings(ret, arg);
        }
        return ret;
    }, strings->concat, 1));

    // $21.1.3.8 String.prototype.indexOf(searchString[, position])
    m_stringPrototype->defineDataProperty(strings->indexOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(str, String, indexOf);
        escargot::ESString* searchStr = instance->currentExecutionContext()->readArgument(0).toString();

        ESValue val = instance->currentExecutionContext()->readArgument(1);
        int pos;
        if (val.isUndefined()) {
            pos = 0;
        } else {
            pos = val.toInteger();
        }

        int len = str->length();
        int start = std::min(std::max(pos, 0), len);
        int result;
        if (str->isASCIIString() && searchStr->isASCIIString())
            result = str->asASCIIString()->find(*searchStr->asASCIIString(), start);
        else if (str->isASCIIString() && !searchStr->isASCIIString())
            result = str->find(searchStr, start);
        else if (!str->isASCIIString() && searchStr->isASCIIString())
            result = str->find(searchStr, start);
        else
            result = str->asUTF16String()->find(*searchStr->asUTF16String(), start);
        return ESValue(result);
    }, strings->indexOf, 1));

    // $21.1.3.9 String.prototype.lastIndexOf ( searchString [ , position ] )
    m_stringPrototype->defineDataProperty(strings->lastIndexOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // Let O be RequireObjectCoercible(this value).
        // Let S be ToString(O).
        RESOLVE_THIS_BINDING_TO_STRING(S, String, indexOf);
        escargot::ESString* searchStr = instance->currentExecutionContext()->readArgument(0).toString();

        double numPos = instance->currentExecutionContext()->readArgument(1).toNumber();
        double pos;
        // If numPos is NaN, let pos be +∞; otherwise, let pos be ToInteger(numPos).
        if (std::isnan(numPos))
            pos = std::numeric_limits<double>::infinity();
        else
            pos = numPos;

        double len = S->length();
        double start = std::min(std::max(pos, 0.0), len);
        int result;

        if (S->isASCIIString() && searchStr->isASCIIString())
            result = S->asASCIIString()->rfind(*searchStr->asASCIIString(), start);
        else if (S->isASCIIString() && !searchStr->isASCIIString())
            result = S->rfind(searchStr, start);
        else if (!S->isASCIIString() && searchStr->isASCIIString())
            result = S->rfind(searchStr, start);
        else
            result = S->asUTF16String()->rfind(*searchStr->asUTF16String(), start);

        return ESValue(result);
    }, strings->lastIndexOf, 1));

    // $21.1.3.10 String.prototype.localeCompare
    m_stringPrototype->defineDataProperty(strings->localeCompare, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(S, String, localeCompare);
        escargot::ESString* That = instance->currentExecutionContext()->readArgument(0).toString();
        return ESValue(stringCompare(*S, *That));
    }, strings->localeCompare.string(), 1));



    // $21.1.3.11 String.prototype.match(regexp)
    m_stringPrototype->defineDataProperty(strings->match, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(thisObject, String, match);

        ESValue argument = instance->currentExecutionContext()->readArgument(0);
        escargot::ESRegExpObject* regexp;
        if (argument.isESPointer() && argument.asESPointer()->isESRegExpObject()) {
            regexp = argument.asESPointer()->asESRegExpObject();
        } else {
            regexp = ESRegExpObject::create(argument, ESValue(ESValue::ESUndefined));
        }

        (void)regexp->lastIndex().toInteger();
        bool isGlobal = regexp->option() & ESRegExpObject::Option::Global;
        if (isGlobal) {
            regexp->set(strings->lastIndex.string(), ESValue(0), true);
        }

        RegexMatchResult result;
        bool testResult = regexp->matchNonGlobally(thisObject, result, false, 0);
        if (!testResult) {
            regexp->set(strings->lastIndex, ESValue(0), true);
            return ESValue(ESValue::ESNull);
        }

        // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/String/match
        // if global flag is on, match method returns an Array containing all matched substrings
        if (isGlobal) {
            return thisObject->createMatchedArray(regexp, result);
        } else {
            return regexp->createRegExpMatchedArray(result, thisObject);
        }
    }, strings->match.string(), 1));

    // $21.1.3.14 String.prototype.replace(searchValue, replaceValue)
    m_stringPrototype->defineDataProperty(strings->replace, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(string, String, replace);
        ESValue searchValue = instance->currentExecutionContext()->readArgument(0);
        ESValue replaceValue = instance->currentExecutionContext()->readArgument(1);
        escargot::ESString* replaceString = nullptr;
        bool replaceValueIsFunction = replaceValue.isESPointer() && replaceValue.asESPointer()->isESFunctionObject();
        if (!replaceValueIsFunction)
            replaceString = replaceValue.toString();
        RegexMatchResult result;

        if (searchValue.isESPointer() && searchValue.asESPointer()->isESRegExpObject()) {
            escargot::ESRegExpObject* regexp = searchValue.asESPointer()->asESRegExpObject();
            (void)regexp->lastIndex().toInteger();
            bool isGlobal = regexp->option() & ESRegExpObject::Option::Global;

            if (isGlobal) {
                regexp->set(strings->lastIndex.string(), ESValue(0), true);
            }
            bool testResult = regexp->matchNonGlobally(string, result, false, 0);
            if (testResult) {
                if (isGlobal) {
                    string->createRegexMatchResult(regexp, result);
                }
            } else {
                regexp->set(strings->lastIndex.string(), ESValue(0), true);
            }
        } else {
            escargot::ESString* searchString = searchValue.toString();
            size_t idx = string->find(searchString);
            if (idx != (size_t)-1) {
                std::vector<RegexMatchResult::RegexMatchResultPiece,
                    pointer_free_allocator<RegexMatchResult::RegexMatchResultPiece> > piece;
                RegexMatchResult::RegexMatchResultPiece p;
                p.m_start = idx;
                p.m_end = idx + searchString->length();
                piece.push_back(std::move(p));
                result.m_matchResults.push_back(std::move(piece));
            }
        }

        if (result.m_matchResults.size() == 0) {
            return string;
        }

        if (replaceValueIsFunction) {
            uint32_t matchCount = result.m_matchResults.size();
            ESValue callee = replaceValue.asESPointer()->asESFunctionObject();

            ESStringBuilder builer;
            builer.appendSubString(string, 0, result.m_matchResults[0][0].m_start);

            for (uint32_t i = 0; i < matchCount ; i ++) {
                int subLen = result.m_matchResults[i].size();
                ESValue* arguments;
                ALLOCA_WRAPPER(instance, arguments, ESValue*, sizeof(ESValue) * (subLen + 2), false);
                for (unsigned j = 0; j < (unsigned)subLen ; j ++) {
                    if (result.m_matchResults[i][j].m_start == std::numeric_limits<unsigned>::max())
                        arguments[j] = ESValue(ESValue::ESUndefined);
                    else {
                        ESStringBuilder argStrBuilder;
                        argStrBuilder.appendSubString(string, result.m_matchResults[i][j].m_start, result.m_matchResults[i][j].m_end);
                        arguments[j] = argStrBuilder.finalize();
                    }
                }
                arguments[subLen] = ESValue((int)result.m_matchResults[i][0].m_start);
                arguments[subLen + 1] = string;
                // 21.1.3.14 (11) it should be called with this as undefined
                escargot::ESString* res = ESFunctionObject::call(instance, callee, ESValue(ESValue::ESUndefined), arguments, subLen + 2, false).toString();
                builer.appendSubString(res, 0, res->length());

                if (i < matchCount - 1) {
                    builer.appendSubString(string, result.m_matchResults[i][0].m_end, result.m_matchResults[i + 1][0].m_start);
                }
            }
            builer.appendSubString(string, result.m_matchResults[matchCount - 1][0].m_end, string->length());
            escargot::ESString* resultString = builer.finalize();
            return resultString;
        } else {
            ASSERT(replaceString);

            bool hasDollar = false;
            for (size_t i = 0; i < replaceString->length() ; i ++) {
                if (replaceString->charAt(i) == '$') {
                    hasDollar = true;
                    break;
                }
            }

            ESStringBuilder builder;
            if (!hasDollar) {
                // flat replace
                int32_t matchCount = result.m_matchResults.size();
                builder.appendSubString(string, 0, result.m_matchResults[0][0].m_start);
                for (int32_t i = 0; i < matchCount ; i ++) {
                    escargot::ESString* res = replaceString;
                    builder.appendString(res);
                    if (i < matchCount - 1) {
                        builder.appendSubString(string, result.m_matchResults[i][0].m_end, result.m_matchResults[i + 1][0].m_start);
                    }
                }
                builder.appendSubString(string, result.m_matchResults[matchCount - 1][0].m_end, string->length());
            } else {
                // dollar replace
                int32_t matchCount = result.m_matchResults.size();
                builder.appendSubString(string, 0, result.m_matchResults[0][0].m_start);
                for (int32_t i = 0; i < matchCount ; i ++) {
                    for (unsigned j = 0; j < replaceString->length() ; j ++) {
                        if (replaceString->charAt(j) == '$' && (j + 1) < replaceString->length()) {
                            char16_t c = replaceString->charAt(j + 1);
                            if (c == '$') {
                                builder.appendChar(replaceString->charAt(j));
                            } else if (c == '&') {
                                builder.appendSubString(string, result.m_matchResults[i][0].m_start, result.m_matchResults[i][0].m_end);
                            } else if (c == '\'') {
                                builder.appendSubString(string, result.m_matchResults[i][0].m_end, string->length());
                            } else if (c == '`') {
                                builder.appendSubString(string, 0, result.m_matchResults[i][0].m_start);
                            } else if ('0' <= c && c <= '9') {
                                size_t idx = c - '0';
                                int peek = replaceString->charAt(j + 2) - '0';
                                bool usePeek = false;
                                if (0 <= peek && peek <= 9) {
                                    idx *= 10;
                                    idx += peek;
                                    usePeek = true;
                                }

                                if (idx < result.m_matchResults[i].size() && idx != 0) {
                                    builder.appendSubString(string, result.m_matchResults[i][idx].m_start, result.m_matchResults[i][idx].m_end);
                                    if (usePeek)
                                        j++;
                                } else {
                                    idx = c - '0';
                                    if (idx < result.m_matchResults[i].size() && idx != 0) {
                                        builder.appendSubString(string, result.m_matchResults[i][idx].m_start, result.m_matchResults[i][idx].m_end);
                                    } else {
                                        builder.appendChar('$');
                                        builder.appendChar(c);
                                    }
                                }
                            } else {
                                builder.appendChar('$');
                                builder.appendChar(c);
                            }
                            j++;
                        } else {
                            builder.appendChar(replaceString->charAt(j));
                        }
                    }
                    if (i < matchCount - 1) {
                        builder.appendSubString(string, result.m_matchResults[i][0].m_end, result.m_matchResults[i + 1][0].m_start);
                    }
                }
                builder.appendSubString(string, result.m_matchResults[matchCount - 1][0].m_end, string->length());
            }
            escargot::ESString* resultString = builder.finalize();
            return resultString;
        }
    }, strings->replace.string(), 2));

    // $21.1.3.15 String.prototype.search
    m_stringPrototype->defineDataProperty(strings->search, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(origStr, String, search);
        ESValue argument = instance->currentExecutionContext()->readArgument(0);

        escargot::ESRegExpObject* regexp;
        if (argument.isESPointer() && argument.asESPointer()->isESRegExpObject()) {
            regexp = argument.asESPointer()->asESRegExpObject();
        } else {
            regexp = ESRegExpObject::create(argument, ESValue(ESValue::ESUndefined));
        }

        RegexMatchResult result;
        regexp->match(origStr, result);
        if (result.m_matchResults.size() != 0) {
            return ESValue(result.m_matchResults[0][0].m_start);
        } else {
            return ESValue(-1);
        }
    }, strings->search.string(), 1));

    // $21.1.3.16 String.prototype.slice(start, end)
    m_stringPrototype->defineDataProperty(strings->slice, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(str, String, slice);
        size_t len = str->length();
        double lenStart = instance->currentExecutionContext()->readArgument(0).toInteger();
        ESValue end = instance->currentExecutionContext()->readArgument(1);
        double doubleEnd = end.isUndefined()? len : end.toInteger();
        int from = (lenStart < 0) ? std::max(len+lenStart, 0.0) : std::min(lenStart, (double)len);
        int to = (doubleEnd < 0) ? std::max(len+doubleEnd, 0.0) : std::min(doubleEnd, (double)len);
        int span = std::max(to-from, 0);
        escargot::ESString* ret;
        if (str->isASCIIString())
            ret = ESString::create(std::move(ASCIIString(str->asASCIIString()->begin()+from, str->asASCIIString()->begin()+from+span)));
        else
            ret = ESString::create(std::move(UTF16String(str->asUTF16String()->begin()+from, str->asUTF16String()->begin()+from+span)));
        return ret;
    }, strings->slice, 2));

    // $15.5.4.14 String.prototype.split(separator, limit)
    m_stringPrototype->defineDataProperty(strings->split, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // 1, 2, 3
        RESOLVE_THIS_BINDING_TO_STRING(S, String, split);
        escargot::ESArrayObject* A = ESArrayObject::create(0);

        // 4, 5
        size_t lengthA = 0;
        size_t lim;
        if (instance->currentExecutionContext()->readArgument(1).isUndefined()) {
            lim = std::pow(2, 32)-1;
        } else {
            lim = instance->currentExecutionContext()->readArgument(1).toUint32();
        }

        // 6, 7
        size_t s = S->length(), p = 0;

        // 8
        ESValue separator = instance->currentExecutionContext()->readArgument(0);
        escargot::ESPointer* P;
        if (separator.isESPointer() && separator.asESPointer()->isESRegExpObject()) {
            P = separator.asESPointer()->asESRegExpObject();
        } else {
            P = separator.toString();
        }

        // 9
        if (lim == 0)
            return A;

        // 10
        if (separator.isUndefined()) {
            A->defineDataProperty(strings->numbers[0].string(), true, true, true, S);
            return A;
        }

        std::function<ESValue(escargot::ESString*, int, escargot::ESString*)> splitMatchUsingStr;
        splitMatchUsingStr = [] (escargot::ESString* S, int q, escargot::ESString* R) -> ESValue {
            int s = S->length();
            int r = R->length();
            if (q + r > s)
                return ESValue(false);
            for (int i = 0; i < r; i++)
                if (S->charAt(q+i) != R->charAt(i))
                    return ESValue(false);
            return ESValue(q+r);
        };
        // 11
        if (s == 0) {
            bool ret = true;
            if (P->isESRegExpObject()) {
                escargot::RegexMatchResult result;
                ret = P->asESRegExpObject()->matchNonGlobally(S, result, false, 0);
            } else {
                ESValue z = splitMatchUsingStr(S, 0, P->asESString());
                if (z.isBoolean()) {
                    ret = z.asBoolean();
                }
            }
            if (ret)
                return A;
            A->defineDataProperty(strings->numbers[0].string(), true, true, true, S);
            return A;
        }

        // 12
        size_t q = p;

        // 13
        if (P->isESRegExpObject()) {
            escargot::ESRegExpObject* R = P->asESRegExpObject();
            while (q != s) {
                RegexMatchResult result;
                bool ret = R->matchNonGlobally(S, result, false, (size_t)q);
                if (!ret) {
                    break;
                }

                if ((size_t)result.m_matchResults[0][0].m_end == p) {
                    q++;
                } else {
                    if (result.m_matchResults[0][0].m_start >= S->length())
                        break;

                    escargot::ESString* T = S->substring(p, result.m_matchResults[0][0].m_start);
                    A->defineDataProperty(ESValue(lengthA++), true, true, true, ESValue(T));
                    if (lengthA == lim)
                        return A;
                    p = result.m_matchResults[0][0].m_end;
                    R->pushBackToRegExpMatchedArray(A, lengthA, lim, result, S);
                    if (lengthA == lim)
                        return A;
                    q = p;
                }
            }
        } else {
            escargot::ESString* R = P->asESString();
            while (q != s) {
                ESValue e = splitMatchUsingStr(S, q, R);
                if (e == ESValue(ESValue::ESFalseTag::ESFalse))
                    q++;
                else {
                    if ((size_t)e.asInt32() == p)
                        q++;
                    else {
                        if (q >= S->length())
                            break;

                        escargot::ESString* T = S->substring(p, q);
                        A->defineDataProperty(ESValue(lengthA++), true, true, true, ESValue(T));
                        if (lengthA == lim)
                            return A;
                        p = e.asInt32();
                        q = p;
                    }
                }
            }
        }

        // 14, 15, 16
        escargot::ESString* T = S->substring(p, s);
        A->defineDataProperty(ESValue(lengthA), true, true, true, ESValue(T));
        return A;
    }, strings->split.string(), 2));

#ifdef USE_ES6_FEATURE
    // $21.1.3.18 String.prototype.startsWith
    m_stringPrototype->defineDataProperty(strings->startsWith, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->startsWith.string(), 1));
#endif

    // $21.1.3.19 String.prototype.substring(start, end)
    m_stringPrototype->defineDataProperty(strings->substring, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(str, String, substring);
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount == 0) {
            return str;
        } else {
            int len = str->length();
            double doubleStart = instance->currentExecutionContext()->arguments()[0].toNumber();
            ESValue end = instance->currentExecutionContext()->readArgument(1);
            double doubleEnd = (argCount < 2 || end.isUndefined()) ? len : end.toNumber();
            doubleStart = (std::isnan(doubleStart)) ? 0 : doubleStart;
            doubleEnd = (std::isnan(doubleEnd)) ? 0 : doubleEnd;

            double finalStart = (int)trunc(std::min(std::max(doubleStart, 0.0), (double)len));
            double finalEnd = (int)trunc(std::min(std::max(doubleEnd, 0.0), (double)len));
            int from = std::min(finalStart, finalEnd);
            int to = std::max(finalStart, finalEnd);
            return str->substring(from, to);
        }


        return ESValue();
    }, strings->substring.string(), 2));

    // $21.1.3.20 String.prototype.toLocaleLowerCase
    m_stringPrototype->defineDataProperty(strings->toLocaleLowerCase, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(str, String, toLocaleLowerCase);
        if (str->isASCIIString()) {
            ASCIIString newstr(*str->asASCIIString());
            // TODO use ICU for this operation
            std::transform(newstr.begin(), newstr.end(), newstr.begin(), ::tolower);
            return ESString::create(std::move(newstr));
        } else {
            UTF16String newstr(*str->asUTF16String());
            icu::UnicodeString _newstr = icu::UnicodeString((const UChar*)newstr.data(), newstr.length());
            _newstr.toLower();
            return ESString::create(_newstr);
        }
    }, strings->toLocaleLowerCase.string(), 0));

    // $21.1.3.22 String.prototype.toLowerCase()
    m_stringPrototype->defineDataProperty(strings->toLowerCase, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(str, String, toLowerCase);
        if (str->isASCIIString()) {
            ASCIIString newstr(*str->asASCIIString());
            // TODO use ICU for this operation
            std::transform(newstr.begin(), newstr.end(), newstr.begin(), ::tolower);
            return ESString::create(std::move(newstr));
        } else {
            UTF16String newstr(*str->asUTF16String());
            icu::UnicodeString _newstr = icu::UnicodeString((const UChar*)newstr.data(), newstr.length());
            _newstr.toLower();
            return ESString::create(_newstr);
        }
    }, strings->toLowerCase.string(), 0));

    // $21.1.3.21 String.prototype.toLocaleUpperCase
    m_stringPrototype->defineDataProperty(strings->toLocaleUpperCase, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(str, String, toLocaleUpperCase);
        if (str->isASCIIString()) {
            ASCIIString newstr(*str->asASCIIString());
            // TODO use ICU for this operation
            std::transform(newstr.begin(), newstr.end(), newstr.begin(), ::toupper);
            return ESString::create(std::move(newstr));
        } else {
            UTF16String newstr(*str->asUTF16String());
            icu::UnicodeString _newstr = icu::UnicodeString((const UChar*)newstr.data(), newstr.length());
            _newstr.toUpper();
            return ESString::create(_newstr);
        }
    }, strings->toLocaleUpperCase.string(), 0));

    // $21.1.3.24 String.prototype.toUpperCase()
    m_stringPrototype->defineDataProperty(strings->toUpperCase, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(str, String, toUpperCase);
        if (str->isASCIIString()) {
            ASCIIString newstr(*str->asASCIIString());
            // TODO use ICU for this operation
            std::transform(newstr.begin(), newstr.end(), newstr.begin(), ::toupper);
            return ESString::create(std::move(newstr));
        } else {
            UTF16String newstr(*str->asUTF16String());
            icu::UnicodeString _newstr = icu::UnicodeString((const UChar*)newstr.data(), newstr.length());
            _newstr.toUpper();
            return ESString::create(_newstr);
        }
    }, strings->toUpperCase.string(), 0));

    // $21.1.3.25 String.prototype.trim()
    m_stringPrototype->defineDataProperty(strings->trim, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(str, String, substring);
        if (str->isASCIIString()) {
            ASCIIString newstr(*str->asASCIIString());
            // trim left
            while (newstr.length()) {
                if (esprima::isWhiteSpace(newstr[0]) || esprima::isLineTerminator(newstr[0])) {
                    newstr.erase(newstr.begin());
                } else {
                    break;
                }
            }

            // trim right
            while (newstr.length()) {
                if (esprima::isWhiteSpace(newstr[newstr.length()-1]) || esprima::isLineTerminator(newstr[newstr.length()-1])) {
                    newstr.erase(newstr.end()-1);
                } else {
                    break;
                }
            }

            return ESString::create(std::move(newstr));
        } else {
            UTF16String newstr(str->toUTF16String());
            // trim left
            while (newstr.length()) {
                if (esprima::isWhiteSpace(newstr[0]) || esprima::isLineTerminator(newstr[0])) {
                    newstr.erase(newstr.begin());
                } else {
                    break;
                }
            }

            // trim right
            while (newstr.length()) {
                if (esprima::isWhiteSpace(newstr[newstr.length()-1]) || esprima::isLineTerminator(newstr[newstr.length()-1])) {
                    newstr.erase(newstr.end()-1);
                } else {
                    break;
                }
            }

            return ESString::create(std::move(newstr));
        }
    }, strings->trim.string(), 0));

    // $21.1.3.26 String.prototype.valueOf ( )
    m_stringPrototype->defineDataProperty(strings->valueOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // Let s be thisStringValue(this value).
        // Return s.
        // The abstract operation thisStringValue(value) performs the following steps:
        // If Type(value) is String, return value.
        // If Type(value) is Object and value has a [[StringData]] internal slot, then
        // Assert: value’s [[StringData]] internal slot is a String value.
        // Return the value of value’s [[StringData]] internal slot.
        // Throw a TypeError exception.
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        if (thisValue.isESString()) {
            return thisValue.toString();
        } else if (thisValue.isESPointer() && thisValue.asESPointer()->isESStringObject()) {
            return thisValue.asESPointer()->asESStringObject()->stringData();
        }
        throwBuiltinError(instance, ErrorCode::TypeError, strings->String, true, strings->valueOf, errorMessage_GlobalObject_ThisNotString);
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->valueOf, 0));


    // $B.2.3.1 String.prototype.substr (start, length)
    m_stringPrototype->defineDataProperty(strings->substr, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_STRING(str, String, substr);
        if (instance->currentExecutionContext()->argumentCount() < 1) {
            return str;
        }
        double intStart = instance->currentExecutionContext()->arguments()[0].toInteger();
        double end;
        if (instance->currentExecutionContext()->argumentCount() > 1) {
            if (instance->currentExecutionContext()->arguments()[1].isUndefined()) {
                end = std::numeric_limits<double>::infinity();
            } else
                end = instance->currentExecutionContext()->arguments()[1].toInteger();
        } else {
            end = std::numeric_limits<double>::infinity();
        }
        double size = str->length();
        if (intStart < 0)
            intStart = std::max(size + intStart, 0.0);
        double resultLength = std::min(std::max(end, 0.0), size - intStart);
        if (resultLength <= 0)
            return strings->emptyString.string();
        return str->substring(intStart, intStart + resultLength);
    }, strings->substr.string(), 2));

    m_stringObjectProxy = ESStringObject::create();
    m_stringObjectProxy->set__proto__(m_string->protoType());
    m_stringObjectProxy->setExtensible(false);
}

template <typename CharType, typename JSONCharType>
ESValue parseJSON(ESVMInstance* instance, const CharType* data)
{
    rapidjson::GenericDocument<JSONCharType> jsonDocument;

    rapidjson::GenericStringStream<JSONCharType> stringStream(data);
    jsonDocument.ParseStream(stringStream);
    if (jsonDocument.HasParseError()) {
        throwBuiltinError(instance, ErrorCode::SyntaxError, strings->JSON, true, strings->parse, rapidjson::GetParseError_En(jsonDocument.GetParseError()));
    }
    // FIXME: JSON.parse treats "__proto__" as a regular property name. (test262: ch15/15.12/15.12.2/S15.12.2_A1.js)
    //        >>> var x1 = JSON.parse('{"__proto__":[]}') // x1.__proto__ = []
    //        >>> var x2 = JSON.parse('{"__proto__":1}') // x2.__proto__ = 1
    //        >>> var y1 = {"__proto__":[]} // y1.__proto__ = []
    //        >>> var y2 = {"__proto__":1} // y2.__proto__ != 1
    //        >>> Object.getPrototypeOf(x1) == Object.prototype // true
    //        >>> Object.getPrototypeOf(x2) == Object.prototype // true
    //        >>> Object.getPrototypeOf(y1) == Object.prototype // false
    //        >>> Object.getPrototypeOf(y2) == Object.prototype // true
    std::function<ESValue(rapidjson::GenericValue<JSONCharType>& value)> fn;
    fn = [&fn](rapidjson::GenericValue<JSONCharType>& value) -> ESValue {
        if (value.IsBool()) {
            return ESValue(value.GetBool());
        } else if (value.IsInt()) {
            return ESValue(value.GetInt());
        } else if (value.IsUint()) {
            return ESValue(value.GetUint());
        } else if (value.IsInt64()) {
            return ESValue(value.GetInt64());
        } else if (value.IsUint64()) {
            return ESValue(value.GetUint64());
        } else if (value.IsDouble()) {
            return ESValue(value.GetDouble());
        } else if (value.IsNull()) {
            return ESValue(ESValue::ESNull);
        } else if (value.IsString()) {
            if (std::is_same<CharType, char16_t>::value) {
                UTF16String str;
                unsigned len = value.GetStringLength();
                str.reserve(len);
                const CharType* chars = value.GetString();
                for (unsigned i = 0; i < len; i++) {
                    str.push_back(chars[i]);
                }
                return ESString::create(str);
            } else {
                const char* valueAsString = (const char*)value.GetString();
                if (isAllASCII(valueAsString, strlen(valueAsString))) {
                    return ESString::create(escargot::ASCIIString(valueAsString));
                } else {
                    return ESString::create(utf8StringToUTF16String(valueAsString, strlen(valueAsString)));
                }
            }
        } else if (value.IsArray()) {
            escargot::ESArrayObject* arr = ESArrayObject::create();
            size_t i = 0;
            auto iter = value.Begin();
            while (iter != value.End()) {
                arr->defineDataProperty(ESValue(i++), true, true, true, fn(*iter));
                iter++;
            }
            return arr;
        } else if (value.IsObject()) {
            escargot::ESObject* obj = ESObject::create();
            auto iter = value.MemberBegin();
            while (iter != value.MemberEnd()) {
                ESValue propertyName = fn(iter->name);
                ASSERT(propertyName.isESString());
                obj->defineDataProperty(propertyName.asESString(), true, true, true, fn(iter->value), true);
                iter++;
            }
            return obj;
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    };

    return fn(jsonDocument);
}

void GlobalObject::installJSON()
{
    // create JSON object
    m_json = ESJSONObject::create();
    m_json->forceNonVectorHiddenClass(true);
    m_json->set__proto__(m_objectPrototype);
    defineDataProperty(strings->JSON, true, false, true, m_json);

    // $24.3.1 JSON.parse(text[, reviver])
    m_json->defineDataProperty(strings->parse, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // 1, 2, 3
        escargot::ESString* JText = instance->currentExecutionContext()->readArgument(0).toString();
        ESValue unfiltered;

        if (JText->isASCIIString()) {
            unfiltered = parseJSON<char, rapidjson::UTF8<char>>(instance, JText->toNullableUTF8String().m_buffer);
        } else {
            unfiltered = parseJSON<char16_t, rapidjson::UTF16<char16_t>>(instance, JText->asUTF16String()->data());
        }

        // 4
        ESValue reviver = instance->currentExecutionContext()->readArgument(1);
        if (reviver.isObject()) {
            if (reviver.isESPointer() && reviver.asESPointer()->isESFunctionObject()) {
                ESValue root = newOperation(instance, instance->globalObject(), instance->globalObject()->object(), NULL, 0);
                root.asESPointer()->asESObject()->defineDataProperty(strings->emptyString, true, true, true, unfiltered);
                std::function<ESValue(ESValue, ESValue)> Walk;
                Walk = [&](ESValue holder, ESValue name) -> ESValue {
                    ESValue val = holder.asESPointer()->asESObject()->get(name);
                    if (val.isESPointer() && val.asESPointer()->isESObject()) {
                        if (val.asESPointer()->isESArrayObject()) {
                            escargot::ESArrayObject* arrObject = val.asESPointer()->asESArrayObject();
                            uint32_t i = 0;
                            uint32_t len = arrObject->length();
                            while (i < len) {
                                ESValue newElement =Walk(val, ESValue(i).toString());
                                if (newElement.isUndefined()) {
                                    arrObject->deleteProperty(ESValue(i).toString());
                                } else {
                                    arrObject->defineDataProperty(ESValue(i).toString(), true, true, true, newElement);
                                }
                                i++;
                            }
                        } else {
                            escargot::ESObject* object = val.asESPointer()->asESObject();
                            ESValueVectorStd keys;
                            object->enumeration([&](ESValue p) {
                                keys.push_back(p.toString());
                            });
                            for (auto key : keys) {
                                if (!object->hasOwnProperty(key))
                                    continue;
                                ESValue newElement = Walk(val, key.toString());
                                if (newElement.isUndefined()) {
                                    object->deleteProperty(key.toString());
                                } else {
                                    object->defineDataProperty(key.toString(), true, true, true, newElement);
                                }
                            }
                        }
                    }
                    ESValue* arguments = (ESValue *)alloca(sizeof(ESValue) * 2);
                    arguments[0] = name;
                    arguments[1] = val;
                    return ESFunctionObject::call(instance, reviver, holder, arguments, 2, false);
                };
                return Walk(root, strings->emptyString.string());
            }
        }

        // 5
        return unfiltered;
    }, strings->parse, 2));

    // $24.3.2 JSON.stringify(value[, replacer[, space ]])
    m_json->defineDataProperty(strings->stringify, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // 1, 2, 3
        ESValue value = instance->currentExecutionContext()->readArgument(0);
        ESValue replacer = instance->currentExecutionContext()->readArgument(1);
        ESValue space = instance->currentExecutionContext()->readArgument(2);
        UTF16String indent = u"";
        ESValueVectorStd stack;
        ESValueVectorStd propertyList;
        bool propertyListTouched = false;

        // 4
        escargot::ESFunctionObject* replacerFunc = NULL;
        if (replacer.isObject()) {
            if (replacer.isESPointer() && replacer.asESPointer()->isESFunctionObject()) {
                replacerFunc = replacer.asESPointer()->asESFunctionObject();
            } else if (replacer.isESPointer() && replacer.asESPointer()->isESArrayObject()) {
                propertyListTouched = true;
                escargot::ESArrayObject* arrObject = replacer.asESPointer()->asESArrayObject();

                std::vector<unsigned> indexes;
                arrObject->enumerationWithNonEnumerable([&indexes](ESValue key, ESHiddenClassPropertyInfo* propertyInfo) {
                    uint32_t idx = key.toIndex();
                    if (idx != ESValue::ESInvalidIndexValue) {
                        indexes.push_back(idx);
                    }
                });
                std::sort(indexes.begin(), indexes.end(), std::less<unsigned>());
                for (uint32_t i = 0; i < indexes.size(); ++i) {
                    ESValue item = ESValue();
                    ESValue property = arrObject->get(indexes[i]);
                    if (property.isESString()) {
                        item = property;
                    } else if (property.isNumber()) {
                        item = property.toString();
                    } else if (property.isObject()) {
                        if (property.asESPointer()->isESStringObject()
                            || property.asESPointer()->isESNumberObject()) {
                            item = property.toString();
                        }
                    }
                    if (!item.isUndefined()) {
                        bool flag = false;
                        for (auto& v : propertyList) {
                            if (*v.toString() == *item.toString()) {
                                flag = true;
                                break;
                            }
                        }
                        if (!flag)
                            propertyList.push_back(std::move(item));
                    }
                }
            }
        }

        // 5
        if (space.isObject()) {
            if (space.isESPointer() && space.asESPointer()->isESNumberObject()) {
                space = ESValue(space.toNumber());
            } else if (space.isESPointer() && space.asESPointer()->isESStringObject()) {
                space =space.toString();
            }
        }

        // 6, 7, 8
        UTF16String gap = u"";
        if (space.isNumber()) {
            int space_cnt = std::min(space.toInteger(), 10.0);
            if (space_cnt >= 1) {
                gap.assign(space_cnt, u' ');
            }
        } else if (space.isESString()) {
            if (space.asESString()->length() <= 10) {
                gap = UTF16String(space.asESString()->toUTF16String());
            } else {
                gap = UTF16String(space.asESString()->toUTF16String()).substr(0, 10);
            }
        }

        std::function<ESValue(ESValue key, ESObject* holder)> Str;
        std::function<ESValue(ESValue value)> JA;
        std::function<ESValue(ESValue value)> JO;
        std::function<UTF16String(ESValue value)> Quote;

        Str = [&](ESValue key, ESObject* holder) -> ESValue {
            ESValue value = holder->get(key);
            if (value.isObject()) {
                ESObject* valObj = value.asESPointer()->asESObject();
                ESValue toJson = valObj->get(strings->toJSON.string());
                if (toJson.isESPointer() && toJson.asESPointer()->isESFunctionObject()) {
                    ESValue* arguments = (ESValue *)alloca(sizeof(ESValue));
                    arguments[0] = key;
                    value = ESFunctionObject::call(instance, toJson, value, arguments, 1, false);
                }
            }

            if (replacerFunc != NULL) {
                ESValue* arguments = (ESValue *)alloca(2 * sizeof(ESValue));
                arguments[0] = key;
                arguments[1] = value;
                value = ESFunctionObject::call(instance, replacerFunc, holder, arguments, 2, false);
            }

            if (value.isObject()) {
                if (value.isESPointer() && value.asESPointer()->isESNumberObject()) {
                    value = ESValue(value.toNumber());
                } else if (value.isESPointer() && value.asESPointer()->isESStringObject()) {
                    value = ESValue(value.toString());
                } else if (value.isESPointer() && value.asESPointer()->isESBooleanObject()) {
                    value = ESValue(value.asESPointer()->asESBooleanObject()->booleanData());
                }
            }
            if (value.isNull()) {
                return strings->null.string();
            }
            if (value.isBoolean()) {
                return value.asBoolean()? strings->stringTrue.string() : strings->stringFalse.string();
            }
            if (value.isESString()) {
                return ESString::create(std::move(Quote(value)));
            }
            if (value.isNumber()) {
                double d = value.toNumber();
                if (std::isfinite(d)) {
                    return value.toString();
                }
                return strings->null.string();
            }
            if (value.isObject()) {
                if (!value.isESPointer() || !value.asESPointer()->isESFunctionObject()) {
                    if (value.isESPointer() && value.asESPointer()->isESArrayObject()) {
                        return JA(value);
                    } else {
                        return JO(value);
                    }
                }
            }

            return ESValue();
        };

        Quote = [&](ESValue value) -> UTF16String {
            UTF16String product = u"\"";
            escargot::ESString* str = value.asESString();
            int len = str->length();

            for (int i = 0; i < len; ++i) {
                char16_t c = str->charAt(i);

                if (c == u'\"' || c == u'\\') {
                    product.append(u"\\");
                    product.append(&c, 1);
                } else if (c == u'\b') {
                    product.append(u"\\");
                    product.append(u"b");
                } else if (c == u'\f') {
                    product.append(u"\\");
                    product.append(u"f");
                } else if (c == u'\n') {
                    product.append(u"\\");
                    product.append(u"n");
                } else if (c == u'\r') {
                    product.append(u"\\");
                    product.append(u"r");
                } else if (c == u'\t') {
                    product.append(u"\\");
                    product.append(u"t");
                } else if (c < u' ') {
                    product.append(u"\\u");
                    product.append(codePointTo4digitString(c));
                } else {
                    product.append(&c, 1);
                }
            }
            product.append(u"\"");
            return product;
        };

        JA = [&](ESValue value) -> ESValue {
            // 1
            for (auto& v : stack) {
                if (v == value) {
                    throwBuiltinError(instance, ErrorCode::TypeError, strings->JSON, false, strings->stringify, errorMessage_GlobalObject_JAError);
                }
            }
            // 2
            stack.push_back(value);
            // 3
            UTF16String stepback = indent;
            // 4
            indent = indent + gap;
            // 5
            std::vector<UTF16String, gc_allocator<UTF16String> > partial;
            escargot::ESArrayObject* arrayObj = value.asESPointer()->asESArrayObject();
            // 6, 7
            uint32_t len = arrayObj->length();
            uint32_t index = 0;
            // 8
            while (index < len) {
                ESValue strP = Str(ESValue(index).toString(), value.asESPointer()->asESObject());
                if (strP.isUndefined()) {
                    partial.push_back(strings->null.string()->toUTF16String());
                } else {
                    partial.push_back(strP.asESString()->toUTF16String());
                }
                index++;
            }
            // 9
            UTF16String final;
            if (partial.size() == 0) {
                final = u"[]";
            } else {
                UTF16String properties;
                int len = partial.size();
                final = u"[";
                if (gap == u"") {
                    for (int i = 0; i < len; ++i) {
                        properties.append(partial[i]);
                        if (i < len - 1) {
                            properties.append(u",");
                        }
                    }
                    final.append(properties);
                    final.append(u"]");
                } else {
                    UTF16String seperator = u",\n" + indent;
                    for (int i = 0; i < len; ++i) {
                        properties.append(partial[i]);
                        if (i < len - 1) {
                            properties.append(seperator);
                        }
                    }
                    final.append(u"\n");
                    final.append(indent);
                    final.append(properties);
                    final.append(u"\n");
                    final.append(stepback);
                    final.append(u"]");
                }
            }
            // 11
            stack.pop_back();
            // 12
            indent = stepback;

            return ESString::create(std::move(final));
        };

        JO = [&](ESValue value) -> ESValue {
            // 1
            for (auto& v : stack) {
                if (v == value) {
                    throwBuiltinError(instance, ErrorCode::TypeError, strings->JSON, false, strings->stringify, errorMessage_GlobalObject_JOError);
                }
            }
            // 2
            stack.push_back(value);
            // 3
            UTF16String stepback = indent;
            // 4
            indent = indent + gap;
            // 5, 6
            ESValueVectorStd k;
            if (propertyListTouched) {
                k = propertyList;
            } else {
                value.asESPointer()->asESObject()->enumeration([&](ESValue key) {
                    k.push_back(key);
                });
            }

            // 7
            std::vector<UTF16String, gc_allocator<UTF16String> > partial;
            // 8
            int len = k.size();
            for (int i = 0; i < len; ++i) {
                ESValue strP = Str(k[i], value.toObject());
                if (!strP.isUndefined()) {
                    UTF16String member = Quote(k[i]);
                    member.append(u":");
                    if (gap != u"") {
                        member.append(u" ");
                    }
                    member.append(strP.toString()->toUTF16String());
                    partial.push_back(std::move(member));
                }
            }
            // 9
            UTF16String final;
            if (partial.size() == 0) {
                final = u"{}";
            } else {
                UTF16String properties;
                int len = partial.size();
                final = u"{";
                if (gap == u"") {
                    for (int i = 0; i < len; ++i) {
                        properties.append(partial[i]);
                        if (i < len - 1) {
                            properties.append(u",");
                        }
                    }
                    final.append(properties);
                    final.append(u"}");
                } else {
                    UTF16String seperator = u",\n" + indent;
                    for (int i = 0; i < len; ++i) {
                        properties.append(partial[i]);
                        if (i < len - 1) {
                            properties.append(seperator);
                        }
                    }
                    final.append(u"\n");
                    final.append(indent);
                    final.append(properties);
                    final.append(u"\n");
                    final.append(stepback);
                    final.append(u"}");
                }
            }
            // 11
            stack.pop_back();
            // 12
            indent = stepback;

            return ESString::create(std::move(final));
        };

        // 9
        ESObject* wrapper = ESObjectCreate();
        // 10
        wrapper->defineDataProperty(strings->emptyString, true, true, true, value);
        return Str(strings->emptyString.string(), wrapper);
    }, strings->stringify, 3));
}

void GlobalObject::installMath()
{
    // create math object
    m_math = ::escargot::ESMathObject::create();
    m_math->forceNonVectorHiddenClass(true);

    // initialize math object: $20.2.1.6 Math.PI
    m_math->defineDataProperty(strings->PI, false, false, false, ESValue(3.1415926535897932));
    // TODO(add reference)
    m_math->defineDataProperty(strings->E, false, false, false, ESValue(2.718281828459045));
    // http://www.ecma-international.org/ecma-262/5.1/#sec-15.8.1.3
    m_math->defineDataProperty(escargot::strings->LN2.string(), false, false, false, ESValue(0.6931471805599453));
    // http://www.ecma-international.org/ecma-262/5.1/#sec-15.8.1.2
    m_math->defineDataProperty(escargot::strings->LN10.string(), false, false, false, ESValue(2.302585092994046));
    // http://www.ecma-international.org/ecma-262/5.1/#sec-15.8.1.4
    m_math->defineDataProperty(escargot::strings->LOG2E.string(), false, false, false, ESValue(1.4426950408889634));
    // http://www.ecma-international.org/ecma-262/5.1/#sec-15.8.1.5
    m_math->defineDataProperty(escargot::strings->LOG10E.string(), false, false, false, ESValue(0.4342944819032518));
    // http://www.ecma-international.org/ecma-262/5.1/#sec-15.8.1.7
    m_math->defineDataProperty(escargot::strings->SQRT1_2.string(), false, false, false, ESValue(0.7071067811865476));
    // http://www.ecma-international.org/ecma-262/5.1/#sec-15.8.1.8
    m_math->defineDataProperty(escargot::strings->SQRT2.string(), false, false, false, ESValue(1.4142135623730951));

    // initialize math object: $20.2.2.1 Math.abs()
    m_math->defineDataProperty(strings->abs, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        return ESValue(std::abs(x.toNumber()));
    }, strings->abs, 1));

    // initialize math object: $20.2.2.2 Math.acos()
    m_math->defineDataProperty(strings->acos, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        return ESValue(acos(x.toNumber()));
    }, strings->acos.string(), 1));

    // initialize math object: $20.2.2.3 Math.acosh()
    m_math->defineDataProperty(strings->acosh, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        return ESValue(acosh(x.toNumber()));
    }, strings->acosh.string(), 1));

    // initialize math object: $20.2.2.4 Math.asin()
    m_math->defineDataProperty(strings->asin, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        return ESValue(asin(x.toNumber()));
    }, strings->asin.string(), 1));

    // initialize math object: $20.2.2.5 Math.asinh()
    m_math->defineDataProperty(strings->asinh, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        return ESValue(asinh(x.toNumber()));
    }, strings->asinh.string(), 1));

    // initialize math object: $20.2.2.6 Math.atan()
    m_math->defineDataProperty(strings->atan, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        return ESValue(atan(x.toNumber()));
    }, strings->atan.string(), 1));

    // initialize math object: $20.2.2.7 Math.atanh()
    m_math->defineDataProperty(strings->atanh, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        return ESValue(atanh(x.toNumber()));
    }, strings->atanh.string(), 1));

    // initialize math object: $20.2.2.8 Math.atan2()
    m_math->defineDataProperty(strings->atan2, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        double x = instance->currentExecutionContext()->readArgument(0).toNumber();
        double y = instance->currentExecutionContext()->readArgument(1).toNumber();
        return ESValue(atan2(x, y));
    }, strings->atan2.string(), 2));

#ifdef USE_ES6_FEATURE
    // initialize math object: $20.2.2.9 Math.cbrt()
    m_math->defineDataProperty(strings->cbrt, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->cbrt.string(), 2));
#endif

    // initialize math object: $20.2.2.10 Math.ceil()
    m_math->defineDataProperty(strings->ceil, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        return ESValue(ceil(x.toNumber()));
    }, strings->ceil, 1));

    // initialize math object: $20.2.2.12 Math.cos()
    m_math->defineDataProperty(strings->cos, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        return ESValue(cos(x.toNumber()));
    }, strings->cos, 1));

    // initialize math object: $20.2.2.14 Math.exp()
    m_math->defineDataProperty(strings->exp, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        return ESValue(exp(x.toNumber()));
    }, strings->exp.string(), 1));

    // initialize math object: $20.2.2.16 Math.floor()
    m_math->defineDataProperty(strings->floor, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        return ESValue(floor(x.toNumber()));
    }, strings->floor, 1));

    // initialize math object: $20.2.2.19 Math.imul()
    m_math->defineDataProperty(strings->imul, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        ESValue y = instance->currentExecutionContext()->readArgument(1);
        uint32_t a = x.toUint32();
        uint32_t b = y.toUint32();
        uint32_t product = (a*b) % 0x100000000ULL;
        if (product >= 0x80000000ULL)
            return ESValue(int(product - 0x100000000ULL));
        return ESValue(product);
    }, strings->imul.string(), 2));

    // initialize math object: $20.2.2.20 Math.log()
    m_math->defineDataProperty(strings->log, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        return ESValue(log(x.toNumber()));
    }, strings->log, 1));

    // initialize math object: $20.2.2.24 Math.max()
    m_math->defineDataProperty(strings->max, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double n_inf = -1 * std::numeric_limits<double>::infinity();
            return ESValue(n_inf);
        } else  {
            double max_value = instance->currentExecutionContext()->arguments()[0].toNumber();
            for (unsigned i = 1; i < arg_size; i++) {
                double value = instance->currentExecutionContext()->arguments()[i].toNumber();
                double qnan = std::numeric_limits<double>::quiet_NaN();
                if (std::isnan(value))
                    return ESValue(qnan);
                if (value > max_value || (!value && !max_value && !std::signbit(value)))
                    max_value = value;
            }
            return ESValue(max_value);
        }
        return ESValue();
    }, strings->max, 2));

    // initialize math object: $20.2.2.25 Math.min()
    m_math->defineDataProperty(strings->min, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            return ESValue(std::numeric_limits<double>::infinity());
        } else {
            double min_value = instance->currentExecutionContext()->arguments()[0].toNumber();
            for (unsigned i = 1; i < arg_size; i++) {
                double value = instance->currentExecutionContext()->arguments()[i].toNumber();
                double qnan = std::numeric_limits<double>::quiet_NaN();
                if (std::isnan(value))
                    return ESValue(qnan);
                if (value < min_value || (!value && !min_value && std::signbit(value)))
                    min_value = value;
            }
            return ESValue(min_value);
        }
        return ESValue();
    }, strings->min, 2));

    // initialize math object: $20.2.2.26 Math.pow()
    m_math->defineDataProperty(strings->pow, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        double x = instance->currentExecutionContext()->readArgument(0).toNumber();
        double y = instance->currentExecutionContext()->readArgument(1).toNumber();
        if (UNLIKELY(std::isnan(y)))
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        if (UNLIKELY(std::abs(x) == 1 && std::isinf(y)))
            return ESValue(std::numeric_limits<double>::quiet_NaN());

        int y_int = static_cast<int>(y);

        if (y == y_int) {
            unsigned n = (y < 0) ? -y : y;
            double m = x;
            double p = 1;
            while (true) {
                if ((n & 1) != 0)
                    p *= m;
                n >>= 1;
                if (n == 0) {
                    if (y < 0) {
                        // Unfortunately, we have to be careful when p has reached
                        // infinity in the computation, because sometimes the higher
                        // internal precision in the pow() implementation would have
                        // given us a finite p. This happens very rarely.

                        double result = 1.0 / p;
                        return (result == 0 && std::isinf(p))
                            ? ESValue(pow(x, static_cast<double>(y))) // Avoid pow(double, int).
                            : ESValue(result);
                    }

                    return ESValue(p);
                }
                m *= m;
            }
        }

        if (std::isinf(x)) {
            if (x > 0) {
                if (y > 0) {
                    return ESValue(std::numeric_limits<double>::infinity());
                } else {
                    return ESValue(0.0);
                }
            } else {
                if (y > 0) {
                    if (y == y_int && y_int % 2) { // odd
                        return ESValue(-std::numeric_limits<double>::infinity());
                    } else {
                        return ESValue(std::numeric_limits<double>::infinity());
                    }
                } else {
                    if (y == y_int && y_int % 2) {
                        return ESValue(-0.0);
                    } else {
                        return ESValue(0.0);
                    }
                }
            }
        }
        // x == -0
        if (1 / x == -std::numeric_limits<double>::infinity()) {
            // y cannot be an odd integer because the case is filtered by "if (y_int == y)" above
            if (y > 0) {
                return ESValue(0);
            } else if (y < 0) {
                return ESValue(std::numeric_limits<double>::infinity());
            }
        }

        if (y == 0.5) {
            return ESValue(sqrt(x));
        } else if (y == -0.5) {
            return ESValue(1.0 / sqrt(x));
        }

        return ESValue(pow(x, y));
    }, strings->pow, 2));

    // initialize math object: $20.2.2.27 Math.random()
    m_math->defineDataProperty(strings->random, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        double rand = (double) std::rand() / RAND_MAX;
        return ESValue(rand);
    }, strings->random, 0));

    // initialize math object: $20.2.2.28 Math.round()
    m_math->defineDataProperty(strings->round, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        double x = instance->currentExecutionContext()->readArgument(0).toNumber();
        if (x == -0.5)
            return ESValue(-0.0);
        else if (x > -0.5)
            return ESValue(round(x));
        else
            return ESValue(floor(x+0.5));
    }, strings->round, 1));

    // initialize math object: $20.2.2.30 Math.sin()
    m_math->defineDataProperty(strings->sin, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        return ESValue(sin(x.toNumber()));
    }, strings->sin, 1));

    // initialize math object: $20.2.2.32 Math.sqrt()
    m_math->defineDataProperty(strings->sqrt, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        return ESValue(sqrt(x.toNumber()));
    }, strings->sqrt, 1));

    // initialize math object: $20.2.2.33 Math.tan()
    m_math->defineDataProperty(strings->tan, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        double x = instance->currentExecutionContext()->readArgument(0).toNumber();
        /*
        If x is NaN, the result is NaN.
        If x is +0, the result is +0.
        If x is −0, the result is −0.
        If x is +∞ or −∞, the result is NaN.
        */
        if (std::isnan(x))
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        else if (x == 0.0) {
            if (std::signbit(x)) {
                return ESValue(ESValue::EncodeAsDouble, -0.0);
            } else {
                return ESValue(0);
            }
        } else if (std::isinf(x))
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        return ESValue(tan(x));
    }, strings->tan, 1));

    // add math to global object
    defineDataProperty(strings->Math, true, false, true, m_math);
}

static int itoa(int64_t value, char *sp, int radix)
{
    char tmp[256]; // be careful with the length of the buffer
    char* tp = tmp;
    int i;
    uint64_t v;

    int sign = (radix == 10 && value < 0);
    if (sign)
        v = -value;
    else
        v = (uint64_t)value;

    while (v || tp == tmp) {
        i = v % radix;
        v /= radix; // v/=radix uses less CPU clocks than v=v/radix does
        if (i < 10)
            *tp++ = i+'0';
        else
            *tp++ = i + 'a' - 10;
    }

    int64_t len = tp - tmp;

    if (sign) {
        *sp++ = '-';
        len++;
    }

    while (tp > tmp) {
        *sp++ = *--tp;
    }
    *sp++ = 0;

    return len;
}

void GlobalObject::installNumber()
{
    // create number object: $20.1.1 The Number Constructor
    m_number = ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        if (instance->currentExecutionContext()->isNewExpression()) {
            if (instance->currentExecutionContext()->argumentCount())
                instance->currentExecutionContext()->resolveThisBindingToObject()->asESNumberObject()->setNumberData(instance->currentExecutionContext()->readArgument(0).toNumber());
            return instance->currentExecutionContext()->resolveThisBinding();
        } else {
            if (instance->currentExecutionContext()->argumentCount())
                return ESValue(instance->currentExecutionContext()->arguments()[0].toNumber());
            else
                return ESValue(0);
        }
    }, strings->Number, 1, true);
    m_number->forceNonVectorHiddenClass(true);
    m_number->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    // create numberPrototype object
    m_numberPrototype = ESNumberObject::create(0.0);
    m_numberPrototype->forceNonVectorHiddenClass(true);

    // initialize number object
    m_number->setProtoType(m_numberPrototype);

    m_numberPrototype->defineDataProperty(strings->constructor, true, false, true, m_number);

    // $ 20.1.2.6 Number.MAX_SAFE_INTEGER
    m_number->defineDataProperty(strings->MAX_SAFE_INTEGER, false, false, false, ESValue(9007199254740991.0));
    // $ 20.1.2.7 Number.MAX_VALUE
    m_number->defineDataProperty(strings->MAX_VALUE, false, false, false, ESValue(1.7976931348623157E+308));
    // $ 20.1.2.8 Number.MIN_SAFE_INTEGER
    m_number->defineDataProperty(strings->MIN_SAFE_INTEGER, false, false, false, ESValue(ESValue::EncodeAsDouble, -9007199254740991.0));
    // $ 20.1.2.9 Number.MIN_VALUE
    m_number->defineDataProperty(strings->MIN_VALUE, false, false, false, ESValue(5E-324));
    // $ 20.1.2.10 Number.NaN
    m_number->defineDataProperty(strings->NaN, false, false, false, ESValue(std::numeric_limits<double>::quiet_NaN()));
    // $ 20.1.2.11 Number.NEGATIVE_INFINITY
    m_number->defineDataProperty(strings->NEGATIVE_INFINITY, false, false, false, ESValue(-std::numeric_limits<double>::infinity()));
    // $ 20.1.2.14 Number.POSITIVE_INFINITY
    m_number->defineDataProperty(strings->POSITIVE_INFINITY, false, false, false, ESValue(std::numeric_limits<double>::infinity()));

    // initialize numberPrototype object
    m_numberPrototype->set__proto__(m_objectPrototype);

    // $20.1.3.2 Number.prototype.toExponential
    m_numberPrototype->defineDataProperty(strings->toExponential, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        double number = 0.0;

        if (thisValue.isNumber())
            number = thisValue.asNumber();
        else if (thisValue.isESPointer() && thisValue.asESPointer()->isESNumberObject())
            number = thisValue.asESPointer()->asESNumberObject()->numberData();
        else
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Number, true, strings->toExponential, errorMessage_GlobalObject_ThisNotNumber);

        int arglen = instance->currentExecutionContext()->argumentCount();
        int digit = 0; // only used when an argument is given
        if (arglen > 0) {
            double fractionDigits = instance->currentExecutionContext()->arguments()[0].toNumber();
            digit = (int) trunc(fractionDigits);
        }
        if (std::isnan(number)) { // 3
            return strings->NaN.string();
        }

        char buf[512];
        std::basic_ostringstream<char> stream;
        std::basic_ostringstream<char> expStream;

        if (number < 0) { // 5
            stream << "-";
            number = -1 * number;
        }
        if (std::isinf(number)) { // 6
            snprintf(buf, sizeof(buf), stream.str().c_str(), number, exp);
            return ESString::concatTwoStrings(ESString::create(buf), strings->Infinity.string());
        }

        if (digit < 0 || digit > 20) {
            throwBuiltinError(instance, ErrorCode::RangeError, strings->Number, true, strings->toExponential, errorMessage_GlobalObject_RangeError);
        }

        int exp = 0;
        if (number == 0) {
            exp = 0;
        } else if (std::abs(number) >= 10) {
            double tmp = number;
            while (tmp >= 10) {
                exp++;
                tmp /= 10.0;
            }
        } else if (std::abs(number) < 1) {
            double tmp = number;
            while (tmp < 1) {
                exp--;
                tmp *= 10.0;
            }
        }

        number /= pow(10, exp);

        if (arglen == 0) {
            stream << "%.15lf";
        } else {
            stream << "%." << digit << "lf";
        }
        snprintf(buf, sizeof(buf), stream.str().c_str(), number);

        // remove trailing zeros
        char* tail = nullptr;
        if (arglen == 0) {
            tail = buf + strlen(buf) - 1;
            while (*tail == '0' && *tail-- != '.') { }
            tail++;
        } else {
            for (size_t i = 0; i < strlen(buf); i++) {
                tail = &buf[i];
                if (*tail == '.') {
                    break;
                }
            }
            tail++;
            for (int i = 0; i < digit; i++)
                tail++;
        }
        if (*(tail-1) == '.')
            tail--;

        expStream << "e";
        if (exp >= 0) {
            expStream << "+";
        }
        expStream << "%d";
        snprintf(tail, 512 - (ptrdiff_t)(buf - tail), expStream.str().c_str(), exp);

        return ESValue(ESString::create(buf));

    }, strings->toExponential.string(), 1));

    // initialize numberPrototype object: $20.1.3.3 Number.prototype.toFixed(fractionDigits)
    m_numberPrototype->defineDataProperty(strings->toFixed, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        double number = 0.0;

        if (thisValue.isNumber()) {
            number = thisValue.asNumber();
        } else if (thisValue.isESPointer() && thisValue.asESPointer()->isESNumberObject()) {
            number = thisValue.asESPointer()->asESNumberObject()->numberData();
        } else {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Number, true, strings->toFixed, errorMessage_GlobalObject_ThisNotNumber);
        }

        int arglen = instance->currentExecutionContext()->argumentCount();
        if (arglen == 0) {
            bool isInteger = (static_cast<int64_t>(number) == number);
            if (isInteger) {
                char buffer[256];
                itoa(static_cast<int64_t>(number), buffer, 10);
                return ESString::create(buffer);
            } else {
                return ESValue(round(number)).toString();
            }
        } else if (arglen >= 1) {
            double digit_d = instance->currentExecutionContext()->arguments()[0].toNumber();
            if (digit_d == 0 || std::isnan(digit_d)) {
                return ESValue(round(number)).toString();
            }
            int digit = (int) trunc(digit_d);
            if (digit < 0 || digit > 20) {
                throwBuiltinError(instance, ErrorCode::RangeError, strings->Number, true, strings->toFixed, errorMessage_GlobalObject_RangeError);
            }
            if (std::isnan(number) || std::isinf(number)) {
                return ESValue(number).toString();
            } else if (std::abs(number) >= pow(10, 21)) {
                return ESValue(round(number)).toString();
            }

            std::basic_ostringstream<char> stream;
            if (number < 0)
                stream << "-";
            stream << "%." << digit << "lf";
            std::string fstr = stream.str();
            char buf[512];
            snprintf(buf, sizeof(buf), fstr.c_str(), std::abs(number));
            return ESValue(ESString::create(buf));
        }
        return ESValue();
    }, strings->toFixed, 1));

    // $20.1.3.5 Number.prototype.toPrecision
    m_numberPrototype->defineDataProperty(strings->toPrecision, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        double number = 0.0;

        if (thisValue.isNumber()) {
            number = thisValue.asNumber();
        } else if (thisValue.isESPointer() && thisValue.asESPointer()->isESNumberObject()) {
            number = thisValue.asESPointer()->asESNumberObject()->numberData();
        } else {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Number, true, strings->toPrecision, errorMessage_GlobalObject_ThisNotNumber);
        }

        int arglen = instance->currentExecutionContext()->argumentCount();
        if (arglen == 0 || instance->currentExecutionContext()->arguments()[0].isUndefined()) {
            return ESValue(number).toString();
        } else if (arglen >= 1) {
            double x = number;
            double p_d = instance->currentExecutionContext()->arguments()[0].toNumber();
            if (std::isnan(x)) {
                return strings->NaN.string();
            }
            std::basic_ostringstream<char> stream;
            if (x < 0) {
                stream << "-";
                x = -x;
            }
            if (std::isinf(x)) {
                stream << "Infinity";
            } else {
                int p = (int) trunc(p_d);
                if (p < 1 || p > 21) {
                    throwBuiltinError(instance, ErrorCode::RangeError, strings->Number, true, strings->toPrecision, errorMessage_GlobalObject_RangeError);
                }

                if (LIKELY(x != 0)) {
                    int log10_num = trunc(log10(x));
                    if (log10_num + 1 <= p && log10_num > -6) {
                        if (std::abs(x) >= 1) {
                            stream << "%" << log10_num + 1 << "." << (p - log10_num - 1) << "lf";
                        } else {
                            stream << "%" << log10_num << "." << (p - log10_num) << "lf";
                        }
                    } else {
                        x = x / pow(10, log10_num);
                        if (std::abs(x) < 1) {
                            x *= 10;
                            log10_num--;
                        }
                        stream << "%1." << (p - 1) << "lf" << "e" << ((log10_num >= 0) ? "+" : "") << log10_num;
                    }
                } else {
                    stream << "%1." << (p - 1) << "lf";
                }
            }
            std::string fstr = stream.str();
            char buf[512];
            snprintf(buf, sizeof(buf), fstr.c_str(), x);
            return ESValue(ESString::create(buf));
        }

        return ESValue();
    }, strings->toPrecision, 1));

    escargot::ESFunctionObject* toStringFunction = escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        double number = 0.0;

        if (thisValue.isNumber()) {
            number = thisValue.asNumber();
        } else if (thisValue.isESPointer() && thisValue.asESPointer()->isESNumberObject()) {
            number = thisValue.asESPointer()->asESNumberObject()->numberData();
        } else {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Number, true, strings->toString, errorMessage_GlobalObject_ThisNotNumber);
        }

        if (std::isnan(number) || std::isinf(number)) {
            return (ESValue(number).toString());
        }
        int arglen = instance->currentExecutionContext()->argumentCount();
        double radix = 10;
        if (arglen >= 1 && !instance->currentExecutionContext()->arguments()[0].isUndefined()) {
            radix = instance->currentExecutionContext()->arguments()[0].toInteger();
            if (radix < 2 || radix > 36)
                throwBuiltinError(instance, ErrorCode::RangeError, strings->Number, true, strings->toString, errorMessage_GlobalObject_RadixInvalidRange);
        }
        if (radix == 10)
            return (ESValue(number).toString());
        else {
            bool isInteger = (static_cast<int64_t>(number) == number);
            if (isInteger) {
                bool minusFlag = (number < 0) ? 1 : 0;
                number = (number < 0) ? (-1 * number) : number;
                char buffer[256];
                if (minusFlag) {
                    buffer[0] = '-';
                    itoa(static_cast<int64_t>(number), &buffer[1], radix);
                } else {
                    itoa(static_cast<int64_t>(number), buffer, radix);
                }
                return (ESString::create(buffer));
            } else {
                ASSERT(ESValue(number).isDouble());
                ESNumberObject::RadixBuffer s;
                const char* str = ESNumberObject::toStringWithRadix(s, number, radix);
                return ESString::create(str);
            }
        }
        // TODO: in case that 'this' is floating point number
        // TODO: parameter 'null' should throw exception
        return ESValue();
    }, strings->toString, 1);
    // initialize numberPrototype object: $20.1.3.6 Number.prototype.toString()
    m_numberPrototype->defineDataProperty(strings->toString, true, false, true, toStringFunction);

    // $20.1.3.4 Number.prototype.toLocaleString
    m_numberPrototype->defineDataProperty(strings->toLocaleString, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        double number = 0.0;

        if (thisValue.isNumber()) {
            number = thisValue.asNumber();
        } else if (thisValue.isESPointer() && thisValue.asESPointer()->isESNumberObject()) {
            number = thisValue.asESPointer()->asESNumberObject()->numberData();
        } else {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Number, true, strings->toLocaleString, errorMessage_GlobalObject_ThisNotNumber);
        }

        if (std::isnan(number) || std::isinf(number)) {
            return (ESValue(number).toString());
        }
        int arglen = instance->currentExecutionContext()->argumentCount();
        double radix = 10;
        if (arglen >= 1 && !instance->currentExecutionContext()->arguments()[0].isUndefined()) {
            radix = instance->currentExecutionContext()->arguments()[0].toInteger();
            if (radix < 2 || radix > 36)
                throwBuiltinError(instance, ErrorCode::RangeError, strings->String, true, strings->toString, errorMessage_GlobalObject_RadixInvalidRange);
        }
        if (radix == 10)
            return (ESValue(number).toString());
        else {
            bool isInteger = (static_cast<int64_t>(number) == number);
            if (isInteger) {
                bool minusFlag = (number < 0) ? 1 : 0;
                number = (number < 0) ? (-1 * number) : number;
                char buffer[256];
                if (minusFlag) {
                    buffer[0] = '-';
                    itoa(static_cast<int64_t>(number), &buffer[1], radix);
                } else {
                    itoa(static_cast<int64_t>(number), buffer, radix);
                }
                return (ESString::create(buffer));
            } else {
                ASSERT(ESValue(number).isDouble());
                ESNumberObject::RadixBuffer s;
                const char* str = ESNumberObject::toStringWithRadix(s, number, radix);
                return ESString::create(str);
            }
        }
        return ESValue();
    }, strings->toLocaleString, 0));

    // $20.1.3.26 Number.prototype.valueOf ( )
    m_numberPrototype->defineDataProperty(strings->valueOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // Let s be thisNumberValue(this value).
        // Return s.
        // The abstract operation thisNumberValue(value) performs the following steps:
        // If Type(value) is Number, return value.
        // If Type(value) is Object and value has a [[NumberData]] internal slot, then
        // Assert: value’s [[NumberData]] internal slot is a Number value.
        // Return the value of value’s [[NumberData]] internal slot.
        // Throw a TypeError exception.
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        if (thisValue.isNumber()) {
            return ESValue(thisValue.asNumber());
        } else if (thisValue.isESPointer() && thisValue.asESPointer()->isESNumberObject()) {
            return ESValue(thisValue.asESPointer()->asESNumberObject()->numberData());
        }
        throwBuiltinError(instance, ErrorCode::TypeError, strings->Number, true, strings->valueOf, errorMessage_GlobalObject_ThisNotNumber);
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->valueOf, 0));

    // add number to global object
    defineDataProperty(strings->Number, true, false, true, m_number);

    m_numberObjectProxy = ESNumberObject::create(0);
    m_numberObjectProxy->set__proto__(m_numberPrototype);
    m_numberObjectProxy->setExtensible(false);
}

void GlobalObject::installBoolean()
{
    // create number object: $19.3.1 The Boolean Constructor
    m_boolean = ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        bool val = false;
        if (arglen >= 1) {
            val = instance->currentExecutionContext()->arguments()[0].toBoolean();
        }
        ESValue ret;
        if (val)
            ret = ESValue(ESValue::ESTrueTag::ESTrue);
        else
            ret = ESValue(ESValue::ESFalseTag::ESFalse);

        if (instance->currentExecutionContext()->isNewExpression() && instance->currentExecutionContext()->resolveThisBindingToObject()->isESBooleanObject()) {
            ::escargot::ESBooleanObject* o = instance->currentExecutionContext()->resolveThisBindingToObject()->asESBooleanObject();
            o->setBooleanData(ret.toBoolean());
            return (o);
        } else // If NewTarget is undefined, return b
            return (ret);
        return ESValue();
    }, strings->Boolean, 1, true);
    m_boolean->forceNonVectorHiddenClass(true);
    m_boolean->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    // create booleanPrototype object
    m_booleanPrototype = ESBooleanObject::create(false);
    m_booleanPrototype->forceNonVectorHiddenClass(true);
    m_booleanPrototype->set__proto__(m_objectPrototype);

    // initialize boolean object
    m_boolean->setProtoType(m_booleanPrototype);

    // initialize booleanPrototype object
    m_booleanPrototype->set__proto__(m_objectPrototype);

    m_booleanPrototype->defineDataProperty(strings->constructor, true, false, true, m_boolean);

    // initialize booleanPrototype object: $19.3.3.2 Boolean.prototype.toString()
    m_booleanPrototype->defineDataProperty(strings->toString, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        if (thisValue.isBoolean()) {
            return ESValue(thisValue.toString());
        } else if (thisValue.isESPointer() && thisValue.asESPointer()->isESBooleanObject()) {
            return ESValue(thisValue.asESPointer()->asESBooleanObject()->booleanData()).toString();
        } else {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Boolean, true, strings->toString, errorMessage_GlobalObject_ThisNotBoolean);
            RELEASE_ASSERT_NOT_REACHED();
        }
    }, strings->toString, 0));

    // initialize booleanPrototype object: $19.3.3.3 Boolean.prototype.valueOf()
    m_booleanPrototype->defineDataProperty(strings->valueOf, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        if (thisValue.isBoolean()) {
            return ESValue(thisValue);
        } else if (thisValue.isESPointer() && thisValue.asESPointer()->isESBooleanObject()) {
            return ESValue(thisValue.asESPointer()->asESBooleanObject()->booleanData());
        }
        throwBuiltinError(instance, ErrorCode::TypeError, strings->Boolean, true, strings->valueOf, errorMessage_GlobalObject_ThisNotBoolean);
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->valueOf, 0));

    // add number to global object
    defineDataProperty(strings->Boolean, true, false, true, m_boolean);

    m_booleanObjectProxy = ESBooleanObject::create(false);
    m_booleanObjectProxy->set__proto__(m_booleanPrototype);
    m_booleanObjectProxy->setExtensible(false);
}

void GlobalObject::installRegExp()
{
    // create regexp object: $21.2.3 The RegExp Constructor
    m_regexp = ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESRegExpObject* regexp =
            ESRegExpObject::create(instance->currentExecutionContext()->readArgument(0),
                instance->currentExecutionContext()->readArgument(1));
        return regexp;
    }, strings->RegExp, 2, true);
    m_regexp->forceNonVectorHiddenClass(true);
    m_regexp->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    // create regexpPrototype object
    m_regexpPrototype = ESRegExpObject::create(strings->defaultRegExpString, ESRegExpObject::Option::None);
    m_regexpPrototype->forceNonVectorHiddenClass(true);
    m_regexpPrototype->set__proto__(m_objectPrototype);

    m_regexpPrototype->defineDataProperty(strings->constructor, true, false, true, m_regexp);

    // initialize regexp object
    m_regexp->setProtoType(m_regexpPrototype);


    // 21.2.5.13 RegExp.prototype.test( S )

    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-regexp.prototype.test
    m_regexpPrototype->defineDataProperty(strings->test, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisVal = instance->currentExecutionContext()->resolveThisBinding();
        if (!thisVal.isESPointer() || !thisVal.asESPointer()->isESRegExpObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->RegExp, true, strings->test, errorMessage_GlobalObject_ThisNotRegExpObject);
        escargot::ESRegExpObject* regexp = thisVal.asESPointer()->asESRegExpObject();
        escargot::ESString* sourceStr = instance->currentExecutionContext()->readArgument(0).toString();
        double lastIndex = regexp->lastIndex().toInteger();
        if ((!regexp->option()) & ESRegExpObject::Option::Global) {
            lastIndex = 0;
        }
        if (lastIndex < 0 || lastIndex > sourceStr->length()) {
            regexp->set(strings->lastIndex, ESValue(0), true);
            return ESValue(false);
        }
        RegexMatchResult result;
        bool testResult = regexp->match(sourceStr, result, true, lastIndex);
        return (ESValue(testResult));
    }, strings->test, 1));

    // 21.2.5.2 RegExp.prototype.exec( string )
    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-regexp.prototype.test
    m_regexpPrototype->defineDataProperty(strings->exec, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisVal = instance->currentExecutionContext()->resolveThisBinding();
        if (!thisVal.isESPointer() || !thisVal.asESPointer()->isESRegExpObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->RegExp, true, strings->exec, errorMessage_GlobalObject_ThisNotRegExpObject);
        escargot::ESRegExpObject* regexp = thisVal.asESPointer()->asESRegExpObject();
        escargot::ESString* sourceStr = instance->currentExecutionContext()->readArgument(0).toString();
        bool isGlobal = regexp->option() & ESRegExpObject::Option::Global;
        double lastIndex = regexp->lastIndex().toInteger();
        if (!isGlobal) {
            lastIndex = 0;
        }
        RegexMatchResult result;

        if (lastIndex < 0 || lastIndex > sourceStr->length()) {
            regexp->set(strings->lastIndex, ESValue(0), true);
            return ESValue(ESValue::ESNull);
        }

        if (regexp->matchNonGlobally(sourceStr, result, false, lastIndex)) {
            if (isGlobal) {
                regexp->set(strings->lastIndex, ESValue(result.m_matchResults[0][0].m_end), true);
            }
            return regexp->createRegExpMatchedArray(result, sourceStr);
        }

        regexp->set(strings->lastIndex, ESValue(0), true);
        return ESValue(ESValue::ESNull);

    }, strings->exec, 1));

    // $21.2.5.14 RegExp.prototype.toString
    m_regexpPrototype->defineDataProperty(strings->toString, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisVal = instance->currentExecutionContext()->resolveThisBinding();
        if (!thisVal.isESPointer() || !thisVal.asESPointer()->isESRegExpObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->RegExp, true, strings->toString, errorMessage_GlobalObject_ThisNotRegExpObject);
        escargot::ESRegExpObject* R = thisVal.asESPointer()->asESRegExpObject();

        escargot::ESString* ret = ESString::concatTwoStrings(ESString::create("/"), R->get(strings->source.string()).toString());
        ret = ESString::concatTwoStrings(ret, ESString::create("/"));
        ESRegExpObject::Option option = R->option();

        char flags[5] = {0};
        int flags_idx = 0;
        if (option & ESRegExpObject::Option::Global) {
            flags[flags_idx++] = 'g';
        }
        if (option & ESRegExpObject::Option::IgnoreCase) {
            flags[flags_idx++] = 'i';
        }
        if (option & ESRegExpObject::Option::MultiLine) {
            flags[flags_idx++] = 'm';
        }
        if (option & ESRegExpObject::Option::Sticky) {
            flags[flags_idx++] = 'y';
        }
        ret = ESString::concatTwoStrings(ret, ESString::create(flags));

        return ret;
    }, strings->toString, 0));

    // $21.2.5.14 RegExp.prototype.compile
    m_regexpPrototype->defineDataProperty(strings->compile, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->compile, 2));

    // add regexp to global object
    defineDataProperty(strings->RegExp, true, false, true, m_regexp);
}

#ifdef USE_ES6_FEATURE
void GlobalObject::installArrayBuffer()
{
    m_arrayBufferPrototype = ESArrayBufferObject::create();
    m_arrayBufferPrototype->forceNonVectorHiddenClass(true);
    m_arrayBufferPrototype->set__proto__(m_objectPrototype);
    m_arrayBufferPrototype->defineDataProperty(strings->constructor, true, false, true, m_arrayBuffer);

    // $24.1.2.1 ArrayBuffer(length)
    m_arrayBuffer = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // if NewTarget is undefined, throw a TypeError
        if (!instance->currentExecutionContext()->isNewExpression())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->ArrayBuffer, false, strings->emptyString, errorMessage_GlobalObject_NotExistNewInArrayBufferConstructor);
        ASSERT(instance->currentExecutionContext()->resolveThisBindingToObject()->isESArrayBufferObject());
        escargot::ESArrayBufferObject* obj = instance->currentExecutionContext()->resolveThisBindingToObject()->asESArrayBufferObject();
        int len = instance->currentExecutionContext()->argumentCount();
        if (len == 0)
            obj->allocateArrayBuffer(0);
        else if (len >= 1) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            double numberLength = val.toNumber();
            double byteLength = ESValue(numberLength).toLength();
            if (numberLength != byteLength)
                throwBuiltinError(instance, ErrorCode::RangeError, strings->ArrayBuffer, false, strings->emptyString, errorMessage_GlobalObject_FirstArgumentInvalidLength);
            obj->allocateArrayBuffer(byteLength);
        }
        return obj;
    }, strings->ArrayBuffer, 1, true);
    m_arrayBuffer->forceNonVectorHiddenClass(true);
    m_arrayBuffer->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    m_arrayBufferPrototype->defineDataProperty(strings->constructor, true, false, true, m_arrayBuffer);

    // $24.1.4.1
    m_arrayBufferPrototype->defineAccessorProperty(strings->byteLength.string(), new ESPropertyAccessorData(
        ESFunctionObject::create(NULL, [](ESVMInstance* instance) -> ESValue {
            ESObject* originalObj = instance->currentExecutionContext()->resolveThisBindingToObject();
            // FIXME find right object from originalObj
            if (originalObj == instance->globalObject()->arrayBufferPrototype())
                throwBuiltinError(instance, ErrorCode::TypeError, strings->ArrayBuffer, true, strings->byteLength, "%s: this object should be ArrayBuffer object");
            return ESValue(originalObj->asESArrayBufferObject()->bytelength());
        }, ESString::create("get byteLength")), NULL)
    , false, false, true);

    // $24.1.4.3 ArrayBuffer.prototype.slice(start, end)
    m_arrayBufferPrototype->defineDataProperty(strings->slice, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
        ESValue end = instance->currentExecutionContext()->readArgument(1);
        if (!thisObject->isESArrayBufferObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->ArrayBuffer, true, strings->slice, "%s: this object is not an ArrayBuffer object");
        escargot::ESArrayBufferObject* obj = thisObject->asESArrayBufferObject();
        if (obj->isDetachedBuffer())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->ArrayBuffer, true, strings->slice, "%s: ArrayBuffer is detached buffer");

        double len = obj->bytelength();
        double relativeStart = instance->currentExecutionContext()->readArgument(0).toInteger();
        unsigned first = (relativeStart < 0) ? std::max(len + relativeStart, 0.0) : std::min(relativeStart, len);
        double relativeEnd = end.isUndefined() ? len : end.toInteger();
        unsigned final_ = (relativeEnd < 0) ? std::max(len + relativeEnd, 0.0) : std::min(relativeEnd, len);
        unsigned newLen = std::max((int)final_ - (int)first, 0);

        escargot::ESArrayBufferObject* newObject;

        ESValue constructor = thisObject->get(strings->constructor.string());
        if (constructor.isUndefined()) {
            newObject = ESArrayBufferObject::createAndAllocate(newLen);
        } else {
            if (!constructor.isObject() || !constructor.asObject()->isESFunctionObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->ArrayBuffer, true, strings->slice, "%s: constructor of ArrayBuffer is not a function");

            ESValue arguments[] = { ESValue(newLen) };
            escargot::ESValue newValue = newOperation(instance, instance->globalObject(), constructor, arguments, 1);
            // TODO : newValue could be non-ArrayBuffer value
            newObject = newValue.asObject()->asESArrayBufferObject();
        }

        if (newObject->isDetachedBuffer()) // 18
            RELEASE_ASSERT_NOT_REACHED();
        if (newObject == obj) // 19
            RELEASE_ASSERT_NOT_REACHED();
        if (newObject->bytelength() < newLen) // 20
            RELEASE_ASSERT_NOT_REACHED();
        if (newObject->isDetachedBuffer()) // 22
            RELEASE_ASSERT_NOT_REACHED();

        newObject->copyDataFrom(obj, first, newLen);

        return newObject;
    }, strings->slice, 2));

    m_arrayBuffer->set__proto__(m_functionPrototype); // empty Function
    m_arrayBuffer->setProtoType(m_arrayBufferPrototype);
    defineDataProperty(strings->ArrayBuffer, true, false, true, m_arrayBuffer);
}

void GlobalObject::installTypedArray()
{
    m_Int8Array = installTypedArray<Int8Adaptor> (strings->Int8Array);
    m_Int16Array = installTypedArray<Int16Adaptor>(strings->Int16Array);
    m_Int32Array = installTypedArray<Int32Adaptor>(strings->Int32Array);
    m_Uint8Array = installTypedArray<Uint8Adaptor>(strings->Uint8Array);
    m_Uint16Array = installTypedArray<Uint16Adaptor>(strings->Uint16Array);
    m_Uint32Array = installTypedArray<Uint32Adaptor>(strings->Uint32Array);
    m_Uint8ClampedArray = installTypedArray<Uint8Adaptor> (strings->Uint8ClampedArray);
    m_Float32Array = installTypedArray<Float32Adaptor> (strings->Float32Array);
    m_Float64Array = installTypedArray<Float64Adaptor>(strings->Float64Array);
    m_Int8ArrayPrototype = m_Int8Array->protoType().asESPointer()->asESObject();
    m_Int16ArrayPrototype = m_Int16Array->protoType().asESPointer()->asESObject();
    m_Int32ArrayPrototype = m_Int32Array->protoType().asESPointer()->asESObject();
    m_Uint8ArrayPrototype = m_Uint8Array->protoType().asESPointer()->asESObject();
    m_Uint16ArrayPrototype = m_Uint16Array->protoType().asESPointer()->asESObject();
    m_Uint32ArrayPrototype = m_Uint32Array->protoType().asESPointer()->asESObject();
    m_Float32ArrayPrototype = m_Float32Array->protoType().asESPointer()->asESObject();
    m_Float64ArrayPrototype = m_Float64Array->protoType().asESPointer()->asESObject();
}

template <typename T>
ESFunctionObject* GlobalObject::installTypedArray(escargot::ESString* ta_name)
{
    escargot::ESObject* ta_prototype = escargot::ESTypedArrayObject<T>::create();
    ta_prototype->forceNonVectorHiddenClass(true);
    ta_prototype->set__proto__(m_objectPrototype);

    // $22.2.1.1~
    escargot::ESFunctionObject* ta_constructor = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // if NewTarget is undefined, throw a TypeError
        if (!instance->currentExecutionContext()->isNewExpression())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->TypedArray, false, strings->emptyString, errorMessage_GlobalObject_NotExistNewInTypedArrayConstructor);
        ASSERT(instance->currentExecutionContext()->resolveThisBindingToObject()->isESTypedArrayObject());
        escargot::ESTypedArrayObject<T>* obj = instance->currentExecutionContext()->resolveThisBindingToObject()->asESTypedArrayObject<T>();
        int len = instance->currentExecutionContext()->argumentCount();
        if (len == 0) {
            obj->allocateTypedArray(0);
        } else if (len >= 1) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            if (!val.isObject()) {
                // $22.2.1.2 %TypedArray%(length)
                int numlen = val.toNumber();
                int elemlen = val.toLength();
                if (numlen != elemlen)
                    throwBuiltinError(instance, ErrorCode::RangeError, strings->TypedArray, false, strings->emptyString, errorMessage_GlobalObject_FirstArgumentInvalidLength);
                obj->allocateTypedArray(elemlen);
            } else if (val.isESPointer() && val.asESPointer()->isESArrayBufferObject()) {
                // $22.2.1.5 %TypedArray%(buffer [, byteOffset [, length] ] )
                unsigned elementSize = obj->elementSize();
                int offset = 0;
                ESValue lenVal;
                if (len >= 2)
                    offset = instance->currentExecutionContext()->arguments()[1].toInt32();
                if (offset < 0) {
                    throwBuiltinError(instance, ErrorCode::RangeError, strings->TypedArray, false, strings->emptyString, errorMessage_GlobalObject_InvalidArrayBufferOffset);
                }
                if (offset % elementSize != 0) {
                    throwBuiltinError(instance, ErrorCode::RangeError, strings->TypedArray, false, strings->emptyString, errorMessage_GlobalObject_InvalidArrayBufferOffset);
                }
                escargot::ESArrayBufferObject* buffer = val.asESPointer()->asESArrayBufferObject();
                unsigned bufferByteLength = buffer->bytelength();
                if (len >= 3) {
                    lenVal = instance->currentExecutionContext()->arguments()[2];
                }
                unsigned newByteLength;
                if (lenVal.isUndefined()) {
                    if (bufferByteLength % elementSize != 0)
                        throwBuiltinError(instance, ErrorCode::RangeError, strings->TypedArray, false, strings->emptyString, errorMessage_GlobalObject_InvalidArrayBufferOffset);
                    newByteLength = bufferByteLength - offset;
                    if (newByteLength < 0)
                        throwBuiltinError(instance, ErrorCode::RangeError, strings->TypedArray, false, strings->emptyString, errorMessage_GlobalObject_InvalidArrayBufferOffset);
                } else {
                    int length = lenVal.toLength();
                    newByteLength = length * elementSize;
                    if (offset + newByteLength > bufferByteLength)
                        throwBuiltinError(instance, ErrorCode::RangeError, strings->TypedArray, false, strings->emptyString, errorMessage_GlobalObject_InvalidArrayBufferOffset);
                }
                obj->setBuffer(buffer);
                obj->setBytelength(newByteLength);
                obj->setByteoffset(offset);
                obj->setArraylength(newByteLength / elementSize);
            } else if (val.isESPointer() && val.asESPointer()->isESObject()) {
                // TODO implement 22.2.1.4
                ESObject* inputObj = val.asESPointer()->asESObject();
                uint32_t length = inputObj->length();
                ASSERT(length >= 0);
                unsigned elementSize = obj->elementSize();
                escargot::ESArrayBufferObject *buffer = ESArrayBufferObject::createAndAllocate(length * elementSize);
                obj->setBuffer(buffer);
                obj->setBytelength(length * elementSize);
                obj->setByteoffset(0);
                obj->setArraylength(length);
                for (uint32_t i = 0; i < length ; i ++) {
                    obj->set(i, inputObj->get(ESValue(i)));
                }
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
            // TODO
            ASSERT(obj->arraylength() < 210000000);
        }
        return obj;
    }, ta_name, 3, true);

    ta_constructor->forceNonVectorHiddenClass(true);
    ta_constructor->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    // $22.2.3.2
    ta_prototype->defineAccessorProperty(strings->byteLength, [](ESObject* self, ESObject* originalObj, ::escargot::ESString* propertyName) -> ESValue {
        // FIXME find right object from originalObj
        return ESValue(originalObj->asESTypedArrayObject<T>()->bytelength());
    }, nullptr, true, false, false);
    // $22.2.3.2
    ta_prototype->defineAccessorProperty(strings->length, [](ESObject* self, ESObject* originalObj, ::escargot::ESString* propertyName) -> ESValue {
        // FIXME find right object from originalObj
        return ESValue(originalObj->asESTypedArrayObject<T>()->arraylength());
    }, nullptr, true, false, false);

    // TODO add reference
    ta_prototype->defineAccessorProperty(strings->buffer, [](ESObject* self, ESObject* originalObj, ::escargot::ESString* propertyName) -> ESValue {
        // FIXME find right object from originalObj
        return ESValue(originalObj->asESTypedArrayObject<T>()->buffer());
    }, nullptr, true, false, false);
    // $22.2.3.22 %TypedArray%.prototype.set(overloaded[, offset])
    ta_prototype->ESObject::defineDataProperty(strings->set, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        RESOLVE_THIS_BINDING_TO_OBJECT(thisBinded, TypedArray, set);
        if (!thisBinded->isESTypedArrayObject() || arglen < 1) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->TypedArray, true, strings->set, errorMessage_GlobalObject_ThisNotTypedArrayObject);
        }
        auto wrapper = thisBinded->asESTypedArrayObjectWrapper();
        int offset = 0;
        if (arglen >= 2)
            offset = instance->currentExecutionContext()->arguments()[1].toInt32();
        if (offset < 0)
            throwBuiltinError(instance, ErrorCode::TypeError, strings->TypedArray, true, strings->set, "");
        auto arg0 = instance->currentExecutionContext()->readArgument(0).asESPointer();
        escargot::ESArrayBufferObject* targetBuffer = wrapper->buffer();
        unsigned targetLength = wrapper->arraylength();
        int targetByteOffset = wrapper->byteoffset();
        int targetElementSize = wrapper->elementSize();
        if (!arg0->isESTypedArrayObject()) {
            ESObject* src = arg0->asESObject();
            uint32_t srcLength = (uint32_t)src->get(strings->length.string()).asInt32();
            if (srcLength + (uint32_t)offset > targetLength)
                throwBuiltinError(instance, ErrorCode::RangeError, strings->TypedArray, true, strings->set, "");

            int targetByteIndex = offset * targetElementSize + targetByteOffset;
            int k = 0;
            int limit = targetByteIndex + targetElementSize * srcLength;

            while (targetByteIndex < limit) {
                escargot::ESString* Pk = ESString::create(k);
                double kNumber = src->get(Pk).toNumber();
                wrapper->set(targetByteIndex / targetElementSize, ESValue(kNumber));
                k++;
                targetByteIndex += targetElementSize;
            }
            return ESValue();
        } else {
            auto arg0Wrapper = arg0->asESTypedArrayObjectWrapper();
            escargot::ESArrayBufferObject* srcBuffer = arg0Wrapper->buffer();
            unsigned srcLength = arg0Wrapper->arraylength();
            int srcByteOffset = arg0Wrapper->byteoffset();
            if (srcLength + (unsigned)offset > targetLength)
                throwBuiltinError(instance, ErrorCode::RangeError, strings->TypedArray, true, strings->set, "");
            int srcByteIndex = 0;
            if (srcBuffer == targetBuffer) {
                // TODO: 24) should clone targetBuffer
                RELEASE_ASSERT_NOT_REACHED();
            } else {
                srcByteIndex = srcByteOffset;
            }
            unsigned targetIndex = (unsigned)offset, srcIndex = 0;
            unsigned targetByteIndex = offset * targetElementSize + targetByteOffset;
            unsigned limit = targetByteIndex + targetElementSize * srcLength;
            if (wrapper->arraytype() != arg0Wrapper->arraytype()) {
                while (targetIndex < offset + srcLength) {
                    ESValue value = arg0Wrapper->get(srcIndex);
                    wrapper->set(targetIndex, value);
                    srcIndex++;
                    targetIndex++;
                }
            } else {
                while (targetByteIndex < limit) {
                    ESValue value = srcBuffer->getValueFromBuffer<uint8_t>(srcByteIndex, escargot::Uint8Array);
                    targetBuffer->setValueInBuffer<Uint8Adaptor>(targetByteIndex, escargot::Uint8Array, value);
                    srcByteIndex++;
                    targetByteIndex++;
                }
            }
            return ESValue();
        }
    }, strings->set));
    // $22.2.3.26 %TypedArray%.prototype.subarray([begin [, end]])
    ta_prototype->ESObject::defineDataProperty(strings->subarray, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        size_t arglen = instance->currentExecutionContext()->argumentCount();
        RESOLVE_THIS_BINDING_TO_OBJECT(thisBinded, TypedArray, subarray);
        if (!thisBinded->isESTypedArrayObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->TypedArray, true, strings->subarray, errorMessage_GlobalObject_ThisNotTypedArrayObject);
        auto wrapper = thisBinded->asESTypedArrayObjectWrapper();
        escargot::ESArrayBufferObject* buffer = wrapper->buffer();
        unsigned srcLength = wrapper->arraylength();
        int relativeBegin = 0;
        unsigned beginIndex;
        if (arglen >= 1)
            relativeBegin = instance->currentExecutionContext()->arguments()[0].toInt32();
        if (relativeBegin < 0)
            beginIndex = (srcLength + relativeBegin) > 0 ? (srcLength + relativeBegin) : 0;
        else
            beginIndex = (unsigned) relativeBegin < srcLength ? relativeBegin : srcLength;
        unsigned relativeEnd = srcLength, endIndex;
        if (arglen >= 2)
            relativeEnd = instance->currentExecutionContext()->arguments()[1].toInt32();
        if (relativeEnd < 0)
            endIndex = (srcLength + relativeEnd) > 0 ? (srcLength + relativeEnd) : 0;
        else
            endIndex = relativeEnd < srcLength ? relativeEnd : srcLength;
        unsigned newLength = 0;
        if (endIndex - beginIndex > 0)
            newLength = endIndex - beginIndex;
        int srcByteOffset = wrapper->byteoffset();

        ESValue arg[3] = {buffer, ESValue(srcByteOffset + beginIndex * wrapper->elementSize()), ESValue(newLength)};
        escargot::ESTypedArrayObject<T>* newobj = escargot::ESTypedArrayObject<T>::create();
        ESValue ret = ESFunctionObject::call(instance, thisBinded->get(strings->constructor.string()), newobj, arg, 3, instance);
        return ret;
    }, strings->subarray));

    ta_constructor->set__proto__(m_functionPrototype); // empty Function
    ta_constructor->setProtoType(ta_prototype);
    ta_prototype->set__proto__(m_objectPrototype);
    ta_prototype->defineDataProperty(strings->constructor, true, false, true, ta_constructor);
    defineDataProperty(ta_name, true, false, true, ta_constructor);
    return ta_constructor;
}

void GlobalObject::installPromise()
{
    m_promisePrototype = ESPromiseObject::create(nullptr);
    m_promisePrototype->forceNonVectorHiddenClass(true);
    m_promisePrototype->set__proto__(m_objectPrototype);
    m_promisePrototype->defineDataProperty(strings->constructor, true, false, true, m_promise);

    // $25.4.3 Promise(executor)
    m_promise = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESPromiseObject* promise;
        if (instance->currentExecutionContext()->isNewExpression()) {
            ESValue executor = instance->currentExecutionContext()->readArgument(0);
            if (!executor.isFunction())
                throwBuiltinError(instance, ErrorCode::TypeError, strings->Promise, false, strings->emptyString, "%s: Promise executor is not a function object");
            promise = instance->currentExecutionContext()->resolveThisBindingToObject()->asESPromiseObject();
            promise->setExecutor(executor.asFunction());
        } else {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Promise, false, strings->emptyString, "%s: Promise constructor should be called with new Promise()");
            RELEASE_ASSERT_NOT_REACHED();
        }

        escargot::ESFunctionObject* promiseResolveFunction = nullptr;
        escargot::ESFunctionObject* promiseRejectFunction = nullptr;
        promise->createResolvingFunctions(instance, promiseResolveFunction, promiseRejectFunction);

        ESValue arguments[] = { promiseResolveFunction, promiseRejectFunction };
        std::jmp_buf tryPosition;
        if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
            ESFunctionObject::call(instance, promise->executor(), ESValue(), arguments, 2, false);
            instance->unregisterTryPos(&tryPosition);
            instance->unregisterCheckedObjectAll();
            return promise;
        } else {
            escargot::ESValue err = instance->getCatchedError();
            escargot::ESObject* internalSlot = promiseResolveFunction->internalSlot();
            if (internalSlot->get(strings->alreadyResolved.string()).asBoolean())
                return promise;
            // ESCARGOT_LOG_INFO("executor run fail with err %s\n", err.toString()->utf8Data());
            ESValue reason[] = { err };
            ESFunctionObject::call(instance, promiseRejectFunction, ESValue(), reason, 1, false);
            return ESValue();
        }
    }, strings->Promise, 1, true);
    m_promise->forceNonVectorHiddenClass(true);
    m_promise->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    m_promisePrototype->defineDataProperty(strings->constructor, true, false, true, m_promise);

    // $25.4.1.3.2 Internal Promise Resolve Function
    m_promiseResolveFunction = [](ESVMInstance* instance) -> ESValue {
        escargot::ESFunctionObject* callee = instance->currentExecutionContext()->resolveCallee().asFunction();
        escargot::ESObject* internalSlot = callee->internalSlot();
        escargot::ESPromiseObject* promise = internalSlot->get(strings->Promise.string()).asObject()->asESPromiseObject();
        /*
        ESCARGOT_LOG_INFO("[Promise %p] Internal promise resolve function %p with arg %s : %s\n",
            promise, callee, instance->currentExecutionContext()->readArgument(0).toString()->utf8Data(),
            data->alreadyResolved() ? "DONE" : "RESOLVE");
        */
        if (internalSlot->get(strings->alreadyResolved.string()).asBoolean())
            return ESValue();
        internalSlot->set(strings->alreadyResolved.string(), ESValue(true));

        ESValue resolutionValue = instance->currentExecutionContext()->readArgument(0);
        if (resolutionValue == ESValue(promise)) {
            promise->rejectPromise(instance, escargot::TypeError::create(escargot::ESString::create("Self resolution error")));
            return ESValue();
        }

        if (!resolutionValue.isObject()) {
            promise->fulfillPromise(instance, resolutionValue);
            return ESValue();
        }
        escargot::ESObject* resolution = resolutionValue.asObject();

        ESValue then;
        std::jmp_buf tryPosition;
        if (setjmp(instance->registerTryPos(&tryPosition)) == 0) {
            then = resolution->get(strings->then.string());
            instance->unregisterTryPos(&tryPosition);
            instance->unregisterCheckedObjectAll();
        } else {
            escargot::ESValue err = instance->getCatchedError();
            promise->rejectPromise(instance, err);
            return ESValue();
        }

        if (then.isFunction()) {
            instance->jobQueue()->enqueueJob(PromiseResolveThenableJob::create(promise, resolution, then.asFunction()));
        } else {
            promise->fulfillPromise(instance, resolution);
            return ESValue();
        }

        return ESValue();
    };

    // $25.4.1.3.1 Internal Promise Reject Function
    m_promiseRejectFunction = [](ESVMInstance* instance) -> ESValue {
        escargot::ESFunctionObject* callee = instance->currentExecutionContext()->resolveCallee().asFunction();
        escargot::ESObject* internalSlot = callee->internalSlot();
        escargot::ESPromiseObject* promise = internalSlot->get(strings->Promise.string()).asObject()->asESPromiseObject();
        /*
        ESCARGOT_LOG_INFO("[Promise %p] Internal promise reject function %p with arg %s : %s\n",
            promise, callee, instance->currentExecutionContext()->readArgument(0).toString()->utf8Data(),
            data->alreadyResolved() ? "DONE" : "REJECT");
        */
        if (internalSlot->get(strings->alreadyResolved.string()).asBoolean())
            return ESValue();
        internalSlot->set(strings->alreadyResolved.string(), ESValue(true));

        promise->rejectPromise(instance, instance->currentExecutionContext()->readArgument(0));
        return ESValue();
    };

    // $25.4.4.1 Promise.all(iterable)
    m_promise->defineDataProperty(strings->all, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue iterableValue = instance->currentExecutionContext()->readArgument(0);
        if (!iterableValue.isObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Promise, false, strings->all, "%s: Promise.all takes iterable object");
        escargot::ESObject* iterable = iterableValue.asObject();

        escargot::ESValue arguments[] = { instance->globalObject()->functionPrototype() };
        escargot::ESPromiseObject* newPromise = newOperation(instance, instance->globalObject(), instance->globalObject()->promise(), arguments, 1).asObject()->asESPromiseObject();

        arguments[0] = iterable;
        escargot::ESFunctionObject::call(instance, newPromise->capability().m_resolveFunction, ESValue(), arguments, 1, false);
        return newPromise;
    }, strings->all, 1));

    // $25.4.4.3 Promise.race(iterable)
    m_promise->defineDataProperty(strings->race, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        return ESValue();
    }, strings->race, 1));

    // $25.4.4.4 Promise.reject(r)
    m_promise->defineDataProperty(strings->reject, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
        ESValue r = instance->currentExecutionContext()->readArgument(0);

        escargot::ESValue arguments[] = { instance->globalObject()->functionPrototype() };
        escargot::ESPromiseObject* newPromise = newOperation(instance, instance->globalObject(), instance->globalObject()->promise(), arguments, 1).asObject()->asESPromiseObject();

        arguments[0] = r;
        escargot::ESFunctionObject::call(instance, newPromise->capability().m_rejectFunction, ESValue(), arguments, 1, false);
        return newPromise;
    }, strings->reject, 1));

    // $25.4.4.5 Promise.resolve(x)
    m_promise->defineDataProperty(strings->resolve, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
        ESValue x = instance->currentExecutionContext()->readArgument(0);
        if (x.isObject() && x.asObject()->isESPromiseObject()) {
            if (x.asObject()->get(strings->constructor.string()) == ESValue(thisObject))
                return x;
        }
        escargot::ESValue arguments[] = { instance->globalObject()->functionPrototype() };
        escargot::ESPromiseObject* newPromise = newOperation(instance, instance->globalObject(), instance->globalObject()->promise(), arguments, 1).asObject()->asESPromiseObject();

        arguments[0] = x;
        escargot::ESFunctionObject::call(instance, newPromise->capability().m_resolveFunction, ESValue(), arguments, 1, false);
        return newPromise;
    }, strings->resolve, 1));

    // $25.4.5.1 Promise.prototype.catch(onRejected)
    m_promisePrototype->defineDataProperty(strings->stringCatch, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        if (!thisValue.isESPointer() || !thisValue.asESPointer()->isESPromiseObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Promise, false, strings->emptyString, "%s: not a Promise object");

        escargot::ESValue onRejected = instance->currentExecutionContext()->readArgument(0);
        escargot::ESValue then = thisValue.asObject()->get(strings->then.string());
        escargot::ESValue arguments[] = { ESValue(), onRejected };
        return escargot::ESFunctionObject::call(instance, then, thisValue, arguments, 2, false);
    }, strings->stringCatch, 1));

    // $25.4.5.1 Promise.prototype.then(onFulfilled, onRejected)
    m_promisePrototype->defineDataProperty(strings->then, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        if (!thisValue.isESPointer() || !thisValue.asESPointer()->isESPromiseObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Promise, false, strings->emptyString, "%s: not a Promise object");
        escargot::ESPromiseObject* promise = thisValue.asESPointer()->asESPromiseObject();

        ESValue onFulfilledValue = instance->currentExecutionContext()->readArgument(0);
        ESValue onRejectedValue = instance->currentExecutionContext()->readArgument(1);

        escargot::ESFunctionObject* onFulfilled = onFulfilledValue.isFunction() ? onFulfilledValue.asFunction() : (escargot::ESFunctionObject*)(1);
        escargot::ESFunctionObject* onRejected = onRejectedValue.isFunction() ? onRejectedValue.asFunction() : (escargot::ESFunctionObject*)(2);

        escargot::ESValue arguments[] = { instance->globalObject()->functionPrototype() };
        escargot::ESPromiseObject* newPromise = newOperation(instance, instance->globalObject(), instance->globalObject()->promise(), arguments, 1).asObject()->asESPromiseObject();

        switch (promise->state()) {
        case ESPromiseObject::PromiseState::Pending:
            {
                // ESCARGOT_LOG_INFO("then: Pending case\n");
                promise->appendReaction(onFulfilled, onRejected, newPromise->capability());
                break;
            }
        case ESPromiseObject::PromiseState::FulFilled:
            {
                // ESCARGOT_LOG_INFO("then: FulFilled case\n");
                Job* job = PromiseReactionJob::create(PromiseReaction(onFulfilled, newPromise->capability()), promise->promiseResult());
                instance->jobQueue()->enqueueJob(job);
                break;
            }
        case ESPromiseObject::PromiseState::Rejected:
            {
                // ESCARGOT_LOG_INFO("then: Rejected case\n");
                Job* job = PromiseReactionJob::create(PromiseReaction(onRejected, newPromise->capability()), promise->promiseResult());
                instance->jobQueue()->enqueueJob(job);
                break;
            }
        default:
            break;
        }
        return newPromise;
    }, strings->then, 2));

    m_promise->set__proto__(m_functionPrototype); // empty Function
    m_promise->setProtoType(m_promisePrototype);
    defineDataProperty(strings->Promise, true, false, true, m_promise);
}

#endif

void GlobalObject::registerCodeBlock(CodeBlock* cb)
{
    m_codeBlocks.push_back(cb);
}

void GlobalObject::unregisterCodeBlock(CodeBlock* cb)
{
    auto iter = std::find(m_codeBlocks.begin(), m_codeBlocks.end(), cb);
    if (iter != m_codeBlocks.end())
        m_codeBlocks.erase(iter);
}

void GlobalObject::propertyDeleted(size_t deletedIdx)
{
    for (unsigned i = 0; i < m_codeBlocks.size() ; i ++) {
        if (m_codeBlocks[i]->m_isBuiltInFunction)
            continue;
        iterateByteCode(m_codeBlocks[i], [&deletedIdx](CodeBlock* block, unsigned idx, ByteCode* code, Opcode opcode) {
            switch (opcode) {
            case GetByGlobalIndexOpcode:
                {
                    if (((GetByGlobalIndex *)code)->m_index == deletedIdx) {
                        ((GetByGlobalIndex *)code)->m_index = SIZE_MAX;
                    }
                    break;
                }
            case SetByGlobalIndexOpcode:
                {
                    if (((SetByGlobalIndex *)code)->m_index == deletedIdx) {
                        ((SetByGlobalIndex *)code)->m_index = SIZE_MAX;
                    }
                    break;
                }
            default: { }
            }
        });
    }
}

void GlobalObject::propertyDefined(size_t newIndex, escargot::ESString* name)
{
    bool isRedefined = false;
    for (size_t i = 0; i < m_hiddenClass->propertyCount(); i ++) {
        if (m_hiddenClass->propertyInfo(i).isDeleted() && *m_hiddenClass->propertyInfo(i).name() == *name) {
            isRedefined = true;
            break;
        }
    }

    if (isRedefined) {
        for (unsigned i = 0; i < m_codeBlocks.size() ; i ++) {
            if (m_codeBlocks[i]->m_isBuiltInFunction)
                continue;
            iterateByteCode(m_codeBlocks[i], [name, newIndex](CodeBlock* block, unsigned idx, ByteCode* code, Opcode opcode) {
                switch (opcode) {
                case GetByGlobalIndexOpcode:
                    {
                        if (*name == *((GetByGlobalIndex *)code)->m_name) {
                            ((GetByGlobalIndex *)code)->m_index = newIndex;
                        }
                        break;
                    }
                case SetByGlobalIndexOpcode:
                    {
                        if (*name == *((SetByGlobalIndex *)code)->m_name) {
                            ((SetByGlobalIndex *)code)->m_index = newIndex;
                        }
                        break;
                    }
                default: { }
                }
            });
        }
    }
}

void GlobalObject::somePrototypeObjectDefineIndexedProperty()
{
    if (!m_didSomePrototypeObjectDefineIndexedProperty) {
#ifndef NDEBUG
        fprintf(stderr, "some prototype object define indexed property.....\n");
#endif
        m_didSomePrototypeObjectDefineIndexedProperty = true;
        for (unsigned i = 0; i < m_codeBlocks.size() ; i ++) {
            // printf("%p..\n", m_codeBlocks[i]);
            if (m_codeBlocks[i]->m_isBuiltInFunction)
                continue;
            iterateByteCode(m_codeBlocks[i], [](CodeBlock* block, unsigned idx, ByteCode* code, Opcode opcode) {
                switch (opcode) {
                case GetObjectPreComputedCaseOpcode:
                    {
                        GetObjectPreComputedCaseSlowMode n(((GetObjectPreComputedCase *)code)->m_propertyValue);
                        n.assignOpcodeInAddress();
#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
                        block->m_extraData[idx].m_opcode = GetObjectPreComputedCaseSlowModeOpcode;
#endif
                        memcpy(code, &n, sizeof(GetObjectPreComputedCaseSlowMode));
                        break;
                    }
                case GetObjectPreComputedCaseAndPushObjectOpcode:
                    {
                        GetObjectPreComputedCaseAndPushObjectSlowMode n(((GetObjectPreComputedCaseAndPushObject *)code)->m_propertyValue);
                        n.assignOpcodeInAddress();
#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
                        block->m_extraData[idx].m_opcode = GetObjectPreComputedCaseAndPushObjectSlowModeOpcode;
#endif
                        memcpy(code, &n, sizeof(GetObjectPreComputedCaseAndPushObjectSlowMode));
                        break;
                    }
                case GetObjectPreComputedCaseWithPeekingOpcode:
                    {
                        GetObjectPreComputedCaseWithPeekingSlowMode n(((GetObjectPreComputedCaseWithPeeking *)code)->m_propertyValue);
                        n.assignOpcodeInAddress();
#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
                        block->m_extraData[idx].m_opcode = GetObjectPreComputedCaseWithPeekingSlowModeOpcode;
#endif
                        memcpy(code, &n, sizeof(GetObjectPreComputedCaseWithPeekingSlowMode));
                        break;
                    }
                case SetObjectPreComputedCaseOpcode:
                    {
                        SetObjectPreComputedCaseSlowMode n(((SetObjectPreComputedCase *)code)->m_propertyValue);
                        n.assignOpcodeInAddress();
#if defined(ENABLE_ESJIT) || !defined(NDEBUG)
                        block->m_extraData[idx].m_opcode = SetObjectPreComputedCaseSlowModeOpcode;
#endif
                        memcpy(code, &n, sizeof(SetObjectPreComputedCaseSlowMode));
                        break;
                    }
                default: { }
                }
            });
        }
    }
}

}
