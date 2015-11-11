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
    throw ESValue(TypeError::create());
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

ESStringData::ESStringData(double number)
{
    m_hashData.m_isHashInited =  false;

    if (number == 0) {
        operator += ('0');
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

    if (sign)
        operator += ('-');
    char* buf = builder.Finalize();
    while (*buf) {
        operator += (*buf);
        buf++;
    }

#ifdef ENABLE_ESJIT
    m_length = u16string::length();
#endif
}

uint32_t ESString::tryToUseAsIndex()
{
    const u16string& s = string();
    bool allOfCharIsDigit = true;
    uint32_t number = 0;
    for (unsigned i = 0; i < s.length(); i ++) {
        char16_t c = s[i];
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
    return ESValue::ESInvalidIndexValue;
}

ESString* ESString::substring(int from, int to) const
{
    ASSERT(0 <= from && from <= to && to <= (int)length());
    if (UNLIKELY(m_string == NULL)) {
        escargot::ESRopeString* rope = (escargot::ESRopeString *)this;
        if (to - from == 1) {
            int len_left = rope->m_left->length();
            char16_t c;
            if (to <= len_left) {
                c = rope->m_left->stringData()->c_str()[from];
            } else {
                c = rope->m_left->stringData()->c_str()[from - len_left];
            }
            if (c < ESCARGOT_ASCII_TABLE_MAX) {
                return strings->asciiTable[c].string();
            }
        }
        int len_left = rope->m_left->length();
        if (to <= len_left) {
            u16string ret(std::move(rope->m_left->stringData()->substr(from, to-from)));
            return ESString::create(std::move(ret));
        } else if (len_left <= from) {
            u16string ret(std::move(rope->m_right->stringData()->substr(from - len_left, to-from)));
            return ESString::create(std::move(ret));
        } else {
            ESString* lstr = nullptr;
            if (from == 0)
                lstr = rope->m_left;
            else {
                u16string left(std::move(rope->m_left->stringData()->substr(from, len_left - from)));
                lstr = ESString::create(std::move(left));
            }
            ESString* rstr = nullptr;
            if (to == length())
                rstr = rope->m_right;
            else {
                u16string right(std::move(rope->m_right->stringData()->substr(0, to - len_left)));
                rstr = ESString::create(std::move(right));
            }
            return ESRopeString::createAndConcat(lstr, rstr);
        }
        ensureNormalString();
    }
    if (to - from == 1) {
        if (string()[from] < ESCARGOT_ASCII_TABLE_MAX) {
            return strings->asciiTable[string()[from]].string();
        }
    }
    u16string ret(std::move(m_string->substr(from, to-from)));
    return ESString::create(std::move(ret));
}


bool ESString::match(ESPointer* esptr, RegexMatchResult& matchResult, bool testOnly, size_t startIndex) const
{
    // NOTE to build normal string(for rope-string), we should call ensureNormalString();
    ensureNormalString();

    ESRegExpObject::Option option = ESRegExpObject::Option::None;
    const u16string* regexSource;
    JSC::Yarr::BytecodePattern* byteCode = NULL;
    ESString* tmpStr;
    if (esptr->isESRegExpObject()) {
        escargot::ESRegExpObject* o = esptr->asESRegExpObject();
        regexSource = &esptr->asESRegExpObject()->source()->string();
        option = esptr->asESRegExpObject()->option();
        byteCode = esptr->asESRegExpObject()->bytecodePattern();
    } else {
        tmpStr = ESValue(esptr).toString();
        regexSource = tmpStr->stringData();
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
    if (length) {
        size_t start = startIndex;
        unsigned result = 0;
        const char16_t* chars = m_string->data();
        unsigned* outputBuf = (unsigned int*)alloca(sizeof(unsigned) * 2 * (subPatternNum + 1));
        outputBuf[1] = start;
        do {
            start = outputBuf[1];
            memset(outputBuf, -1, sizeof(unsigned) * 2 * (subPatternNum + 1));
            if (start > length)
                break;
            result = JSC::Yarr::interpret(NULL, byteCode, chars, length, start, outputBuf);
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
    }
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
bool ESObject::DefineOwnProperty(ESValue& key, ESObject* desc, bool throwFlag)
{
    ESObject* O = this;
    bool isEnumerable = false;
    bool isConfigurable = false;
    bool isWritable = false;

    // ToPropertyDescriptor : (start) we need to seperate this part
    bool descHasEnumerable = desc->hasProperty(ESString::create(u"enumerable"));
    bool descHasConfigurable = desc->hasProperty(ESString::create(u"configurable"));
    bool descHasWritable = desc->hasProperty(ESString::create(u"writable"));
    bool descHasValue = desc->hasProperty(ESString::create(u"value"));
    bool descHasGetter = desc->hasProperty(ESString::create(u"get"));
    bool descHasSetter = desc->hasProperty(ESString::create(u"set"));
    ESValue descE = desc->get(ESString::create(u"enumerable"));
    ESValue descC = desc->get(ESString::create(u"configurable"));
    ESValue descW = desc->get(ESString::create(u"writable"));
    escargot::ESFunctionObject* descGet = NULL;
    escargot::ESFunctionObject* descSet = NULL;
    if (descHasGetter || descHasSetter) {
        if (descHasGetter) {
            ESValue get = desc->get(ESString::create(u"get")); // 8.10.5 ToPropertyDescriptor 7.a
            if (!(get.isESPointer() && get.asESPointer()->isESFunctionObject()) && !get.isUndefined())
                throw ESValue(TypeError::create(ESString::create("ToPropertyDescriptor 7.b"))); // 8.10.5 ToPropertyDescriptor 7.b
            else if (!get.isUndefined())
                descGet = get.asESPointer()->asESFunctionObject(); // 8.10.5 ToPropertyDescriptor 7.c
        }
        if (descHasSetter) {
            ESValue set = desc->get(ESString::create(u"set"));
            if (set.isESPointer() && set.asESPointer()->isESFunctionObject())
                descSet = set.asESPointer()->asESFunctionObject();
            else if (!set.isUndefined())
                throw ESValue(TypeError::create(ESString::create("ToPropertyDescriptor 8.b")));
        }
        if (descHasValue || !descW.isUndefined())
            throw ESValue(TypeError::create(ESString::create("Type error, Property cannot have [getter|setter] and [value|writable] together")));
    }
    // ToPropertyDescriptor : (end)

    // 1
    // Our ESObject::getOwnProperty differs from [[GetOwnProperty]] in Spec
    // Hence, we use OHasCurrent and propertyInfo of current instead of Property Descriptor which is return of [[GetOwnProperty]] here.
    bool OHasCurrent = true;
    size_t idx = O->hiddenClass()->findProperty(key.toString());
    ESValue current = ESValue();
    if (idx != SIZE_MAX)
        current = O->hiddenClass()->read(O, O, idx);
    else {
        current = O->getOwnProperty(key);
        if (current.isUndefined())
            OHasCurrent = false;
    }

    // 2
    bool extensible = O->isExtensible();

    // 3, 4
    if (!OHasCurrent) {
        // 3
        if (!extensible) {
            if (throwFlag)
                throw ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty")));
            else
                return false;
        } else { // 4
            if (escargot::PropertyDescriptor::IsDataDescriptor(desc) || escargot::PropertyDescriptor::IsGenericDescriptor(desc)) {
                // Refer to Table 7 of ES 5.1 for default attribute values
                O->defineDataProperty(key, descW.isUndefined() ? isWritable : descW.toBoolean(),
                    descE.isUndefined() ? isEnumerable : descE.toBoolean(),
                    descC.isUndefined() ? isConfigurable : descC.toBoolean(), desc->get(ESString::create(u"value")));
            } else {
                ASSERT(escargot::PropertyDescriptor::IsAccessorDescriptor(desc));
                O->defineAccessorProperty(key, new ESPropertyAccessorData(descGet, descSet), descW.isUndefined() ? isWritable : descW.asBoolean(),
                    descE.isUndefined() ? isEnumerable : descE.asBoolean(),
                    descC.isUndefined() ? isConfigurable : descC.toBoolean());
            }
            return true;
        }
    }

    // 5
    if (!descHasEnumerable && !descHasWritable && !descHasConfigurable && !descHasValue && !descHasGetter && !descHasSetter)
        return true;

    // 7
    const ESHiddenClassPropertyInfo& propertyInfo = O->hiddenClass()->propertyInfo(idx);
    if (!propertyInfo.m_flags.m_isConfigurable) {
        if (descHasConfigurable && descC.toBoolean()) {
            if (throwFlag)
                throw ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty 7.a")));
            else
                return false;
        } else {
            if (descHasEnumerable && propertyInfo.m_flags.m_isEnumerable != descE.toBoolean()) {
                if (throwFlag)
                    throw ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty 7.b")));
                else
                    return false;
            }
        }
    }

    // 8, 9, 10
    bool isCurrenDataDescriptor = escargot::PropertyDescriptor::IsDataDescriptor(current);
    bool isDescDataDescriptor = escargot::PropertyDescriptor::IsDataDescriptor(desc);
    if (escargot::PropertyDescriptor::IsGenericDescriptor(desc)) {

    } if (escargot::PropertyDescriptor::IsDataDescriptor(current) != escargot::PropertyDescriptor::IsDataDescriptor(desc)) {
        if (!propertyInfo.m_flags.m_isConfigurable) {
            if (throwFlag)
                throw ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty 9.a")));
            else
                return false;
        }
    } else {
        if (!propertyInfo.m_flags.m_isConfigurable) {
            if (!propertyInfo.m_flags.m_isWritable) {
                if (descW.toBoolean()) {
                    if (throwFlag)
                        throw ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty 10.a.i")));
                    else
                        return false;
                } else {
//                    (descHasValue && current != )
                    RELEASE_ASSERT_NOT_REACHED();
                }
            }
        }
    }

    //

    // 12
    if (descHasGetter || descHasSetter) {
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
        O->defineAccessorProperty(key, new ESPropertyAccessorData(getter, setter),
            descHasWritable ? descW.toBoolean() : propertyInfo.m_flags.m_isWritable,
            descHasEnumerable ? descE.toBoolean(): propertyInfo.m_flags.m_isEnumerable,
            descHasConfigurable ? descC.toBoolean() : propertyInfo.m_flags.m_isConfigurable);
    } else {
        if (descHasValue)
            O->set(key, desc->get(ESString::create(u"value")));
        if (descHasEnumerable)
            O->hiddenClass()->setEnumerable(idx, descE.toBoolean());
        if (descHasConfigurable)
            O->hiddenClass()->setConfigurable(idx, descC.toBoolean());
        if (descHasWritable)
            O->hiddenClass()->setWritable(idx, descW.toBoolean());
    }

    // 13
    return true;
}

const unsigned ESArrayObject::MAX_FASTMODE_SIZE;

ESArrayObject::ESArrayObject(int length)
    : ESObject((Type)(Type::ESObject | Type::ESArrayObject), ESVMInstance::currentInstance()->globalObject()->arrayPrototype(), 3)
    , m_vector(0)
    , m_fastmode(true)
{
    m_length = 0;
    if (length == -1)
        convertToSlowMode();
    else if (length > 0) {
        setLength(length);
    }

    // defineAccessorProperty(strings->length.string(), ESVMInstance::currentInstance()->arrayLengthAccessorData(), true, false, false);
    m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForArrayObject();
    m_hiddenClassData.push_back((ESPointer *)ESVMInstance::currentInstance()->arrayLengthAccessorData());
}

// ES 5.1: 15.4.5.1
bool ESArrayObject::DefineOwnProperty(ESValue& key, ESObject* desc, bool throwFlag)
{
    ESArrayObject* A = this;
    ESObject* O = this;
    bool isEnumerable = false;
    bool isConfigurable = false;
    bool isWritable = false;

    // ToPropertyDescriptor : (start) we need to seperate this part
    bool descHasEnumerable = desc->hasProperty(ESString::create(u"enumerable"));
    bool descHasConfigurable = desc->hasProperty(ESString::create(u"configurable"));
    bool descHasWritable = desc->hasProperty(ESString::create(u"writable"));
    bool descHasValue = desc->hasProperty(ESString::create(u"value"));
    bool descHasGetter = desc->hasProperty(ESString::create(u"get"));
    bool descHasSetter = desc->hasProperty(ESString::create(u"set"));
    ESValue descE = desc->get(ESString::create(u"enumerable"));
    ESValue descC = desc->get(ESString::create(u"configurable"));
    ESValue descW = desc->get(ESString::create(u"writable"));
    escargot::ESFunctionObject* descGet = NULL;
    escargot::ESFunctionObject* descSet = NULL;
    if (descHasGetter || descHasSetter) {
        if (descHasGetter) {
            ESValue get = desc->get(ESString::create(u"get")); // 8.10.5 ToPropertyDescriptor 7.a
            if (!(get.isESPointer() && get.asESPointer()->isESFunctionObject()) && !get.isUndefined())
                throw ESValue(TypeError::create(ESString::create("ToPropertyDescriptor 7.b"))); // 8.10.5 ToPropertyDescriptor 7.b
            else if (!get.isUndefined())
                descGet = get.asESPointer()->asESFunctionObject(); // 8.10.5 ToPropertyDescriptor 7.c
        }
        if (descHasSetter) {
            ESValue set = desc->get(ESString::create(u"set"));
            if (set.isESPointer() && set.asESPointer()->isESFunctionObject())
                descSet = set.asESPointer()->asESFunctionObject();
            else if (!set.isUndefined())
                throw ESValue(TypeError::create(ESString::create("ToPropertyDescriptor 8.b")));
        }
        if (descHasValue || !descW.isUndefined())
            throw ESValue(TypeError::create(ESString::create("Type error, Property cannot have [getter|setter] and [value|writable] together")));
    }
    // ToPropertyDescriptor : (end)

    // 1
    ESValue oldLenDesc = A->getOwnProperty(ESString::create(u"length"));
    ASSERT(!oldLenDesc.isUndefined());
//    ASSERT(!escargot::PropertyDescriptor::IsAccessorDescriptor(oldLenDesc)); // Our implementation differs from Spec so that length property is accessor property.
    ASSERT(O->hiddenClass()->findProperty(ESString::create(u"length")) == 1);
    const ESHiddenClassPropertyInfo& oldLePropertyInfo = O->hiddenClass()->propertyInfo(1);

    // 2
    int32_t oldLen = oldLenDesc.toInt32();

    // 3
    if (key.toString()->string() == u"length") {
        ESValue descV = desc->get(ESString::create(u"value"));
        // a
        if (!descHasValue) {
            return A->asESObject()->DefineOwnProperty(key, desc, throwFlag);
        }
        // b
        ESObject* newLenDesc = ESObject::create();
        if (descHasEnumerable)
            newLenDesc->set(ESString::create(u"eunumerable"), descE);
        if (descHasWritable)
                    newLenDesc->set(ESString::create(u"writable"), descW);
        if (descHasConfigurable)
                    newLenDesc->set(ESString::create(u"configurable"), descC);
        if (descHasValue)
                    newLenDesc->set(ESString::create(u"value"), descV);
        if (descHasGetter)
                    newLenDesc->set(ESString::create(u"get"), descGet);
        if (descHasSetter)
                    newLenDesc->set(ESString::create(u"set"), descSet);

        // c
        uint32_t newLen = descV.toUint32();

        // d
        if (newLen != descV.toNumber())
            throw ESValue(RangeError::create(ESString::create("ArrayObject.DefineOwnProperty 3.d")));

        // e
        newLenDesc->set(ESString::create(u"value"), ESValue(newLen));

        // f
        if (newLen >= oldLen)
            return A->asESObject()->DefineOwnProperty(key, newLenDesc, throwFlag);

        // g
        if (!oldLePropertyInfo.m_flags.m_isWritable) {
            if (throwFlag)
                throw ESValue(TypeError::create(ESString::create("Type error, ArrayObject.DefineOwnProperty 3.g")));
            else
                return false;
        }

        // h
        bool newLenDescHasWritable = newLenDesc->hasProperty(ESString::create(u"writable"));
        bool newLenDescW = newLenDesc->get(ESString::create(u"writable")).toBoolean();
        bool newWritable;
        if (!newLenDescHasWritable || newLenDescW)
            newWritable = true;
        else {
            newWritable = false;
            RELEASE_ASSERT_NOT_REACHED();
        }

        // j
        bool succeeded = A->asESObject()->DefineOwnProperty(key, newLenDesc, throwFlag);

        // k
        if (!succeeded)
            return false;

        // l
        while (newLen < oldLen) {
            oldLen--;
            bool deleteSucceeded = A->deleteProperty(ESValue(oldLen).toString());
            if (!deleteSucceeded) {
                newLenDesc->set(ESString::create(u"value"), ESValue(oldLen+1));
                if (!newWritable)
                    newLenDesc->set(ESString::create(u"writable"), ESValue(false));
                A->asESObject()->DefineOwnProperty(key, newLenDesc, false);
                if (throwFlag)
                    throw ESValue(TypeError::create(ESString::create("Type error, ArrayObject.DefineOwnProperty 3.l.iii")));
                else
                    return false;
            }
        }

        if (!newWritable)
            RELEASE_ASSERT_NOT_REACHED();

        return true;
    }

    // 1
    // Our ESObject::getOwnProperty differs from [[GetOwnProperty]] in Spec
    // Hence, we use OHasCurrent and propertyInfo of current instead of Property Descriptor which is return of [[GetOwnProperty]] here.
    bool OHasCurrent = true;
    size_t idx = O->hiddenClass()->findProperty(key.toString());
    ESValue current = ESValue();
    if (idx != SIZE_MAX)
        current = O->hiddenClass()->read(O, O, idx);
    else {
        if (O->isESArrayObject() && (descHasEnumerable || descHasWritable || descHasConfigurable)) {
            if (descHasGetter || descHasSetter) {
                O->defineAccessorProperty(key, new ESPropertyAccessorData(descGet, descSet), descW.isUndefined() ? isWritable : descW.asBoolean(),
                    descE.isUndefined() ? isEnumerable : descE.asBoolean(),
                    descC.isUndefined() ? isConfigurable : descC.toBoolean());
            } else {
                O->defineDataProperty(key, descW.isUndefined() ? isWritable : descW.toBoolean(),
                    descE.isUndefined() ? isEnumerable : descE.toBoolean(),
                    descC.isUndefined() ? isConfigurable : descC.toBoolean(), desc->get(ESString::create(u"value")));
            }
            return true;
        } else {
            current = O->getOwnProperty(key);
            if (current.isUndefined())
                OHasCurrent = false;
        }
    }

    // 2
    bool extensible = O->isExtensible();

    // 3, 4
    if (!OHasCurrent) {
        // 3
        if (!extensible) {
            if (throwFlag)
                throw ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty")));
            else
                return false;
        } else { // 4
            if (escargot::PropertyDescriptor::IsDataDescriptor(desc) || escargot::PropertyDescriptor::IsGenericDescriptor(desc)) {
                // Refer to Table 7 of ES 5.1 for default attribute values
                O->defineDataProperty(key, descW.isUndefined() ? isWritable : descW.toBoolean(),
                    descE.isUndefined() ? isEnumerable : descE.toBoolean(),
                    descC.isUndefined() ? isConfigurable : descC.toBoolean(), desc->get(ESString::create(u"value")));
            } else {
                ASSERT(escargot::PropertyDescriptor::IsAccessorDescriptor(desc));
                O->defineAccessorProperty(key, new ESPropertyAccessorData(descGet, descSet), descW.isUndefined() ? isWritable : descW.asBoolean(),
                    descE.isUndefined() ? isEnumerable : descE.asBoolean(),
                    descC.isUndefined() ? isConfigurable : descC.toBoolean());
            }
            return true;
        }
    }

    // 7
    const ESHiddenClassPropertyInfo& propertyInfo = O->hiddenClass()->propertyInfo(idx);
    if (!propertyInfo.m_flags.m_isConfigurable) {
        if (descHasConfigurable && descC.toBoolean()) {
            if (throwFlag)
                throw ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty 7.a")));
            else
                return false;
        } else {
            if (descHasEnumerable && propertyInfo.m_flags.m_isEnumerable != descE.toBoolean()) {
                if (throwFlag)
                    throw ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty 7.b")));
                else
                    return false;
            }
        }
    }

    // 9
    bool isArrayLength = false;
    if (O->isESArrayObject() && idx == 1)
        isArrayLength = true;
    if ((escargot::PropertyDescriptor::IsDataDescriptor(current) || isArrayLength) != escargot::PropertyDescriptor::IsDataDescriptor(desc)) {
        if (!propertyInfo.m_flags.m_isConfigurable) {
            if (throwFlag)
                throw ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty 9.a")));
            else
                return false;
        }
    }

    // 12
    if (descHasGetter || descHasSetter) {
        ESPropertyAccessorData* currentAccessorData = O->accessorData(idx);
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
        O->defineAccessorProperty(key, new ESPropertyAccessorData(getter, setter),
            descHasWritable ? descW.toBoolean() : propertyInfo.m_flags.m_isWritable,
            descHasEnumerable ? descE.toBoolean(): propertyInfo.m_flags.m_isEnumerable,
            descHasConfigurable ? descC.toBoolean() : propertyInfo.m_flags.m_isConfigurable);
    } else {
        if (descHasValue) {
            ESValue val = desc->get(ESString::create(u"value"));
            if (isArrayLength)
                if (O->asESArrayObject()->length() != val.toInt32())
                    throw ESValue(TypeError::create(ESString::create("Type error, DefineOwnProperty cannot change array's length")));
            O->set(key, val);
        }
        if (descHasEnumerable)
            O->hiddenClass()->setEnumerable(idx, descE.toBoolean());
        if (descHasConfigurable)
            O->hiddenClass()->setConfigurable(idx, descC.toBoolean());
        if (descHasWritable)
            O->hiddenClass()->setWritable(idx, descW.toBoolean());
    }

    // 13
    return true;
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

    defineAccessorProperty(escargot::ESString::create(u"ignoreCase"), [](ESObject* self, ESObject* originalObj) -> ESValue {
        return ESValue((bool)(self->asESRegExpObject()->option() & ESRegExpObject::Option::IgnoreCase));
    }, nullptr, true, false, false);

    defineAccessorProperty(escargot::ESString::create(u"global"), [](ESObject* self, ESObject* originalObj) -> ESValue {
        return ESValue((bool)(self->asESRegExpObject()->option() & ESRegExpObject::Option::Global));
    }, nullptr, true, false, false);

    defineAccessorProperty(escargot::ESString::create(u"multiline"), [](ESObject* self, ESObject* originalObj) -> ESValue {
        return ESValue((bool)(self->asESRegExpObject()->option() & ESRegExpObject::Option::MultiLine));
    }, nullptr, true, false, false);

    defineAccessorProperty(strings->lastIndex.string(), [](ESObject* self, ESObject* originalObj) -> ESValue {
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
    m_yarrPattern = new JSC::Yarr::YarrPattern(src->string(), m_option & ESRegExpObject::Option::IgnoreCase, m_option & ESRegExpObject::Option::MultiLine, &yarrError);
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
    m_nonConstructor = false;
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
    m_is_bound_func = false;
}

ESFunctionObject::ESFunctionObject(LexicalEnvironment* outerEnvironment, NativeFunctionType fn, escargot::ESString* name, unsigned length, bool isConstructor)
    : ESFunctionObject(outerEnvironment, (CodeBlock *)NULL, name, length, isConstructor)
{
    m_codeBlock = CodeBlock::create(true);
    m_codeBlock->pushCode(ExecuteNativeFunction(fn));
#ifdef ENABLE_ESJIT
    m_codeBlock->m_dontJIT = true;
#endif
    m_name = name;
    if (!isConstructor)
        m_nonConstructor = true;
    m_is_bound_func = false;
}

ALWAYS_INLINE void functionCallerInnerProcess(ExecutionContext* newEC, ESFunctionObject* fn, ESValue& receiver, ESValue arguments[], const size_t& argumentCount, ESVMInstance* ESVMInstance)
{
    bool strict = fn->codeBlock()->shouldUseStrictMode();
    newEC->setStrictMode(strict);

    // http://www.ecma-international.org/ecma-262/6.0/#sec-ordinarycallbindthis
    if (!strict) {
        if (receiver.isUndefinedOrNull()) {
            receiver = ESVMInstance->globalObject();
        } else {
            receiver = receiver.toObject();
        }
    }

    ExecutionContext* currentExecutionContext = ESVMInstance->currentExecutionContext();
    ((FunctionEnvironmentRecord *)currentExecutionContext->environment()->record())->bindThisValue(receiver);
    DeclarativeEnvironmentRecord* functionRecord = currentExecutionContext->environment()->record()->toDeclarativeEnvironmentRecord();

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
        ESValue* buf = currentExecutionContext->cachedDeclarativeEnvironmentRecordESValue();
        memcpy(buf, arguments, sizeof(ESValue) * (std::min(params.size(), argumentCount)));
        if (argumentCount < params.size()) {
            std::fill(&buf[argumentCount], &buf[params.size()], ESValue());
        }
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
    ESJIT::JITFunction jitFunction = fn->codeBlock()->m_cachedJITFunction;

    if (!jitFunction && !fn->codeBlock()->m_dontJIT) {

        // update profile data
        char* code = fn->codeBlock()->m_code.data();
        size_t siz = fn->codeBlock()->m_byteCodeIndexesHaveToProfile.size();
        for (unsigned i = 0; i < siz; i ++) {
            size_t pos = fn->codeBlock()->m_extraData[fn->codeBlock()->m_byteCodeIndexesHaveToProfile[i]].m_codePosition;
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
            bool dontJIT = false;
            // check jit support for debug
#ifndef NDEBUG
            {
                char* code = fn->codeBlock()->m_code.data();
                ByteCode* currentCode;
                bool compileNextTime = false;
                char* end = &fn->codeBlock()->m_code.data()[fn->codeBlock()->m_code.size()];
                while (&code[idx] < end) {
                    Opcode opcode = fn->codeBlock()->m_extraData[bytecodeCounter].m_opcode;
                    switch (opcode) {
#define DECLARE_EXECUTE_NEXTCODE(opcode, pushCount, popCount, peekCount, JITSupported, hasProfileData) \
                    case opcode##Opcode: \
                        if (!JITSupported) { \
                            dontJIT = true; \
                            LOG_VJ("> Unsupported ByteCode %s (idx %u). Stop trying JIT.\n", #opcode, (unsigned)idx); \
                            break; \
                        } \
                        idx += sizeof(opcode); \
                        bytecodeCounter++; \
                        break;
                        FOR_EACH_BYTECODE_OP(DECLARE_EXECUTE_NEXTCODE);
#undef DECLARE_EXECUTE_NEXTCODE
                    case OpcodeKindEnd:
                        break;
                    }

                    if (dontJIT) {
                        fn->codeBlock()->m_dontJIT = true;
                        compileNextTime = true;
                        break;
                    }
                }
            }
#endif
            bool compileNextTime = false;
            // check profile data
            char* code = fn->codeBlock()->m_code.data();
            for (unsigned i = 0; i < fn->codeBlock()->m_byteCodeIndexesHaveToProfile.size(); i ++) {
                size_t pos = fn->codeBlock()->m_extraData[fn->codeBlock()->m_byteCodeIndexesHaveToProfile[i]].m_codePosition;
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
                GC_disable();
                jitFunction = reinterpret_cast<ESJIT::JITFunction>(ESJIT::JITCompile(fn->codeBlock(), instance));
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
                }
                ESJIT::ESJITAllocator::freeAll();
                GC_enable();
            } else {
                size_t threshold = fn->codeBlock()->m_jitThreshold;
                LOG_VJ("> Doubling JIT compilation threshold from %d to %d for function %s\n", threshold, threshold*2, functionName);
                fn->codeBlock()->m_jitThreshold *= 2;
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

        result = ESValue::fromRawDouble(jitFunction(instance));
        // printf("JIT Result %s\n", result.toString()->utf8Data());
        if (ec.inOSRExit()) {
            fn->codeBlock()->m_osrExitCount++;
            LOG_VJ("> OSR Exit from function %s (codeBlock %p), exit count %zu\n", functionName, fn->codeBlock(), fn->codeBlock()->m_osrExitCount);
            int32_t tmpIndex = result.asInt32();
            char* code = fn->codeBlock()->m_code.data();
            size_t idx = 0;
            size_t bytecodeCounter = 0;
            unsigned maxStackPos = 0;
            char* end = &fn->codeBlock()->m_code.data()[fn->codeBlock()->m_code.size()];
            while (&code[idx] < end) {
                ByteCode* currentCode = (ByteCode *)(&code[idx]);
                Opcode opcode = fn->codeBlock()->m_extraData[bytecodeCounter].m_opcode;
                ByteCodeExtraData* extraData = &fn->codeBlock()->m_extraData[bytecodeCounter];
                if (extraData->m_targetIndex0 == tmpIndex) {
                    maxStackPos = ec.getStackPos();
                    break;
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
#ifndef NDEBUG
                if (ESVMInstance::currentInstance()->m_reportOSRExitedFunction) {
                    printf("%s ", fn->codeBlock()->m_nonAtomicId ? (fn->codeBlock()->m_nonAtomicId->utf8Data()):"(anonymous)");
                    ESVMInstance::currentInstance()->m_osrExitedFunctions++;
                }
#endif
            }
            result = interpret(instance, fn->codeBlock(), idx, maxStackPos);
            fn->codeBlock()->m_executeCount++;
        }
    } else {
        result = interpret(instance, fn->codeBlock());
        fn->codeBlock()->m_executeCount++;
    }

    return result;
}

#endif

ESValue ESFunctionObject::call(ESVMInstance* instance, const ESValue& callee, const ESValue& receiverInput, ESValue arguments[], const size_t& argumentCount, bool isNewExpression)
{
    ESValue result(ESValue::ESForceUninitialized);
    if (LIKELY(callee.isESPointer() && callee.asESPointer()->isESFunctionObject())) {
        ESValue receiver = receiverInput;
        ExecutionContext* currentContext = instance->currentExecutionContext();
        ESFunctionObject* fn = callee.asESPointer()->asESFunctionObject();

        if (UNLIKELY(fn->codeBlock()->m_needsActivation)) {
            instance->m_currentExecutionContext = new ExecutionContext(LexicalEnvironment::newFunctionEnvironment(arguments, argumentCount, fn), true, isNewExpression,
                currentContext,
                arguments, argumentCount);
            functionCallerInnerProcess(instance->m_currentExecutionContext, fn, receiver, arguments, argumentCount, instance);
            // ESVMInstance->invalidateIdentifierCacheCheckCount();
            // execute;
#ifdef ENABLE_ESJIT
            result = executeJIT(fn, instance, *instance->m_currentExecutionContext);
#else
            result = interpret(instance, fn->codeBlock());
#endif
            instance->m_currentExecutionContext = currentContext;
        } else {
            ESValue* storage = (::escargot::ESValue *)alloca(sizeof(::escargot::ESValue) * fn->m_codeBlock->m_innerIdentifiers.size());
            FunctionEnvironmentRecord envRec(
                arguments, argumentCount,
                storage,
                &fn->m_codeBlock->m_innerIdentifiers);

            // envRec.m_functionObject = fn;
            // envRec.m_newTarget = receiver;

            LexicalEnvironment env(&envRec, fn->outerEnvironment());
            ExecutionContext ec(&env, false, isNewExpression, currentContext, arguments, argumentCount, storage);
            instance->m_currentExecutionContext = &ec;
            functionCallerInnerProcess(&ec, fn, receiver, arguments, argumentCount, instance);
            // ESVMInstance->invalidateIdentifierCacheCheckCount();
            // execute;
#ifdef ENABLE_ESJIT
            result = executeJIT(fn, instance, ec);
#else
            result = interpret(instance, fn->codeBlock());
#endif
            instance->m_currentExecutionContext = currentContext;
        }
    } else {
        throw ESValue(TypeError::create(ESString::create(u"Callee is not a function object")));
    }

    return result;
}

ESDateObject::ESDateObject(ESPointer::Type type)
    : ESObject((Type)(Type::ESObject | Type::ESDateObject), ESVMInstance::currentInstance()->globalObject()->datePrototype())
{
    m_isCacheDirty = true;
}

void ESDateObject::parseYmdhmsToDate(struct tm* timeinfo, int year, int month, int date, int hour, int minute, int second)
{
    char buffer[255];
    snprintf(buffer, 255, "%d-%d-%d-%d-%d-%d", year, month + 1, date, hour, minute, second);
    strptime(buffer, "%Y-%m-%d-%H-%M-%S", timeinfo);
}

void ESDateObject::parseStringToDate(struct tm* timeinfo, escargot::ESString* istr)
{
    int len = istr->length();
    char* buffer = (char*)istr->utf8Data();
    if (isalpha(buffer[0])) {
        strptime(buffer, "%B %d %Y %H:%M:%S %z", timeinfo);
    } else if (isdigit(buffer[0])) {
        strptime(buffer, "%m/%d/%Y %H:%M:%S", timeinfo);
    }
    GC_free(buffer);
}

const double hoursPerDay = 24.0;
const double minutesPerHour = 60.0;
const double secondsPerHour = 60.0 * 60.0;
const double secondsPerMinute = 60.0;
const double msPerSecond = 1000.0;
const double msPerMinute = 60.0 * 1000.0;
const double msPerHour = 60.0 * 60.0 * 1000.0;
const double msPerDay = 24.0 * 60.0 * 60.0 * 1000.0;
const double msPerMonth = 2592000000.0;

static inline double ymdhmsToSeconds(long year, int mon, int day, int hour, int minute, double second)
{
    double days = (day - 32075)
        + floor(1461 * (year + 4800.0 + (mon - 14) / 12) / 4)
        + 367 * (mon - 2 - (mon - 14) / 12 * 12) / 12
        - floor(3 * ((year + 4900.0 + (mon - 14) / 12) / 100) / 4)
        - 2440588;
    return ((days * hoursPerDay + hour) * minutesPerHour + minute) * secondsPerMinute + second;
}


void ESDateObject::setTimeValue()
{
    clock_gettime(CLOCK_REALTIME, &m_time);
    m_isCacheDirty = true;
}

void ESDateObject::setTimeValue(const ESValue str)
{
    escargot::ESString* istr = str.toString();
    parseStringToDate(&m_cachedTM, istr);
    m_cachedTM.tm_isdst = true;
    m_time.tv_sec = ymdhmsToSeconds(m_cachedTM.tm_year+1900, m_cachedTM.tm_mon + 1, m_cachedTM.tm_mday, m_cachedTM.tm_hour, m_cachedTM.tm_min, m_cachedTM.tm_sec);
}

void ESDateObject::setTimeValue(int year, int month, int date, int hour, int minute, int second, int millisecond)
{
    parseYmdhmsToDate(&m_cachedTM, year, month, date, hour, minute, second);
    m_cachedTM.tm_isdst = true;
    m_time.tv_sec = ymdhmsToSeconds(m_cachedTM.tm_year+1900, m_cachedTM.tm_mon + 1, m_cachedTM.tm_mday, m_cachedTM.tm_hour, m_cachedTM.tm_min, m_cachedTM.tm_sec);
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

int ESDateObject::getTimezoneOffset()
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
    if (std::isnan(m_primitiveValue)) {
        return NULL;
    } else {
        time_t raw_t = (time_t) floor(m_primitiveValue);
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
    escargot::ESFunctionObject* fn = ESVMInstance::currentInstance()->globalObject()->error();
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
            obj->set(ESString::create(u"get"), m_jsGetter);
        else
            obj->set(ESString::create(u"get"), ESValue());
        if (m_jsSetter)
            obj->set(ESString::create(u"set"), m_jsSetter);
        else
            obj->set(ESString::create(u"set"), ESValue());
        return;
    }

    if (m_nativeGetter || m_nativeSetter) {
        ASSERT(!m_jsGetter && !m_jsSetter);
        obj->set(ESString::create(u"writable"), ESValue(false));
        return;
    }
}

}
