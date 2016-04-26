#include "Escargot.h"
#include "ESValue.h"

#include "parser/esprima.h"
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

const char* errorMessage_DefineProperty_Default = "Cannot define property '%s'";
const char* errorMessage_DefineProperty_LengthNotWritable = "Cannot modify property '%s': 'length' is not writable";
const char* errorMessage_DefineProperty_NotWritable = "Cannot modify non-writable property '%s'";
const char* errorMessage_DefineProperty_RedefineNotConfigurable = "Cannot redefine non-configurable property '%s'";
const char* errorMessage_DefineProperty_NotExtensible ="Cannot define property '%s': object is not extensible";
const char* errorMessage_ObjectToPrimitiveValue = "Cannot convert object to primitive value";
const char* errorMessage_NullToObject = "cannot convert null into object";
const char* errorMessage_UndefinedToObject = "cannot convert undefined into object";
const char* errorMessage_Call_NotFunction = "Callee is not a function object";
const char* errorMessage_Get_FromUndefined = "Cannot get property '%s' of undefined";
const char* errorMessage_Get_FromNull = "Cannot get property '%s' of null";
const char* errorMessage_Set_ToUndefined = "Cannot set property '%s' of undefined";
const char* errorMessage_Set_ToNull = "Cannot set property '%s' of null";
const char* errorMessage_ArgumentsOrCaller_InStrictMode = "'caller' and 'arguments' are restricted function properties and cannot be accessed in this context.";

NEVER_INLINE bool reject(bool throwFlag, ESErrorObject::Code code, const char* templateString, ESString* property)
{
    if (throwFlag) {
        ESVMInstance::currentInstance()->throwError(code, templateString, property);
    }
    return false;
}

ESHiddenClassPropertyInfo dummyPropertyInfo(nullptr, Data | PropertyDescriptor::defaultAttributes);
ESHiddenClassPropertyInfo::ESHiddenClassPropertyInfo()
{
    m_name = NULL;
    m_attributes = Data | Deleted;
}

unsigned ESHiddenClassPropertyInfo::buildAttributes(unsigned property, bool writable, bool enumerable, bool configurable)
{
    unsigned attributes = property;

    if (writable)
        attributes |= Writable;
    if (enumerable)
        attributes |= Enumerable;
    if (configurable)
        attributes |= Configurable;

    return attributes;
}

unsigned ESHiddenClassPropertyInfo::hiddenClassPopretyInfoVecIndex(bool isData, bool writable, bool enumerable, bool configurable)
{
    return ESHiddenClassPropertyInfo::buildAttributes(None, writable, enumerable, configurable) | isData;
}

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
    ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create(errorMessage_ObjectToPrimitiveValue))));
    RELEASE_ASSERT_NOT_REACHED();
}

