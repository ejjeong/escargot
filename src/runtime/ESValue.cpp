#include "Escargot.h"
#include "ESValue.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#include "ast/AST.h"
#include "jit/ESJIT.h"
#include "bytecode/ByteCode.h"

#include "Yarr.h"

#include "fast-dtoa.h"
#include "bignum-dtoa.h"

namespace escargot {

ESHiddenClassPropertyInfo dummyPropertyInfo(nullptr, true, false, false, false);

ESValue ESValue::toPrimitiveSlowCase(PrimitiveTypeHint preferredType) const
{
    ASSERT(!isPrimitive());
    ESObject* obj = asESPointer()->asESObject();
    if (preferredType == PrimitiveTypeHint::PreferString) {
        ESValue toString = obj->get(ESValue(strings->toString.string()));
        if (toString.isESPointer() && toString.asESPointer()->isESFunctionObject()) {
            ESValue str = ESFunctionObject::call(ESVMInstance::currentInstance(), toString, obj, NULL, 0, false);
            if (str.isPrimitive())
                return str;
        }

        ESValue valueOf = obj->get(ESValue(strings->valueOf.string()));
        if (valueOf.isESPointer() && valueOf.asESPointer()->isESFunctionObject()) {
            ESValue val = ESFunctionObject::call(ESVMInstance::currentInstance(), valueOf, obj, NULL, 0, false);
            if (val.isPrimitive())
                return val;
        }
    } else { // preferNumber
        ESValue valueOf = obj->get(ESValue(strings->valueOf.string()));
        if (valueOf.isESPointer() && valueOf.asESPointer()->isESFunctionObject()) {
            ESValue val = ESFunctionObject::call(ESVMInstance::currentInstance(), valueOf, obj, NULL, 0, false);
            if (val.isPrimitive())
                return val;
        }

        ESValue toString = obj->get(ESValue(strings->toString.string()));
        if (toString.isESPointer() && toString.asESPointer()->isESFunctionObject()) {
            ESValue str = ESFunctionObject::call(ESVMInstance::currentInstance(), toString, obj, NULL, 0, false);
            if (str.isPrimitive())
                return str;
        }
    }
    ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create()));
    RELEASE_ASSERT_NOT_REACHED();
}

ESString* ESValue::toStringSlowCase() const
{
    ASSERT(!isESString());
    if (isInt32()) {
        int num = asInt32();
        if (num >= 0 && num < ESCARGOT_STRINGS_NUMBERS_MAX)
            return strings->numbers[num].string();
        return ESString::create(num);
    } else if (isNumber()) {
        double d = asNumber();
        if (std::isnan(d))
            return strings->NaN.string();
        if (std::isinf(d)) {
            if (std::signbit(d))
                return strings->NegativeInfinity.string();
            else
                return strings->Infinity.string();
        }
        // convert -0.0 into 0.0
        // in c++, d = -0.0, d == 0.0 is true
        if (d == 0.0)
            d = 0;

        return ESString::create(d);
    } else if (isUndefined()) {
        return strings->undefined.string();
    } else if (isNull()) {
        return strings->null.string();
    } else if (isBoolean()) {
        if (asBoolean())
            return strings->stringTrue.string();
        else
            return strings->stringFalse.string();
    } else {
        return toPrimitive(PreferString).toString();
    }
}

bool ESValue::abstractEqualsToSlowCase(const ESValue& val)
{
    if (isNumber() && val.isNumber()) {
        double a = asNumber();
        double b = val.asNumber();

        if (std::isnan(a) || std::isnan(b))
            return false;
        else if (a == b)
            return true;

        return false;
    } else {
        if (isUndefinedOrNull() && val.isUndefinedOrNull())
            return true;

        if (isNumber() && val.isESString()) {
            // If Type(x) is Number and Type(y) is String,
            // return the result of the comparison x == ToNumber(y).
            return asNumber() == val.toNumber();
        } else if (isESString() && val.isNumber()) {
            // If Type(x) is String and Type(y) is Number,
            // return the result of the comparison ToNumber(x) == y.
            return val.asNumber() == toNumber();
        } else if (isBoolean()) {
            // If Type(x) is Boolean, return the result of the comparison ToNumber(x) == y.
            // return the result of the comparison ToNumber(x) == y.
            ESValue x(toNumber());
            return x.abstractEqualsTo(val);
        } else if (val.isBoolean()) {
            // If Type(y) is Boolean, return the result of the comparison x == ToNumber(y).
            // return the result of the comparison ToNumber(x) == y.
            return abstractEqualsTo(ESValue(val.toNumber()));
        } else if ((isESString() || isNumber()) && val.isObject()) {
            // If Type(x) is either String, Number, or Symbol and Type(y) is Object, then
            return abstractEqualsTo(val.toPrimitive());
        } else if (isObject() && (val.isESString() || val.isNumber())) {
            // If Type(x) is Object and Type(y) is either String, Number, or Symbol, then
            return toPrimitive().abstractEqualsTo(val);
        }

        if (isESPointer() && val.isESPointer()) {
            ESPointer* o = asESPointer();
            ESPointer* comp = val.asESPointer();

            if (o->isESString() && comp->isESString())
                return *o->asESString() == *comp->asESString();
            return equalsTo(val);
        }
    }
    return false;
}

enum Flags {
    NO_FLAGS = 0,
    EMIT_POSITIVE_EXPONENT_SIGN = 1,
    EMIT_TRAILING_DECIMAL_POINT = 2,
    EMIT_TRAILING_ZERO_AFTER_POINT = 4,
    UNIQUE_ZERO = 8
};

void CreateDecimalRepresentation(
    int flags_,
    const char* decimal_digits,
    int length,
    int decimal_point,
    int digits_after_point,
    double_conversion::StringBuilder* result_builder) {
    // Create a representation that is padded with zeros if needed.
    if (decimal_point <= 0) {
        // "0.00000decimal_rep".
        result_builder->AddCharacter('0');
        if (digits_after_point > 0) {
            result_builder->AddCharacter('.');
            result_builder->AddPadding('0', -decimal_point);
            ASSERT(length <= digits_after_point - (-decimal_point));
            result_builder->AddSubstring(decimal_digits, length);
            int remaining_digits = digits_after_point - (-decimal_point) - length;
            result_builder->AddPadding('0', remaining_digits);
        }
    } else if (decimal_point >= length) {
        // "decimal_rep0000.00000" or "decimal_rep.0000"
        result_builder->AddSubstring(decimal_digits, length);
        result_builder->AddPadding('0', decimal_point - length);
        if (digits_after_point > 0) {
            result_builder->AddCharacter('.');
            result_builder->AddPadding('0', digits_after_point);
        }
    } else {
        // "decima.l_rep000"
        ASSERT(digits_after_point > 0);
        result_builder->AddSubstring(decimal_digits, decimal_point);
        result_builder->AddCharacter('.');
        ASSERT(length - decimal_point <= digits_after_point);
        result_builder->AddSubstring(&decimal_digits[decimal_point],
        length - decimal_point);
        int remaining_digits = digits_after_point - (length - decimal_point);
        result_builder->AddPadding('0', remaining_digits);
    }
    if (digits_after_point == 0) {
        if ((flags_ & EMIT_TRAILING_DECIMAL_POINT) != 0) {
            result_builder->AddCharacter('.');
        }
        if ((flags_ & EMIT_TRAILING_ZERO_AFTER_POINT) != 0) {
            result_builder->AddCharacter('0');
        }
    }
}

void CreateExponentialRepresentation(
    int flags_,
    const char* decimal_digits,
    int length,
    int exponent,
    double_conversion::StringBuilder* result_builder) {
    ASSERT(length != 0);
    result_builder->AddCharacter(decimal_digits[0]);
    if (length != 1) {
        result_builder->AddCharacter('.');
        result_builder->AddSubstring(&decimal_digits[1], length-1);
    }
    result_builder->AddCharacter('e');
    if (exponent < 0) {
        result_builder->AddCharacter('-');
        exponent = -exponent;
    } else {
        if ((flags_ & EMIT_POSITIVE_EXPONENT_SIGN) != 0) {
            result_builder->AddCharacter('+');
        }
    }
    if (exponent == 0) {
        result_builder->AddCharacter('0');
        return;
    }
    ASSERT(exponent < 1e4);
    const int kMaxExponentLength = 5;
    char buffer[kMaxExponentLength + 1];
    buffer[kMaxExponentLength] = '\0';
    int first_char_pos = kMaxExponentLength;
    while (exponent > 0) {
        buffer[--first_char_pos] = '0' + (exponent % 10);
        exponent /= 10;
    }
    result_builder->AddSubstring(&buffer[first_char_pos],
    kMaxExponentLength - first_char_pos);
}

