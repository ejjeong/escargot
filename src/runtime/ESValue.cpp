#include "Escargot.h"
#include "ESValue.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#include "runtime/NullableString.h"
#include "ast/AST.h"

#include "Yarr.h"

namespace escargot {

/*
static ESNumber s_nan(std::numeric_limits<double>::quiet_NaN());
ESNumber* esNaN = &s_nan;

static ESNumber s_infinity(std::numeric_limits<double>::infinity());
ESNumber* esInfinity = &s_infinity;
static ESNumber s_ninfinity(-std::numeric_limits<double>::infinity());
ESNumber* esNegInfinity = &s_ninfinity;

static ESNumber s_nzero(-0.0);
ESNumber* esMinusZero = &s_nzero;
*/

// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-abstract-equality-comparison
bool ESValue::abstractEqualsTo(const ESValue& val)
{
    if (isInt32() && val.isInt32()) {
        return asInt32() == val.asInt32();
    } else if (isNumber() && val.isNumber()) {
        double a = asNumber();
        double b = val.asNumber();
        if (std::isnan(a) || std::isnan(b)) return false;
        else if (a == b) return true;
        else return false;
    } else if (isUndefined() && val.isUndefined()) {
        return true;
    } else if (isNull() && val.isNull()) {
        return true;
    } else if (isBoolean() && val.isBoolean()) {
        return asBoolean() == val.asBoolean();
    } else if (isESString() && val.isESString()) {
        return *asESString() == *val.asESString();
    } else if (isESPointer() && val.isESPointer()) {
        ESPointer* o = asESPointer();
        ESPointer* comp = val.asESPointer();
        if (o->type() == comp->type())
            return equalsTo(val);
    } else {
        if (isNull() && val.isUndefined()) return true;
        else if (isUndefined() && val.isNull()) return true;
        else if (isNumber() && (val.isESString() || val.isBoolean())) {
            return asNumber() == val.toNumber();
        }
        else if ((isESString() || isBoolean()) && val.isNumber()) {
            return val.asNumber() == toNumber();
        }
        else if ((isESString() || isNumber()) && val.isObject()) {
            return abstractEqualsTo(val.toPrimitive());
        }
        else if (isObject() && (val.isESString() || val.isNumber())) {
            return toPrimitive().abstractEqualsTo(val);
        }
    }
    return false;
}

bool ESValue::equalsTo(const ESValue& val)
{
    if(isNumber()) {
        return asNumber() == val.toNumber();
    }
    if(isBoolean())
        return asBoolean() == val.toBoolean();

    if(isESPointer()) {
        ESPointer* o = asESPointer();
        if (!val.isESPointer())
            return false;
        ESPointer* o2 = val.asESPointer();
        if (o->type() != o2->type())
            return false;
        //Strict Equality Comparison: ===
        if(o->isESString() && o->asESString()->string() == o2->asESString()->string())
            return true;
        if(o->isESStringObject() && o->asESStringObject()->getStringData()->string() == o2->asESStringObject()->getStringData()->string())
            return true;
        //TODO
        return false;
    }
    RELEASE_ASSERT_NOT_REACHED();
}



ESObject* ESValue::toObject() const
{
    ESFunctionObject* function;
    ESObject* receiver;
    if (isNumber()) {
        function = ESVMInstance::currentInstance()->globalObject()->number();
        receiver = ESNumberObject::create(toNumber());
    } else if (isBoolean()) {
        function = ESVMInstance::currentInstance()->globalObject()->boolean();
        ESValue ret;
        if (toBoolean())
            ret = ESValue(ESValue::ESTrueTag::ESTrue);
        else
            ret = ESValue(ESValue::ESFalseTag::ESFalse);
        receiver = ESBooleanObject::create(ret);
    } else if (isESString()) {
        function = ESVMInstance::currentInstance()->globalObject()->string();
        receiver = ESStringObject::create(asESPointer()->asESString());
    } else if (isESPointer() && asESPointer()->isESObject()) {
        return asESPointer()->asESObject();
    } else if(isNull()){
        throw ESValue(TypeError::create(ESString::create(u"cannot convert null into object")));
    } else if(isUndefined()){
        throw ESValue(TypeError::create(ESString::create(u"cannot convert undefined into object")));
    } else {
        RELEASE_ASSERT_NOT_REACHED();
    }
    receiver->setConstructor(function);
    receiver->set__proto__(function->protoType());
    return receiver;
}


ESValue ESObject::valueOf()
{
    if(isESDateObject())
        return asESDateObject()->valueOf();
    else if (isESStringObject())
        return asESStringObject()->valueOf();
    else if(isESArrayObject())
        // Array.prototype do not have valueOf() function
        RELEASE_ASSERT_NOT_REACHED();
    else
        return ESValue(this).toString();

    /*
    if (hint == ESValue::PreferString) {
        ESValue underScoreProto = get(strings->__proto__);
        ESValue toStringMethod = underScoreProto.asESPointer()->asESObject()->get(u"toString");
        std::vector<ESValue> arguments;
        return ESFunctionObject::call(toStringMethod, this, &arguments[0], arguments.size(), ESVMInstance::currentInstance());
    } else {
        //TODO
        RELEASE_ASSERT_NOT_REACHED();
    }
    */
}

ESString* ESString::substring(int from, int to) const
{
    ASSERT(0 <= from && from <= to && to <= (int)length());
    if(UNLIKELY(m_string == NULL)) {
        escargot::ESRopeString* rope = (escargot::ESRopeString *)this;
        if(to - from == 1) {
            int len_left = rope->m_left->length();
            char16_t c;
            if (to <= len_left) {
                c = rope->m_left->stringData()->c_str()[from];
            } else {
                c = rope->m_left->stringData()->c_str()[from - len_left];
            }
            if(c < ESCARGOT_ASCII_TABLE_MAX) {
                return strings->asciiTable[c];
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
    if(to - from == 1) {
        if(string()[from] < ESCARGOT_ASCII_TABLE_MAX) {
            return strings->asciiTable[string()[from]];
        }
    }
    u16string ret(std::move(m_string->substr(from, to-from)));
    return ESString::create(std::move(ret));
}


bool ESString::match(ESPointer* esptr, RegexMatchResult& matchResult, bool testOnly) const
{
    //NOTE to build normal string(for rope-string), we should call ensureNormalString();
    ensureNormalString();

    ESRegExpObject::Option option = ESRegExpObject::Option::None;
    const u16string* regexSource;
    JSC::Yarr::BytecodePattern* byteCode = NULL;
    if (esptr->isESRegExpObject()) {
        escargot::ESRegExpObject* o = esptr->asESRegExpObject();
        regexSource = &esptr->asESRegExpObject()->source()->string();
        option = esptr->asESRegExpObject()->option();
        byteCode = esptr->asESRegExpObject()->bytecodePattern();
    } else if (esptr->isESString()) {
        regexSource = &esptr->asESString()->string();
    } else {
        //TODO
        RELEASE_ASSERT_NOT_REACHED();
    }

    bool isGlobal = option & ESRegExpObject::Option::Global;
    if(!byteCode) {
        JSC::Yarr::ErrorCode yarrError;
        JSC::Yarr::YarrPattern yarrPattern(*regexSource, option & ESRegExpObject::Option::IgnoreCase, option & ESRegExpObject::Option::MultiLine, &yarrError);
        if (yarrError) {
            matchResult.m_subPatternNum = 0;
            return false;
        }
        WTF::BumpPointerAllocator *bumpAlloc = ESVMInstance::currentInstance()->bumpPointerAllocator();
        JSC::Yarr::OwnPtr<JSC::Yarr::BytecodePattern> ownedBytecode = JSC::Yarr::byteCompileEscargot(yarrPattern, bumpAlloc);
        byteCode = ownedBytecode.leakPtr();
        if(esptr->isESRegExpObject()) {
            esptr->asESRegExpObject()->setBytecodePattern(byteCode);
        }
    }

    unsigned subPatternNum = byteCode->m_body->m_numSubpatterns;
    matchResult.m_subPatternNum = (int) subPatternNum;
    size_t length = m_string->length();
    if(length) {
        size_t start = 0;
        unsigned result = 0;
        const char16_t *chars = m_string->data();
        unsigned* outputBuf = (unsigned int*)alloca(sizeof (unsigned) * 2 * (subPatternNum + 1));
        outputBuf[1] = 0;
        do {
            start = outputBuf[1];
            result = JSC::Yarr::interpret(NULL, byteCode, chars, length, start, outputBuf);
            if(result != JSC::Yarr::offsetNoMatch) {
                if(UNLIKELY(testOnly)) {
                    return true;
                }
                std::vector<ESString::RegexMatchResult::RegexMatchResultPiece> piece;
                piece.reserve(subPatternNum + 1);
                for(unsigned i = 0; i < subPatternNum + 1 ; i ++) {
                    ESString::RegexMatchResult::RegexMatchResultPiece p;
                    p.m_start = outputBuf[i*2];
                    p.m_end = outputBuf[i*2 + 1];
                    piece.push_back(p);
                }
                matchResult.m_matchResults.push_back(std::move(piece));
                if(!isGlobal)
                    break;
            } else {
                break;
            }
        } while(result != JSC::Yarr::offsetNoMatch);
    }
    return matchResult.m_matchResults.size();
}

ESObject::ESObject(ESPointer::Type type)
    : ESPointer(type)
    , m_map(NULL)
{
    //m_map = new(GC) ESObjectMap(16);
    //m_hiddenClass = nullptr;
    m_hiddenClassData.reserve(6);
    m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForObject();

    //definePropertyOrThrow(strings->constructor, true, false, false);
    //defineAccessorProperty(strings->__proto__, ESVMInstance::currentInstance()->object__proto__AccessorData(), true, false, false);

    m_hiddenClassData.push_back(ESValue(ESValue::ESUndefined));
    m_hiddenClassData.push_back(ESValue((ESPointer *)ESVMInstance::currentInstance()->object__proto__AccessorData()));

    //convertIntoMapMode();
}

ESArrayObject::ESArrayObject(int length)
    : ESObject((Type)(Type::ESObject | Type::ESArrayObject))
    , m_vector(0)
    , m_fastmode(true)
{
    m_length = 0;
    if (length == -1)
        convertToSlowMode();
    else {
        setLength(length);
    }

    m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForArrayObject();
    m_hiddenClassData.push_back(ESValue((ESPointer *)ESVMInstance::currentInstance()->arrayLengthAccessorData()));
}

ESRegExpObject::ESRegExpObject(escargot::ESString* source, const Option& option)
    : ESObject((Type)(Type::ESObject | Type::ESRegExpObject))
{
    m_source = source;
    m_option = option;
    m_bytecodePattern = NULL;

}

void ESRegExpObject::setSource(escargot::ESString* src)
{
    m_bytecodePattern = NULL;
    m_source = src;
}
void ESRegExpObject::setOption(const Option& option)
{
    m_bytecodePattern = NULL;
    m_option = option;
}

ESFunctionObject::ESFunctionObject(LexicalEnvironment* outerEnvironment, FunctionNode* functionAST, ESObject* proto)
    : ESObject((Type)(Type::ESObject | Type::ESFunctionObject))
{
    m_outerEnvironment = outerEnvironment;
    m_functionAST = functionAST;

    //defineAccessorProperty(strings->prototype, ESVMInstance::currentInstance()->functionPrototypeAccessorData(), true, false, false);
    //definePropertyOrThrow(strings->name);

    m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForFunction();
    m_hiddenClassData.push_back(ESValue((ESPointer *)ESVMInstance::currentInstance()->functionPrototypeAccessorData()));
    m_hiddenClassData.push_back(ESValue(ESValue::ESUndefined));

    if (proto != NULL)
        set__proto__(proto);
    else
        set__proto__(ESVMInstance::currentInstance()->globalFunctionPrototype());
}

ALWAYS_INLINE void functionCallerInnerProcess(ESFunctionObject* fn, ESValue receiver, ESValue arguments[], size_t argumentCount, bool needsArgumentsObject, ESVMInstance* ESVMInstance)
{
    //ESVMInstance->invalidateIdentifierCacheCheckCount();

    ((FunctionEnvironmentRecord *)ESVMInstance->currentExecutionContext()->environment()->record())->bindThisValue(receiver.asESPointer()->asESObject());
    DeclarativeEnvironmentRecord* functionRecord = ESVMInstance->currentExecutionContext()->environment()->record()->toDeclarativeEnvironmentRecord();

    if(UNLIKELY(fn->functionAST()->needsActivation())) {
        const InternalAtomicStringVector& params = fn->functionAST()->params();
        const ESStringVector& nonAtomicParams = fn->functionAST()->nonAtomicParams();
        for(unsigned i = 0; i < params.size() ; i ++) {
            if(i < argumentCount) {
                *functionRecord->bindingValueForActivationMode(i) = arguments[i];
            }
        }
    } else {
        const InternalAtomicStringVector& params = fn->functionAST()->params();
        const ESStringVector& nonAtomicParams = fn->functionAST()->nonAtomicParams();
        for(unsigned i = 0; i < params.size() ; i ++) {
            if(i < argumentCount) {
                ESVMInstance->currentExecutionContext()->cachedDeclarativeEnvironmentRecordESValue()[i] = arguments[i];
                //functionRecord->setMutableBinding(params[i], nonAtomicParams[i], arguments[i], true);
            }
        }
    }

    if(UNLIKELY(needsArgumentsObject)) {
        ESObject* argumentsObject = ESObject::create();
        unsigned i = 0;
        argumentsObject->set(strings->length, ESValue(argumentCount));
        for(; i < argumentCount && i < ESCARGOT_STRINGS_NUMBERS_MAX ; i ++) {
            argumentsObject->set(strings->nonAtomicNumbers[i], arguments[i]);
        }
        for( ; i < argumentCount ; i ++) {
            argumentsObject->set(ESString::create((int)i), arguments[i]);
        }

        if(!fn->functionAST()->needsActivation()) {
            for(size_t i = 0 ; i < fn->functionAST()->innerIdentifiers().size() ; i++ ) {
                if(fn->functionAST()->innerIdentifiers()[i] == strings->atomicArguments) {
                    *functionRecord->bindingValueForNonActivationMode(i) = argumentsObject;
                    break;
                }
            }
            ASSERT(functionRecord->hasBinding(strings->atomicArguments, strings->arguments).hasData());
        } else {
            functionRecord->createMutableBinding(strings->atomicArguments, strings->arguments, false);
            functionRecord->setMutableBinding(strings->atomicArguments, strings->arguments, argumentsObject, true);
        }
    }

}


ESValue ESFunctionObject::call(ESValue callee, ESValue receiver, ESValue arguments[], size_t argumentCount, bool isNewExpression)
{
    ESVMInstance* ESVMInstance = ESVMInstance::currentInstance();
    ESValue result(ESValue::ESForceUninitialized);
    if(LIKELY(callee.isESPointer() && callee.asESPointer()->isESFunctionObject())) {
        ExecutionContext* currentContext = ESVMInstance->currentExecutionContext();
        ESFunctionObject* fn = callee.asESPointer()->asESFunctionObject();
        if(UNLIKELY(fn->functionAST()->needsActivation())) {
            ESVMInstance->m_currentExecutionContext = new ExecutionContext(LexicalEnvironment::newFunctionEnvironment(fn, receiver), true, isNewExpression, currentContext, arguments, argumentCount);
            functionCallerInnerProcess(fn, receiver, arguments, argumentCount, true, ESVMInstance);
            //ESVMInstance->invalidateIdentifierCacheCheckCount();
            int r = setjmp(ESVMInstance->currentExecutionContext()->returnPosition());
            if(r != 1) {
                fn->functionAST()->body()->execute(ESVMInstance);
            }
            result = ESVMInstance->currentExecutionContext()->returnValue();
            ESVMInstance->m_currentExecutionContext = currentContext;
        } else {
            ESValue* storage = (::escargot::ESValue *)alloca(sizeof(::escargot::ESValue) * fn->functionAST()->innerIdentifiers().size());
            FunctionEnvironmentRecord envRec(
                    storage,
                    &fn->functionAST()->innerIdentifiers());

            //envRec.m_functionObject = fn;
            //envRec.m_newTarget = receiver;

            LexicalEnvironment env(&envRec, fn->outerEnvironment());
            ExecutionContext ec(&env, false, isNewExpression, currentContext, arguments, argumentCount, storage);
            ESVMInstance->m_currentExecutionContext = &ec;
            functionCallerInnerProcess(fn, receiver, arguments, argumentCount, fn->functionAST()->needsArgumentsObject(), ESVMInstance);
            //ESVMInstance->invalidateIdentifierCacheCheckCount();
            int r = setjmp(ESVMInstance->currentExecutionContext()->returnPosition());
            if(r != 1) {
                fn->functionAST()->body()->execute(ESVMInstance);
            }
            result = ESVMInstance->currentExecutionContext()->returnValue();
            ESVMInstance->m_currentExecutionContext = currentContext;
        }
    } else {
        throw TypeError(ESString::create(u"Callee is not a function object"));
    }

    return result;
}

void ESDateObject::parseStringToDate(struct tm* timeinfo, escargot::ESString* istr) {
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


void ESDateObject::setTimeValue(ESValue str) {
    if (str.isUndefined()) {
        clock_gettime(CLOCK_REALTIME,&m_time);
    } else {
        escargot::ESString* istr = str.toString();
        struct tm timeinfo;
        parseStringToDate(&timeinfo, istr);
        timeinfo.tm_isdst = true;
        m_time.tv_sec = ymdhmsToSeconds(timeinfo.tm_year+1900, timeinfo.tm_mon + 1, timeinfo.tm_mday, timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);;
        //m_time.tv_sec = mktime(&timeinfo);
    }
}

int ESDateObject::getDate() {
    return ESVMInstance::currentInstance()->computeLocalTime(m_time)->tm_mday;
}

int ESDateObject::getDay() {
    return ESVMInstance::currentInstance()->computeLocalTime(m_time)->tm_wday;
}

int ESDateObject::getFullYear() {
    return ESVMInstance::currentInstance()->computeLocalTime(m_time)->tm_year + 1900;
}

int ESDateObject::getHours() {
    return ESVMInstance::currentInstance()->computeLocalTime(m_time)->tm_hour;
}

int ESDateObject::getMinutes() {
    return ESVMInstance::currentInstance()->computeLocalTime(m_time)->tm_min;
}

int ESDateObject::getMonth() {
    return ESVMInstance::currentInstance()->computeLocalTime(m_time)->tm_mon;
}

int ESDateObject::getSeconds() {
    return ESVMInstance::currentInstance()->computeLocalTime(m_time)->tm_sec;
}

int ESDateObject::getTimezoneOffset() {
    return ESVMInstance::currentInstance()->timezoneOffset();

}

void ESDateObject::setTime(double t) {
    time_t raw_t = (time_t) floor(t);
    m_time.tv_sec = raw_t / 1000;
    m_time.tv_nsec = (raw_t % 10000) * 1000000;
}

ESStringObject::ESStringObject(escargot::ESString* str)
    : ESObject((Type)(Type::ESObject | Type::ESStringObject))
{
    m_stringData = str;

    //$21.1.4.1 String.length
    defineAccessorProperty(strings->length, ESVMInstance::currentInstance()->stringObjectLengthAccessorData(), false, true, false);
}

ESErrorObject::ESErrorObject(escargot::ESString* message)
       : ESObject((Type)(Type::ESObject | Type::ESErrorObject))
{
    m_message = message;
    escargot::ESFunctionObject* fn = ESVMInstance::currentInstance()->globalObject()->error();
    setConstructor(fn);
    set__proto__(fn);
}

}