ESString* ESValue::toStringSlowCase(bool emptyStringOnError) const
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
        if (emptyStringOnError) {
            static bool tryPositionRegistered = false;
            std::jmp_buf tryPosition;
            if (tryPositionRegistered) {
                return toPrimitive(PreferString).toString();
            }
            if (setjmp(ESVMInstance::currentInstance()->registerTryPos(&tryPosition)) == 0) {
                tryPositionRegistered = true;
                ESString* ret = toPrimitive(PreferString).toString();
                ESVMInstance::currentInstance()->unregisterTryPos(&tryPosition);
                ESVMInstance::currentInstance()->unregisterCheckedObjectAll();
                tryPositionRegistered = false;
                return ret;
            } else {
                escargot::ESValue err = ESVMInstance::currentInstance()->getCatchedError();
                if (err.isESPointer() && err.asESPointer()->isESErrorObject()
                    && err.asESPointer()->asESErrorObject()->errorCode() == ESErrorObject::Code::RangeError) {
                    tryPositionRegistered = false;
                    return strings->emptyString.string();
                } else {
                    ESVMInstance::currentInstance()->throwError(err);
                }
            }
            RELEASE_ASSERT_NOT_REACHED();
        } else {
            return toPrimitive(PreferString).toString();
        }
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
            if (val.asESPointer()->isESDateObject())
                return abstractEqualsTo(val.toPrimitive(ESValue::PreferString));
            else
                return abstractEqualsTo(val.toPrimitive());
        } else if (isObject() && (val.isESString() || val.isNumber())) {
            // If Type(x) is Object and Type(y) is either String, Number, or Symbol, then
            if (asESPointer()->isESDateObject())
                return toPrimitive(ESValue::PreferString).abstractEqualsTo(val);
            else
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

ASCIIString dtoa(double number)
{
    ASCIIString str;
    if (number == 0) {
        str.append({'0'});
        return std::move(str);
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
        str += '-';
    char* buf = builder.Finalize();
    while (*buf) {
        str += *buf;
        buf++;
    }
    return std::move(str);
}

uint32_t ESString::tryToUseAsIndex()
{
    if (isASCIIString()) {
        bool allOfCharIsDigit = true;
        uint32_t number = 0;
        size_t len = length();
        const char* data = asciiData();

        if (UNLIKELY(len == 0)) {
            return ESValue::ESInvalidIndexValue;
        }

        if (len > 1) {
            if (data[0] == '0') {
                return ESValue::ESInvalidIndexValue;
            }
        }

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
        const char16_t* data = utf16Data();
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
    if (to - from == 1) {
        char16_t c;
        c = charAt(from);
        if (c < ESCARGOT_ASCII_TABLE_MAX) {
            return strings->asciiTable[c].string();
        } else {
            return ESString::create(c);
        }
    }

    if (isASCIIString()) {
        return ESString::create(asASCIIString()->substr(from, to-from));
    } else {
        return ESString::create(asUTF16String()->substr(from, to-from));
    }
}

size_t ESString::createRegexMatchResult(escargot::ESRegExpObject* regexp, RegexMatchResult& result)
{
    size_t len = 0, previousLastIndex = 0;
    bool testResult;
    RegexMatchResult temp;
    temp.m_matchResults.push_back(result.m_matchResults[0]);
    result.m_matchResults.clear();
    do {
        const size_t maximumReasonableMatchSize = 1000000000;
        if (len > maximumReasonableMatchSize) {
            ESVMInstance::currentInstance()->throwError(RangeError::create(ESString::create("Maximum Reasonable match size exceeded.")));
        }

        if (regexp->lastIndex().toIndex() == previousLastIndex) {
            regexp->set(strings->lastIndex.string(), ESValue(previousLastIndex++), true);
        } else {
            previousLastIndex = regexp->lastIndex().toIndex();
        }

        // FIXME: I am not sure when m_matchResults[i][0].m_start returned with max
        size_t end = temp.m_matchResults[0][0].m_end;
        size_t length = end - temp.m_matchResults[0][0].m_start;
        if (!length) {
            ++end;
        }

        result.m_matchResults.insert(result.m_matchResults.end(), temp.m_matchResults.begin(), temp.m_matchResults.end());
        len++;
        temp.m_matchResults.clear();
        testResult = regexp->matchNonGlobally(this, temp, false, end);
    } while (testResult);

    return len;
}

ESArrayObject* ESString::createMatchedArray(escargot::ESRegExpObject* regexp, RegexMatchResult& result)
{
    escargot::ESArrayObject* ret = ESArrayObject::create();
    size_t len = createRegexMatchResult(regexp, result);
    ret->create(len);
    for (size_t idx = 0; idx < len; idx++) {
        ret->defineDataProperty(ESValue(idx), true, true, true, substring(result.m_matchResults[idx][0].m_start, result.m_matchResults[idx][0].m_end));
    }

    return ret;
}

ESRopeString* ESRopeString::createAndConcat(ESString* lstr, ESString* rstr)
{
    size_t llen = lstr->length();
    size_t rlen = rstr->length();

    if (static_cast<int64_t>(llen) > static_cast<int64_t>(ESString::maxLength() - rlen))
        ESVMInstance::currentInstance()->throwOOMError();

    ESRopeString* rope = ESRopeString::create();
    rope->m_contentLength = llen + rlen;
    rope->m_left = lstr;
    rope->m_right = rstr;

    bool hasNonASCIIChild = false;
    if (lstr->isESRopeString()) {
        hasNonASCIIChild |= lstr->asESRopeString()->hasNonASCIIChild();
    } else {
        hasNonASCIIChild |= !lstr->isASCIIString();
    }

    if (rstr->isESRopeString()) {
        hasNonASCIIChild |= rstr->asESRopeString()->hasNonASCIIChild();
    } else {
        hasNonASCIIChild |= !rstr->isASCIIString();
    }

    rope->m_hasNonASCIIString = hasNonASCIIChild;
    return rope;
}

unsigned PropertyDescriptor::defaultAttributes = Configurable | Enumerable | Writable;

PropertyDescriptor::PropertyDescriptor(ESObject* obj)
    : m_value(ESValue::ESEmptyValue)
    , m_getter(ESValue::ESEmptyValue)
    , m_setter(ESValue::ESEmptyValue)
    , m_attributes(0)
    , m_seenAttributes(0)
{
    if (obj->hasProperty(strings->enumerable.string())) {
        setEnumerable(obj->get(strings->enumerable.string()).toBoolean());
    }
    if (obj->hasProperty(strings->configurable.string())) {
        setConfigurable(obj->get(strings->configurable.string()).toBoolean());
    }
    if (obj->hasProperty(strings->value.string())) {
        setValue(obj->get(strings->value.string()));
    }
    if (obj->hasProperty(strings->writable.string())) {
        setWritable(obj->get(strings->writable.string()).toBoolean());
    }
    if (obj->hasProperty(strings->get.string())) {
        setGetter(obj->get(strings->get.string()));
    }
    if (obj->hasProperty(strings->set.string())) {
        setSetter(obj->get(strings->set.string()));
    }
    checkValidity();
}

void PropertyDescriptor::checkValidity() const
{
    bool descHasWritable = hasWritable();
    bool descHasValue = hasValue();
    bool descHasGetter = hasGetter();
    bool descHasSetter = hasSetter();

    if (descHasGetter || descHasSetter) {
        if (descHasGetter) {
            ESValue get = m_getter; // 8.10.5 ToPropertyDescriptor 7.a
            if (!(get.isESPointer() && get.asESPointer()->isESFunctionObject()) && !get.isUndefined())
                ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Getter must be a function")))); // 8.10.5 ToPropertyDescriptor 7.b
        }
        if (descHasSetter) {
            ESValue set = m_setter; // 8.10.5 ToPropertyDescriptor 8.a
            if (!(set.isESPointer() && set.asESPointer()->isESFunctionObject()) && !set.isUndefined())
                ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Setter must be a function")))); // 8.10.5 ToPropertyDescriptor 8.b
        }
        if (descHasValue || descHasWritable)
            ESVMInstance::currentInstance()->throwError(ESValue(TypeError::create(ESString::create("Invalid property: property cannot have [getter|setter] and [value|writable] together"))));
    }
}

bool PropertyDescriptor::writable() const
{
    ASSERT(!isAccessorDescriptor());
    return (m_attributes & Writable);
}

bool PropertyDescriptor::enumerable() const
{
    return (m_attributes & Enumerable);
}

bool PropertyDescriptor::configurable() const
{
    return (m_attributes & Configurable);
}

bool PropertyDescriptor::isDataDescriptor() const
{
    return !m_value.isEmpty() || (m_seenAttributes & WritablePresent);
}

bool PropertyDescriptor::isGenericDescriptor() const
{
    return !isAccessorDescriptor() && !isDataDescriptor();
}

bool PropertyDescriptor::isAccessorDescriptor() const
{
    return !m_getter.isEmpty() || !m_setter.isEmpty();
}

ESFunctionObject* PropertyDescriptor::getterFunction() const
{
    ASSERT(isAccessorDescriptor());
    return (hasGetter() && !m_getter.isUndefined()) ? m_getter.asESPointer()->asESFunctionObject() : nullptr;
}

ESFunctionObject* PropertyDescriptor::setterFunction() const
{
    ASSERT(isAccessorDescriptor());
    return (hasSetter() && !m_setter.isUndefined()) ? m_setter.asESPointer()->asESFunctionObject() : nullptr;
}

void PropertyDescriptor::setWritable(bool writable)
{
    if (writable)
        m_attributes |= Writable;
    else
        m_attributes &= ~Writable;
    m_seenAttributes |= WritablePresent;
}

void PropertyDescriptor::setEnumerable(bool enumerable)
{
    if (enumerable)
        m_attributes |= Enumerable;
    else
        m_attributes &= ~Enumerable;
    m_seenAttributes |= EnumerablePresent;
}

void PropertyDescriptor::setConfigurable(bool configurable)
{
    if (configurable)
        m_attributes |= Configurable;
    else
        m_attributes &= ~Configurable;
    m_seenAttributes |= ConfigurablePresent;
}

void PropertyDescriptor::setSetter(ESValue setter)
{
    m_setter = setter;
    m_attributes |= JSAccessor;
    m_attributes &= ~Writable;
}

void PropertyDescriptor::setGetter(ESValue getter)
{
    m_getter = getter;
    m_attributes |= JSAccessor;
    m_attributes &= ~Writable;
}

// ES5.1 8.10.4 FromPropertyDescriptor
ESValue PropertyDescriptor::fromPropertyDescriptor(ESObject* descSrc, ESString* propertyName, size_t idx)
{
    bool isActualDataProperty = false;
    if (descSrc->isESArrayObject() && idx == 1) {
        isActualDataProperty = true;
    }
    const ESHiddenClassPropertyInfo& propertyInfo = descSrc->hiddenClass()->propertyInfo(idx);
    ESObject* obj = ESObject::create();
    if (propertyInfo.isDataProperty() || isActualDataProperty) {
        obj->set(strings->value.string(), descSrc->hiddenClass()->read(descSrc, descSrc, propertyName, idx));
        obj->set(strings->writable.string(), ESValue(propertyInfo.writable()));
    } else if (descSrc->accessorData(idx)->getJSGetter()
        || descSrc->accessorData(idx)->getJSSetter()
        || (!descSrc->accessorData(idx)->getNativeGetter() && !descSrc->accessorData(idx)->getNativeSetter())) {
        ESObject* getDesc = ESObject::create();
        getDesc->set(strings->value.string(), descSrc->accessorData(idx)->getJSGetter() ? descSrc->accessorData(idx)->getJSGetter() : ESValue());
        getDesc->set(strings->writable.string(), ESValue(true));
        getDesc->set(strings->enumerable.string(), ESValue(true));
        getDesc->set(strings->configurable.string(), ESValue(true));
        ESValue getStr = strings->get.string();
        obj->defineOwnProperty(getStr, getDesc, false);

        ESObject* setDesc = ESObject::create();
        setDesc->set(strings->value.string(), descSrc->accessorData(idx)->getJSSetter() ? descSrc->accessorData(idx)->getJSSetter() : ESValue());
        setDesc->set(strings->writable.string(), ESValue(true));
        setDesc->set(strings->enumerable.string(), ESValue(true));
        setDesc->set(strings->configurable.string(), ESValue(true));
        ESValue setStr = strings->set.string();
        obj->defineOwnProperty(setStr, setDesc, false);
    } else {
        obj->set(strings->value.string(), descSrc->hiddenClass()->read(descSrc, descSrc, propertyName, idx));
        descSrc->accessorData(idx)->setGetterAndSetterTo(obj, &propertyInfo);
    }
    obj->set(strings->enumerable.string(), ESValue(propertyInfo.enumerable()));
    obj->set(strings->configurable.string(), ESValue(propertyInfo.configurable()));
    return obj;
}

// For ArrayFastMode
ESValue PropertyDescriptor::fromPropertyDescriptorForIndexedProperties(ESObject* obj, uint32_t index)
{
    if (obj->isESArrayObject() && obj->asESArrayObject()->isFastmode()) {
        if (index != ESValue::ESInvalidIndexValue) {
            if (LIKELY(index < obj->asESArrayObject()->length())) {
                ESValue e = obj->asESArrayObject()->data()[index];
                if (LIKELY(!e.isEmpty())) {
                    ESObject* ret = ESObject::create();
                    ret->set(strings->value.string(), e);
                    ret->set(strings->writable.string(), ESValue(true));
                    ret->set(strings->enumerable.string(), ESValue(true));
                    ret->set(strings->configurable.string(), ESValue(true));
                    return ret;
                }
            }
        }
    }
    if (obj->isESStringObject()) {
        if (index != ESValue::ESInvalidIndexValue) {
            if (LIKELY(index < obj->asESStringObject()->length())) {
                ESValue e = obj->asESStringObject()->getCharacterAsString(index);
                ESObject* ret = ESObject::create();
                ret->set(strings->value.string(), e);
                ret->set(strings->writable.string(), ESValue(false));
                ret->set(strings->enumerable.string(), ESValue(true));
                ret->set(strings->configurable.string(), ESValue(false));
                return ret;
            }
        }
    }
    return ESValue();
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
    m_flags.m_extraData = 0;

    m_objectRareData = NULL;

    m_hiddenClassData.reserve(initialKeyCount);
    m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForObject();

    m_hiddenClassData.push_back(ESValue((ESPointer *)ESVMInstance::currentInstance()->object__proto__AccessorData()));

    set__proto__(__proto__);
}

void ESObject::setValueAsProtoType(const ESValue& obj)
{
    if (obj.isObject()) {
        obj.asESPointer()->asESObject()->m_flags.m_isEverSetAsPrototypeObject = true;
        obj.asESPointer()->asESObject()->m_hiddenClass->setHasEverSetAsPrototypeObjectHiddenClass();
        if (obj.asESPointer()->isESArrayObject()) {
            obj.asESPointer()->asESArrayObject()->convertToSlowMode();
        }
        if (obj.asESPointer()->asESObject()->hiddenClass()->hasIndexedProperty()) {
            ESVMInstance::currentInstance()->globalObject()->somePrototypeObjectDefineIndexedProperty();
        }
    }
}

NEVER_INLINE bool ESObject::setSlowPath(const escargot::ESValue& key, const ESValue& val, escargot::ESValue* receiver)
{
    if (UNLIKELY(hasPropertyInterceptor() && hasKeyForPropertyInterceptor(key))) {
        return false;
    }

    escargot::ESString* keyString = key.toString();
    size_t idx = m_hiddenClass->findProperty(keyString);

    if (m_flags.m_isEverSetAsPrototypeObject && keyString->hasOnlyDigit()) {
        ESVMInstance::currentInstance()->globalObject()->somePrototypeObjectDefineIndexedProperty();
    }
    if (idx == SIZE_MAX) {
        ESValue target = __proto__();
        bool foundInPrototype = false; // for GetObjectPreComputedCase vector mode cache
        while (true) {
            if (!target.isObject()) {
                break;
            }
            ESObject* targetObj = target.asESPointer()->asESObject();
            size_t t = targetObj->hiddenClass()->findProperty(keyString);
            if (t != SIZE_MAX) {
                // http://www.ecma-international.org/ecma-262/5.1/#sec-8.12.5
                // If IsAccessorDescriptor(desc) is true, then
                // Let setter be desc.[[Set]] which cannot be undefined.
                // Call the [[Call]] internal method of setter providing O as the this value and providing V as the sole argument.
                if (!targetObj->hiddenClass()->m_propertyInfo[t].isDataProperty()) {
                    if (!foundInPrototype) {
                        ESPropertyAccessorData* data = targetObj->accessorData(t);
                        if (data->isAccessorDescriptor()) {
                            if (data->getJSSetter()) {
                                ESValue receiverVal(this);
                                if (receiver)
                                    receiverVal = *receiver;
                                ESValue args[] = {val};
                                ESFunctionObject::call(ESVMInstance::currentInstance(), data->getJSSetter(), receiverVal, args, 1, false);
                                return true;
                            }
                            return false;
                        }
                        if (data->getNativeSetter()) {
                            if (!targetObj->hiddenClass()->m_propertyInfo[t].writable()) {
                                return false;
                            }
                            foundInPrototype = true;
                            break;
                        } else {
                            return false;
                        }
                    }
                } else {
                    if (!foundInPrototype) {
                        if (!targetObj->hiddenClass()->m_propertyInfo[t].writable()) {
                            return false;
                        }
                        foundInPrototype = true;
                        break;
                    }
                }
            } else if (targetObj->isESStringObject()) {
                uint32_t idx = key.toIndex();
                if (idx != ESValue::ESInvalidIndexValue)
                    if (idx < targetObj->asESStringObject()->length())
                        return false;
            }
            target = targetObj->__proto__();
        }
        if (UNLIKELY(!isExtensible()))
            return false;
        m_hiddenClass = m_hiddenClass->defineProperty(keyString, Data | PropertyDescriptor::defaultAttributes, foundInPrototype);
        m_hiddenClassData.push_back(val);

        if (UNLIKELY(m_flags.m_isGlobalObject))
            ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
        if (UNLIKELY(isESArrayObject())) {
            uint32_t index = key.toIndex();
            uint32_t oldLen = asESArrayObject()->length();
            if (index != ESValue::ESInvalidIndexValue && index >= oldLen)
                asESArrayObject()->setLength(index+1);
        }
        return true;
    } else {
        return m_hiddenClass->write(this, this, keyString, idx, val);
    }
}

bool ESObject::defineOwnProperty(const ESValue& P, const PropertyDescriptor& desc, bool throwFlag)
{
    ESObject* O = this;

    // ToPropertyDescriptor : (start) we need to seperate this part
    bool descHasEnumerable = desc.hasEnumerable();
    bool descHasConfigurable = desc.hasConfigurable();
    bool descHasWritable = desc.hasWritable();
    bool descHasValue = desc.hasValue();
    bool descHasGetter = desc.hasGetter();
    bool descHasSetter = desc.hasSetter();
    // ToPropertyDescriptor : (end)

    // 1
    // Our ESObject::getOwnProperty differs from [[GetOwnProperty]] in Spec
    // Hence, we use OHasCurrent and propertyInfo of current instead of Property Descriptor which is return of [[GetOwnProperty]] here.
    bool OHasCurrent = true;
    size_t idx = O->hiddenClass()->findProperty(P.toString());
    ESValue current = ESValue();
    if (idx != SIZE_MAX) {
        ESHiddenClassPropertyInfo info = O->hiddenClass()->propertyInfo(idx);
        if (info.isDataProperty()) {
            current = O->hiddenClassData()[idx];
        } else {
            ESPropertyAccessorData* data = (ESPropertyAccessorData *)O->hiddenClassData()[idx].asESPointer();
            if (data->getNativeGetter() || data->getNativeSetter()) {
                current = data->value(O, O, P.toString());
            } else {
                current = ESValue();
            }
        }
    } else {
        if (O->isESArrayObject()
            && (descHasEnumerable || descHasWritable || descHasConfigurable || descHasValue || descHasGetter || descHasSetter)
            && O->hasOwnProperty(P)) {
            current = O->getOwnProperty(P);
            if (descHasGetter || descHasSetter) {
                O->defineAccessorProperty(P, new ESPropertyAccessorData(desc.getterFunction(), desc.setterFunction()),
                    descHasWritable ? desc.writable() : true,
                    descHasEnumerable ? desc.enumerable() : true,
                    descHasConfigurable ? desc.configurable() : true);
            } else {
                O->defineDataProperty(P, descHasWritable ? desc.writable() : true,
                    descHasEnumerable ? desc.enumerable() : true,
                    descHasConfigurable ? desc.configurable() : true, descHasValue ? desc.value() : current);
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
    bool isDescDataDescriptor = desc.isDataDescriptor();
    bool isDescGenericDescriptor = desc.isGenericDescriptor();
    if (!OHasCurrent) {
        // 3
        if (!extensible) {
            return reject(throwFlag, ErrorCode::TypeError, errorMessage_DefineProperty_NotExtensible, P.toString());
        } else { // 4
//            O->deleteProperty(P);
            if (isDescDataDescriptor || isDescGenericDescriptor) {
                // Refer to Table 7 of ES 5.1 for default attribute values
                O->defineDataProperty(P, descHasWritable ? desc.writable() : false,
                    descHasEnumerable ? desc.enumerable() : false,
                    descHasConfigurable ? desc.configurable() : false, desc.value());
            } else {
                ASSERT(desc.isAccessorDescriptor());
                O->defineAccessorProperty(P, new ESPropertyAccessorData(desc.getterFunction(), desc.setterFunction()),
                    descHasWritable ? desc.writable() : false,
                    descHasEnumerable ? desc.enumerable() : false,
                    descHasConfigurable ? desc.configurable() : false);
            }
            return true;
        }
    }

    // 5
    if (!descHasEnumerable && !descHasWritable && !descHasConfigurable && !descHasValue && !descHasGetter && !descHasSetter)
        return true;

    // 6
    idx = O->hiddenClass()->findProperty(P.toString());
    ESHiddenClassPropertyInfo propertyInfo;
    if (idx == SIZE_MAX) {
        if (O->isESStringObject()) {
            ASSERT(P.toIndex() != ESValue::ESInvalidIndexValue);
            ASSERT(P.toIndex() < O->length());
            propertyInfo = ESHiddenClassPropertyInfo(P.toString(), Data | Enumerable);
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    } else {
        propertyInfo = O->hiddenClass()->propertyInfo(idx);
    }
    if ((!descHasEnumerable || desc.enumerable() == propertyInfo.enumerable())
        && (!descHasWritable || ((propertyInfo.isDataProperty() || O->accessorData(idx)->getNativeGetter() || O->accessorData(idx)->getNativeSetter()) && (desc.writable() == propertyInfo.writable())))
        && (!descHasConfigurable || desc.configurable() == propertyInfo.configurable())
        && (!descHasValue || ((propertyInfo.isDataProperty() || O->accessorData(idx)->getNativeGetter() || O->accessorData(idx)->getNativeSetter()) && desc.value().equalsToByTheSameValueAlgorithm(current)))
        && (!descHasGetter || (O->get(strings->get.string()).isESPointer() && O->get(strings->get.string()).asESPointer()->isESFunctionObject()
            && desc.getterFunction() == O->get(strings->get.string()).asESPointer()->asESFunctionObject()))
        && (!descHasSetter || (O->get(strings->set.string()).isESPointer() && O->get(strings->set.string()).asESPointer()->isESFunctionObject()
            && desc.setterFunction() == O->get(strings->set.string()).asESPointer()->asESFunctionObject())))
        return true;

    // 7
    if (!propertyInfo.configurable()) {
        if (descHasConfigurable && desc.configurable()) {
            return reject(throwFlag, ErrorCode::TypeError, errorMessage_DefineProperty_RedefineNotConfigurable, P.toString());
        } else {
            if (descHasEnumerable && propertyInfo.enumerable() != desc.enumerable()) {
                return reject(throwFlag, ErrorCode::TypeError, errorMessage_DefineProperty_RedefineNotConfigurable, P.toString());
            }
        }
    }

    // 8, 9, 10, 11
    bool isCurrentDataDescriptor = propertyInfo.isDataProperty() || O->accessorData(idx)->getNativeGetter() || O->accessorData(idx)->getNativeSetter();
    bool shouldRemoveOriginalNativeGetterSetter = false;
    if (isDescGenericDescriptor) { // 8
    } else if (isCurrentDataDescriptor != isDescDataDescriptor) { // 9
        if (!propertyInfo.configurable()) { // 9.a
            return reject(throwFlag, ErrorCode::TypeError, errorMessage_DefineProperty_RedefineNotConfigurable, P.toString());
        }
        if (isCurrentDataDescriptor) { // 9.b
            O->deleteProperty(P);
            O->defineAccessorProperty(P, new ESPropertyAccessorData(desc.getterFunction(), desc.setterFunction()), descHasWritable ? desc.writable() : false, descHasEnumerable ? desc.enumerable() : propertyInfo.enumerable(), descHasConfigurable ? desc.configurable() : propertyInfo.configurable(), true);
        } else { // 9.c
            O->deleteProperty(P);
            O->defineDataProperty(P, descHasWritable ? desc.writable() : false, descHasEnumerable ? desc.enumerable() : propertyInfo.enumerable(), descHasConfigurable ? desc.configurable() : propertyInfo.configurable(), desc.value(), true);
        }
        return true;
    } else if (isCurrentDataDescriptor && isDescDataDescriptor) { // 10
        if (!propertyInfo.configurable()) {
            if (!propertyInfo.writable()) {
                if (desc.writable()) {
                    return reject(throwFlag, ErrorCode::TypeError, errorMessage_DefineProperty_NotWritable, P.toString());
                } else {
                    if (descHasValue && current != desc.value()) {
                        return reject(throwFlag, ErrorCode::TypeError, errorMessage_DefineProperty_NotWritable, P.toString());
                    }
                }
            }
        }
        if (UNLIKELY(O->isESArgumentsObject() && !propertyInfo.isDataProperty())) { // ES6.0 $9.4.4.2
            if (desc.hasValue())
                O->set(P, desc.value());
            if ((descHasWritable && !desc.writable()) || (descHasEnumerable && !desc.enumerable()) || (descHasConfigurable && !desc.configurable()))
                shouldRemoveOriginalNativeGetterSetter = true;
        }
    } else {
        ASSERT(!propertyInfo.isDataProperty() && desc.isAccessorDescriptor());
        if (!propertyInfo.configurable()) {
            if (descHasSetter && (desc.setterFunction() != O->accessorData(idx)->getJSSetter())) {
                return reject(throwFlag, ErrorCode::TypeError, errorMessage_DefineProperty_RedefineNotConfigurable, P.toString());
            }
            if (descHasGetter && (desc.getterFunction() != O->accessorData(idx)->getJSGetter())) {
                return reject(throwFlag, ErrorCode::TypeError, errorMessage_DefineProperty_RedefineNotConfigurable, P.toString());
            }
        }
    }

    //

    // 12
    if (descHasGetter || descHasSetter || (!propertyInfo.isDataProperty() && !O->accessorData(idx)->getNativeGetter() && !O->accessorData(idx)->getNativeSetter())) {
        escargot::ESFunctionObject* getter = desc.getterFunction();
        escargot::ESFunctionObject* setter = desc.setterFunction();
        if (!propertyInfo.isDataProperty()) {
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
            descHasWritable ? desc.writable() : propertyInfo.writable(),
            descHasEnumerable ? desc.enumerable(): propertyInfo.enumerable(),
            descHasConfigurable ? desc.configurable() : propertyInfo.configurable(), true);
    } else if (!propertyInfo.isDataProperty() && (O->accessorData(idx)->getNativeGetter() || O->accessorData(idx)->getNativeSetter()) && !shouldRemoveOriginalNativeGetterSetter) {
        escargot::ESNativeGetter getter = O->accessorData(idx)->getNativeGetter();
        escargot::ESNativeSetter setter = O->accessorData(idx)->getNativeSetter();
        O->set(P, descHasValue ? desc.value() : current);

        O->deleteProperty(P, true);
        O->defineAccessorProperty(P, new ESPropertyAccessorData(getter, setter),
            descHasWritable ? desc.writable() : propertyInfo.writable(),
            descHasEnumerable ? desc.enumerable(): propertyInfo.enumerable(),
            descHasConfigurable ? desc.configurable() : propertyInfo.configurable(), true);

    } else {
        O->deleteProperty(P, true);
        O->defineDataProperty(P,
            descHasWritable ? desc.writable() : propertyInfo.writable(),
            descHasEnumerable ? desc.enumerable(): propertyInfo.enumerable(),
            descHasConfigurable ? desc.configurable() : propertyInfo.configurable(),
            descHasValue ? desc.value() : current, true);
    }

    // 13
    return true;
}

// ES 5.1: 8.12.9
bool ESObject::defineOwnProperty(const ESValue& P, ESObject* obj, bool throwFlag)
{
    return defineOwnProperty(P, PropertyDescriptor { obj }, throwFlag);
}

const unsigned ESArrayObject::MAX_FASTMODE_SIZE;

ESArrayObject::ESArrayObject(int length)
    : ESObject((Type)(Type::ESObject | Type::ESArrayObject), ESVMInstance::currentInstance()->globalObject()->arrayPrototype(), 3)
    , m_vector(0)
{
    setGlobalObject(ESVMInstance::currentInstance()->globalObject());
    if (!globalObject()->didSomePrototypeObjectDefineIndexedProperty())
        m_flags.m_isFastMode = true;

    // defineAccessorProperty(strings->length.string(), ESVMInstance::currentInstance()->arrayLengthAccessorData(), true, false, false);
    m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForArrayObject();
    m_hiddenClassData.push_back((ESPointer *)ESVMInstance::currentInstance()->arrayLengthAccessorData());

    m_length = 0;
    if (length == -1)
        convertToSlowMode();
    else if (length > 0) {
        setLength(length);
    } else {
        m_vector.reserve(6);
    }
}

bool ESArrayObject::defineOwnProperty(const ESValue& P, const PropertyDescriptor& desc, bool throwFlag)
{
    ESArrayObject* A = this;
    ESObject* O = this;
    uint32_t index;

    // ToPropertyDescriptor : (start) we need to seperate this part
    bool descHasEnumerable = desc.hasEnumerable();
    bool descHasConfigurable = desc.hasConfigurable();
    bool descHasWritable = desc.hasWritable();
    bool descHasValue = desc.hasValue();
    bool descHasGetter = desc.hasGetter();
    bool descHasSetter = desc.hasSetter();
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
        ESValue descV = desc.value();
        // 3.a
        if (!descHasValue) {
            bool ret = A->asESObject()->defineOwnProperty(P, desc, throwFlag);
            if (ret) {
                if (!descHasWritable || !desc.writable()) {
                    A->convertToSlowMode();
                }
            }
            return ret;
        }
        // 3.b
        ESObject* newLenDesc = ESObject::create();
        if (descHasEnumerable)
            newLenDesc->set(strings->enumerable.string(), ESValue(desc.enumerable()));
        if (descHasWritable)
            newLenDesc->set(strings->writable.string(), ESValue(desc.writable()));
        if (descHasConfigurable)
            newLenDesc->set(strings->configurable.string(), ESValue(desc.configurable()));
        if (descHasValue)
            newLenDesc->set(strings->value.string(), descV);
        if (descHasGetter)
            newLenDesc->set(strings->get.string(), desc.getterFunction());
        if (descHasSetter)
            newLenDesc->set(strings->set.string(), desc.setterFunction());

        // 3.c
        uint32_t newLen = descV.toUint32();

        // 3.d
        if (newLen != descV.toNumber())
            ESVMInstance::currentInstance()->throwError(ESValue(RangeError::create(ESString::create("invalid array length"))));

        // 3.e
        newLenDesc->set(strings->value.string(), ESValue(newLen));

        // 3.f
        if (newLen >= oldLen)
            return A->asESObject()->defineOwnProperty(P, newLenDesc, throwFlag);

        // 3.g
        if (!oldLePropertyInfo.writable()) {
            return reject(throwFlag, ErrorCode::TypeError, errorMessage_DefineProperty_NotWritable, P.toString());
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
                return reject(throwFlag, ErrorCode::TypeError, errorMessage_DefineProperty_Default, P.toString());
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
    } else if ((index = P.toIndex()) != ESValue::ESInvalidIndexValue) { // 4
        // 4.a

        // 4.b
        if (index >= oldLen && !oldLePropertyInfo.writable()) {
            return reject(throwFlag, ErrorCode::TypeError, errorMessage_DefineProperty_LengthNotWritable, P.toString());
        }

        // 4.c
        bool succeeded = A->asESObject()->defineOwnProperty(P, desc, false);

        // 4.d
        if (!succeeded) {
            return reject(throwFlag, ErrorCode::TypeError, errorMessage_DefineProperty_Default, P.toString());
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

// ES 5.1: 15.4.5.1
bool ESArrayObject::defineOwnProperty(const ESValue& P, ESObject* obj, bool throwFlag)
{
    return defineOwnProperty(P, PropertyDescriptor { obj }, throwFlag);
}

int64_t ESArrayObject::nextIndexForward(ESObject* obj, const int64_t cur, const int64_t end, const bool skipUndefined)
{
    ESValue ptr = obj;
    int64_t ret = end;
    while (ptr.isESPointer() && ptr.asESPointer()->isESObject()) {
        ptr.asESPointer()->asESObject()->enumerationWithNonEnumerable([&](ESValue key, ESHiddenClassPropertyInfo* propertyInfo) {
            uint32_t index = ESValue::ESInvalidIndexValue;
            if ((index = key.toIndex()) != ESValue::ESInvalidIndexValue) {
                if (skipUndefined && ptr.asESPointer()->asESObject()->get(key).isUndefined()) {
                    return;
                }
                if (index > cur) {
                    ret = std::min(static_cast<int64_t>(index), ret);
                }
            }
        });
        ptr = ptr.asESPointer()->asESObject()->__proto__();
    }
    return ret;
}

int64_t ESArrayObject::nextIndexBackward(ESObject* obj, const int64_t cur, const int64_t end, const bool skipUndefined)
{
    ESValue ptr = obj;
    int64_t ret = end;
    while (ptr.isESPointer() && ptr.asESPointer()->isESObject()) {
        ptr.asESPointer()->asESObject()->enumerationWithNonEnumerable([&](ESValue key, ESHiddenClassPropertyInfo* propertyInfo) {
            uint32_t index = ESValue::ESInvalidIndexValue;
            if ((index = key.toIndex()) != ESValue::ESInvalidIndexValue) {
                if (skipUndefined && ptr.asESPointer()->asESObject()->get(key).isUndefined()) {
                    return;
                }
                if (index < cur) {
                    ret = std::max(static_cast<int64_t>(index), ret);
                }
            }
        });
        ptr = ptr.asESPointer()->asESObject()->__proto__();
    }
    return ret;
}

void ESArrayObject::setLength(unsigned newLength)
{
    if (m_flags.m_isFastMode) {
        if (shouldConvertToSlowMode(newLength)) {
            convertToSlowMode();
            ESObject::set(strings->length, ESValue(newLength));
            m_length = newLength;
            return;
        }
        if (newLength < m_length) {
            m_vector.resize(newLength);
        } else if (newLength > m_length) {
            if (m_vector.capacity() < newLength) {
                size_t reservedSpace = std::min(MAX_FASTMODE_SIZE, (unsigned)(newLength*1.5f));
                m_vector.reserve(reservedSpace);
            }
            m_vector.resize(newLength, ESValue(ESValue::ESEmptyValue));
        }
    } else {
        unsigned currentLength = m_length;
        if (newLength < currentLength) {
            std::vector<unsigned> indexes;
            enumerationWithNonEnumerable([&](ESValue key, ESHiddenClassPropertyInfo* propertyInfo) {
                uint32_t index = key.toIndex();
                if (index != ESValue::ESInvalidIndexValue) {
                    if (index >= newLength && index < currentLength)
                        indexes.push_back(index);
                }
            });
            std::sort(indexes.begin(), indexes.end(), std::greater<uint32_t>());
            for (auto index : indexes) {
                if (deleteProperty(ESValue(index))) {
                    m_length--;
                    continue;
                }
                m_length = index + 1;
                if (globalObject()->instance()->currentExecutionContext()->isStrictMode()) {
                    ESVMInstance::currentInstance()->throwError(TypeError::create(ESString::create(u"Unable to delete array property while setting array length")));
                }
                return;
            }
        } else {
            size_t idx = hiddenClass()->findProperty(strings->length);
            if (idx != SIZE_MAX) {
                ESHiddenClassPropertyInfo propertyInfo = hiddenClass()->propertyInfo(idx);
                if (!propertyInfo.writable()) {
                    if (globalObject()->instance()->currentExecutionContext()->isStrictMode()) {
                        ESVMInstance::currentInstance()->throwError(TypeError::create(ESString::create(u"length is non-writable")));
                    }
                    return;
                }
                m_length = newLength;
                return;
            }
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
    m_length = newLength;
}

inline ESString* escapeSlashInPattern(ESString* patternStr)
{
    if (patternStr->length() == 0)
        return patternStr;
    else if (patternStr->isASCIIString()) {
        ASCIIString pattern(*patternStr->asASCIIString());
        size_t len = patternStr->length();
        ASCIIString buf = "";
        size_t i, start = 0;
        bool slashFlag = false;
        ESString* retval = strings->emptyString.string();
        while (true) {
            for (i = 0; start + i < len; i++) {
                if (UNLIKELY(pattern[start + i] == '/' && (i == 0 || pattern[start + i - 1] != '\\'))) {
                    slashFlag = true;
                    buf.append(pattern.substr(start, i));
                    buf.push_back('\\');
                    buf.push_back('/');
                    retval = ESString::concatTwoStrings(retval, ESString::create(buf));

                    start = start + i + 1;
                    i = 0;
                    buf = "";
                    break;
                }
            }
            if (start + i >= len) {
                if (UNLIKELY(slashFlag)) {
                    buf.append(pattern.substr(start, i));
                    retval = ESString::concatTwoStrings(retval, ESString::create(buf));
                }
                break;
            }
        }
        if (!slashFlag)
            return patternStr;
        else
            return retval;

    } else {
        UTF16String pattern(*patternStr->asUTF16String());
        size_t len = patternStr->length();
        UTF16String buf = u"";
        size_t i, start = 0;
        bool slashFlag = false;
        ESString* retval = ESString::create(u"");
        while (true) {
            for (i = 0; start + i < len; i++) {
                if (UNLIKELY(pattern[start + i] == '/' && (i == 0 || pattern[start + i - 1] != '\\'))) {
                    slashFlag = true;
                    buf.append(pattern.substr(start, i));
                    buf.push_back('\\');
                    buf.push_back('/');
                    retval = ESString::concatTwoStrings(retval, ESString::create(buf));

                    start = i + 1;
                    i = 0;
                    buf = u"";
                    break;
                }
            }
            if (start + i >= len) {
                if (UNLIKELY(slashFlag)) {
                    buf.append(pattern.substr(start, i));
                    retval = ESString::concatTwoStrings(retval, ESString::create(buf));
                }
                break;
            }
        }
        if (!slashFlag)
            return patternStr;
        else
            return retval;
    }
}

ESRegExpObject* ESRegExpObject::create(const ESValue patternValue, const ESValue optionValue)
{
    if (patternValue.isESPointer() && patternValue.asESPointer()->isESRegExpObject()) {
        if (optionValue.isUndefined()) {
            return patternValue.asESPointer()->asESRegExpObject();
        }
        ESVMInstance::currentInstance()->throwError(TypeError::create(ESString::create(u"Cannot supply flags when constructing one RegExp from another")));
    }

    escargot::ESString* patternStr = patternValue.isUndefined() ? strings->defaultRegExpString.string() : patternValue.toString();
    if (patternStr->length() == 0)
        patternStr = strings->defaultRegExpString.string();
    ESRegExpObject::Option option = parseOption(optionValue.isUndefined()? strings->emptyString.string(): optionValue.toString());

    return new ESRegExpObject(patternStr, option);
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

    ESVMInstance* instance = ESVMInstance::currentInstance();
    m_hiddenClass = instance->initialHiddenClassForRegExpObject();
    m_hiddenClassData.push_back((ESPointer*)instance->regexpAccessorData(0));
    m_hiddenClassData.push_back((ESPointer*)instance->regexpAccessorData(1));
    m_hiddenClassData.push_back((ESPointer*)instance->regexpAccessorData(2));
    m_hiddenClassData.push_back((ESPointer*)instance->regexpAccessorData(3));
    m_hiddenClassData.push_back((ESPointer*)instance->regexpAccessorData(4));

    setSource(m_source);
}

void ESRegExpObject::setSource(escargot::ESString* src)
{
    escargot::ESString* escapedSrc = escapeSlashInPattern(src);
    auto entry = getCacheEntryAndCompileIfNeeded(escapedSrc, m_option);
    if (entry.m_yarrError)
        ESVMInstance::currentInstance()->throwError(ESValue(SyntaxError::create(ESString::create(u"RegExp has invalid source"))));

    m_source = escapedSrc;
    m_yarrPattern = entry.m_yarrPattern;
    m_bytecodePattern = entry.m_bytecodePattern;
}

void ESRegExpObject::setOption(const Option& option)
{
    if (((m_option & ESRegExpObject::Option::MultiLine) != (option & ESRegExpObject::Option::MultiLine))
        || ((m_option & ESRegExpObject::Option::IgnoreCase) != (option & ESRegExpObject::Option::IgnoreCase))
        ) {
        ASSERT(!m_yarrPattern);
        m_bytecodePattern = NULL;
    }
    m_option = option;
}

ESRegExpObject::RegExpCacheEntry& ESRegExpObject::getCacheEntryAndCompileIfNeeded(escargot::ESString* source, const Option& option)
{
    auto cache = ESVMInstance::currentInstance()->regexpCache();
    auto it = cache->find(RegExpCacheKey(source, option));
    if (it != cache->end()) {
        return it->second;
    } else {
        const char* yarrError = nullptr;
        auto yarrPattern = new JSC::Yarr::YarrPattern(*source, option & ESRegExpObject::Option::IgnoreCase, option & ESRegExpObject::Option::MultiLine, &yarrError);
        return cache->insert(std::make_pair(RegExpCacheKey(source, option), RegExpCacheEntry(yarrError, yarrPattern))).first->second;
    }
}

ESRegExpObject::Option ESRegExpObject::parseOption(escargot::ESString* optionString)
{
    ESRegExpObject::Option option = ESRegExpObject::Option::None;

    for (size_t i = 0; i < optionString->length(); i++) {
        switch (optionString->charAt(i)) {
        case 'g':
            if (option & ESRegExpObject::Option::Global)
                ESVMInstance::currentInstance()->throwError(SyntaxError::create(ESString::create(u"RegExp has multiple 'g' flags")));
            option = (ESRegExpObject::Option) (option | ESRegExpObject::Option::Global);
            break;
        case 'i':
            if (option & ESRegExpObject::Option::IgnoreCase)
                ESVMInstance::currentInstance()->throwError(SyntaxError::create(ESString::create(u"RegExp has multiple 'i' flags")));
            option = (ESRegExpObject::Option) (option | ESRegExpObject::Option::IgnoreCase);
            break;
        case 'm':
            if (option & ESRegExpObject::Option::MultiLine)
                ESVMInstance::currentInstance()->throwError(SyntaxError::create(ESString::create(u"RegExp has multiple 'm' flags")));
            option = (ESRegExpObject::Option) (option | ESRegExpObject::Option::MultiLine);
            break;
        /*
        case 'y':
            if (option & ESRegExpObject::Option::Sticky)
                ESVMInstance::currentInstance()->throwError(SyntaxError::create(ESString::create(u"RegExp has multiple 'y' flags")));
            option = (ESRegExpObject::Option) (option | ESRegExpObject::Option::Sticky);
            break;
        */
        default:
            ESVMInstance::currentInstance()->throwError(SyntaxError::create(ESString::create(u"RegExp has invalid flag")));
        }
    }

    return option;
}

bool ESRegExpObject::match(const escargot::ESString* str, RegexMatchResult& matchResult, bool testOnly, size_t startIndex)
{
    m_lastExecutedString = str;

    if (!m_bytecodePattern) {
        RegExpCacheEntry& entry = getCacheEntryAndCompileIfNeeded(m_source, m_option);
        if (entry.m_yarrError) {
            matchResult.m_subPatternNum = 0;
            return false;
        }
        m_yarrPattern = entry.m_yarrPattern;

        if (entry.m_bytecodePattern) {
            m_bytecodePattern = entry.m_bytecodePattern;
        } else {
            WTF::BumpPointerAllocator *bumpAlloc = ESVMInstance::currentInstance()->bumpPointerAllocator();
            JSC::Yarr::OwnPtr<JSC::Yarr::BytecodePattern> ownedBytecode = JSC::Yarr::byteCompile(*m_yarrPattern, bumpAlloc);
            m_bytecodePattern = ownedBytecode.leakPtr();
            entry.m_bytecodePattern = m_bytecodePattern;
        }
    }

    unsigned subPatternNum = m_bytecodePattern->m_body->m_numSubpatterns;
    matchResult.m_subPatternNum = (int) subPatternNum;
    size_t length = str->length();
    size_t start = startIndex;
    unsigned result = 0;
    const void* chars;
    if (str->isASCIIString())
        chars = str->asciiData();
    else
        chars = str->utf16Data();
    bool isGlobal = option() & ESRegExpObject::Option::Global;
    unsigned* outputBuf;
    ALLOCA_WRAPPER(ESVMInstance::currentInstance(), outputBuf, unsigned int*, sizeof(unsigned) * 2 * (subPatternNum + 1), true);
    outputBuf[1] = start;
    do {
        start = outputBuf[1];
        memset(outputBuf, -1, sizeof(unsigned) * 2 * (subPatternNum + 1));
        if (start > length)
            break;
        if (str->isASCIIString())
            result = JSC::Yarr::interpret(m_bytecodePattern, (const char *)chars, length, start, outputBuf);
        else
            result = JSC::Yarr::interpret(m_bytecodePattern, (const char16_t *)chars, length, start, outputBuf);
        if (result != JSC::Yarr::offsetNoMatch) {
            if (UNLIKELY(testOnly)) {
                // outputBuf[1] should be set to lastIndex
                if (isGlobal)
                    set(strings->lastIndex, ESValue(outputBuf[1]), true);
                return true;
            }
            std::vector<RegexMatchResult::RegexMatchResultPiece, pointer_free_allocator<RegexMatchResult::RegexMatchResultPiece> > piece;
            piece.reserve(subPatternNum + 1);

            for (unsigned i = 0; i < subPatternNum + 1; i ++) {
                RegexMatchResult::RegexMatchResultPiece p;
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
    if (UNLIKELY(testOnly)) {
        set(strings->lastIndex, ESValue(0), true);
    }
    return matchResult.m_matchResults.size();
}

ESArrayObject* ESRegExpObject::createRegExpMatchedArray(const RegexMatchResult& result, const escargot::ESString* input)
{
    escargot::ESArrayObject* arr = escargot::ESArrayObject::create();

    arr->defineOwnProperty(strings->index.string(),
        PropertyDescriptor { ESValue(result.m_matchResults[0][0].m_start), PropertyDescriptor::defaultAttributes }, true);
    arr->defineOwnProperty(strings->input.string(),
        PropertyDescriptor { input, PropertyDescriptor::defaultAttributes }, true);

    int idx = 0;
    for (unsigned i = 0; i < result.m_matchResults.size() ; i ++) {
        for (unsigned j = 0; j < result.m_matchResults[i].size() ; j ++) {
            if (result.m_matchResults[i][j].m_start == std::numeric_limits<unsigned>::max()) {
                arr->defineOwnProperty(ESValue(idx++),
                    PropertyDescriptor { ESValue(ESValue::ESUndefined), PropertyDescriptor::defaultAttributes }, true);
            } else {
                arr->defineOwnProperty(ESValue(idx++),
                    PropertyDescriptor { input->substring(result.m_matchResults[i][j].m_start, result.m_matchResults[i][j].m_end),
                    PropertyDescriptor::defaultAttributes },  true);
            }
        }
    }
    return arr;
}

ESArrayObject* ESRegExpObject::pushBackToRegExpMatchedArray(escargot::ESArrayObject* array, size_t& index, const size_t limit, const RegexMatchResult& result, const escargot::ESString* str)
{
    bool global = option() && Option::Global;

    if (global) {
        for (unsigned i = 0; i < result.m_matchResults.size(); i++) {
            if (i == 0)
                continue;

            if (std::numeric_limits<unsigned>::max() == result.m_matchResults[i][0].m_start)
                array->defineDataProperty(ESValue(index++), true, true, true, ESValue(ESValue::ESUndefined));
            else
                array->defineDataProperty(ESValue(index++), true, true, true, str->substring(result.m_matchResults[i][0].m_start, result.m_matchResults[i][0].m_end));
            if (index == limit)
                return array;
        }
    } else {
        for (unsigned i = 0; i < result.m_matchResults.size(); i ++) {
            for (unsigned j = 0; j < result.m_matchResults[i].size(); j ++) {
                if (i == 0 && j == 0)
                    continue;

                if (std::numeric_limits<unsigned>::max() == result.m_matchResults[i][j].m_start)
                    array->defineDataProperty(ESValue(index++), true, true, true, ESValue(ESValue::ESUndefined));
                else
                    array->defineDataProperty(ESValue(index++), true, true, true, str->substring(result.m_matchResults[i][j].m_start, result.m_matchResults[i][j].m_end));
                if (index == limit)
                    return array;
            }
        }
    }

    return array;
}

ESFunctionObject::ESFunctionObject(LexicalEnvironment* outerEnvironment, CodeBlock* cb, escargot::ESString* name, unsigned length, bool hasPrototype, bool isBuiltIn)
    : ESObject((Type)(Type::ESObject | Type::ESFunctionObject), ESVMInstance::currentInstance()->globalFunctionPrototype(), 4)
{
    m_outerEnvironment = outerEnvironment;
    m_codeBlock = cb;
    m_flags.m_nonConstructor = false;

    if (hasPrototype) {
        m_protoType = ESObject::create(2);

        // m_protoType.asESPointer()->asESObject()->defineDataProperty(strings->constructor.string(), true, false, true, this);
        m_protoType.asESPointer()->asESObject()->m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForPrototypeObject();
        m_protoType.asESPointer()->asESObject()->m_hiddenClassData.push_back(this);
    }

    // $19.2.4 Function Instances
    // these define in ESVMInstance::ESVMInstance()
    // defineDataProperty(strings->length, false, false, true, ESValue(length));
    // defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), true, false, false);
    // defineDataProperty(strings->name.string(), false, false, true, name);
    if (hasPrototype && !isBuiltIn) {
        m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForFunctionObject();
        m_hiddenClassData.push_back(ESValue(length));
        m_hiddenClassData.push_back(ESValue((ESPointer *)ESVMInstance::currentInstance()->functionPrototypeAccessorData()));
        m_hiddenClassData.push_back(ESValue(name));
    } else {
        m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForFunctionObjectWithoutPrototype();
        m_hiddenClassData.push_back(ESValue(length));
        m_hiddenClassData.push_back(ESValue(name));
    }
    if (cb && cb->shouldUseStrictMode()) {
        defineAccessorProperty(strings->arguments.string(), ESVMInstance::currentInstance()->throwerAccessorData(), false, false, false);
        defineAccessorProperty(strings->caller.string(), ESVMInstance::currentInstance()->throwerAccessorData(), false, false, false);
    }
    m_flags.m_isBoundFunction = false;
}

ESFunctionObject::ESFunctionObject(LexicalEnvironment* outerEnvironment, NativeFunctionType fn, escargot::ESString* name, unsigned length, bool isConstructor, bool isBuiltIn)
    : ESFunctionObject(outerEnvironment, (CodeBlock *)NULL, name, length, isConstructor, isBuiltIn)
{
    m_codeBlock = CodeBlock::create(ExecutableType::FunctionCode, 0, true);
    m_codeBlock->m_hasCode = true;
    m_codeBlock->pushCode(ExecuteNativeFunction(fn));
#ifndef NDEBUG
    m_codeBlock->m_nonAtomicId = name;
#endif
#ifdef ENABLE_ESJIT
    m_codeBlock->m_dontJIT = true;
#endif
    if (!isConstructor)
        m_flags.m_nonConstructor = true;
    m_flags.m_isBoundFunction = false;
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
            size_t pos = fn->codeBlock()->m_byteCodePositionsHaveToProfile[i];
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
                size_t pos = fn->codeBlock()->m_byteCodePositionsHaveToProfile[i];
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

ALWAYS_INLINE void functionCallerInnerProcess(ExecutionContext* newEC, ESFunctionObject* fn, CodeBlock* cb, DeclarativeEnvironmentRecord* functionRecord, ESValue* stackStorage, const ESValue& receiver, ESValue arguments[], const size_t& argumentCount, ESVMInstance* ESVMInstance)
{
    // http://www.ecma-international.org/ecma-262/6.0/#sec-ordinarycallbindthis
    if (newEC->isStrictMode() || cb->m_isBuiltInFunction) {
        newEC->setThisBinding(receiver);
    } else {
        if (receiver.isUndefinedOrNull()) {
            newEC->setThisBinding(ESVMInstance->globalObject());
        } else {
            newEC->setThisBinding(receiver.toObject());
        }
    }

    // if FunctionExpressionNode has own name, should bind own function object
    if (cb->m_isFunctionExpression && cb->m_functionExpressionNameIndex != SIZE_MAX) {
        if (cb->m_isFunctionExpressionNameHeapAllocated) {
            *functionRecord->bindingValueForHeapAllocatedData(cb->m_functionExpressionNameIndex) = ESValue(fn);
        } else {
            stackStorage[cb->m_functionExpressionNameIndex] = ESValue(fn);
        }
    }

    const FunctionParametersInfoVector& info = cb->m_paramsInformation;
    size_t siz = std::min(argumentCount, info.size());
    if (UNLIKELY(cb->m_needsComplexParameterCopy)) {
        for (size_t i = 0; i < siz; i ++) {
            if (info[i].m_isHeapAllocated) {
                *functionRecord->bindingValueForHeapAllocatedData(info[i].m_index) = arguments[i];
            } else {
                stackStorage[info[i].m_index] = arguments[i];
            }
        }
    } else {
        for (size_t i = 0; i < siz; i ++) {
            stackStorage[i] = arguments[i];
        }
    }

}


ESValue ESFunctionObject::call(ESVMInstance* instance, const ESValue& callee, const ESValue& receiver, ESValue arguments[], const size_t& argumentCount, bool isNewExpression)
{
    instance->stackCheck();
    instance->argumentCountCheck(argumentCount);

    ESValue result(ESValue::ESForceUninitialized);
    if (LIKELY(callee.isESPointer() && callee.asESPointer()->isESFunctionObject())) {
        ExecutionContext* currentContext = instance->currentExecutionContext();
        ESFunctionObject* fn = callee.asESPointer()->asESFunctionObject();
        CodeBlock* const cb = fn->codeBlock();
        if (UNLIKELY(!cb->m_hasCode)) {
            ParserContextInformation parserContextInformation;
            generateByteCode(cb, nullptr, ExecutableType::FunctionCode, parserContextInformation, true);
        }

        ESValue* stackStorage;
        ALLOCA_WRAPPER(instance, stackStorage, ESValue*, sizeof(ESValue) * cb->m_stackAllocatedIdentifiersCount, false);
        if (cb->m_needsHeapAllocatedExecutionContext) {
            auto FE = LexicalEnvironment::newFunctionEnvironment(cb->m_needsToPrepareGenerateArgumentsObject,
                stackStorage, cb->m_stackAllocatedIdentifiersCount, cb->m_heapAllocatedIdentifiers, arguments, argumentCount, fn, cb->m_needsActivation, cb->m_functionExpressionNameIndex);
            instance->m_currentExecutionContext = new ExecutionContext(FE, isNewExpression, cb->shouldUseStrictMode(), arguments, argumentCount);
            FunctionEnvironmentRecord* record = (FunctionEnvironmentRecord *)FE->record();
            functionCallerInnerProcess(instance->m_currentExecutionContext, fn, cb, record, stackStorage, receiver, arguments, argumentCount, instance);

#ifdef ENABLE_ESJIT
            result = executeJIT(fn, instance, *instance->m_currentExecutionContext);
#else
            result = interpret(instance, cb, 0, stackStorage, &record->heapAllocatedData());
#endif
            instance->m_currentExecutionContext = currentContext;
        } else {
            if (UNLIKELY(cb->m_needsToPrepareGenerateArgumentsObject)) {
                FunctionEnvironmentRecordWithArgumentsObject envRec(
                    arguments, argumentCount, fn,
                    stackStorage, cb->m_stackAllocatedIdentifiersCount, cb->m_heapAllocatedIdentifiers, cb->m_needsActivation, cb->m_functionExpressionNameIndex);
                LexicalEnvironment env(&envRec, fn->outerEnvironment());
                ExecutionContext ec(&env, isNewExpression, cb->shouldUseStrictMode(), arguments, argumentCount);
                instance->m_currentExecutionContext = &ec;
                functionCallerInnerProcess(&ec, fn, cb, &envRec, stackStorage, receiver, arguments, argumentCount, instance);
#ifdef ENABLE_ESJIT
                result = executeJIT(fn, instance, ec);
#else
                result = interpret(instance, cb, 0, stackStorage, &envRec.heapAllocatedData());
#endif
                instance->m_currentExecutionContext = currentContext;
            } else {
                FunctionEnvironmentRecord envRec(
                    stackStorage, cb->m_stackAllocatedIdentifiersCount, cb->m_heapAllocatedIdentifiers, cb->m_needsActivation, cb->m_functionExpressionNameIndex);
                LexicalEnvironment env(&envRec, fn->outerEnvironment());
                ExecutionContext ec(&env, isNewExpression, cb->shouldUseStrictMode(), arguments, argumentCount);
                instance->m_currentExecutionContext = &ec;
                functionCallerInnerProcess(&ec, fn, cb, &envRec, stackStorage, receiver, arguments, argumentCount, instance);
#ifdef ENABLE_ESJIT
                result = executeJIT(fn, instance, ec);
#else
                result = interpret(instance, cb, 0, stackStorage, &envRec.heapAllocatedData());
#endif
                instance->m_currentExecutionContext = currentContext;
            }
        }
    } else {
        instance->throwError(ErrorCode::TypeError, errorMessage_Call_NotFunction);
    }

    return result;
}

ESDateObject::ESDateObject(ESPointer::Type type)
    : ESObject((Type)(Type::ESObject | Type::ESDateObject), ESVMInstance::currentInstance()->globalObject()->datePrototype())
{
    m_isCacheDirty = true;
    m_hasValidDate = false;
}

void ESDateObject::setTimeValue()
{
    struct timespec time;
    clock_gettime(CLOCK_REALTIME, &time);
    m_primitiveValue = time.tv_sec * (time64_t) 1000 + floor(time.tv_nsec / 1000000);
    m_isCacheDirty = true;
    m_hasValidDate = true;
}

void ESDateObject::setTimeValue(time64IncludingNaN t)
{
    setTime(t);
}

void ESDateObject::setTimeValue(const ESValue str)
{
    escargot::ESString* istr = str.toString();

    time64IncludingNaN primitiveValue = parseStringToDate(istr);
    if (isnan(primitiveValue)) {
        setTimeValueAsNaN();
    } else {
        m_primitiveValue = (time64_t) primitiveValue;
        m_isCacheDirty = true;
        m_hasValidDate = true;
    }
}

void ESDateObject::setTimeValue(int year, int month, int date, int hour, int minute, int64_t second, int64_t millisecond, bool convertToUTC)
{
    int ym = year + floor(month / 12.0);
    int mn = month % 12;
    if (mn < 0)
        mn = (mn + 12) % 12;

    time64IncludingNaN primitiveValue = ymdhmsToSeconds(ym, mn, date, hour, minute, second) * 1000. + millisecond;

    if (convertToUTC) {
        primitiveValue = toUTC((time64_t) primitiveValue);
    }

    if (LIKELY(!std::isnan(primitiveValue)) && primitiveValue <= options::MaximumDatePrimitiveValue && primitiveValue >= -options::MaximumDatePrimitiveValue) {
        m_hasValidDate = true;
        m_primitiveValue = (time64_t) primitiveValue;
        resolveCache();
    } else {
        setTimeValueAsNaN();
    }
    m_isCacheDirty = true;
}

time64IncludingNaN ESDateObject::toUTC(time64_t t)
{
    int tzOffsetAsSec = ESVMInstance::currentInstance()->timezoneOffset(); // For example, it returns 28800 in GMT-8 zone
    time64IncludingNaN primitiveValue = t + (double) tzOffsetAsSec * 1000.;
    if (primitiveValue <= options::MaximumDatePrimitiveValue && primitiveValue >= -options::MaximumDatePrimitiveValue) {
        primitiveValue += computeDaylightSaving((time64_t) primitiveValue) * msPerSecond;
        return primitiveValue;
    }
    return std::numeric_limits<double>::quiet_NaN();
}

time64_t ESDateObject::ymdhmsToSeconds(int year, int mon, int day, int hour, int minute, int64_t second)
{
    return (makeDay(year, mon, day) * msPerDay + (hour * msPerHour + minute * msPerMinute + second * msPerSecond /* + millisecond */)) / 1000.0;
}

static const struct KnownZone {
#if !OS(WINDOWS)
    const
#endif
        char tzName[4];
    int tzOffset;
} known_zones[] = {
    { "UT", 0 },
    { "GMT", 0 },
    { "EST", -300 },
    { "EDT", -240 },
    { "CST", -360 },
    { "CDT", -300 },
    { "MST", -420 },
    { "MDT", -360 },
    { "PST", -480 },
    { "PDT", -420 }
};
// returns 0-11 (Jan-Dec); -1 on failure
static int findMonth(const char* monthStr)
{
    ASSERT(monthStr);
    char needle[4];
    for (int i = 0; i < 3; ++i) {
        if (!*monthStr)
            return -1;
//        needle[i] = static_cast<char>(toASCIILower(*monthStr++));
        needle[i] = (*monthStr++) | (uint8_t)0x20;
    }
    needle[3] = '\0';
    const char* haystack = "janfebmaraprmayjunjulaugsepoctnovdec";
    const char* str = strstr(haystack, needle);
    if (str) {
        int position = static_cast<int>(str - haystack);
        if (position % 3 == 0)
            return position / 3;
    }
    return -1;
}
inline bool isASCIISpace(char c)
{
    return c <= ' ' && (c == ' ' || (c <= 0xD && c >= 0x9));
}
inline bool isASCIIDigit(char c)
{
    return c >= '0' && c <= '9';
}
inline static void skipSpacesAndComments(const char*& s)
{
    int nesting = 0;
    char ch;
    while ((ch = *s)) {
        if (!isASCIISpace(ch)) {
            if (ch == '(')
                nesting++;
            else if (ch == ')' && nesting > 0)
                nesting--;
            else if (nesting == 0)
                break;
        }
        s++;
    }
}
static bool parseInt(const char* string, char** stopPosition, int base, int* result)
{
    long longResult = strtol(string, stopPosition, base);
    // Avoid the use of errno as it is not available on Windows CE
    if (string == *stopPosition || longResult <= std::numeric_limits<int>::min() || longResult >= std::numeric_limits<int>::max())
        return false;
    *result = static_cast<int>(longResult);
    return true;
}

static bool parseLong(const char* string, char** stopPosition, int base, long* result, int digits = 0)
{
    if (digits == 0) {
        *result = strtol(string, stopPosition, base);
        // Avoid the use of errno as it is not available on Windows CE
        if (string == *stopPosition || *result == std::numeric_limits<long>::min() || *result == std::numeric_limits<long>::max())
            return false;
        return true;
    } else {
        strtol(string, stopPosition, base); // for compute stopPosition

        char s[4]; // 4 is temporary number for case (digit == 3)..
        s[0] = string[0];
        s[1] = string[1];
        s[2] = string[2];
        s[3] = '\0';

        *result = strtol(s, NULL, base);
        if (string == *stopPosition || *result == std::numeric_limits<long>::min() || *result == std::numeric_limits<long>::max())
            return false;
        return true;
    }
}

double ESDateObject::parseStringToDate_1(escargot::ESString* istr, bool& haveTZ, int& offset)
{
    haveTZ = false;
    offset = 0;


    long month = -1;
    const char* dateString = istr->utf8Data();
    const char* wordStart = dateString;

    skipSpacesAndComments(dateString);

    while (*dateString && !isASCIIDigit(*dateString)) {
        if (isASCIISpace(*dateString) || *dateString == '(') {
            if (dateString - wordStart >= 3)
                month = findMonth(wordStart);
            skipSpacesAndComments(dateString);
            wordStart = dateString;
        } else
            dateString++;
    }

    if (month == -1 && wordStart != dateString)
        month = findMonth(wordStart);

    skipSpacesAndComments(dateString);

    if (!*dateString)
        return std::numeric_limits<double>::quiet_NaN();

    char* newPosStr;
    long int day;
    if (!parseLong(dateString, &newPosStr, 10, &day))
        return std::numeric_limits<double>::quiet_NaN();
    dateString = newPosStr;

    if (!*dateString)
        return std::numeric_limits<double>::quiet_NaN();

    if (day < 0)
        return std::numeric_limits<double>::quiet_NaN();

    int year = 0;
    if (day > 31) {
        // ### where is the boundary and what happens below?
        if (*dateString != '/')
            return std::numeric_limits<double>::quiet_NaN();
        // looks like a YYYY/MM/DD date
        if (!*++dateString)
            return std::numeric_limits<double>::quiet_NaN();
        if (day <= std::numeric_limits<int>::min() || day >= std::numeric_limits<int>::max())
            return std::numeric_limits<double>::quiet_NaN();
        year = static_cast<int>(day);
        if (!parseLong(dateString, &newPosStr, 10, &month))
            return std::numeric_limits<double>::quiet_NaN();
        month -= 1;
        dateString = newPosStr;
        if (*dateString++ != '/' || !*dateString)
            return std::numeric_limits<double>::quiet_NaN();
        if (!parseLong(dateString, &newPosStr, 10, &day))
            return std::numeric_limits<double>::quiet_NaN();
        dateString = newPosStr;
    } else if (*dateString == '/' && month == -1) {
        dateString++;
        // This looks like a MM/DD/YYYY date, not an RFC date.
        month = day - 1; // 0-based
        if (!parseLong(dateString, &newPosStr, 10, &day))
            return std::numeric_limits<double>::quiet_NaN();
        if (day < 1 || day > 31)
            return std::numeric_limits<double>::quiet_NaN();
        dateString = newPosStr;
        if (*dateString == '/')
            dateString++;
        if (!*dateString)
            return std::numeric_limits<double>::quiet_NaN();
    } else {
        if (*dateString == '-')
            dateString++;

        skipSpacesAndComments(dateString);

        if (*dateString == ',')
            dateString++;

        if (month == -1) { // not found yet
            month = findMonth(dateString);
            if (month == -1)
                return std::numeric_limits<double>::quiet_NaN();

            while (*dateString && *dateString != '-' && *dateString != ',' && !isASCIISpace(*dateString))
                dateString++;

            if (!*dateString)
                return std::numeric_limits<double>::quiet_NaN();

            // '-99 23:12:40 GMT'
            if (*dateString != '-' && *dateString != '/' && *dateString != ',' && !isASCIISpace(*dateString))
                return std::numeric_limits<double>::quiet_NaN();
            dateString++;
        }
    }

    if (month < 0 || month > 11)
        return std::numeric_limits<double>::quiet_NaN();

    // '99 23:12:40 GMT'
    if (year <= 0 && *dateString) {
        if (!parseInt(dateString, &newPosStr, 10, &year))
            return std::numeric_limits<double>::quiet_NaN();
    }

    // Don't fail if the time is missing.
    long hour = 0;
    long minute = 0;
    long second = 0;
    if (!*newPosStr)
        dateString = newPosStr;
    else {
        // ' 23:12:40 GMT'
        if (!(isASCIISpace(*newPosStr) || *newPosStr == ',')) {
            if (*newPosStr != ':')
                return std::numeric_limits<double>::quiet_NaN();
            // There was no year; the number was the hour.
            year = -1;
        } else {
            // in the normal case (we parsed the year), advance to the next number
            dateString = ++newPosStr;
            skipSpacesAndComments(dateString);
        }

        parseLong(dateString, &newPosStr, 10, &hour);
        // Do not check for errno here since we want to continue
        // even if errno was set becasue we are still looking
        // for the timezone!

        // Read a number? If not, this might be a timezone name.
        if (newPosStr != dateString) {
            dateString = newPosStr;

            if (hour < 0 || hour > 23)
                return std::numeric_limits<double>::quiet_NaN();

            if (!*dateString)
                return std::numeric_limits<double>::quiet_NaN();

            // ':12:40 GMT'
            if (*dateString++ != ':')
                return std::numeric_limits<double>::quiet_NaN();

            if (!parseLong(dateString, &newPosStr, 10, &minute))
                return std::numeric_limits<double>::quiet_NaN();
            dateString = newPosStr;

            if (minute < 0 || minute > 59)
                return std::numeric_limits<double>::quiet_NaN();

            // ':40 GMT'
            if (*dateString && *dateString != ':' && !isASCIISpace(*dateString))
                return std::numeric_limits<double>::quiet_NaN();

            // seconds are optional in rfc822 + rfc2822
            if (*dateString ==':') {
                dateString++;

                if (!parseLong(dateString, &newPosStr, 10, &second))
                    return std::numeric_limits<double>::quiet_NaN();
                dateString = newPosStr;

                if (second < 0 || second > 59)
                    return std::numeric_limits<double>::quiet_NaN();
            }

            skipSpacesAndComments(dateString);

            if (strncasecmp(dateString, "AM", 2) == 0) {
                if (hour > 12)
                    return std::numeric_limits<double>::quiet_NaN();
                if (hour == 12)
                    hour = 0;
                dateString += 2;
                skipSpacesAndComments(dateString);
            } else if (strncasecmp(dateString, "PM", 2) == 0) {
                if (hour > 12)
                    return std::numeric_limits<double>::quiet_NaN();
                if (hour != 12)
                    hour += 12;
                dateString += 2;
                skipSpacesAndComments(dateString);
            }
        }
    }

    // The year may be after the time but before the time zone.
    if (isASCIIDigit(*dateString) && year == -1) {
        if (!parseInt(dateString, &newPosStr, 10, &year))
            return std::numeric_limits<double>::quiet_NaN();
        dateString = newPosStr;
        skipSpacesAndComments(dateString);
    }

    // Don't fail if the time zone is missing.
    // Some websites omit the time zone (4275206).
    if (*dateString) {
        if (strncasecmp(dateString, "GMT", 3) == 0 || strncasecmp(dateString, "UTC", 3) == 0) {
            dateString += 3;
            haveTZ = true;
        }
        if (*dateString == '+' || *dateString == '-') {
            int o;
            if (!parseInt(dateString, &newPosStr, 10, &o))
                return std::numeric_limits<double>::quiet_NaN();
            dateString = newPosStr;

            if (o < -9959 || o > 9959)
                return std::numeric_limits<double>::quiet_NaN();

            int sgn = (o < 0) ? -1 : 1;
            o = abs(o);
            if (*dateString != ':') {
                if (o >= 24)
                    offset = ((o / 100) * 60 + (o % 100)) * sgn;
                else
                    offset = o * 60 * sgn;
            } else { // GMT+05:00
                ++dateString; // skip the ':'
                int o2;
                if (!parseInt(dateString, &newPosStr, 10, &o2))
                    return std::numeric_limits<double>::quiet_NaN();
                dateString = newPosStr;
                offset = (o * 60 + o2) * sgn;
            }
            haveTZ = true;
        } else {
            size_t arrlenOfTZ = sizeof(known_zones) / sizeof(struct KnownZone);
            for (size_t i = 0; i < arrlenOfTZ; ++i) {
                if (0 == strncasecmp(dateString, known_zones[i].tzName, strlen(known_zones[i].tzName))) {
                    offset = known_zones[i].tzOffset;
                    dateString += strlen(known_zones[i].tzName);
                    haveTZ = true;
                    break;
                }
            }
        }
    }
    skipSpacesAndComments(dateString);

    if (*dateString && year == -1) {
        if (!parseInt(dateString, &newPosStr, 10, &year))
            return std::numeric_limits<double>::quiet_NaN();
        dateString = newPosStr;
        skipSpacesAndComments(dateString);
    }

    // Trailing garbage
    if (*dateString)
        return std::numeric_limits<double>::quiet_NaN();

    // Y2K: Handle 2 digit years.
    if (year >= 0 && year < 100) {
        if (year < 50)
            year += 2000;
        else
            year += 1900;
    }

    return ymdhmsToSeconds(year, month, day, hour, minute, second) * msPerSecond;
}
static char* parseES5DatePortion(const char* currentPosition, int& year, long& month, long& day)
{
    char* postParsePosition;

    // This is a bit more lenient on the year string than ES5 specifies:
    // instead of restricting to 4 digits (or 6 digits with mandatory +/-),
    // it accepts any integer value. Consider this an implementation fallback.
    if (!parseInt(currentPosition, &postParsePosition, 10, &year))
        return 0;

    // Check for presence of -MM portion.
    if (*postParsePosition != '-')
        return postParsePosition;
    currentPosition = postParsePosition + 1;

    if (!isASCIIDigit(*currentPosition))
        return 0;
    if (!parseLong(currentPosition, &postParsePosition, 10, &month))
        return 0;
    if ((postParsePosition - currentPosition) != 2)
        return 0;

    // Check for presence of -DD portion.
    if (*postParsePosition != '-')
        return postParsePosition;
    currentPosition = postParsePosition + 1;

    if (!isASCIIDigit(*currentPosition))
        return 0;
    if (!parseLong(currentPosition, &postParsePosition, 10, &day))
        return 0;
    if ((postParsePosition - currentPosition) != 2)
        return 0;
    return postParsePosition;
}
static char* parseES5TimePortion(char* currentPosition, long& hours, long& minutes, double& seconds, long& timeZoneSeconds, bool& haveTZ)
{
    char* postParsePosition;
    if (!isASCIIDigit(*currentPosition))
        return 0;
    if (!parseLong(currentPosition, &postParsePosition, 10, &hours))
        return 0;
    if (*postParsePosition != ':' || (postParsePosition - currentPosition) != 2)
        return 0;
    currentPosition = postParsePosition + 1;

    if (!isASCIIDigit(*currentPosition))
        return 0;
    if (!parseLong(currentPosition, &postParsePosition, 10, &minutes))
        return 0;
    if ((postParsePosition - currentPosition) != 2)
        return 0;
    currentPosition = postParsePosition;

    // Seconds are optional.
    if (*currentPosition == ':') {
        ++currentPosition;

        long intSeconds;
        if (!isASCIIDigit(*currentPosition))
            return 0;
        if (!parseLong(currentPosition, &postParsePosition, 10, &intSeconds))
            return 0;
        if ((postParsePosition - currentPosition) != 2)
            return 0;
        seconds = intSeconds;
        if (*postParsePosition == '.') {
            currentPosition = postParsePosition + 1;

            // In ECMA-262-5 it's a bit unclear if '.' can be present without milliseconds, but
            // a reasonable interpretation guided by the given examples and RFC 3339 says "no".
            // We check the next character to avoid reading +/- timezone hours after an invalid decimal.
            if (!isASCIIDigit(*currentPosition))
                return 0;

            long fracSeconds;
            if (!parseLong(currentPosition, &postParsePosition, 10, &fracSeconds, 3))
                return 0;

            long numFracDigits = std::min((long)(postParsePosition - currentPosition), 3L);
            seconds += fracSeconds * pow(10.0, static_cast<double>(-numFracDigits));
        }
        currentPosition = postParsePosition;
    }

    if (*currentPosition == 'Z') {
        haveTZ = true;
        return currentPosition + 1;
    }

    bool tzNegative;
    if (*currentPosition == '-')
        tzNegative = true;
    else if (*currentPosition == '+')
        tzNegative = false;
    else
        return currentPosition; // no timezone
    ++currentPosition;
    haveTZ = true;

    long tzHours;
    long tzHoursAbs;
    long tzMinutes;

    if (!isASCIIDigit(*currentPosition))
        return 0;
    if (!parseLong(currentPosition, &postParsePosition, 10, &tzHours))
        return 0;
    if (postParsePosition - currentPosition == 4) {
        tzMinutes = tzHours % 100;
        tzHours = tzHours / 100;
        tzHoursAbs = labs(tzHours);
    } else if (postParsePosition - currentPosition == 2) {
        if (*postParsePosition != ':')
            return 0;

        tzHoursAbs = labs(tzHours);
        currentPosition = postParsePosition + 1;

        if (!isASCIIDigit(*currentPosition))
            return 0;
        if (!parseLong(currentPosition, &postParsePosition, 10, &tzMinutes))
            return 0;
        if ((postParsePosition - currentPosition) != 2)
            return 0;
    } else
        return 0;

    currentPosition = postParsePosition;

    if (tzHoursAbs > 24)
        return 0;
    if (tzMinutes < 0 || tzMinutes > 59)
        return 0;

    timeZoneSeconds = 60 * (tzMinutes + (60 * tzHoursAbs));
    if (tzNegative)
        timeZoneSeconds = -timeZoneSeconds;

    return currentPosition;
}

double ESDateObject::parseStringToDate_2(escargot::ESString* istr, bool& haveTZ)
{
    haveTZ = true;
    const char* dateString = istr->utf8Data();

    static const long daysPerMonth[12] = { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    // The year must be present, but the other fields may be omitted - see ES5.1 15.9.1.15.
    int year = 0;
    long month = 1;
    long day = 1;
    long hours = 0;
    long minutes = 0;
    double seconds = 0;
    long timeZoneSeconds = 0;

    // Parse the date YYYY[-MM[-DD]]
    char* currentPosition = parseES5DatePortion(dateString, year, month, day);
    if (!currentPosition)
        return std::numeric_limits<double>::quiet_NaN();
    // Look for a time portion.
    if (*currentPosition == 'T') {
        haveTZ = false;
        // Parse the time HH:mm[:ss[.sss]][Z|(+|-)00:00]
        currentPosition = parseES5TimePortion(currentPosition + 1, hours, minutes, seconds, timeZoneSeconds, haveTZ);
        if (!currentPosition)
            return std::numeric_limits<double>::quiet_NaN();
    }
    // Check that we have parsed all characters in the string.
    if (*currentPosition)
        return std::numeric_limits<double>::quiet_NaN();

    // A few of these checks could be done inline above, but since many of them are interrelated
    // we would be sacrificing readability to "optimize" the (presumably less common) failure path.
    if (month < 1 || month > 12)
        return std::numeric_limits<double>::quiet_NaN();
    if (day < 1 || day > daysPerMonth[month - 1])
        return std::numeric_limits<double>::quiet_NaN();
    if (month == 2 && day > 28 && daysInYear(year) != 366)
        return std::numeric_limits<double>::quiet_NaN();
    if (hours < 0 || hours > 24)
        return std::numeric_limits<double>::quiet_NaN();
    if (hours == 24 && (minutes || seconds))
        return std::numeric_limits<double>::quiet_NaN();
    if (minutes < 0 || minutes > 59)
        return std::numeric_limits<double>::quiet_NaN();
    if (seconds < 0 || seconds >= 61)
        return std::numeric_limits<double>::quiet_NaN();
    if (seconds > 60) {
        // Discard leap seconds by clamping to the end of a minute.
        seconds = 60;
    }

    double dateSeconds = ymdhmsToSeconds(year, month - 1, day, hours, minutes, (int) seconds) - timeZoneSeconds;
    return dateSeconds * msPerSecond + (seconds - (int) seconds) * 1000;
}

double ESDateObject::parseStringToDate(escargot::ESString* istr)
{
    bool haveTZ;
    int offset;
    double primitiveValue = parseStringToDate_2(istr, haveTZ);
    if (!std::isnan(primitiveValue)) {
        if (!haveTZ) { // add local timezone offset
            primitiveValue = toUTC(primitiveValue);
        }
    } else {
        primitiveValue = parseStringToDate_1(istr, haveTZ, offset);
        if (!std::isnan(primitiveValue)) {
            if (!haveTZ) {
                primitiveValue = toUTC(primitiveValue);
            } else {
                primitiveValue = primitiveValue - (offset * msPerMinute);
            }
        }
    }

    if (primitiveValue <= options::MaximumDatePrimitiveValue && primitiveValue >= -options::MaximumDatePrimitiveValue) {
        return primitiveValue;
    } else {
        return std::numeric_limits<double>::quiet_NaN();
    }

/*    struct tm timeinfo;
    double primitiveValue = 0.0;
    timeinfo.tm_mday = 1; // set special initial case for mday. if we don't initialize it, it would be set to 0

    char* buffer = (char*)istr->toNullableUTF8String().m_buffer;
    char* parse_returned;
    size_t fmt_length = istr->length();

    parse_returned = strptime(buffer, "%Y", &timeinfo); // Date format with UTC timezone
    if (buffer + fmt_length == parse_returned) {
        primitiveValue = ymdhmsToSeconds(timeinfo.tm_year+1900, 0, 1, 0, 0, 0) * 1000.;
        return primitiveValue;
    }

    parse_returned = strptime(buffer, "%B %d %Y %H:%M:%S %z", &timeinfo); // Date format with specific timezone
    if (buffer + fmt_length == parse_returned) { 
        primitiveValue = ymdhmsToSeconds(timeinfo.tm_year+1900, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec) * 1000.;
#if defined(__USE_BSD) || defined(__USE_MISC)
        primitiveValue = primitiveValue - timeinfo.tm_gmtoff * 1000;
#else
        primitiveValue = primitiveValue - timeinfo.__tm_gmtoff * 1000;
#endif
        return primitiveValue;
    }

    parse_returned = strptime(buffer, "%A %B %d %Y %H:%M:%S GMT", &timeinfo); // Date format with specific timezone
    if (parse_returned) { // consider as "%A %B %d %Y %H:%M:%S GMT+9 (KST)" like format
        // TODO : consider of setting daylightsaving flag (timeinfo->tm_isdst)
        timeinfo.tm_gmtoff = 1 * 60 * 60; // 1 hour
        if (*parse_returned == '-') {
            timeinfo.tm_gmtoff *= -1;
        }
        parse_returned++;
        int tz = *parse_returned - '0';
        if (*(parse_returned+1) != ' ') {
            tz *= 10;
            tz += *(parse_returned+1) - '0';
            timeinfo.tm_gmtoff *= tz;
        } else {
            timeinfo.tm_gmtoff *= tz;
        }
//        if (buffer + fmt_length == parse_returned) { 
            primitiveValue = ymdhmsToSeconds(timeinfo.tm_year+1900, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec) * 1000.;
#if defined(__USE_BSD) || defined(__USE_MISC)
            primitiveValue = primitiveValue - timeinfo.tm_gmtoff * 1000;
#else
            primitiveValue = primitiveValue - timeinfo.__tm_gmtoff * 1000;
#endif
            return primitiveValue;
//        }
    }

    parse_returned = strptime(buffer, "%Y-%m-%dT%H:%M:%S.", &timeinfo); // Date format with UTC timezone
    if (buffer + fmt_length == parse_returned + 3) { // for milliseconds part
        primitiveValue = ymdhmsToSeconds(timeinfo.tm_year+1900, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec) * 1000.;
        return primitiveValue;
    }

    parse_returned = strptime(buffer, "%Y-%m-%dT%H:%M:%S.", &timeinfo); // Date format with UTC timezone
    if (buffer + fmt_length == parse_returned + 4) { // for milliseconds part and 'Z' part
        primitiveValue = ymdhmsToSeconds(timeinfo.tm_year+1900, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec) * 1000.;
        return primitiveValue;
    }

    parse_returned = strptime(buffer, "%m/%d/%Y %H:%M:%S", &timeinfo); // Date format with local timezone
    if (buffer + fmt_length == parse_returned) {
        primitiveValue = ymdhmsToSeconds(timeinfo.tm_year+1900, timeinfo.tm_mon, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec) * 1000.;
        primitiveValue = toUTC(primitiveValue);
        return primitiveValue;
    }
    return std::numeric_limits<double>::quiet_NaN();    
    */
}

int ESDateObject::daysInYear(int year)
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

int ESDateObject::dayFromYear(int year) // day number of the first day of year 'y'
{
    return 365 * (year - 1970) + floor((year - 1969) / 4.0) - floor((year - 1901) / 100.0) + floor((year - 1601) / 400.0);
}

int ESDateObject::yearFromTime(time64_t t)
{
    long estimate = ceil(t / msPerDay / 365.0) + 1970;

    while (makeDay(estimate, 0, 1) * msPerDay > t) {
        estimate--;
    }

    while (makeDay(estimate + 1, 0, 1) * msPerDay <= t) {
        estimate++;
    }


    return estimate;
}

int ESDateObject::inLeapYear(time64_t t)
{
    int days = daysInYear(yearFromTime(t));
    if (days == 365) {
        return 0;
    } else if (days == 366) {
        return 1;
    }
    RELEASE_ASSERT_NOT_REACHED();
}

int ESDateObject::dayFromMonth(int year, int month)
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

int ESDateObject::monthFromTime(time64_t t)
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

int ESDateObject::dateFromTime(time64_t t)
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

time64_t ESDateObject::makeDay(int year, int month, int date)
{
    // TODO: have to check whether year or month is infinity
//    if(year == infinity || month == infinity){
//        return nan;
//    }
    // adjustment on argument[0],[1] is performed at setTimeValue(with 7 arguments) function
    ASSERT(0 <= month && month < 12);
    long ym = year;
    int mn = month;
    time64_t t = timeFromYear(ym) + dayFromMonth(ym, mn) * msPerDay;
    return day(t) + date - 1;
}


time64_t ESDateObject::getSecondSundayInMarch(time64_t t)
{
    int year = yearFromTime(t);
    int leap = inLeapYear(t);

    time64_t march = timeFromYear(year) + (31 * msPerDay) + (28 * msPerDay) + (leap * msPerDay);

    int sundayCount = 0;
    bool flag = true;
    time64_t second_sunday;
    for (second_sunday = march; flag; second_sunday += msPerDay) {
        if ((((int) (day(second_sunday) + 4) % 7) + 7) % 7 == 0) {
            if (++sundayCount == 2)
                flag = false;
        }
    }

    return second_sunday - msPerDay;
}
time64_t ESDateObject::getFirstSundayInNovember(time64_t t)
{
    int year = yearFromTime(t);
    int leap = inLeapYear(t);

    time64_t nov = timeFromYear(year) + (31 * msPerDay) * 6 + (30 * msPerDay) * 3 + (28 * msPerDay) + (leap * msPerDay);

    time64_t first_sunday;
    for (first_sunday = nov; (((int) (day(first_sunday) + 4) % 7) + 7) % 7 > 0;
        first_sunday += msPerDay) { }
    return first_sunday;
}

// it should return -3600(sec.) if daylightsaving is applied
int ESDateObject::computeDaylightSaving(time64_t primitiveValue)
{
    time64_t primitiveValueToUTC = primitiveValue;

    if (ESVMInstance::currentInstance()->timezoneOffset() == 28800) {
        time64_t dst_start = getSecondSundayInMarch(primitiveValueToUTC) + 10 * msPerHour;
        time64_t dst_end = getFirstSundayInNovember(primitiveValueToUTC) + 10 * msPerHour;

        if (primitiveValueToUTC >= dst_start && primitiveValueToUTC < dst_end)
            return -secondsPerHour;
        else
            return 0.0;
    }

    return 0.0;


//    tm localTM = *localtime_r(&primitiveValueToUTC);

/*    if (localTM.tm_isdst) {
        return -secondsPerHour;
    } else {
        return 0.0;
    }*/
}
// code from WebKit 3369f50e501f85e27b6e7baffd0cc7ac70931cc3
// WTF/wtf/DateMath.cpp:340
int equivalentYearForDST(int year)
{
    // It is ok if the cached year is not the current year as long as the rules
    // for DST did not change between the two years; if they did the app would need
    // to be restarted.
    static int minYear = 2010;
    int maxYear = 2037;

    int difference;
    if (year > maxYear)
        difference = minYear - year;
    else if (year < minYear)
        difference = maxYear - year;
    else
        return year;

    int quotient = difference / 28;
    int product = (quotient) * 28;

    year += product;
    ASSERT((year >= minYear && year <= maxYear) || (product - year == static_cast<int>(std::numeric_limits<double>::quiet_NaN())));
    return year;
//    return product;
}

void ESDateObject::resolveCache()
{
    if (m_isCacheDirty) {
        struct timespec time;
        time.tv_sec = floor(m_primitiveValue / 1000.0);
        time.tv_nsec = (m_primitiveValue % 1000) * 1000000;
        if (time.tv_nsec < 0)
            time.tv_nsec = (time.tv_nsec + 1000000000) % 1000000000;

        int tmpyear = yearFromTime(m_primitiveValue - ESVMInstance::currentInstance()->timezoneOffset() * msPerSecond);

        if (tmpyear != equivalentYearForDST(tmpyear)) {
            time.tv_sec = time.tv_sec + (timeFromYear(equivalentYearForDST(tmpyear)) - timeFromYear(tmpyear)) / 1000;
        }
        memcpy(&m_cachedTM, ESVMInstance::currentInstance()->computeLocalTime(time), sizeof(tm));
        m_cachedTM.tm_year = tmpyear - 1900;
        m_isCacheDirty = false;
    }
}

ESString* ESDateObject::toDateString()
{
    static char days[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    static char months[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    resolveCache();
    char buffer[512];
    if (!isnan(timeValueAsDouble())) {
        snprintf(buffer, 512, "%s %s %02d %d"
            , days[getDay()], months[getMonth()], getDate(), getFullYear());
        return ESString::create(buffer);
    } else {
        return ESString::create(u"Invalid Date");
    } 
}

ESString* ESDateObject::toTimeString()
{
    resolveCache();
    char buffer[512];
    if (!isnan(timeValueAsDouble())) {
        int gmt = getTimezoneOffset() / -36;
        snprintf(buffer, 512, "%02d:%02d:%02d GMT%s%04d (%s)"
            , getHours(), getMinutes(), getSeconds()
            , (gmt < 0) ? "-" : "+"
            , std::abs(gmt), m_cachedTM.tm_isdst? tzname[1] : tzname[0]);
        return ESString::create(buffer);
    } else {
        return ESString::create(u"Invalid Date");
    }
}

ESString* ESDateObject::toFullString()
{

    if (!isnan(timeValueAsDouble())) {
        resolveCache();
        ::escargot::ESString* tmp = ESString::concatTwoStrings(toDateString(), ESString::create(u" "));
        return ESString::concatTwoStrings(tmp, toTimeString());
    } else {
        return ESString::create(u"Invalid Date");
    }
    
}
int ESDateObject::getDate()
{
    resolveCache();
    return m_cachedTM.tm_mday;
}

int ESDateObject::getDay()
{
    return (((int) (day(m_primitiveValue - getTimezoneOffset() * msPerSecond) + 4) % 7) + 7) % 7;
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
int ESDateObject::getMilliseconds()
{
    return (((long long) m_primitiveValue % (int) msPerSecond) + (int) msPerSecond) % (int) msPerSecond;
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
//    return ESVMInstance::currentInstance()->timezoneOffset();
    resolveCache();
    return -1 * m_cachedTM.tm_gmtoff;
}

void ESDateObject::setTime(double t)
{
    if (isnan(t)) {
        setTimeValueAsNaN();
        return;
    }

    m_primitiveValue = floor(t);
    
    if (m_primitiveValue <= 8640000000000000 && m_primitiveValue >= -8640000000000000) {
        m_isCacheDirty = true;
        m_hasValidDate = true;
    } else {
        setTimeValueAsNaN();
    }
}

int ESDateObject::getUTCDate()
{
    return dateFromTime(m_primitiveValue);
}

int ESDateObject::getUTCDay()
{
    return (((int) (day(m_primitiveValue) + 4) % 7) + 7) % 7;
}

int ESDateObject::getUTCFullYear()
{
    return yearFromTime(m_primitiveValue);
}

int ESDateObject::getUTCHours()
{
    return (((long long) floor(m_primitiveValue / msPerHour) % (int) hoursPerDay) + (int) hoursPerDay) % (int) hoursPerDay;
}
int ESDateObject::getUTCMilliseconds()
{
    return (((long long) m_primitiveValue % (int) msPerSecond) + (int) msPerSecond) % (int) msPerSecond;
}
int ESDateObject::getUTCMinutes()
{
    return (((long long) floor(m_primitiveValue / msPerMinute) % (int) minutesPerHour) + (int) minutesPerHour) % (int) minutesPerHour;
}

int ESDateObject::getUTCMonth()
{
    return monthFromTime(m_primitiveValue);
}

int ESDateObject::getUTCSeconds()
{
    return (((long long) floor(m_primitiveValue / msPerSecond) % (int) secondsPerMinute) + (int) secondsPerMinute) % (int) secondsPerMinute;
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

///////////////////////////////////////////////////////////////////////////////
//// CODE FROM JAVASCRIPTCORE /////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

// decompose 'number' to its sign, exponent, and mantissa components.
// The result is interpreted as:
//     (sign ? -1 : 1) * pow(2, exponent) * (mantissa / (1 << 52))
inline void decomposeDouble(double number, bool& sign, int32_t& exponent, uint64_t& mantissa)
{
    ASSERT(std::isfinite(number));

    sign = std::signbit(number);

    uint64_t bits = bitwise_cast<uint64_t>(number);
    exponent = (static_cast<int32_t>(bits >> 52) & 0x7ff) - 0x3ff;
    mantissa = bits & 0xFFFFFFFFFFFFFull;

    // Check for zero/denormal values; if so, adjust the exponent,
    // if not insert the implicit, omitted leading 1 bit.
    if (exponent == -0x3ff)
        exponent = mantissa ? -0x3fe : 0;
    else
        mantissa |= 0x10000000000000ull;
}

// This is used in converting the integer part of a number to a string.
class BigInteger {
public:
    BigInteger(double number)
    {
        m_values.reserve(36);
        ASSERT(std::isfinite(number) && !std::signbit(number));
        ASSERT(number == floor(number));

        bool sign;
        int32_t exponent;
        uint64_t mantissa;
        decomposeDouble(number, sign, exponent, mantissa);
        ASSERT(!sign && exponent >= 0);

        int32_t zeroBits = exponent - 52;

        if (zeroBits < 0) {
            mantissa >>= -zeroBits;
            zeroBits = 0;
        }

        while (zeroBits >= 32) {
            m_values.push_back(0);
            zeroBits -= 32;
        }

        // Left align the 53 bits of the mantissa within 96 bits.
        uint32_t values[3];
        values[0] = static_cast<uint32_t>(mantissa);
        values[1] = static_cast<uint32_t>(mantissa >> 32);
        values[2] = 0;
        // Shift based on the remainder of the exponent.
        if (zeroBits) {
            values[2] = values[1] >> (32 - zeroBits);
            values[1] = (values[1] << zeroBits) | (values[0] >> (32 - zeroBits));
            values[0] = (values[0] << zeroBits);
        }
        m_values.push_back(values[0]);
        m_values.push_back(values[1]);
        m_values.push_back(values[2]);

        // Canonicalize; remove all trailing zeros.
        while (m_values.size() && !m_values.back())
            m_values.pop_back();
    }

    uint32_t divide(uint32_t divisor)
    {
        uint32_t carry = 0;

        for (size_t i = m_values.size(); i; ) {
            --i;
            uint64_t dividend = (static_cast<uint64_t>(carry) << 32) + static_cast<uint64_t>(m_values[i]);

            uint64_t result = dividend / static_cast<uint64_t>(divisor);
            ASSERT(result == static_cast<uint32_t>(result));
            uint64_t remainder = dividend % static_cast<uint64_t>(divisor);
            ASSERT(remainder == static_cast<uint32_t>(remainder));

            m_values[i] = static_cast<uint32_t>(result);
            carry = static_cast<uint32_t>(remainder);
        }

        // Canonicalize; remove all trailing zeros.
        while (m_values.size() && !m_values.back())
            m_values.pop_back();

        return carry;
    }

    bool operator!() { return !m_values.size(); }

private:
    std::vector<uint32_t> m_values;
};

// Would be nice if this was a static const member, but the OS X linker
// seems to want a symbol in the binary in that case...
#define oneGreaterThanMaxUInt16 0x10000

// A uint16_t with an infinite precision fraction. Upon overflowing
// the uint16_t range, this class will clamp to oneGreaterThanMaxUInt16.
// This is used in converting the fraction part of a number to a string.
class Uint16WithFraction {
public:
    explicit Uint16WithFraction(double number, uint16_t divideByExponent = 0)
    {
        m_values.reserve(36);
        ASSERT(number && std::isfinite(number) && !std::signbit(number));

        // Check for values out of uint16_t range.
        if (number >= oneGreaterThanMaxUInt16) {
            m_values.push_back(oneGreaterThanMaxUInt16);
            m_leadingZeros = 0;
            return;
        }

        // Append the units to m_values.
        double integerPart = floor(number);
        m_values.push_back(static_cast<uint32_t>(integerPart));

        bool sign;
        int32_t exponent;
        uint64_t mantissa;
        decomposeDouble(number - integerPart, sign, exponent, mantissa);
        ASSERT(!sign && exponent < 0);
        exponent -= divideByExponent;

        int32_t zeroBits = -exponent;
        --zeroBits;

        // Append the append words for to m_values.
        while (zeroBits >= 32) {
            m_values.push_back(0);
            zeroBits -= 32;
        }

        // Left align the 53 bits of the mantissa within 96 bits.
        uint32_t values[3];
        values[0] = static_cast<uint32_t>(mantissa >> 21);
        values[1] = static_cast<uint32_t>(mantissa << 11);
        values[2] = 0;
        // Shift based on the remainder of the exponent.
        if (zeroBits) {
            values[2] = values[1] << (32 - zeroBits);
            values[1] = (values[1] >> zeroBits) | (values[0] << (32 - zeroBits));
            values[0] = (values[0] >> zeroBits);
        }
        m_values.push_back(values[0]);
        m_values.push_back(values[1]);
        m_values.push_back(values[2]);

        // Canonicalize; remove any trailing zeros.
        while (m_values.size() > 1 && !m_values.back())
            m_values.pop_back();

        // Count the number of leading zero, this is useful in optimizing multiplies.
        m_leadingZeros = 0;
        while (m_leadingZeros < m_values.size() && !m_values[m_leadingZeros])
            ++m_leadingZeros;
    }

    Uint16WithFraction& operator*=(uint16_t multiplier)
    {
        ASSERT(checkConsistency());

        // iteratate backwards over the fraction until we reach the leading zeros,
        // passing the carry from one calculation into the next.
        uint64_t accumulator = 0;
        for (size_t i = m_values.size(); i > m_leadingZeros; ) {
            --i;
            accumulator += static_cast<uint64_t>(m_values[i]) * static_cast<uint64_t>(multiplier);
            m_values[i] = static_cast<uint32_t>(accumulator);
            accumulator >>= 32;
        }

        if (!m_leadingZeros) {
            // With a multiplicand and multiplier in the uint16_t range, this cannot carry
            // (even allowing for the infinity value).
            ASSERT(!accumulator);
            // Check for overflow & clamp to 'infinity'.
            if (m_values[0] >= oneGreaterThanMaxUInt16) {
                ASSERT(m_values.size() <= 1);
                m_values.shrink_to_fit();
                m_values[0] = oneGreaterThanMaxUInt16;
                m_leadingZeros = 0;
                return *this;
            }
        } else if (accumulator) {
            // Check for carry from the last multiply, if so overwrite last leading zero.
            m_values[--m_leadingZeros] = static_cast<uint32_t>(accumulator);
            // The limited range of the multiplier should mean that even if we carry into
            // the units, we don't need to check for overflow of the uint16_t range.
            ASSERT(m_values[0] < oneGreaterThanMaxUInt16);
        }

        // Multiplication by an even value may introduce trailing zeros; if so, clean them
        // up. (Keeping the value in a normalized form makes some of the comparison operations
        // more efficient).
        while (m_values.size() > 1 && !m_values.back())
            m_values.pop_back();
        ASSERT(checkConsistency());
        return *this;
    }

    bool operator<(const Uint16WithFraction& other)
    {
        ASSERT(checkConsistency());
        ASSERT(other.checkConsistency());

        // Iterate over the common lengths of arrays.
        size_t minSize = std::min(m_values.size(), other.m_values.size());
        for (size_t index = 0; index < minSize; ++index) {
            // If we find a value that is not equal, compare and return.
            uint32_t fromThis = m_values[index];
            uint32_t fromOther = other.m_values[index];
            if (fromThis != fromOther)
                return fromThis < fromOther;
        }
        // If these numbers have the same lengths, they are equal,
        // otherwise which ever number has a longer fraction in larger.
        return other.m_values.size() > minSize;
    }

    // Return the floor (non-fractional portion) of the number, clearing this to zero,
    // leaving the fractional part unchanged.
    uint32_t floorAndSubtract()
    {
        // 'floor' is simple the integer portion of the value.
        uint32_t floor = m_values[0];

        // If floor is non-zero,
        if (floor) {
            m_values[0] = 0;
            m_leadingZeros = 1;
            while (m_leadingZeros < m_values.size() && !m_values[m_leadingZeros])
                ++m_leadingZeros;
        }

        return floor;
    }

    // Compare this value to 0.5, returns -1 for less than, 0 for equal, 1 for greater.
    int comparePoint5()
    {
        ASSERT(checkConsistency());
        // If units != 0, this is greater than 0.5.
        if (m_values[0])
            return 1;
        // If size == 1 this value is 0, hence < 0.5.
        if (m_values.size() == 1)
            return -1;
        // Compare to 0.5.
        if (m_values[1] > 0x80000000ul)
            return 1;
        if (m_values[1] < 0x80000000ul)
            return -1;
        // Check for more words - since normalized numbers have no trailing zeros, if
        // there are more that two digits we can assume at least one more is non-zero,
        // and hence the value is > 0.5.
        return m_values.size() > 2 ? 1 : 0;
    }

    // Return true if the sum of this plus addend would be greater than 1.
    bool sumGreaterThanOne(const Uint16WithFraction& addend)
    {
        ASSERT(checkConsistency());
        ASSERT(addend.checkConsistency());

        // First, sum the units. If the result is greater than one, return true.
        // If equal to one, return true if either number has a fractional part.
        uint32_t sum = m_values[0] + addend.m_values[0];
        if (sum)
            return sum > 1 || std::max(m_values.size(), addend.m_values.size()) > 1;

        // We could still produce a result greater than zero if addition of the next
        // word from the fraction were to carry, leaving a result > 0.

        // Iterate over the common lengths of arrays.
        size_t minSize = std::min(m_values.size(), addend.m_values.size());
        for (size_t index = 1; index < minSize; ++index) {
            // Sum the next word from this & the addend.
            uint32_t fromThis = m_values[index];
            uint32_t fromAddend = addend.m_values[index];
            sum = fromThis + fromAddend;

            // Check for overflow. If so, check whether the remaining result is non-zero,
            // or if there are any further words in the fraction.
            if (sum < fromThis)
                return sum || (index + 1) < std::max(m_values.size(), addend.m_values.size());

            // If the sum is uint32_t max, then we would carry a 1 if addition of the next
            // digits in the number were to overflow.
            if (sum != 0xFFFFFFFF)
                return false;
        }
        return false;
    }

private:
    bool checkConsistency() const
    {
        // All values should have at least one value.
        return (m_values.size())
            // The units value must be a uint16_t, or the value is the overflow value.
            && (m_values[0] < oneGreaterThanMaxUInt16 || (m_values[0] == oneGreaterThanMaxUInt16 && m_values.size() == 1))
            // There should be no trailing zeros (unless this value is zero!).
            && (m_values.back() || m_values.size() == 1);
    }

    // The internal storage of the number. This vector is always at least one entry in size,
    // with the first entry holding the portion of the number greater than zero. The first
    // value always hold a value in the uint16_t range, or holds the value oneGreaterThanMaxUInt16 to
    // indicate the value has overflowed to >= 0x10000. If the units value is oneGreaterThanMaxUInt16,
    // there can be no fraction (size must be 1).
    //
    // Subsequent values in the array represent portions of the fractional part of this number.
    // The total value of the number is the sum of (m_values[i] / pow(2^32, i)), for each i
    // in the array. The vector should contain no trailing zeros, except for the value '0',
    // represented by a vector contianing a single zero value. These constraints are checked
    // by 'checkConsistency()', above.
    //
    // The inline capacity of the vector is set to be able to contain any IEEE double (1 for
    // the units column, 32 for zeros introduced due to an exponent up to -3FE, and 2 for
    // bits taken from the mantissa).
    std::vector<uint32_t> m_values;

    // Cache a count of the number of leading zeros in m_values. We can use this to optimize
    // methods that would otherwise need visit all words in the vector, e.g. multiplication.
    size_t m_leadingZeros;
};



// Mapping from integers 0..35 to digit identifying this value, for radix 2..36.
static const char radixDigits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

char* ESNumberObject::toStringWithRadix(RadixBuffer& buffer, double number, unsigned radix)
{
    ASSERT(std::isfinite(number));
    ASSERT(radix >= 2 && radix <= 36);

    // Position the decimal point at the center of the string, set
    // the startOfResultString pointer to point at the decimal point.
    char* decimalPoint = buffer + sizeof(buffer) / 2;
    char* startOfResultString = decimalPoint;

    // Extract the sign.
    bool isNegative = number < 0;
    if (std::signbit(number))
        number = -number;
    double integerPart = floor(number);

    // We use this to test for odd values in odd radix bases.
    // Where the base is even, (e.g. 10), to determine whether a value is even we need only
    // consider the least significant digit. For example, 124 in base 10 is even, because '4'
    // is even. if the radix is odd, then the radix raised to an integer power is also odd.
    // E.g. in base 5, 124 represents (1 * 125 + 2 * 25 + 4 * 5). Since each digit in the value
    // is multiplied by an odd number, the result is even if the sum of all digits is even.
    //
    // For the integer portion of the result, we only need test whether the integer value is
    // even or odd. For each digit of the fraction added, we should invert our idea of whether
    // the number is odd if the new digit is odd.
    //
    // Also initialize digit to this value; for even radix values we only need track whether
    // the last individual digit was odd.
    bool integerPartIsOdd = integerPart <= static_cast<double>(0x1FFFFFFFFFFFFFull) && static_cast<int64_t>(integerPart) & 1;
    ASSERT(integerPartIsOdd == static_cast<bool>(fmod(integerPart, 2)));
    bool isOddInOddRadix = integerPartIsOdd;
    uint32_t digit = integerPartIsOdd;

    // Check if the value has a fractional part to convert.
    double fractionPart = number - integerPart;
    if (fractionPart) {
        // Write the decimal point now.
        *decimalPoint = '.';

        // Higher precision representation of the fractional part.
        Uint16WithFraction fraction(fractionPart);

        bool needsRoundingUp = false;
        char* endOfResultString = decimalPoint + 1;

        // Calculate the delta from the current number to the next & previous possible IEEE numbers.
        double nextNumber = nextafter(number, std::numeric_limits<double>::infinity());
        double lastNumber = nextafter(number, -std::numeric_limits<double>::infinity());
        ASSERT(std::isfinite(nextNumber) && !std::signbit(nextNumber));
        ASSERT(std::isfinite(lastNumber) && !std::signbit(lastNumber));
        double deltaNextDouble = nextNumber - number;
        double deltaLastDouble = number - lastNumber;
        ASSERT(std::isfinite(deltaNextDouble) && !std::signbit(deltaNextDouble));
        ASSERT(std::isfinite(deltaLastDouble) && !std::signbit(deltaLastDouble));

        // We track the delta from the current value to the next, to track how many digits of the
        // fraction we need to write. For example, if the value we are converting is precisely
        // 1.2345, so far we have written the digits "1.23" to a string leaving a remainder of
        // 0.45, and we want to determine whether we can round off, or whether we need to keep
        // appending digits ('4'). We can stop adding digits provided that then next possible
        // lower IEEE value is further from 1.23 than the remainder we'd be rounding off (0.45),
        // which is to say, less than 1.2255. Put another way, the delta between the prior
        // possible value and this number must be more than 2x the remainder we'd be rounding off
        // (or more simply half the delta between numbers must be greater than the remainder).
        //
        // Similarly we need track the delta to the next possible value, to dertermine whether
        // to round up. In almost all cases (other than at exponent boundaries) the deltas to
        // prior and subsequent values are identical, so we don't need track then separately.
        if (deltaNextDouble != deltaLastDouble) {
            // Since the deltas are different track them separately. Pre-multiply by 0.5.
            Uint16WithFraction halfDeltaNext(deltaNextDouble, 1);
            Uint16WithFraction halfDeltaLast(deltaLastDouble, 1);

            while (true) {
                // examine the remainder to determine whether we should be considering rounding
                // up or down. If remainder is precisely 0.5 rounding is to even.
                int dComparePoint5 = fraction.comparePoint5();
                if (dComparePoint5 > 0 || (!dComparePoint5 && (radix & 1 ? isOddInOddRadix : digit & 1))) {
                    // Check for rounding up; are we closer to the value we'd round off to than
                    // the next IEEE value would be?
                    if (fraction.sumGreaterThanOne(halfDeltaNext)) {
                        needsRoundingUp = true;
                        break;
                    }
                } else {
                    // Check for rounding down; are we closer to the value we'd round off to than
                    // the prior IEEE value would be?
                    if (fraction < halfDeltaLast)
                        break;
                }

                ASSERT(endOfResultString < (buffer + sizeof(buffer) - 1));
                // Write a digit to the string.
                fraction *= radix;
                digit = fraction.floorAndSubtract();
                *endOfResultString++ = radixDigits[digit];
                // Keep track whether the portion written is currently even, if the radix is odd.
                if (digit & 1)
                    isOddInOddRadix = !isOddInOddRadix;

                // Shift the fractions by radix.
                halfDeltaNext *= radix;
                halfDeltaLast *= radix;
            }
        } else {
            // This code is identical to that above, except since deltaNextDouble != deltaLastDouble
            // we don't need to track these two values separately.
            Uint16WithFraction halfDelta(deltaNextDouble, 1);

            while (true) {
                int dComparePoint5 = fraction.comparePoint5();
                if (dComparePoint5 > 0 || (!dComparePoint5 && (radix & 1 ? isOddInOddRadix : digit & 1))) {
                    if (fraction.sumGreaterThanOne(halfDelta)) {
                        needsRoundingUp = true;
                        break;
                    }
                } else if (fraction < halfDelta)
                    break;

                ASSERT(endOfResultString < (buffer + sizeof(buffer) - 1));
                fraction *= radix;
                digit = fraction.floorAndSubtract();
                if (digit & 1)
                    isOddInOddRadix = !isOddInOddRadix;
                *endOfResultString++ = radixDigits[digit];

                halfDelta *= radix;
            }
        }

        // Check if the fraction needs rounding off (flag set in the loop writing digits, above).
        if (needsRoundingUp) {
            // Whilst the last digit is the maximum in the current radix, remove it.
            // e.g. rounding up the last digit in "12.3999" is the same as rounding up the
            // last digit in "12.3" - both round up to "12.4".
            while (endOfResultString[-1] == radixDigits[radix - 1])
                --endOfResultString;

            // Radix digits are sequential in ascii/unicode, except for '9' and 'a'.
            // E.g. the first 'if' case handles rounding 67.89 to 67.8a in base 16.
            // The 'else if' case handles rounding of all other digits.
            if (endOfResultString[-1] == '9')
                endOfResultString[-1] = 'a';
            else if (endOfResultString[-1] != '.')
                ++endOfResultString[-1];
            else {
                // One other possibility - there may be no digits to round up in the fraction
                // (or all may be been rounded off already), in which case we may need to
                // round into the integer portion of the number. Remove the decimal point.
                --endOfResultString;
                // In order to get here there must have been a non-zero fraction, in which case
                // there must be at least one bit of the value's mantissa not in use in the
                // integer part of the number. As such, adding to the integer part should not
                // be able to lose precision.
                ASSERT((integerPart + 1) - integerPart == 1);
                ++integerPart;
            }
        } else {
            // We only need to check for trailing zeros if the value does not get rounded up.
            while (endOfResultString[-1] == '0')
                --endOfResultString;
        }

        *endOfResultString = '\0';
        ASSERT(endOfResultString < buffer + sizeof(buffer));
    } else
        *decimalPoint = '\0';

    BigInteger units(integerPart);

    // Always loop at least once, to emit at least '0'.
    do {
        ASSERT(buffer < startOfResultString);

        // Read a single digit and write it to the front of the string.
        // Divide by radix to remove one digit from the value.
        digit = units.divide(radix);
        *--startOfResultString = radixDigits[digit];
    } while (!!units);

    // If the number is negative, prepend '-'.
    if (isNegative)
        *--startOfResultString = '-';
    ASSERT(buffer <= startOfResultString);

    return startOfResultString;
}
//
///////////////////////////////////////////////////////////////////////////////
//// CODE FROM JAVASCRIPTCORE ENDS ////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////


ESBooleanObject::ESBooleanObject(bool value)
    : ESObject((Type)(Type::ESObject | Type::ESBooleanObject), ESVMInstance::currentInstance()->globalObject()->booleanPrototype())
{
    m_primitiveValue = value;
}

ESErrorObject::ESErrorObject(escargot::ESString* message, Code code)
    : ESObject((Type)(Type::ESObject | Type::ESErrorObject), ESVMInstance::currentInstance()->globalObject()->errorPrototype())
    , m_code(code)
{
    if (message != strings->emptyString.string())
        set(strings->message, message);
}

ESErrorObject* ESErrorObject::create(escargot::ESString* message, Code code)
{
    switch (code) {
    case ESErrorObject::Code::TypeError:
        return TypeError::create(message);
    case ESErrorObject::Code::ReferenceError:
        return ReferenceError::create(message);
    case ESErrorObject::Code::SyntaxError:
        return SyntaxError::create(message);
    case ESErrorObject::Code::RangeError:
        return RangeError::create(message);
    case ESErrorObject::Code::URIError:
        return URIError::create(message);
    case ESErrorObject::Code::EvalError:
        return EvalError::create(message);
    default:
        return new ESErrorObject(message, code);
    }
}

ReferenceError::ReferenceError(escargot::ESString* message)
    : ESErrorObject(message, Code::ReferenceError)
{
    set(strings->name, strings->ReferenceError.string());
    set__proto__(ESVMInstance::currentInstance()->globalObject()->referenceErrorPrototype());
}

TypeError::TypeError(escargot::ESString* message)
    : ESErrorObject(message, Code::TypeError)
{
    set(strings->name, strings->TypeError.string());
    set__proto__(ESVMInstance::currentInstance()->globalObject()->typeErrorPrototype());
}

RangeError::RangeError(escargot::ESString* message)
    : ESErrorObject(message, Code::RangeError)
{
    set(strings->name, strings->RangeError.string());
    set__proto__(ESVMInstance::currentInstance()->globalObject()->rangeErrorPrototype());
}

SyntaxError::SyntaxError(escargot::ESString* message)
    : ESErrorObject(message, Code::SyntaxError)
{
    set(strings->name, strings->SyntaxError.string());
    set__proto__(ESVMInstance::currentInstance()->globalObject()->syntaxErrorPrototype());
}

URIError::URIError(escargot::ESString* message)
    : ESErrorObject(message, Code::URIError)
{
    set(strings->name, strings->URIError.string());
    set__proto__(ESVMInstance::currentInstance()->globalObject()->uriErrorPrototype());
}

EvalError::EvalError(escargot::ESString* message)
    : ESErrorObject(message, Code::EvalError)
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

ESValue ESTypedArrayObjectWrapper::get(uint32_t key)
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
bool ESTypedArrayObjectWrapper::set(uint32_t key, ESValue val)
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

ESArgumentsObject::ESArgumentsObject(FunctionEnvironmentRecordWithArgumentsObject* environment)
    : ESObject((Type)(Type::ESObject | Type::ESArgumentsObject), ESVMInstance::currentInstance()->globalObject()->objectPrototype(), 6)
    , m_environment(environment)
{
}

ESJSONObject::ESJSONObject(ESPointer::Type type)
    : ESObject((Type)(Type::ESObject | Type::ESJSONObject), ESVMInstance::currentInstance()->globalObject()->objectPrototype(), 6)
{
}

void ESPropertyAccessorData::setGetterAndSetterTo(ESObject* obj, const ESHiddenClassPropertyInfo* propertyInfo)
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
        obj->set(strings->writable.string(), ESValue(propertyInfo->writable()));
        return;
    }
}

}