ESStringDataASCII::ESStringDataASCII(double number)
{
    if (number == 0) {
        append({'0'});
        m_data.m_isASCIIString = true;
        initData();
        return;
    }
    const int flags = UNIQUE_ZERO | EMIT_POSITIVE_EXPONENT_SIGN;
    bool sign = false;
    if (number < 0) {
        sign = true;
        number = -number;
    }
    // The maximal number of digits that are needed to emit a double in base 10.
    // A higher precision can be achieved by using more digits, but the shortest
    // accurate representation of any double will never use more digits than
    // kBase10MaximalLength.
    // Note that DoubleToAscii null-terminates its input. So the given buffer
    // should be at least kBase10MaximalLength + 1 characters long.
    const int kBase10MaximalLength = 17;
    const int kDecimalRepCapacity = kBase10MaximalLength + 1;
    char decimal_rep[kDecimalRepCapacity];
    int decimal_rep_length;
    int decimal_point;
    double_conversion::Vector<char> vector(decimal_rep, kDecimalRepCapacity);
    bool fast_worked = FastDtoa(number, double_conversion::FAST_DTOA_SHORTEST, 0, vector, &decimal_rep_length, &decimal_point);
    if (!fast_worked) {
        BignumDtoa(number, double_conversion::BIGNUM_DTOA_SHORTEST, 0, vector, &decimal_rep_length, &decimal_point);
        vector[decimal_rep_length] = '\0';
    }

    /* reserve(decimal_rep_length + sign ? 1 : 0);
    if (sign)
        operator +=('-');
    for (unsigned i = 0; i < decimal_rep_length; i ++) {
        operator +=(decimal_rep[i]);
    }*/

    const int bufferLength = 128;
    char buffer[bufferLength];
    double_conversion::StringBuilder builder(buffer, bufferLength);

    int exponent = decimal_point - 1;
    const int decimal_in_shortest_low_ = -6;
    const int decimal_in_shortest_high_ = 21;
    if ((decimal_in_shortest_low_ <= exponent)
        && (exponent < decimal_in_shortest_high_)) {
            CreateDecimalRepresentation(flags, decimal_rep, decimal_rep_length,
                decimal_point,
                double_conversion::Max(0, decimal_rep_length - decimal_point),
                &builder);
    } else {
        CreateExponentialRepresentation(flags, decimal_rep, decimal_rep_length, exponent,
            &builder);
    }
    m_data.m_isASCIIString = true;
    ASCIIString* as = (ASCIIString*)asASCIIString();
    if (sign)
        (*as) += '-';
    char* buf = builder.Finalize();
    while (*buf) {
        (*as) += *buf;
        buf++;
    }
    initData();
}

uint32_t ESString::tryToUseAsIndex()
{
    if (stringData()->isASCIIString()) {
        bool allOfCharIsDigit = true;
        uint32_t number = 0;
        size_t len = length();
        const char* data = stringData()->asciiData();
        for (unsigned i = 0; i < len; i ++) {
            char c = data[i];
            if (c < '0' || c > '9') {
                allOfCharIsDigit = false;
                break;
            } else {
                uint32_t cnum = c-'0';
                if (number > (std::numeric_limits<uint32_t>::max() - cnum) / 10)
                    return ESValue::ESInvalidIndexValue;
                number = number*10 + cnum;
            }
        }
        if (allOfCharIsDigit) {
            return number;
        }
    } else {
        bool allOfCharIsDigit = true;
        uint32_t number = 0;
        size_t len = length();
        const char16_t* data = stringData()->utf16Data();
        for (unsigned i = 0; i < len; i ++) {
            char16_t c = data[i];
            if (c < '0' || c > '9') {
                allOfCharIsDigit = false;
                break;
            } else {
                uint32_t cnum = c-'0';
                if (number > (std::numeric_limits<uint32_t>::max() - cnum) / 10)
                    return ESValue::ESInvalidIndexValue;
                number = number*10 + cnum;
            }
        }
        if (allOfCharIsDigit) {
            return number;
        }
    }
    return ESValue::ESInvalidIndexValue;
}

ESString* ESString::substring(int from, int to) const
{
    ASSERT(0 <= from && from <= to && to <= (int)length());
    ensureNormalString();
    if (to - from == 1) {
        char16_t c;
        c = stringData()->charAt(from);
        if (c < ESCARGOT_ASCII_TABLE_MAX) {
            return strings->asciiTable[c].string();
        } else {
            return ESString::create(c);
        }
    }

    if (stringData()->isASCIIString()) {
        return ESString::create(stringData()->asASCIIString()->substr(from, to-from));
    } else {
        return ESString::create(stringData()->asUTF16String()->substr(from, to-from));
    }
}


bool ESString::match(ESPointer* esptr, RegexMatchResult& matchResult, bool testOnly, size_t startIndex) const
{
    // NOTE to build normal string(for rope-string), we should call ensureNormalString();
    ensureNormalString();

    ESRegExpObject::Option option = ESRegExpObject::Option::None;
    const ESString* regexSource;
    JSC::Yarr::BytecodePattern* byteCode = NULL;
    ESString* tmpStr;
    if (esptr->isESRegExpObject()) {
        regexSource = esptr->asESRegExpObject()->source();
        option = esptr->asESRegExpObject()->option();
        byteCode = esptr->asESRegExpObject()->bytecodePattern();
    } else {
        tmpStr = ESValue(esptr).toString();
        regexSource = tmpStr;
    }

    bool isGlobal = option & ESRegExpObject::Option::Global;
    if (!byteCode) {
        JSC::Yarr::ErrorCode yarrError = JSC::Yarr::ErrorCode::NoError;
        JSC::Yarr::YarrPattern* yarrPattern;
        if (esptr->isESRegExpObject() && esptr->asESRegExpObject()->yarrPattern())
            yarrPattern = esptr->asESRegExpObject()->yarrPattern();
        else
            yarrPattern = new JSC::Yarr::YarrPattern(*regexSource, option & ESRegExpObject::Option::IgnoreCase, option & ESRegExpObject::Option::MultiLine, &yarrError);
        if (yarrError) {
            matchResult.m_subPatternNum = 0;
            return false;
        }
        WTF::BumpPointerAllocator *bumpAlloc = ESVMInstance::currentInstance()->bumpPointerAllocator();
        JSC::Yarr::OwnPtr<JSC::Yarr::BytecodePattern> ownedBytecode = JSC::Yarr::byteCompileEscargot(*yarrPattern, bumpAlloc);
        byteCode = ownedBytecode.leakPtr();
        if (esptr->isESRegExpObject()) {
            esptr->asESRegExpObject()->setBytecodePattern(byteCode);
        }
    }

    unsigned subPatternNum = byteCode->m_body->m_numSubpatterns;
    matchResult.m_subPatternNum = (int) subPatternNum;
    size_t length = m_string->length();
    size_t start = startIndex;
    unsigned result = 0;
    const void* chars;
    if (m_string->isASCIIString())
        chars = m_string->asciiData();
    else
        chars = m_string->utf16Data();
    unsigned* outputBuf = (unsigned int*)alloca(sizeof(unsigned) * 2 * (subPatternNum + 1));
    outputBuf[1] = start;
    do {
        start = outputBuf[1];
        memset(outputBuf, -1, sizeof(unsigned) * 2 * (subPatternNum + 1));
        if (start > length)
            break;
        if (m_string->isASCIIString())
            result = JSC::Yarr::interpret(NULL, byteCode, (const char *)chars, length, start, outputBuf);
        else
            result = JSC::Yarr::interpret(NULL, byteCode, (const char16_t *)chars, length, start, outputBuf);
        if (result != JSC::Yarr::offsetNoMatch) {
            if (UNLIKELY(testOnly)) {
                return true;
            }
            std::vector<ESString::RegexMatchResult::RegexMatchResultPiece> piece;
            piece.reserve(subPatternNum + 1);

            for (unsigned i = 0; i < subPatternNum + 1; i ++) {
                ESString::RegexMatchResult::RegexMatchResultPiece p;
                p.m_start = outputBuf[i*2];
                p.m_end = outputBuf[i*2 + 1];
                piece.push_back(p);
            }
            matchResult.m_matchResults.push_back(std::move(piece));
            if (!isGlobal)
                break;
            if (start == outputBuf[1]) {
                outputBuf[1]++;
                if (outputBuf[1] > length) {
                    break;
                }
            }
        } else {
            break;
        }
    } while (result != JSC::Yarr::offsetNoMatch);
    return matchResult.m_matchResults.size();
}

ESObject* ESObject::create(size_t initialKeyCount)
{
    return new ESObject(ESPointer::Type::ESObject, ESVMInstance::currentInstance()->globalObject()->objectPrototype(), initialKeyCount);
}

ESObject::ESObject(ESPointer::Type type, ESValue __proto__, size_t initialKeyCount)
    : ESPointer(type)
{
    m_flags.m_isExtensible = true;
    m_flags.m_isGlobalObject = false;
    m_flags.m_isEverSetAsPrototypeObject = false;

    m_hiddenClassData.reserve(initialKeyCount);
    m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForObject();

    m_hiddenClassData.push_back(ESValue((ESPointer *)ESVMInstance::currentInstance()->object__proto__AccessorData()));

    set__proto__(__proto__);
}

void ESObject::setValueAsProtoType(const ESValue& obj)
{
    if (obj.isObject()) {
        obj.asESPointer()->asESObject()->m_flags.m_isEverSetAsPrototypeObject = true;
        if (obj.asESPointer()->isESArrayObject()) {
            obj.asESPointer()->asESArrayObject()->convertToSlowMode();
        }
        if (obj.asESPointer()->asESObject()->hiddenClass()->hasIndexedProperty()) {
            ESVMInstance::currentInstance()->globalObject()->somePrototypeObjectDefineIndexedProperty();
        }
    }
}

