#include "Escargot.h"
#include "ESValue.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#include "runtime/NullableString.h"
#include "ast/AST.h"
#include "jit/ESJIT.h"

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
    if(isUndefined())
        return val.isUndefined();

    if(isNull())
        return val.isNull();

    if(isBoolean())
        return val.isBoolean() && asBoolean() == val.asBoolean();

    if(isNumber())
        return val.isNumber() && asNumber() == val.asNumber();

    if(isESPointer()) {
        ESPointer* o = asESPointer();
        if (!val.isESPointer())
            return false;
        ESPointer* o2 = val.asESPointer();
        if(o == o2)
            return true;
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

}
#ifdef ESCARGOT_LITTLE_ENDIAN
#define IEEE_8087
#else
#define IEEE_MC68k
#endif

#define NO_GLOBAL_STATE
#define MALLOC malloc
#define FREE free

#ifndef Long
#define Long int32_t
#endif

#ifndef ULong
#define ULong uint32_t
#endif

#include <dtoa.c>

namespace escargot {

ESStringData::ESStringData(double number)
{
    m_hashData.m_isHashInited =  false;

    int decPt;        /* Offset of decimal point from first digit */
    int sign;         /* Nonzero if the sign bit was set in d */
    int nDigits;      /* Number of significand digits returned by js_dtoa */
    char *numBegin;   /* Pointer to the digits returned by js_dtoa */
    char *numEnd = 0; /* Pointer past the digits returned by js_dtoa */

    DtoaState* state = (DtoaState *)ESVMInstance::currentInstance()->dtoaState();
    if(UNLIKELY(!state)) {
        state = newdtoa();
        ESVMInstance::currentInstance()->setDtoaState(state);
    }

    U u;
    dval(u) = number;
    numBegin = dtoa(state, u, 0, 100, &decPt, &sign, &numEnd);
    ASSERT(numBegin);
    unsigned resCnt = 0;
    if(sign) {
        resCnt++;
    }
    if(decPt == 0) {
        resCnt += 2;
        resCnt += numEnd - numBegin;
    } else if(decPt < 0) {
        resCnt += 2;
        resCnt += -decPt;
        resCnt += numEnd - numBegin;
    } else {
        resCnt += 1;
        resCnt += decPt;
        resCnt += numEnd - numBegin;
    }
    reserve(resCnt);
    if(sign) {
        operator +=('-');
    }
    char *s = numBegin;
    if(decPt == 0) {
        operator +=('0');
        operator +=('.');
        while(s != numEnd) {
            operator +=(*s);
            s++;
        }
    } else if(decPt < 0) {
        operator +=('0');
        operator +=('.');
        for(int i = 0; i > decPt; i --) {
            operator +=('0');
        }

        while(s != numEnd) {
            operator +=(*s);
            s++;
        }
    } else {
        for(unsigned i = 0; i < decPt; i ++) {
            if(s < numEnd) {
                char16_t c = *s;
                operator +=(c);
                s++;
            } else {
                operator +=('0');
            }
        }
        if(s != numEnd)
            operator +=('.');

        while(s != numEnd) {
            operator +=(*s);
            s++;
        }
    }

    freedtoa(state, numBegin);
    /*
    char16_t buf[512];
    char chbuf[128];
    char* end = rapidjson::internal::dtoa(number, chbuf);
    int i = 0;
    for (char* p = chbuf; p != end; ++p) {
        buf[i++] = (char16_t) *p;
    }

    if(i >= 3 && buf[i-1] == '0' && buf[i-2] == '.') {
        i -= 2;
    }

    buf[i] = u'\0';

    reserve(i);
    append(&buf[0], &buf[i]);
    */
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


bool ESString::match(ESPointer* esptr, RegexMatchResult& matchResult, bool testOnly, size_t startIndex) const
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
        size_t start = startIndex;
        unsigned result = 0;
        const char16_t *chars = m_string->data();
        unsigned* outputBuf = (unsigned int*)alloca(sizeof (unsigned) * 2 * (subPatternNum + 1));
        outputBuf[1] = start;
        do {
            start = outputBuf[1];
            memset(outputBuf,-1,sizeof (unsigned) * 2 * (subPatternNum + 1));
            if(start >= length)
                break;
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
                if(start == outputBuf[1]) {
                    outputBuf[1]++;
                    if(outputBuf[1] > length) {
                        break;
                    }
                }
            } else {
                break;
            }
        } while(result != JSC::Yarr::offsetNoMatch);
    }
    return matchResult.m_matchResults.size();
}