// ES 5.1: 8.12.9
bool ESObject::defineOwnProperty(ESValue& P, ESObject* desc, bool throwFlag)
{
    ESObject* O = this;
    bool isEnumerable = false;
    bool isConfigurable = false;
    bool isWritable = false;

    // ToPropertyDescriptor : (start) we need to seperate this part
    bool descHasEnumerable = desc->hasProperty(strings->enumerable.string());
    bool descHasConfigurable = desc->hasProperty(strings->configurable.string());
    bool descHasWritable = desc->hasProperty(strings->writable.string());
    bool descHasValue = desc->hasProperty(strings->value.string());
    bool descHasGetter = desc->hasProperty(strings->get.string());
    bool descHasSetter = desc->hasProperty(strings->set.string());
    bool descE = desc->get(strings->enumerable.string()).toBoolean();
    bool descC = desc->get(strings->configurable.string()).toBoolean();
    bool descW = desc->get(strings->writable.string()).toBoolean();
    ESValue descV = desc->get(strings->value.string());
    escargot::ESFunctionObject* descGet = NULL;
    escargot::ESFunctionObject* descSet = NULL;
    if (descHasGetter || descHasSetter) {
        if (descHasGetter) {
            ESValue get = desc->get(strings->get.string()); // 8.10.5 ToPropertyDescriptor 7.a
            if (!(get.isESPointer() && get.asESPointer()->isESFunctionObject()) && !get.isUndefined())
                ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("ToPropertyDescriptor 7.b")))); // 8.10.5 ToPropertyDescriptor 7.b
            else if (!get.isUndefined())
                descGet = get.asESPointer()->asESFunctionObject(); // 8.10.5 ToPropertyDescriptor 7.c
        }
        if (descHasSetter) {
            ESValue set = desc->get(strings->set.string()); // 8.10.5 ToPropertyDescriptor 8.a
            if (!(set.isESPointer() && set.asESPointer()->isESFunctionObject()) && !set.isUndefined())
                ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("ToPropertyDescriptor 8.b")))); // 8.10.5 ToPropertyDescriptor 8.b
            else if (!set.isUndefined())
                descSet = set.asESPointer()->asESFunctionObject(); // 8.10.5 ToPropertyDescriptor 8.c
        }
        if (descHasValue || descHasWritable)
            ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Type error, Property cannot have [getter|setter] and [value|writable] together"))));
    }
    // ToPropertyDescriptor : (end)

    // 1
    // Our ESObject::getOwnProperty differs from [[GetOwnProperty]] in Spec
    // Hence, we use OHasCurrent and propertyInfo of current instead of Property Descriptor which is return of [[GetOwnProperty]] here.
    bool OHasCurrent = true;
    size_t idx = O->hiddenClass()->findProperty(P.toString());
    ESValue current = ESValue();
    if (idx != SIZE_MAX)
        current = O->hiddenClass()->read(O, O, idx);
    else {
        if (O->isESArrayObject()
            && (descHasEnumerable || descHasWritable || descHasConfigurable || descHasValue || descHasGetter || descHasSetter)
            && O->hasOwnProperty(P)) {
            current = O->getOwnProperty(P);
            if (descHasGetter || descHasSetter) {
                O->defineAccessorProperty(P, new ESPropertyAccessorData(descGet, descSet),
                    descHasWritable ? descW : true,
                    descHasEnumerable ? descE : true,
                    descHasConfigurable ? descC : true);
            } else {
                O->defineDataProperty(P, descHasWritable ? descW : true,
                    descHasEnumerable ? descE : true,
                    descHasConfigurable ? descC : true, descHasValue ? desc->get(strings->value.string()) : current);
            }
            return true;
        } else {
            current = O->getOwnProperty(P);
            if (current.isUndefined())
                OHasCurrent = false;
        }
    }

    // 2
    bool extensible = O->isExtensible();

    // 3, 4
    bool isDescDataDescriptor = escargot::PropertyDescriptor::IsDataDescriptor(desc);
    bool isDescGenericDescriptor = escargot::PropertyDescriptor::IsGenericDescriptor(desc);
    if (!OHasCurrent) {
        // 3
        if (!extensible) {
            if (throwFlag)
                ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty"))));
            else
                return false;
        } else { // 4
//            O->deleteProperty(P);
            if (isDescDataDescriptor || isDescGenericDescriptor) {
                // Refer to Table 7 of ES 5.1 for default attribute values
                O->defineDataProperty(P, descHasWritable ? descW : isWritable,
                    descHasEnumerable ? descE : isEnumerable,
                    descHasConfigurable ? descC : isConfigurable, descV);
            } else {
                ASSERT(escargot::PropertyDescriptor::IsAccessorDescriptor(desc));
                O->defineAccessorProperty(P, new ESPropertyAccessorData(descGet, descSet),
                    descHasWritable ? descW : isWritable,
                    descHasEnumerable ? descE : isEnumerable,
                    descHasConfigurable ? descC : isConfigurable);
            }
            return true;
        }
    }

    // 5
    if (!descHasEnumerable && !descHasWritable && !descHasConfigurable && !descHasValue && !descHasGetter && !descHasSetter)
        return true;

    // 6
    const ESHiddenClassPropertyInfo& propertyInfo = O->hiddenClass()->propertyInfo(idx);
    if ((!descHasEnumerable || descE == propertyInfo.m_flags.m_isEnumerable)
        && (!descHasWritable || descW == propertyInfo.m_flags.m_isWritable)
        && (!descHasConfigurable || descC == propertyInfo.m_flags.m_isConfigurable)
        && (!descHasValue || ((propertyInfo.m_flags.m_isDataProperty || O->accessorData(idx)->getNativeGetter() || O->accessorData(idx)->getNativeSetter()) && descV.equalsToByTheSameValueAlgorithm(current)))
        && (!descHasGetter || (O->get(strings->get.string()).isESPointer() && O->get(strings->get.string()).asESPointer()->isESFunctionObject()
            && descGet == O->get(strings->get.string()).asESPointer()->asESFunctionObject()))
        && (!descHasSetter || (O->get(strings->set.string()).isESPointer() && O->get(strings->set.string()).asESPointer()->isESFunctionObject()
            && descSet == O->get(strings->set.string()).asESPointer()->asESFunctionObject())))
        return true;

    // 7
    if (!propertyInfo.m_flags.m_isConfigurable) {
        if (descHasConfigurable && descC) {
            if (throwFlag)
                ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty 7.a"))));
            else
                return false;
        } else {
            if (descHasEnumerable && propertyInfo.m_flags.m_isEnumerable != descE) {
                if (throwFlag)
                    ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty 7.b"))));
                else
                    return false;
            }
        }
    }

    // 8, 9, 10, 11
    bool isCurrentDataDescriptor = propertyInfo.m_flags.m_isDataProperty || O->accessorData(idx)->getNativeGetter() || O->accessorData(idx)->getNativeSetter();
    if (isDescGenericDescriptor) { // 8
    } else if (isCurrentDataDescriptor != isDescDataDescriptor) { // 9
        if (!propertyInfo.m_flags.m_isConfigurable) { // 9.a
            if (throwFlag)
                ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Object.DefineOwnProperty 9.a"))));
            else
                return false;
        }
        if (isCurrentDataDescriptor) { // 9.b
            O->deleteProperty(P);
            O->defineAccessorProperty(P, new ESPropertyAccessorData(descGet, descSet), descHasWritable ? descW : false, propertyInfo.m_flags.m_isEnumerable, propertyInfo.m_flags.m_isConfigurable);
        } else { // 9.c
            O->deleteProperty(P);
            O->defineDataProperty(P, descHasWritable ? descW : false, propertyInfo.m_flags.m_isEnumerable, propertyInfo.m_flags.m_isConfigurable, descV);
        }
        return true;
    } else if (isCurrentDataDescriptor && isDescDataDescriptor) { // 10
        if (!propertyInfo.m_flags.m_isConfigurable) {
            if (!propertyInfo.m_flags.m_isWritable) {
                if (descW) {
                    if (throwFlag)
                        ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty 10.a.i"))));
                    else
                        return false;
                } else {
                    if (descHasValue && current != desc->get(strings->value.string())) {
                        if (throwFlag)
                            ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty 10.a.ii"))));
                        else
                            return false;
                    }
                }
            }
        }
    } else {
        ASSERT(!propertyInfo.m_flags.m_isDataProperty && escargot::PropertyDescriptor::IsAccessorDescriptor(desc));
        if (!propertyInfo.m_flags.m_isConfigurable) {
            if (descHasSetter && (descSet != O->accessorData(idx)->getJSSetter())) {
                if (throwFlag)
                    ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty 11.a.i"))));
                else
                    return false;
            }
            if (descHasGetter && (descGet != O->accessorData(idx)->getJSGetter())) {
                if (throwFlag)
                    ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty 11.a.ii"))));
                else
                    return false;
            }
        }
    }

    //

    // 12
    if (descHasGetter || descHasSetter || (!propertyInfo.m_flags.m_isDataProperty && (O->accessorData(idx)->getJSGetter() || O->accessorData(idx)->getJSSetter()))) {
        escargot::ESFunctionObject* getter = descGet;
        escargot::ESFunctionObject* setter = descSet;
        if (!propertyInfo.m_flags.m_isDataProperty) {
            ESPropertyAccessorData* currentAccessorData = O->accessorData(idx);
            if (!descHasGetter && currentAccessorData->getJSGetter()) {
                getter = currentAccessorData->getJSGetter();
            }
            if (!descHasSetter && currentAccessorData->getJSSetter()) {
                setter = currentAccessorData->getJSSetter();
            }
        }
        O->deleteProperty(P, true);
        O->defineAccessorProperty(P, new ESPropertyAccessorData(getter, setter),
            descHasWritable ? descW : propertyInfo.m_flags.m_isWritable,
            descHasEnumerable ? descE: propertyInfo.m_flags.m_isEnumerable,
            descHasConfigurable ? descC : propertyInfo.m_flags.m_isConfigurable);
    } else {
        O->deleteProperty(P, true);
        O->defineDataProperty(P,
            descHasWritable ? descW : propertyInfo.m_flags.m_isWritable,
            descHasEnumerable ? descE: propertyInfo.m_flags.m_isEnumerable,
            descHasConfigurable ? descC : propertyInfo.m_flags.m_isConfigurable,
            descHasValue ? descV : current);
    }

    // 13
    return true;
}

const unsigned ESArrayObject::MAX_FASTMODE_SIZE;

ESArrayObject::ESArrayObject(int length)
    : ESObject((Type)(Type::ESObject | Type::ESArrayObject), ESVMInstance::currentInstance()->globalObject()->arrayPrototype(), 3)
    , m_vector(0)
{
    m_flags.m_isFastMode = true;
    m_length = 0;
    if (length == -1)
        convertToSlowMode();
    else if (length > 0) {
        setLength(length);
    } else {
        m_vector.reserve(6);
    }

    // defineAccessorProperty(strings->length.string(), ESVMInstance::currentInstance()->arrayLengthAccessorData(), true, false, false);
    m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForArrayObject();
    m_hiddenClassData.push_back((ESPointer *)ESVMInstance::currentInstance()->arrayLengthAccessorData());
}