ESObject::ESObject(ESPointer::Type type, size_t initialKeyCount)
    : ESPointer(type)
    , m_map(NULL)
{
    m_flags.m_isFrozen = false;
    m_flags.m_propertyIndex = 0;

    m_hiddenClassData.reserve(initialKeyCount);
    m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForObject();

    m_hiddenClassData.push_back(ESValue(ESValue::ESUndefined));
    m_hiddenClassData.push_back(ESValue((ESPointer *)ESVMInstance::currentInstance()->object__proto__AccessorData()));
}

const unsigned ESArrayObject::MAX_FASTMODE_SIZE;

ESArrayObject::ESArrayObject(int length)
    : ESObject((Type)(Type::ESObject | Type::ESArrayObject))
    , m_vector(0)
    , m_fastmode(true)
{
    m_length = 0;
    if (length == -1)
        convertToSlowMode();
    else if (length > 0) {
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
    m_lastIndex = 0;
    m_lastExecutedString = NULL;
}

void ESRegExpObject::setSource(escargot::ESString* src)
{
    m_bytecodePattern = NULL;
    m_source = src;
}
void ESRegExpObject::setOption(const Option& option)
{
    if(((m_option & ESRegExpObject::Option::MultiLine) != (option & ESRegExpObject::Option::MultiLine)) ||
            ((m_option & ESRegExpObject::Option::IgnoreCase) != (option & ESRegExpObject::Option::IgnoreCase))
            ) {
        m_bytecodePattern = NULL;
    }
    m_option = option;
}

ESFunctionObject::ESFunctionObject(LexicalEnvironment* outerEnvironment, CodeBlock* cb, escargot::ESString* name, ESObject* proto)
    : ESObject((Type)(Type::ESObject | Type::ESFunctionObject))
{
    m_name = name;
    m_outerEnvironment = outerEnvironment;
    m_codeBlock = cb;

    m_hiddenClass = ESVMInstance::currentInstance()->initialHiddenClassForFunction();
    m_hiddenClassData.push_back(ESValue((ESPointer *)ESVMInstance::currentInstance()->functionPrototypeAccessorData()));
    m_hiddenClassData.push_back(ESValue(ESValue::ESUndefined));

    if (proto != NULL)
        set__proto__(proto);
    else {
        //for avoiding assert
        m___proto__ = ESVMInstance::currentInstance()->globalFunctionPrototype();
    }
}

ESFunctionObject::ESFunctionObject(LexicalEnvironment* outerEnvironment, NativeFunctionType fn, escargot::ESString* name, ESObject* proto)
    : ESFunctionObject(outerEnvironment, (CodeBlock *)NULL, name, proto)
{
    m_codeBlock = CodeBlock::create();
    m_codeBlock->pushCode(ExecuteNativeFunction(fn), NULL);
    m_codeBlock->m_isBuiltInFunction = true;
#ifdef ENABLE_ESJIT
    m_codeBlock->m_dontJIT = true;
#endif
    m_name = name;
}

ALWAYS_INLINE void functionCallerInnerProcess(ExecutionContext* newEC, ESFunctionObject* fn, ESValue& receiver, ESValue arguments[], const size_t& argumentCount, ESVMInstance* ESVMInstance)
{
    bool strict = fn->codeBlock()->shouldUseStrictMode();
    newEC->setStrictMode(strict);

    //http://www.ecma-international.org/ecma-262/6.0/#sec-ordinarycallbindthis
    if(!strict) {
        if(receiver.isUndefinedOrNull()) {
            receiver = ESVMInstance->globalObject();
        } else {
            receiver = receiver.toObject();
        }
    }

    ((FunctionEnvironmentRecord *)ESVMInstance->currentExecutionContext()->environment()->record())->bindThisValue(receiver);
    DeclarativeEnvironmentRecord* functionRecord = ESVMInstance->currentExecutionContext()->environment()->record()->toDeclarativeEnvironmentRecord();

    if(UNLIKELY(fn->codeBlock()->m_needsActivation)) {
        const InternalAtomicStringVector& params = fn->codeBlock()->m_params;
        const ESStringVector& nonAtomicParams = fn->codeBlock()->m_nonAtomicParams;
        for(unsigned i = 0; i < params.size() ; i ++) {
            if(i < argumentCount) {
                *functionRecord->bindingValueForActivationMode(i) = arguments[i];
            }
        }
    } else {
        const InternalAtomicStringVector& params = fn->codeBlock()->m_params;
        const ESStringVector& nonAtomicParams = fn->codeBlock()->m_nonAtomicParams;
        for(unsigned i = 0; i < params.size() ; i ++) {
            if(i < argumentCount) {
                ESVMInstance->currentExecutionContext()->cachedDeclarativeEnvironmentRecordESValue()[i] = arguments[i];
            }
        }
    }
}

ESValue ESFunctionObject::call(ESVMInstance* instance, const ESValue& callee, const ESValue& receiverInput, ESValue arguments[], const size_t& argumentCount, bool isNewExpression)
{
    ESValue result(ESValue::ESForceUninitialized);
    if(LIKELY(callee.isESPointer() && callee.asESPointer()->isESFunctionObject())) {
        ESValue receiver = receiverInput;
        ExecutionContext* currentContext = instance->currentExecutionContext();
        ESFunctionObject* fn = callee.asESPointer()->asESFunctionObject();

        if(UNLIKELY(fn->codeBlock()->m_needsActivation)) {
            instance->m_currentExecutionContext = new ExecutionContext(LexicalEnvironment::newFunctionEnvironment(arguments, argumentCount, fn), true, isNewExpression,
                    currentContext,
                    arguments, argumentCount);
            functionCallerInnerProcess(instance->m_currentExecutionContext, fn, receiver, arguments, argumentCount, instance);
            //ESVMInstance->invalidateIdentifierCacheCheckCount();
            //execute;
            result = interpret(instance, fn->codeBlock());
            instance->m_currentExecutionContext = currentContext;
        } else {
            ESValue* storage = (::escargot::ESValue *)alloca(sizeof(::escargot::ESValue) * fn->m_codeBlock->m_innerIdentifiers.size());
            FunctionEnvironmentRecord envRec(
                    arguments, argumentCount,
                    storage,
                    &fn->m_codeBlock->m_innerIdentifiers);

            //envRec.m_functionObject = fn;
            //envRec.m_newTarget = receiver;

            LexicalEnvironment env(&envRec, fn->outerEnvironment());
            ExecutionContext ec(&env, false, isNewExpression, currentContext, arguments, argumentCount, storage);
            instance->m_currentExecutionContext = &ec;
            functionCallerInnerProcess(&ec, fn, receiver, arguments, argumentCount, instance);
            //ESVMInstance->invalidateIdentifierCacheCheckCount();
            //execute;
#ifdef ENABLE_ESJIT
            ESJIT::JITFunction jitFunction = fn->codeBlock()->m_cachedJITFunction;
            if (!jitFunction && !fn->codeBlock()->m_dontJIT && fn->codeBlock()->m_executeCount > 1) {
#ifndef NDEBUG
                if (ESVMInstance::currentInstance()->m_verboseJIT)
                    printf("Trying JIT Compile for function %s...\n", fn->codeBlock()->m_nonAtomicId ? (fn->codeBlock()->m_nonAtomicId->utf8Data()):"(anonymous)");
#endif
                jitFunction = reinterpret_cast<ESJIT::JITFunction>(ESJIT::JITCompile(fn->codeBlock()));
                if (jitFunction) {
#ifndef NDEBUG
                    if (ESVMInstance::currentInstance()->m_verboseJIT)
                        printf("> Compilation successful! Cache jit function %p\n", jitFunction);
#endif
                    fn->codeBlock()->m_cachedJITFunction = jitFunction;
                } else {
#ifndef NDEBUG
                    if (ESVMInstance::currentInstance()->m_verboseJIT)
                        printf("> Compilation failed! disable jit compilation from now on\n");
#endif
                    fn->codeBlock()->m_dontJIT = true;
                }
            }

            if (jitFunction) {
                ESValue res = jitFunction(instance);
                if (ec.inOSRExit()) {
                    printf("OSR EXIT from tmp%ld\n", res.asRawData());
                    RELEASE_ASSERT_NOT_REACHED();
                } else {
                    result = ESValue(res.asRawData()); // TODO: Make it as ESValue format in the JIT function
                }
            } else {
                //printf("JIT failed! Execute interpreter\n");
                result = interpret(instance, fn->codeBlock());
                fn->codeBlock()->m_executeCount++;
            }
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

void ESDateObject::parseYmdhmsToDate(struct tm* timeinfo, int year, int month, int date, int hour, int minute, int second) {
      char buffer[255];
      snprintf(buffer, 255, "%d-%d-%d-%d-%d-%d", year, month, date, hour, minute, second);
      strptime(buffer, "%Y-%m-%d-%H-%M-%S", timeinfo);
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


void ESDateObject::setTimeValue() {
    clock_gettime(CLOCK_REALTIME,&m_time);
    m_isCacheDirty = true;
}

void ESDateObject::setTimeValue(const ESValue str) {
    escargot::ESString* istr = str.toString();
    parseStringToDate(&m_cachedTM, istr);
    m_cachedTM.tm_isdst = true;
    m_time.tv_sec = ymdhmsToSeconds(m_cachedTM.tm_year+1900, m_cachedTM.tm_mon + 1, m_cachedTM.tm_mday, m_cachedTM.tm_hour, m_cachedTM.tm_min, m_cachedTM.tm_sec);
}

void ESDateObject::setTimeValue(int year, int month, int date, int hour, int minute, int second, int millisecond) {
    parseYmdhmsToDate(&m_cachedTM, year, month, date, hour, minute, second);
    m_cachedTM.tm_isdst = true;
    m_time.tv_sec = ymdhmsToSeconds(m_cachedTM.tm_year+1900, m_cachedTM.tm_mon + 1, m_cachedTM.tm_mday, m_cachedTM.tm_hour, m_cachedTM.tm_min, m_cachedTM.tm_sec);
}

void ESDateObject::resolveCache()
{
    if(m_isCacheDirty) {
        memcpy(&m_cachedTM, ESVMInstance::currentInstance()->computeLocalTime(m_time), sizeof (tm));
        m_isCacheDirty = false;
    }
}

int ESDateObject::getDate() {
    resolveCache();
    return m_cachedTM.tm_mday;
}

int ESDateObject::getDay() {
    resolveCache();
    return m_cachedTM.tm_wday;
}

int ESDateObject::getFullYear() {
    resolveCache();
    return m_cachedTM.tm_year + 1900;
}

int ESDateObject::getHours() {
    resolveCache();
    return m_cachedTM.tm_hour;
}

int ESDateObject::getMinutes() {
    resolveCache();
    return m_cachedTM.tm_min;
}

int ESDateObject::getMonth() {
    resolveCache();
    return m_cachedTM.tm_mon;
}

int ESDateObject::getSeconds() {
    resolveCache();
    return m_cachedTM.tm_sec;
}

int ESDateObject::getTimezoneOffset() {
    return ESVMInstance::currentInstance()->timezoneOffset();

}

void ESDateObject::setTime(double t) {
    time_t raw_t = (time_t) floor(t);
    m_time.tv_sec = raw_t / 1000;
    m_time.tv_nsec = (raw_t % 10000) * 1000000;

    m_isCacheDirty = true;
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
    set(strings->message, message);
    set(strings->name, strings->Error);
    escargot::ESFunctionObject* fn = ESVMInstance::currentInstance()->globalObject()->error();
    setConstructor(fn);
    set__proto__(ESVMInstance::currentInstance()->globalObject()->errorPrototype());
}

ReferenceError::ReferenceError(escargot::ESString* message)
    : ESErrorObject(message)
{
    set(strings->name, strings->ReferenceError);
    setConstructor(ESVMInstance::currentInstance()->globalObject()->referenceError());
    set__proto__(ESVMInstance::currentInstance()->globalObject()->referenceErrorPrototype());
}

TypeError::TypeError(escargot::ESString* message)
    : ESErrorObject(message)
{
    set(strings->name, strings->TypeError);
    setConstructor(ESVMInstance::currentInstance()->globalObject()->typeError());
    set__proto__(ESVMInstance::currentInstance()->globalObject()->typeErrorPrototype());
}

RangeError::RangeError(escargot::ESString* message)
    : ESErrorObject(message)
{
    set(strings->name, strings->RangeError);
    setConstructor(ESVMInstance::currentInstance()->globalObject()->rangeError());
    set__proto__(ESVMInstance::currentInstance()->globalObject()->rangeErrorPrototype());
}

SyntaxError::SyntaxError(escargot::ESString* message)
    : ESErrorObject(message)
{
    set(strings->name, strings->SyntaxError);
    setConstructor(ESVMInstance::currentInstance()->globalObject()->syntaxError());
    set__proto__(ESVMInstance::currentInstance()->globalObject()->syntaxErrorPrototype());
}

ESArrayBufferObject::ESArrayBufferObject(ESObject* proto,
                                        ESPointer::Type type)
    : ESObject((Type)(Type::ESObject | Type::ESArrayBufferObject)),
    m_data(NULL),
    m_bytelength(0)
{
    if( proto != NULL )
        set__proto__(proto);
    else
        set__proto__(ESVMInstance::currentInstance()->globalObject()->arrayBufferPrototype());
    setConstructor(ESVMInstance::currentInstance()->globalObject()->arrayBuffer());
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

}