// ES 5.1: 15.4.5.1
bool ESArrayObject::defineOwnProperty(ESValue& P, ESObject* desc, bool throwFlag)
{
    ESArrayObject* A = this;
    ESObject* O = this;

    // ToPropertyDescriptor : (start) we need to seperate this part
    bool descHasEnumerable = desc->hasProperty(strings->enumerable.string());
    bool descHasConfigurable = desc->hasProperty(strings->configurable.string());
    bool descHasWritable = desc->hasProperty(strings->writable.string());
    bool descHasValue = desc->hasProperty(strings->value.string());
    bool descHasGetter = desc->hasProperty(strings->get.string());
    bool descHasSetter = desc->hasProperty(strings->set.string());
    ESValue descE = desc->get(strings->enumerable.string());
    ESValue descC = desc->get(strings->configurable.string());
    ESValue descW = desc->get(strings->writable.string());
    escargot::ESFunctionObject* descGet = NULL;
    escargot::ESFunctionObject* descSet = NULL;
    if (descHasGetter || descHasSetter) {
        if (descHasGetter) {
            ESValue get = desc->get(strings->get.string()); // 8.10.5 ToPropertyDescriptor 7.a
            if (!(get.isESPointer() && get.asESPointer()->isESFunctionObject()) && !get.isUndefined())
                ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("ToPropertyDescriptor 7.b")))); // 8.10.5 ToPropertyDescriptor 7.b
            else if (!get.isUndefined())
                descGet = get.asESPointer()->asESFunctionObject(); // 8.10.5 ToPropertyDescriptor 7.c
        }
        if (descHasSetter) {
            ESValue set = desc->get(strings->set.string()); // 8.10.5 ToPropertyDescriptor 8.a
            if (!(set.isESPointer() && set.asESPointer()->isESFunctionObject()) && !set.isUndefined())
                ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("ToPropertyDescriptor 8.b")))); // 8.10.5 ToPropertyDescriptor 8.b
            else if (!set.isUndefined())
                descSet = set.asESPointer()->asESFunctionObject(); // 8.10.5 ToPropertyDescriptor 8.c
        }
        if (descHasValue || !descW.isUndefined())
            ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Type error, Property cannot have [getter|setter] and [value|writable] together"))));
    }
    // ToPropertyDescriptor : (end)

    // 1
    ESValue oldLenDesc = A->getOwnProperty(strings->length.string());
    ASSERT(!oldLenDesc.isUndefined());
//    ASSERT(!escargot::PropertyDescriptor::IsAccessorDescriptor(oldLenDesc)); // Our implementation differs from Spec so that length property is accessor property.
    size_t lengthIdx = O->hiddenClass()->findProperty(strings->length.string());
//    ASSERT(O->hiddenClass()->findProperty(strings->length.string()) == 1);
    const ESHiddenClassPropertyInfo& oldLePropertyInfo = O->hiddenClass()->propertyInfo(lengthIdx);

    // 2
    size_t oldLen = oldLenDesc.toUint32();

    // 3
    ESValue lenStr = strings->length.string();
    if (*P.toString() == *strings->length.string()) {
        ESValue descV = desc->get(strings->value.string());
        // 3.a
        if (!descHasValue) {
            return A->asESObject()->defineOwnProperty(P, desc, throwFlag);
        }
        // 3.b
        ESObject* newLenDesc = ESObject::create();
        if (descHasEnumerable)
            newLenDesc->set(strings->enumerable.string(), descE);
        if (descHasWritable)
            newLenDesc->set(strings->writable.string(), descW);
        if (descHasConfigurable)
            newLenDesc->set(strings->configurable.string(), descC);
        if (descHasValue)
            newLenDesc->set(strings->value.string(), descV);
        if (descHasGetter)
            newLenDesc->set(strings->get.string(), descGet);
        if (descHasSetter)
            newLenDesc->set(strings->set.string(), descSet);

        // 3.c
        uint32_t newLen = descV.toUint32();

        // 3.d
        if (newLen != descV.toNumber())
            ESVMInstance::currentInstance()->throwError(ESValue(RangeError::create(ESString::create("ArrayObject.DefineOwnProperty 3.d"))));

        // 3.e
        newLenDesc->set(strings->value.string(), ESValue(newLen));

        // 3.f
        if (newLen >= oldLen)
            return A->asESObject()->defineOwnProperty(P, newLenDesc, throwFlag);

        // 3.g
        if (!oldLePropertyInfo.m_flags.m_isWritable) {
            if (throwFlag)
                ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("ArrayObject.DefineOwnProperty 3.g"))));
            else
                return false;
        }

        // 3.h
        bool newLenDescHasWritable = newLenDesc->hasProperty(strings->writable.string());
        bool newLenDescW = newLenDesc->get(strings->writable.string()).toBoolean();
        bool newWritable;
        if (!newLenDescHasWritable || newLenDescW)
            newWritable = true;
        else {
            newWritable = false;
            newLenDesc->set(strings->writable.string(), ESValue(true));
        }

        // 3.j
        bool succeeded = A->asESObject()->defineOwnProperty(P, newLenDesc, throwFlag);

        // 3.k
        if (!succeeded)
            return false;

        // 3.l
        while (newLen < oldLen) {
            oldLen--;
            bool deleteSucceeded = A->deleteProperty(ESValue(oldLen).toString());
            if (!deleteSucceeded) {
                newLenDesc->set(strings->value.string(), ESValue(oldLen+1));
                if (!newWritable)
                    newLenDesc->set(strings->writable.string(), ESValue(false));
                A->asESObject()->defineOwnProperty(P, newLenDesc, false);
                if (throwFlag)
                    ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("ArrayObject.DefineOwnProperty 3.l.iii"))));
                else
                    return false;
            }
        }

        // 3.m
        if (!newWritable) {
            ESObject* descWritableIsFalse = ESObject::create();
            descWritableIsFalse->set(strings->writable.string(), ESValue(false));
            A->asESObject()->defineOwnProperty(P, descWritableIsFalse, false);
            return true;
        }
        return true;
    } else if (*ESValue(P.toUint32()).toString() == *P.toString() && P.toUint32() != 2*32-1) { // 4
        // 4.a
        uint32_t index = P.toUint32();

        // 4.b
        if (index >= oldLen && !oldLePropertyInfo.m_flags.m_isWritable) {
            if (throwFlag)
                ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("ArrayObject.DefineOwnProperty 4.b"))));
            else
                return false;
        }

        // 4.c
        bool succeeded = A->asESObject()->defineOwnProperty(P, desc, false);

        // 4.d
        if (!succeeded) {
            if (throwFlag)
                ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("ArrayObject.DefineOwnProperty 4.d"))));
            else
                return false;
        }

        // 4.e
        if (index >= oldLen) {
            ESObject* oldLenDescAsObject = ESObject::create();
            oldLenDescAsObject->set(strings->value.string(), ESValue(index + 1));
            A->asESObject()->defineOwnProperty(lenStr, oldLenDescAsObject, false);
        }

        // 4.f
        return true;
    }

    // 5
    return A->asESObject()->defineOwnProperty(P, desc, throwFlag);
}

ESRegExpObject::ESRegExpObject(escargot::ESString* source, const Option& option)
    : ESObject((Type)(Type::ESObject | Type::ESRegExpObject), ESVMInstance::currentInstance()->globalObject()->regexpPrototype())
{
    m_source = source;
    m_option = option;
    m_yarrPattern = NULL;
    m_bytecodePattern = NULL;
    m_lastIndex = ESValue(0);
    m_lastExecutedString = NULL;

    defineAccessorProperty(strings->source, [](ESObject* self, ESObject* originalObj) -> ESValue {
        return self->asESRegExpObject()->source();
    }, nullptr, true, false, false);

    defineAccessorProperty(strings->ignoreCase, [](ESObject* self, ESObject* originalObj) -> ESValue {
        return ESValue((bool)(self->asESRegExpObject()->option() & ESRegExpObject::Option::IgnoreCase));
    }, nullptr, true, false, false);

    defineAccessorProperty(strings->global, [](ESObject* self, ESObject* originalObj) -> ESValue {
        return ESValue((bool)(self->asESRegExpObject()->option() & ESRegExpObject::Option::Global));
    }, nullptr, true, false, false);

    defineAccessorProperty(strings->multiline, [](ESObject* self, ESObject* originalObj) -> ESValue {
        return ESValue((bool)(self->asESRegExpObject()->option() & ESRegExpObject::Option::MultiLine));
    }, nullptr, true, false, false);

    defineAccessorProperty(strings->lastIndex, [](ESObject* self, ESObject* originalObj) -> ESValue {
        return self->asESRegExpObject()->lastIndex();
    }, [](ESObject* self, ESObject* originalObj, const ESValue& index)
    {
        self->asESRegExpObject()->setLastIndex(index);
    }, true, false, false);
}

bool ESRegExpObject::setSource(escargot::ESString* src)
{
    m_bytecodePattern = NULL;
    m_source = src;
    JSC::Yarr::ErrorCode yarrError;
    m_yarrPattern = new JSC::Yarr::YarrPattern(*src, m_option & ESRegExpObject::Option::IgnoreCase, m_option & ESRegExpObject::Option::MultiLine, &yarrError);
    if (yarrError)
        return false;
    return true;
}
void ESRegExpObject::setOption(const Option& option)
{
    if (((m_option & ESRegExpObject::Option::MultiLine) != (option & ESRegExpObject::Option::MultiLine))
        || ((m_option & ESRegExpObject::Option::IgnoreCase) != (option & ESRegExpObject::Option::IgnoreCase))
        ) {
        m_bytecodePattern = NULL;
    }
    m_option = option;
}

ESFunctionObject::ESFunctionObject(LexicalEnvironment* outerEnvironment, CodeBlock* cb, escargot::ESString* name, unsigned length, bool hasPrototype)
    : ESObject((Type)(Type::ESObject | Type::ESFunctionObject), ESVMInstance::currentInstance()->globalFunctionPrototype(), 4)
{
    m_name = name;
    m_outerEnvironment = outerEnvironment;
    m_codeBlock = cb;
    m_flags.m_nonConstructor = false;
    m_protoType = ESObject::create(2);

    // m_protoType.asESPointer()->asESObject()->defineDataProperty(strings->constructor.string(), true, false, true, this);
    m_protoType.asESPointer()->asESObject()->m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForPrototypeObject();
    m_protoType.asESPointer()->asESObject()->m_hiddenClassData.push_back(this);

    // $19.2.4 Function Instances
    // these define in ESVMInstance::ESVMInstance()
    // defineDataProperty(strings->length, false, false, true, ESValue(length));
    // defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), true, false, false);
    // defineDataProperty(strings->name.string(), false, false, true, name);
    if (hasPrototype) {
        m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForFunctionObject();
        m_hiddenClassData.push_back(ESValue(length));
        m_hiddenClassData.push_back(ESValue((ESPointer *)ESVMInstance::currentInstance()->functionPrototypeAccessorData()));
        m_hiddenClassData.push_back(ESValue(name));
    } else {
        m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForFunctionObjectWithoutPrototype();
        m_hiddenClassData.push_back(ESValue(length));
        m_hiddenClassData.push_back(ESValue(name));
    }
    m_flags.m_isBoundFunction = false;
}

ESFunctionObject::ESFunctionObject(LexicalEnvironment* outerEnvironment, NativeFunctionType fn, escargot::ESString* name, unsigned length, bool isConstructor)
    : ESFunctionObject(outerEnvironment, (CodeBlock *)NULL, name, length, isConstructor)
{
    m_codeBlock = CodeBlock::create(0, true);
    m_codeBlock->pushCode(ExecuteNativeFunction(fn));
#ifdef ENABLE_ESJIT
    m_codeBlock->m_dontJIT = true;
#endif
    m_name = name;
    if (!isConstructor)
        m_flags.m_nonConstructor = true;
    m_flags.m_isBoundFunction = false;
}

ALWAYS_INLINE void functionCallerInnerProcess(ExecutionContext* newEC, ESFunctionObject* fn, const ESValue& receiver, ESValue arguments[], const size_t& argumentCount, ESVMInstance* ESVMInstance)
{
    bool strict = fn->codeBlock()->shouldUseStrictMode();
    newEC->setStrictMode(strict);

    // http://www.ecma-international.org/ecma-262/6.0/#sec-ordinarycallbindthis
    if (!strict) {
        if (receiver.isUndefinedOrNull()) {
            newEC->setThisBinding(ESVMInstance->globalObject());
        } else {
            newEC->setThisBinding(receiver.toObject());
        }
    } else {
        newEC->setThisBinding(receiver);
    }


    DeclarativeEnvironmentRecord* functionRecord = newEC->environment()->record()->toDeclarativeEnvironmentRecord();
    if (UNLIKELY(fn->codeBlock()->m_needsActivation)) {
        const InternalAtomicStringVector& params = fn->codeBlock()->m_params;
        for (unsigned i = 0; i < params.size(); i ++) {
            if (i < argumentCount) {
                *functionRecord->bindingValueForActivationMode(i) = arguments[i];
            }
        }
        // if FunctionExpressionNode has own name, should bind own name
        if (fn->codeBlock()->m_isFunctionExpression && fn->name()->length())
            *functionRecord->bindingValueForActivationMode(params.size()) = ESValue(fn);
    } else {
        const InternalAtomicStringVector& params = fn->codeBlock()->m_params;
        ESValue* buf = newEC->cachedDeclarativeEnvironmentRecordESValue();
        memcpy(buf, arguments, sizeof(ESValue) * (std::min(params.size(), argumentCount)));
        // if FunctionExpressionNode has own name, should bind own name
        if (fn->codeBlock()->m_isFunctionExpression && fn->name()->length())
            buf[params.size()] = ESValue(fn);
    }
}

#ifdef ENABLE_ESJIT

ESValue executeJIT(ESFunctionObject* fn, ESVMInstance* instance, ExecutionContext& ec)
{
    ESValue result(ESValue::ESForceUninitialized);
#ifndef NDEBUG
    const char* functionName = fn->codeBlock()->m_nonAtomicId ? (fn->codeBlock()->m_nonAtomicId->utf8Data()):"(anonymous)";
#endif
    JITFunction jitFunction = fn->codeBlock()->m_cachedJITFunction;

    if (!jitFunction && !fn->codeBlock()->m_dontJIT) {

        // update profile data
        char* code = fn->codeBlock()->m_code.data();
        size_t siz = fn->codeBlock()->m_byteCodeIndexesHaveToProfile.size();
        for (unsigned i = 0; i < siz; i ++) {
            size_t pos = fn->codeBlock()->m_extraData[fn->codeBlock()->m_byteCodeIndexesHaveToProfile[i]].m_decoupledData->m_codePosition;
            ByteCode* currentCode = (ByteCode *)&code[pos];
            Opcode opcode = fn->codeBlock()->m_extraData[fn->codeBlock()->m_byteCodeIndexesHaveToProfile[i]].m_opcode;
            switch (opcode) {
            case GetByIdOpcode: {
                reinterpret_cast<GetById*>(currentCode)->m_profile.updateProfiledType();
                break;
            }
            case GetByIndexOpcode: {
                reinterpret_cast<GetByIndex*>(currentCode)->m_profile.updateProfiledType();
                break;
            }
            case GetByIndexWithActivationOpcode: {
                reinterpret_cast<GetByIndexWithActivation*>(currentCode)->m_profile.updateProfiledType();
                break;
            }
            case GetByGlobalIndexOpcode: {
                reinterpret_cast<GetByGlobalIndex*>(currentCode)->m_profile.updateProfiledType();
                break;
            }
            case GetObjectOpcode: {
                reinterpret_cast<GetObject*>(currentCode)->m_profile.updateProfiledType();
                break;
            }
            case GetObjectAndPushObjectOpcode: {
                reinterpret_cast<GetObjectAndPushObject*>(currentCode)->m_profile.updateProfiledType();
                break;
            }
            case GetObjectWithPeekingOpcode: {
                reinterpret_cast<GetObjectWithPeeking*>(currentCode)->m_profile.updateProfiledType();
                break;
            }
            case GetObjectPreComputedCaseOpcode: {
                reinterpret_cast<GetObjectPreComputedCase*>(currentCode)->m_profile.updateProfiledType();
                break;
            }
            case GetObjectWithPeekingPreComputedCaseOpcode: {
                reinterpret_cast<GetObjectWithPeekingPreComputedCase*>(currentCode)->m_profile.updateProfiledType();
                break;
            }
            case GetObjectPreComputedCaseAndPushObjectOpcode: {
                reinterpret_cast<GetObjectPreComputedCaseAndPushObject*>(currentCode)->m_profile.updateProfiledType();
                break;
            }
            case ThisOpcode: {
                reinterpret_cast<This*>(currentCode)->m_profile.updateProfiledType();
                break;
            }
            case CallFunctionOpcode: {
                reinterpret_cast<CallFunction*>(currentCode)->m_profile.updateProfiledType();
                break;
            }
            case CallFunctionWithReceiverOpcode: {
                reinterpret_cast<CallFunctionWithReceiver*>(currentCode)->m_profile.updateProfiledType();
                break;
            }
            default:
                break;
            }
        }

        if (fn->codeBlock()->m_executeCount >= fn->codeBlock()->m_jitThreshold) {
            LOG_VJ("==========Trying JIT Compile for function %s... (codeBlock %p)==========\n", functionName, fn->codeBlock());
            size_t idx = 0;
            size_t bytecodeCounter = 0;
            bool compileNextTime = false;
            // check jit support for debug
#ifndef NDEBUG
            {
                char* code = fn->codeBlock()->m_code.data();
                char* end = &fn->codeBlock()->m_code.data()[fn->codeBlock()->m_code.size()];
                while (&code[idx] < end) {
                    Opcode opcode = fn->codeBlock()->m_extraData[bytecodeCounter].m_opcode;
                    switch (opcode) {
#define DECLARE_EXECUTE_NEXTCODE(opcode, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
                    case opcode##Opcode: \
                        if (!JITSupported) { \
                            fn->codeBlock()->m_dontJIT = true; \
                            compileNextTime = true; \
                            LOG_VJ("> Unsupported ByteCode %s (idx %u). Stop trying JIT.\n", #opcode, (unsigned)idx); \
                        } \
                        idx += sizeof(opcode); \
                        bytecodeCounter++; \
                        break;
                        FOR_EACH_BYTECODE_OP(DECLARE_EXECUTE_NEXTCODE);
#undef DECLARE_EXECUTE_NEXTCODE
                    case OpcodeKindEnd:
                        break;
                    }
                }
            }
#endif
            // check profile data
            char* code = fn->codeBlock()->m_code.data();
            for (unsigned i = 0; i < fn->codeBlock()->m_byteCodeIndexesHaveToProfile.size(); i ++) {
                size_t pos = fn->codeBlock()->m_extraData[fn->codeBlock()->m_byteCodeIndexesHaveToProfile[i]].m_decoupledData->m_codePosition;
                Opcode opcode = fn->codeBlock()->m_extraData[fn->codeBlock()->m_byteCodeIndexesHaveToProfile[i]].m_opcode;
                ByteCode* currentCode = (ByteCode *)&code[pos];
                switch (opcode) {
                case GetByIdOpcode: {
                    if (reinterpret_cast<GetById*>(currentCode)->m_profile.getType().isBottomType()) {
                        LOG_VJ("> Cannot Compile JIT Function due to idx %u is not profiled yet\n", (unsigned)pos);
                        compileNextTime = true;
                        break;
                    }
                    break;
                }
                case GetByIndexOpcode: {
                    if (reinterpret_cast<GetByIndex*>(currentCode)->m_profile.getType().isBottomType()) {
                        LOG_VJ("> Cannot Compile JIT Function due to idx %u is not profiled yet\n", (unsigned)pos);
                        compileNextTime = true;
                        break;
                    }
                    break;
                }
                case GetByIndexWithActivationOpcode: {
                    if (reinterpret_cast<GetByIndexWithActivation*>(currentCode)->m_profile.getType().isBottomType()) {
                        LOG_VJ("> Cannot Compile JIT Function due to idx %u is not profiled yet\n", (unsigned)pos);
                        compileNextTime = true;
                        break;
                    }
                    break;
                }
                case GetByGlobalIndexOpcode: {
                    if (reinterpret_cast<GetByGlobalIndex*>(currentCode)->m_profile.getType().isBottomType()) {
                        LOG_VJ("> Cannot Compile JIT Function due to idx %u is not profiled yet\n", (unsigned)pos);
                        compileNextTime = true;
                        break;
                    }
                    break;
                }
                case GetObjectOpcode: {
                    if (reinterpret_cast<GetObject*>(currentCode)->m_profile.getType().isBottomType()) {
                        LOG_VJ("> Cannot Compile JIT Function due to idx %u is not profiled yet\n", (unsigned)pos);
                        compileNextTime = true;
                        break;
                    }
                    break;
                }
                case GetObjectAndPushObjectOpcode: {
                    if (reinterpret_cast<GetObjectAndPushObject*>(currentCode)->m_profile.getType().isBottomType()) {
                        LOG_VJ("> Cannot Compile JIT Function due to idx %u is not profiled yet\n", (unsigned)pos);
                        compileNextTime = true;
                        break;
                    }
                    break;
                }
                case GetObjectWithPeekingOpcode: {
                    if (reinterpret_cast<GetObjectWithPeeking*>(currentCode)->m_profile.getType().isBottomType()) {
                        LOG_VJ("> Cannot Compile JIT Function due to idx %u is not profiled yet\n", (unsigned)pos);
                        compileNextTime = true;
                        break;
                    }
                    break;
                }
                case GetObjectPreComputedCaseOpcode: {
                    if (reinterpret_cast<GetObjectPreComputedCase*>(currentCode)->m_profile.getType().isBottomType()) {
                        LOG_VJ("> Cannot Compile JIT Function due to idx %u is not profiled yet\n", (unsigned)pos);
                        compileNextTime = true;
                        break;
                    }
                    break;
                }
                case GetObjectWithPeekingPreComputedCaseOpcode: {
                    if (reinterpret_cast<GetObjectWithPeekingPreComputedCase*>(currentCode)->m_profile.getType().isBottomType()) {
                        LOG_VJ("> Cannot Compile JIT Function due to idx %u is not profiled yet\n", (unsigned)pos);
                        compileNextTime = true;
                        break;
                    }
                    break;
                }
                case GetObjectPreComputedCaseAndPushObjectOpcode: {
                    if (reinterpret_cast<GetObjectPreComputedCaseAndPushObject*>(currentCode)->m_profile.getType().isBottomType()) {
                        LOG_VJ("> Cannot Compile JIT Function due to idx %u is not profiled yet\n", (unsigned)pos);
                        compileNextTime = true;
                        break;
                    }
                    break;
                }
                case ThisOpcode: {
                    if (reinterpret_cast<This*>(currentCode)->m_profile.getType().isBottomType()) {
                        LOG_VJ("> Cannot Compile JIT Function due to idx %u is not profiled yet\n", (unsigned)pos);
                        compileNextTime = true;
                        break;
                    }
                    break;
                }
                case CallFunctionOpcode: {
                    if (reinterpret_cast<CallFunction*>(currentCode)->m_profile.getType().isBottomType()) {
                        LOG_VJ("> Cannot Compile JIT Function due to idx %u is not profiled yet\n", (unsigned)pos);
                        compileNextTime = true;
                        break;
                    }
                    break;
                }
                case CallFunctionWithReceiverOpcode: {
                    if (reinterpret_cast<CallFunctionWithReceiver*>(currentCode)->m_profile.getType().isBottomType()) {
                        LOG_VJ("> Cannot Compile JIT Function due to idx %u is not profiled yet\n", (unsigned)pos);
                        compileNextTime = true;
                        break;
                    }
                    break;
                }
                default:
                    break;
                }


            }

            if (!compileNextTime) {
                jitFunction = reinterpret_cast<JITFunction>(ESJIT::JITCompile(fn->codeBlock(), instance));
                if (jitFunction) {
                    LOG_VJ("> Compilation successful for function %s (codeBlock %p)! Cache jit function %p\n", functionName, fn->codeBlock(), jitFunction);
#ifndef NDEBUG
                    if (ESVMInstance::currentInstance()->m_reportCompiledFunction) {
                        printf("%s ", fn->codeBlock()->m_nonAtomicId ? (fn->codeBlock()->m_nonAtomicId->utf8Data()):"(anonymous)");
                        ESVMInstance::currentInstance()->m_compiledFunctions++;
                    }
#endif
                    fn->codeBlock()->m_cachedJITFunction = jitFunction;
                } else {
                    LOG_VJ("> Compilation failed! disable jit compilation for function %s (codeBlock %p) from now on\n", functionName, fn->codeBlock());
                    fn->codeBlock()->m_dontJIT = true;
                    fn->codeBlock()->removeJITInfo();
                }
            } else {
                size_t threshold = fn->codeBlock()->m_jitThreshold;
                if (threshold > 1024) {
                    LOG_VJ("> No profile infos. Stop trying JIT for function %s.\n", functionName);
                    fn->codeBlock()->m_dontJIT = true;
                    fn->codeBlock()->removeJITInfo();
                } else {
                    LOG_VJ("> Doubling JIT compilation threshold from %d to %d for function %s\n", threshold, threshold*2, functionName);
                    fn->codeBlock()->m_jitThreshold *= 2;
                }
            }
        }
    }


    if (jitFunction) {
        unsigned stackSiz = fn->codeBlock()->m_requiredStackSizeInESValueSize * sizeof(ESValue);
#ifndef NDEBUG
        stackSiz *= 2;
#endif
        char* stackBuf = (char *)alloca(stackSiz);
        ec.setBp(stackBuf);

        fn->codeBlock()->m_recursionDepth++;
        result = ESValue::fromRawDouble(jitFunction(instance));
        fn->codeBlock()->m_recursionDepth--;
        // printf("JIT Result %s (%jx)\n", result.toString()->utf8Data(), result.asRawData());
        if (ec.inOSRExit()) {
            fn->codeBlock()->m_osrExitCount++;
            LOG_VJ("> OSR Exit from function %s (codeBlock %p), exit count %zu\n", functionName, fn->codeBlock(), fn->codeBlock()->m_osrExitCount);
            int32_t tmpIndex = result.asInt32();
            char* code = fn->codeBlock()->m_code.data();
            size_t idx = 0;
            size_t bytecodeCounter = 0;
            unsigned maxStackPos = 0;
            bool found = false;
            char* end = &fn->codeBlock()->m_code.data()[fn->codeBlock()->m_code.size()];
            while (&code[idx] < end) {
                if (found) {
                    break;
                }

                Opcode opcode = fn->codeBlock()->m_extraData[bytecodeCounter].m_opcode;
                ByteCodeExtraData* extraData = &fn->codeBlock()->m_extraData[bytecodeCounter];
                if (extraData->m_decoupledData->m_targetIndex0 == tmpIndex) {
                    maxStackPos = ec.getStackPos();
                    if (ec.executeNextByteCode()) {
                        found = true;
                    } else {
                        break;
                    }
                }

                switch (opcode) {
#define DECLARE_EXECUTE_NEXTCODE(code, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
                case code##Opcode: \
                    idx += sizeof(code); \
                    bytecodeCounter++; \
                    continue;
                    FOR_EACH_BYTECODE_OP(DECLARE_EXECUTE_NEXTCODE);
#undef DECLARE_EXECUTE_NEXTCODE
                default:
                    RELEASE_ASSERT_NOT_REACHED();
                    break;
                }
            }
            if (fn->codeBlock()->m_osrExitCount >= ESVMInstance::currentInstance()->m_osrExitThreshold) {
                LOG_VJ("Too many exits; Disable JIT for function %s (codeBlock %p) from now on\n", functionName, fn->codeBlock());
                fn->codeBlock()->m_cachedJITFunction = nullptr;
                // Fixme(JMP): We have to compile to JIT code again when gathering enough type data
                fn->codeBlock()->m_dontJIT = true;
                if (!fn->codeBlock()->m_recursionDepth) {
                    fn->codeBlock()->removeJITInfo();
                    fn->codeBlock()->removeJITCode();
                }
#ifndef NDEBUG
                if (ESVMInstance::currentInstance()->m_reportOSRExitedFunction) {
                    printf("%s ", fn->codeBlock()->m_nonAtomicId ? (fn->codeBlock()->m_nonAtomicId->utf8Data()):"(anonymous)");
                    ESVMInstance::currentInstance()->m_osrExitedFunctions++;
                }
#endif
            }
            if (idx == fn->codeBlock()->m_code.size()) {
                result = ESValue();
            } else {
                result = interpret(instance, fn->codeBlock(), idx, maxStackPos);
            }
            fn->codeBlock()->m_executeCount++;
        }
        if (UNLIKELY(fn->codeBlock()->m_dontJIT)) {
            if (!fn->codeBlock()->m_recursionDepth) {
                fn->codeBlock()->removeJITInfo();
                fn->codeBlock()->removeJITCode();
            }
        }
    } else {
        result = interpret(instance, fn->codeBlock());
        fn->codeBlock()->m_executeCount++;
    }

    return result;
}

#endif

ESValue ESFunctionObject::call(ESVMInstance* instance, const ESValue& callee, const ESValue& receiver, ESValue arguments[], const size_t& argumentCount, bool isNewExpression)
{
    ESValue result(ESValue::ESForceUninitialized);
    if (LIKELY(callee.isESPointer() && callee.asESPointer()->isESFunctionObject())) {
        ExecutionContext* currentContext = instance->currentExecutionContext();
        ESFunctionObject* fn = callee.asESPointer()->asESFunctionObject();
        CodeBlock* const cb = fn->codeBlock();

        if (UNLIKELY(!cb->m_code.size())) {
            FunctionNode* node = (FunctionNode *)cb->m_ast;
            cb->m_innerIdentifiers = std::move(node->m_innerIdentifiers);
            cb->m_needsActivation = node->m_needsActivation;
            cb->m_params = std::move(node->m_params);
            cb->m_isStrict = node->m_isStrict;
            cb->m_isFunctionExpression = node->isExpression();

            ByteCodeGenerateContext newContext(cb);
            node->body()->generateStatementByteCode(cb, newContext);

#ifdef ENABLE_ESJIT
            cb->m_tempRegisterSize = newContext.m_currentSSARegisterCount;
#endif
            cb->pushCode(ReturnFunction(), newContext, node);
            cb->m_ast = NULL;

#ifndef NDEBUG
            cb->m_id = node->m_id;
            cb->m_nonAtomicId = node->m_nonAtomicId;
            if (ESVMInstance::currentInstance()->m_dumpByteCode) {
                char* code = cb->m_code.data();
                ByteCode* currentCode = (ByteCode *)(&code[0]);
                if (currentCode->m_orgOpcode != ExecuteNativeFunctionOpcode) {
                    cb->m_nonAtomicId = node->m_nonAtomicId;
                    dumpBytecode(cb);
                }
            }
            if (ESVMInstance::currentInstance()->m_reportUnsupportedOpcode) {
                char* code = cb->m_code.data();
                ByteCode* currentCode = (ByteCode *)(&code[0]);
                if (currentCode->m_orgOpcode != ExecuteNativeFunctionOpcode) {
                    dumpUnsupported(cb);
                }
            }
#endif
#ifdef ENABLE_ESJIT
            newContext.cleanupSSARegisterCount();
#endif
        }

        if (UNLIKELY(cb->m_needsActivation)) {
            instance->m_currentExecutionContext = new ExecutionContext(LexicalEnvironment::newFunctionEnvironment(arguments, argumentCount, fn), true, isNewExpression,
                arguments, argumentCount);
            functionCallerInnerProcess(instance->m_currentExecutionContext, fn, receiver, arguments, argumentCount, instance);
#ifdef ENABLE_ESJIT
            result = executeJIT(fn, instance, *instance->m_currentExecutionContext);
#else
            result = interpret(instance, cb);
#endif
            instance->m_currentExecutionContext = currentContext;
        } else {
            ESValue* storage = (::escargot::ESValue *)alloca(sizeof(::escargot::ESValue) * cb->m_innerIdentifiers.size());
            FunctionEnvironmentRecord envRec(
                arguments, argumentCount,
                storage,
                &fn->m_codeBlock->m_innerIdentifiers);

            // envRec.m_functionObject = fn;
            // envRec.m_newTarget = receiver;

            LexicalEnvironment env(&envRec, fn->outerEnvironment());
            ExecutionContext ec(&env, false, isNewExpression, arguments, argumentCount, storage);
            instance->m_currentExecutionContext = &ec;
            functionCallerInnerProcess(&ec, fn, receiver, arguments, argumentCount, instance);
#ifdef ENABLE_ESJIT
            result = executeJIT(fn, instance, ec);
#else
            result = interpret(instance, cb);
#endif
            instance->m_currentExecutionContext = currentContext;
        }
    } else {
        instance->throwError(ESValue(TypeError::create(ESString::create("Callee is not a function object"))));
    }

    return result;
}

ESDateObject::ESDateObject(ESPointer::Type type)
    : ESObject((Type)(Type::ESObject | Type::ESDateObject), ESVMInstance::currentInstance()->globalObject()->datePrototype())
{
    m_isCacheDirty = true;
    m_hasVaildDate = false;
}

void ESDateObject::parseYmdhmsToDate(struct tm* timeinfo, int year, int month, int date, int hour, int minute, int second)
{
    char buffer[255];
    snprintf(buffer, 255, "%d-%d-%d-%d-%d-%d", year, month + 1, date, hour, minute, second);
    strptime(buffer, "%Y-%m-%d-%H-%M-%S", timeinfo);
}

void ESDateObject::parseStringToDate(struct tm* timeinfo, bool* timezoneSet, escargot::ESString* istr)
{
    char* buffer = (char*)istr->toNullableUTF8String().m_buffer;
    if (isalpha(buffer[0])) {
        strptime(buffer, "%B %d %Y %H:%M:%S %z", timeinfo);
        *timezoneSet = true;
    } else if (isdigit(buffer[0])) {
        strptime(buffer, "%m/%d/%Y %H:%M:%S", timeinfo);
    }
}

int ESDateObject::daysInYear(long year)
{
    long y = year;
    if (y % 4 != 0) {
        return 365;
    } else if (y % 100 != 0) {
        return 366;
    } else if (y % 400 != 0) {
        return 365;
    } else { // y % 400 == 0
        return 366;
    }
}

int ESDateObject::dayFromYear(long year) // day number of the first day of year 'y'
{
    return 365 * (year - 1970) + floor((year - 1969) / 4) - floor((year - 1901) / 100) + floor((year - 1601) / 400);
}

long ESDateObject::yearFromTime(double t)
{
    long estimate = ceil(t / msPerDay / 365.0);
    while (timeFromYear(estimate) > t) {
        estimate--;
    }
    return estimate;
}

int ESDateObject::inLeapYear(double t)
{
    int days = daysInYear(yearFromTime(t));
    if (days == 365) {
        return 0;
    } else if (days == 366) {
        return 1;
    }
    RELEASE_ASSERT_NOT_REACHED();
}

int ESDateObject::dayFromMonth(long year, int month)
{
    int ds[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (daysInYear(year) == 366) {
        ds[1] = 29;
    }
    int retval = 0;
    for (int i = 0;  i < month; i++) {
        retval += ds[i];
    }
    return retval;
}

int ESDateObject::monthFromTime(double t)
{
    int dayWithinYear = day(t) - dayFromYear(yearFromTime(t));
    int leap = inLeapYear(t);
    if (dayWithinYear < 0) {
        RELEASE_ASSERT_NOT_REACHED();
    } else if (dayWithinYear < 31) {
        return 0;
    } else if (dayWithinYear < 59 + leap) {
        return 1;
    } else if (dayWithinYear < 90 + leap) {
        return 2;
    } else if (dayWithinYear < 120 + leap) {
        return 3;
    } else if (dayWithinYear < 151 + leap) {
        return 4;
    } else if (dayWithinYear < 181 + leap) {
        return 5;
    } else if (dayWithinYear < 212 + leap) {
        return 6;
    } else if (dayWithinYear < 243 + leap) {
        return 7;
    } else if (dayWithinYear < 273 + leap) {
        return 8;
    } else if (dayWithinYear < 304 + leap) {
        return 9;
    } else if (dayWithinYear < 334 + leap) {
        return 10;
    } else if (dayWithinYear < 365 + leap) {
        return 11;
    } else {
        RELEASE_ASSERT_NOT_REACHED();
    }
}

int ESDateObject::dateFromTime(double t)
{
    int dayWithinYear = day(t) - dayFromYear(yearFromTime(t));
    int leap = inLeapYear(t);
    int retval = dayWithinYear - leap;
    switch (monthFromTime(t)) {
    case 0:
        return retval + 1 + leap;
    case 1:
        return retval - 30 + leap;
    case 2:
        return retval - 58;
    case 3:
        return retval - 89;
    case 4:
        return retval - 119;
    case 5:
        return retval - 150;
    case 6:
        return retval - 180;
    case 7:
        return retval - 211;
    case 8:
        return retval - 242;
    case 9:
        return retval - 272;
    case 10:
        return retval - 303;
    case 11:
        return retval - 333;
    default:
        RELEASE_ASSERT_NOT_REACHED();
    }
}

double ESDateObject::makeDay(long year, int month, int date)
{
    // TODO: have to check whether year or month is infinity
//    if(year == infinity || month == infinity){
//        return nan;
//    }
    long ym = year + floor(month / 12);
    int mn = month % 12;
    double t = timeFromYear(ym) + dayFromMonth(ym, mn) * msPerDay;
    return day(t) + date - 1;
}

double ESDateObject::ymdhmsToSeconds(long year, int mon, int day, int hour, int minute, double second)
{
    return (makeDay(year, mon, day) * msPerDay + (hour * msPerHour + minute * msPerMinute + second * msPerSecond /* + millisecond */)) / 1000.0;
}

void ESDateObject::setTimeValue()
{
    clock_gettime(CLOCK_REALTIME, &m_time);
    m_isCacheDirty = true;
    m_hasVaildDate = true;
}

void ESDateObject::setTimeValue(const ESValue str)
{
    escargot::ESString* istr = str.toString();
    bool timezoneSet = false;
    parseStringToDate(&m_cachedTM, &timezoneSet, istr);
    m_cachedTM.tm_isdst = true;
    m_time.tv_sec = ymdhmsToSeconds(m_cachedTM.tm_year+1900, m_cachedTM.tm_mon, m_cachedTM.tm_mday, m_cachedTM.tm_hour, m_cachedTM.tm_min, m_cachedTM.tm_sec);
    if (timezoneSet) {
        m_time.tv_sec += -m_cachedTM.tm_gmtoff - ESVMInstance::currentInstance()->timezoneOffset();
    }
    m_hasVaildDate = true;
}

void ESDateObject::setTimeValue(int year, int month, int date, int hour, int minute, int second, int millisecond)
{
    long ym = year + floor(month / 12);
    int mn = month % 12;
    parseYmdhmsToDate(&m_cachedTM, ym, mn, date, hour, minute, second);
    m_cachedTM.tm_isdst = true;
    m_time.tv_sec = ymdhmsToSeconds(m_cachedTM.tm_year+1900, m_cachedTM.tm_mon, m_cachedTM.tm_mday, m_cachedTM.tm_hour, m_cachedTM.tm_min, m_cachedTM.tm_sec);
    m_time.tv_nsec = millisecond * 1000000;
    m_hasVaildDate = true;
}

void ESDateObject::resolveCache()
{
    if (m_isCacheDirty) {
        memcpy(&m_cachedTM, ESVMInstance::currentInstance()->computeLocalTime(m_time), sizeof(tm));
        m_isCacheDirty = false;
    }
}

int ESDateObject::getDate()
{
    resolveCache();
    return m_cachedTM.tm_mday;
}

int ESDateObject::getDay()
{
    resolveCache();
    return m_cachedTM.tm_wday;
}

int ESDateObject::getFullYear()
{
    resolveCache();
    return m_cachedTM.tm_year + 1900;
}

int ESDateObject::getHours()
{
    resolveCache();
    return m_cachedTM.tm_hour;
}

int ESDateObject::getMinutes()
{
    resolveCache();
    return m_cachedTM.tm_min;
}

int ESDateObject::getMonth()
{
    resolveCache();
    return m_cachedTM.tm_mon;
}

int ESDateObject::getSeconds()
{
    resolveCache();
    return m_cachedTM.tm_sec;
}

long ESDateObject::getTimezoneOffset()
{
    return ESVMInstance::currentInstance()->timezoneOffset();
}

void ESDateObject::setTime(double t)
{
    if (isnan(t))
        return;

    time_t raw_t = (time_t) floor(t);
    m_time.tv_sec = raw_t / 1000;
    m_time.tv_nsec = (raw_t % 10000) * 1000000;

    m_isCacheDirty = true;
}

tm* ESDateObject::getGmtTime()
{
    if (!m_hasVaildDate) {
        return NULL;
    } else {
        time_t raw_t = (time_t) floor(getTimeAsMillisec());
        tm* ret = gmtime(&raw_t);
        int KST = 9; // TODO it's temp
        ret->tm_gmtoff = KST * 60 * 60;
        ret->tm_hour = (ret->tm_hour + KST) % 24;
        return ret;
    }
}

ESMathObject::ESMathObject(ESPointer::Type type)
    : ESObject((Type)(Type::ESObject | Type::ESMathObject), ESVMInstance::currentInstance()->globalObject()->objectPrototype())
{
}

ESStringObject::ESStringObject(escargot::ESString* str)
    : ESObject((Type)(Type::ESObject | Type::ESStringObject), ESVMInstance::currentInstance()->globalObject()->stringPrototype())
{
    setStringData(str);

    // $21.1.4.1 String.length
    defineAccessorProperty(strings->length.string(), ESVMInstance::currentInstance()->stringObjectLengthAccessorData(), false, false, false);
}

ESNumberObject::ESNumberObject(double value)
    : ESObject((Type)(Type::ESObject | Type::ESNumberObject), ESVMInstance::currentInstance()->globalObject()->numberPrototype())
{
    m_primitiveValue = value;
}

ESBooleanObject::ESBooleanObject(bool value)
    : ESObject((Type)(Type::ESObject | Type::ESBooleanObject), ESVMInstance::currentInstance()->globalObject()->booleanPrototype())
{
    m_primitiveValue = value;
}

ESErrorObject::ESErrorObject(escargot::ESString* message)
    : ESObject((Type)(Type::ESObject | Type::ESErrorObject), ESVMInstance::currentInstance()->globalObject()->errorPrototype())
{
    if (message != strings->emptyString.string())
        set(strings->message, message);
    set(strings->name, strings->Error.string());
}

ReferenceError::ReferenceError(escargot::ESString* message)
    : ESErrorObject(message)
{
    set(strings->name, strings->ReferenceError.string());
    set__proto__(ESVMInstance::currentInstance()->globalObject()->referenceErrorPrototype());
}

TypeError::TypeError(escargot::ESString* message)
    : ESErrorObject(message)
{
    set(strings->name, strings->TypeError.string());
    set__proto__(ESVMInstance::currentInstance()->globalObject()->typeErrorPrototype());
}

RangeError::RangeError(escargot::ESString* message)
    : ESErrorObject(message)
{
    set(strings->name, strings->RangeError.string());
    set__proto__(ESVMInstance::currentInstance()->globalObject()->rangeErrorPrototype());
}

SyntaxError::SyntaxError(escargot::ESString* message)
    : ESErrorObject(message)
{
    set(strings->name, strings->SyntaxError.string());
    set__proto__(ESVMInstance::currentInstance()->globalObject()->syntaxErrorPrototype());
}

URIError::URIError(escargot::ESString* message)
    : ESErrorObject(message)
{
    set(strings->name, strings->URIError.string());
    set__proto__(ESVMInstance::currentInstance()->globalObject()->uriErrorPrototype());
}

EvalError::EvalError(escargot::ESString* message)
    : ESErrorObject(message)
{
    set(strings->name, strings->EvalError.string());
    set__proto__(ESVMInstance::currentInstance()->globalObject()->evalErrorPrototype());
}

ESArrayBufferObject::ESArrayBufferObject(ESPointer::Type type)
    : ESObject((Type)(Type::ESObject | Type::ESArrayBufferObject), ESVMInstance::currentInstance()->globalObject()->arrayBufferPrototype())
    , m_data(NULL)
    , m_bytelength(0)
{
    set__proto__(ESVMInstance::currentInstance()->globalObject()->arrayBufferPrototype());
}

ESArrayBufferView::ESArrayBufferView(ESPointer::Type type, ESValue __proto__)
    : ESObject((Type)(Type::ESObject | Type::ESArrayBufferView | type), __proto__)
{
}

ESValue ESTypedArrayObjectWrapper::get(int key)
{
    switch (m_arraytype) {
    case TypedArrayType::Int8Array:
        return (reinterpret_cast<ESInt8Array *>(this))->get(key);
    case TypedArrayType::Uint8Array:
        return (reinterpret_cast<ESUint8Array *>(this))->get(key);
    case TypedArrayType::Uint8ClampedArray:
        return (reinterpret_cast<ESUint8ClampedArray *>(this))->get(key);
    case TypedArrayType::Int16Array:
        return (reinterpret_cast<ESInt16Array *>(this))->get(key);
    case TypedArrayType::Uint16Array:
        return (reinterpret_cast<ESUint16Array *>(this))->get(key);
    case TypedArrayType::Int32Array:
        return (reinterpret_cast<ESInt32Array *>(this))->get(key);
    case TypedArrayType::Uint32Array:
        return (reinterpret_cast<ESUint32Array *>(this))->get(key);
    case TypedArrayType::Float32Array:
        return (reinterpret_cast<ESFloat32Array *>(this))->get(key);
    case TypedArrayType::Float64Array:
        return (reinterpret_cast<ESFloat64Array *>(this))->get(key);
    }
    RELEASE_ASSERT_NOT_REACHED();
}
bool ESTypedArrayObjectWrapper::set(int key, ESValue val)
{
    switch (m_arraytype) {
    case TypedArrayType::Int8Array:
        return (reinterpret_cast<ESInt8Array *>(this))->set(key, val);
    case TypedArrayType::Uint8Array:
        return (reinterpret_cast<ESUint8Array *>(this))->set(key, val);
    case TypedArrayType::Uint8ClampedArray:
        return (reinterpret_cast<ESUint8ClampedArray *>(this))->set(key, val);
    case TypedArrayType::Int16Array:
        return (reinterpret_cast<ESInt16Array *>(this))->set(key, val);
    case TypedArrayType::Uint16Array:
        return (reinterpret_cast<ESUint16Array *>(this))->set(key, val);
    case TypedArrayType::Int32Array:
        return (reinterpret_cast<ESInt32Array *>(this))->set(key, val);
    case TypedArrayType::Uint32Array:
        return (reinterpret_cast<ESUint32Array *>(this))->set(key, val);
    case TypedArrayType::Float32Array:
        return (reinterpret_cast<ESFloat32Array *>(this))->set(key, val);
    case TypedArrayType::Float64Array:
        return (reinterpret_cast<ESFloat64Array *>(this))->set(key, val);
    }
    RELEASE_ASSERT_NOT_REACHED();
}

ESArgumentsObject::ESArgumentsObject(ESPointer::Type type)
    : ESObject((Type)(Type::ESObject | Type::ESArgumentsObject), ESVMInstance::currentInstance()->globalObject()->objectPrototype(), 6)
{
}

ESJSONObject::ESJSONObject(ESPointer::Type type)
    : ESObject((Type)(Type::ESObject | Type::ESJSONObject), ESVMInstance::currentInstance()->globalObject()->objectPrototype(), 6)
{
}

void ESPropertyAccessorData::setGetterAndSetterTo(ESObject* obj)
{
    if (m_jsGetter || m_jsSetter) {
        ASSERT(!m_nativeGetter && !m_nativeSetter);
        if (m_jsGetter)
            obj->set(strings->get.string(), m_jsGetter);
        else
            obj->set(strings->get.string(), ESValue());
        if (m_jsSetter)
            obj->set(strings->set.string(), m_jsSetter);
        else
            obj->set(strings->set.string(), ESValue());
        return;
    }

    if (m_nativeGetter || m_nativeSetter) {
        ASSERT(!m_jsGetter && !m_jsSetter);
        obj->set(strings->writable.string(), ESValue(false));
        return;
    }
}

}
