#include "Escargot.h"
#include "runtime/GlobalObject.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#include "bytecode/ByteCodeOperations.h"

namespace escargot {

void GlobalObject::installDate()
{
    m_datePrototype = ESDateObject::create();
    m_datePrototype->forceNonVectorHiddenClass(true);
    m_datePrototype->set__proto__(m_objectPrototype);

    // http://www.ecma-international.org/ecma-262/5.1/#sec-15.9.3
    m_date = ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESDateObject* thisObject;
        if (instance->currentExecutionContext()->isNewExpression()) {
            thisObject = instance->currentExecutionContext()->resolveThisBindingToObject()->asESDateObject();

            size_t arg_size = instance->currentExecutionContext()->argumentCount();
            if (arg_size == 0) {
                thisObject->setTimeValue();
            } else if (arg_size == 1) {
                ESValue v = instance->currentExecutionContext()->arguments()[0].toPrimitive();
                if (v.isESString()) {
                    thisObject->setTimeValue(v);
                } else {
                    double V = v.toNumber();
                    thisObject->setTimeValue(ESDateObject::timeClip(V));
                }
            } else {
                double args[7] = {0, 0, 1, 0, 0, 0, 0}; // default value of year, month, date, hour, minute, second, millisecond
                for (size_t i = 0; i < arg_size; i++) {
                    args[i] = instance->currentExecutionContext()->readArgument(i).toNumber();
                }
                double year = args[0];
                double month = args[1];
                double date = args[2];
                double hour = args[3];
                double minute = args[4];
                double second = args[5];
                double millisecond = args[6];

                if ((int) year >= 0 && (int) year <= 99) {
                    year += 1900;
                }
                if (std::isnan(year) || std::isnan(month) || std::isnan(date) || std::isnan(hour) || std::isnan(minute) || std::isnan(second) || std::isnan(millisecond)) {
                    thisObject->setTimeValueAsNaN();
                    return ESString::create("Invalid Date");
                }
                thisObject->setTimeValue((int) year, (int) month, (int) date, (int) hour, (int) minute, second, millisecond);
            }

            return thisObject->toFullString();
        } else {
            thisObject = (escargot::ESDateObject*)GC_MALLOC(sizeof(escargot::ESDateObject));
            thisObject->setTimeValue();
            escargot::ESString* retval = thisObject->toFullString();
            GC_FREE(thisObject);
            return retval;
        }
    }, strings->Date, 7, true); // $20.3.3 Properties of the Date Constructor: the length property is 7.
    m_date->forceNonVectorHiddenClass(true);
    m_date->defineAccessorProperty(strings->prototype.string(), ESVMInstance::currentInstance()->functionPrototypeAccessorData(), false, false, false);

    // Date.prototype.toString
    // http://www.ecma-international.org/ecma-262/5.1/#sec-15.9.5.2
    m_datePrototype->defineDataProperty(strings->toString, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue e = instance->currentExecutionContext()->resolveThisBinding();
        if (e.isESPointer() && e.asESPointer()->isESDateObject()) {
            escargot::ESDateObject* obj = e.asESPointer()->asESDateObject();
            if (!std::isnan(obj->timeValueAsDouble())) {
                return obj->toFullString();
            } else {
                return ESString::create("Invalid Date");
            }
        } else {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->toString, errorMessage_GlobalObject_ThisNotDateObject);
            RELEASE_ASSERT_NOT_REACHED();
        }
    }, strings->toString, 0));

    m_date->setProtoType(m_datePrototype);

    m_datePrototype->defineDataProperty(strings->constructor, true, false, true, m_date);

    defineDataProperty(strings->Date, true, false, true, m_date);

    // $20.3.3.1 Date.now()
    m_date->defineDataProperty(strings->now, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        struct timespec nowTime;
        clock_gettime(CLOCK_REALTIME, &nowTime);
        double ret = (double)nowTime.tv_sec*1000. + floor((double)nowTime.tv_nsec / 1000000.);
        return ESValue(ret);
    }, strings->now.string(), 0));

    // $20.3.3.2 Date.parse()
    m_date->defineDataProperty(strings->parse, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue v = instance->currentExecutionContext()->readArgument(0).toPrimitive(ESValue::PreferString);
        if (v.isESString()) {
            return ESValue(ESDateObject::parseStringToDate(v.asESString()));
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
//            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, false, strings->parse, errorMessage_GlobalObject_FirstArgumentNotString);
        }
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->parse.string(), 1));

    // $20.3.3.4 Date.UTC
    m_date->defineDataProperty(strings->UTC, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        double args[7] = {0, 0, 1, 0, 0, 0, 0}; // default value of year, month, date, hour, minute, second, millisecond
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        for (size_t i = 0; i < arg_size; i++) {
            args[i] = instance->currentExecutionContext()->readArgument(i).toNumber();
        }
        double year = args[0];
        double month = args[1];
        double date = args[2];
        double hour = args[3];
        double minute = args[4];
        double second = args[5];
        double millisecond = args[6];

        if ((int) year >= 0 && (int) year <= 99) {
            year += 1900;
        }
        if (arg_size < 2 || std::isnan(year) || std::isnan(month) || std::isnan(date) || std::isnan(hour) || std::isnan(minute) || std::isnan(second) || std::isnan(millisecond)) {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
        ESObject* tmp = ESDateObject::create();
        double t = ESDateObject::timeClip(tmp->asESDateObject()->ymdhmsToSeconds((int) year, (int) month, (int) date, (int) hour, (int) minute, (int) second) * 1000 + millisecond);
        return ESValue(t);
    }, strings->UTC.string(), 7));

    // $20.3.4.2 Date.prototype.getDate()
    m_datePrototype->defineDataProperty(strings->getDate, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getDate);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getDate, errorMessage_GlobalObject_ThisNotDateObject);
        }
        if (thisObject->asESDateObject()->isValid()) {
            return ESValue(thisObject->asESDateObject()->getDate());
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
    }, strings->getDate, 0));

    // $20.3.4.3 Date.prototype.getDay()
    m_datePrototype->defineDataProperty(strings->getDay, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getDay);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getDay, errorMessage_GlobalObject_ThisNotDateObject);
        }
        if (thisObject->asESDateObject()->isValid()) {
            return ESValue(thisObject->asESDateObject()->getDay());
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
    }, strings->getDay, 0));

    // $20.3.4.4 Date.prototype.getFullYear()
    m_datePrototype->defineDataProperty(strings->getFullYear, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getFullYear);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getFullYear, errorMessage_GlobalObject_ThisNotDateObject);
        }
        if (thisObject->asESDateObject()->isValid()) {
            return ESValue(thisObject->asESDateObject()->getFullYear());
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
    }, strings->getFullYear, 0));

    // $20.3.4.5 Date.prototype.getHours()
    m_datePrototype->defineDataProperty(strings->getHours, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getHours);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getHours, errorMessage_GlobalObject_ThisNotDateObject);
        }
        if (thisObject->asESDateObject()->isValid()) {
            return ESValue(thisObject->asESDateObject()->getHours());
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
    }, strings->getHours, 0));

    // $20.3.4.6 Date.prototype.getMilliseconds()
    m_datePrototype->defineDataProperty(strings->getMilliseconds, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getMilliseconds);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getMilliseconds, errorMessage_GlobalObject_ThisNotDateObject);
        }
        if (thisObject->asESDateObject()->isValid()) {
            return ESValue(thisObject->asESDateObject()->getMilliseconds());
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
    }, strings->getMilliseconds, 0));

    // $20.3.4.7 Date.prototype.getMinutes()
    m_datePrototype->defineDataProperty(strings->getMinutes, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getMinutes);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getMinutes, errorMessage_GlobalObject_ThisNotDateObject);
        }
        if (thisObject->asESDateObject()->isValid()) {
            return ESValue(thisObject->asESDateObject()->getMinutes());
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
    }, strings->getMinutes, 0));

    // $20.3.4.8 Date.prototype.getMonth()
    m_datePrototype->defineDataProperty(strings->getMonth, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getMonth);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getMonth, errorMessage_GlobalObject_ThisNotDateObject);
        }
        if (thisObject->asESDateObject()->isValid()) {
            return ESValue(thisObject->asESDateObject()->getMonth());
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
    }, strings->getMonth, 0));

    // $20.3.4.9 Date.prototype.getSeconds()
    m_datePrototype->defineDataProperty(strings->getSeconds, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getSeconds);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getSeconds, errorMessage_GlobalObject_ThisNotDateObject);
        }
        if (thisObject->asESDateObject()->isValid()) {
            return ESValue(thisObject->asESDateObject()->getSeconds());
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
    }, strings->getSeconds, 0));

    // $20.3.4.10 Date.prototype.getTime()
    m_datePrototype->defineDataProperty(strings->getTime, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getTime);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getTime, errorMessage_GlobalObject_ThisNotDateObject);
        }
        double primitiveValue = thisObject->asESDateObject()->timeValueAsDouble();
        return ESValue(primitiveValue);
    }, strings->getTime, 0));

    // $20.3.4.11 Date.prototype.getTimezoneOffset()
    m_datePrototype->defineDataProperty(strings->getTimezoneOffset, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getTimezoneOffset);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getTimezoneOffset, errorMessage_GlobalObject_ThisNotDateObject);
        }
        double ret = thisObject->asESDateObject()->getTimezoneOffset() / 60.0;
        return ESValue(ret);
    }, strings->getTimezoneOffset, 0));

    // $20.3.4.12 Date.prototype.getUTCDate()
    m_datePrototype->defineDataProperty(strings->getUTCDate, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getUTCDate);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getUTCDate, errorMessage_GlobalObject_ThisNotDateObject);
        }
        if (thisObject->asESDateObject()->isValid()) {
            return ESValue(thisObject->asESDateObject()->getUTCDate());
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
    }, strings->getUTCDate, 0));

    // $20.3.4.13 Date.prototype.getUTCDay()
    m_datePrototype->defineDataProperty(strings->getUTCDay, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getUTCDay);
        if (!thisObject->isESDateObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getUTCDay, errorMessage_GlobalObject_ThisNotDateObject);

        if (thisObject->asESDateObject()->isValid())
            return ESValue(thisObject->asESDateObject()->getUTCDay());
        else
            return ESValue(std::numeric_limits<double>::quiet_NaN());
    }, strings->getUTCDay, 0));

    // $20.3.4.14 Date.prototype.getUTCFullYear()
    m_datePrototype->defineDataProperty(strings->getUTCFullYear, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getUTCFullYear);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getUTCFullYear, errorMessage_GlobalObject_ThisNotDateObject);
        }
        if (thisObject->asESDateObject()->isValid()) {
            return ESValue(thisObject->asESDateObject()->getUTCFullYear());
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
    }, strings->getUTCFullYear, 0));

    // $20.3.4.15 Date.prototype.getUTCHours()
    m_datePrototype->defineDataProperty(strings->getUTCHours, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getUTCHours);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getUTCHours, errorMessage_GlobalObject_ThisNotDateObject);
        }
        if (thisObject->asESDateObject()->isValid()) {
            return ESValue(thisObject->asESDateObject()->getUTCHours());
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
    }, strings->getUTCHours, 0));

    // $20.3.4.16 Date.prototype.getUTCMilliseconds()
    m_datePrototype->defineDataProperty(strings->getUTCMilliseconds, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getUTCMilliseconds);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getUTCMilliseconds, errorMessage_GlobalObject_ThisNotDateObject);
        }
        if (thisObject->asESDateObject()->isValid()) {
            return ESValue(thisObject->asESDateObject()->getUTCMilliseconds());
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
    }, strings->getUTCMilliseconds, 0));

    // $20.3.4.17 Date.prototype.getUTCMinutes()
    m_datePrototype->defineDataProperty(strings->getUTCMinutes, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getUTCMinutes);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getUTCMinutes, errorMessage_GlobalObject_ThisNotDateObject);
        }
        if (thisObject->asESDateObject()->isValid()) {
            return ESValue(thisObject->asESDateObject()->getUTCMinutes());
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
    }, strings->getUTCMinutes, 0));

    // $20.3.4.18 Date.prototype.getUTCMonth()
    m_datePrototype->defineDataProperty(strings->getUTCMonth, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getUTCMonth);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getUTCMonth, errorMessage_GlobalObject_ThisNotDateObject);
        }
        if (thisObject->asESDateObject()->isValid()) {
            return ESValue(thisObject->asESDateObject()->getUTCMonth());
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
    }, strings->getUTCMonth, 0));

    // $20.3.4.19 Date.prototype.getUTCSeconds()
    m_datePrototype->defineDataProperty(strings->getUTCSeconds, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getUTCSeconds);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getUTCSeconds, errorMessage_GlobalObject_ThisNotDateObject);
        }
        if (thisObject->asESDateObject()->isValid()) {
            return ESValue(thisObject->asESDateObject()->getUTCSeconds());
        } else {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        }
    }, strings->getUTCSeconds, 0));

    // $20.3.4.20 Date.prototype.setDate()
    m_datePrototype->defineDataProperty(strings->setDate, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setDate);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setDate, errorMessage_GlobalObject_ThisNotDateObject);
        }
        escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        double args[1] = {0};

        if (arg_size < 1) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (std::isnan(thisDateObject->timeValueAsDouble())) {
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        args[0] = instance->currentExecutionContext()->readArgument(0).toNumber();

        if (std::isnan(args[0])) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        thisDateObject->setTimeValue(thisDateObject->getFullYear(), thisDateObject->getMonth(), (int) args[0]
            , thisDateObject->getHours(), thisDateObject->getMinutes(), thisDateObject->getSeconds(), thisDateObject->getMilliseconds());

        return ESValue(thisDateObject->timeValueAsDouble());
    }, strings->setDate, 1));

    // $20.3.4.21 Date.prototype.setFullYear()
    m_datePrototype->defineDataProperty(strings->setFullYear, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setFullYear);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setFullYear, errorMessage_GlobalObject_ThisNotDateObject);
        }
        escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();

        if (arg_size < 1) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (std::isnan(thisDateObject->timeValueAsDouble())) {
            thisDateObject->setTimeValue(1970, 0, 1, 0, 0, 0, 0, true);
        }

        double args[3] = {0, (double) thisDateObject->getMonth(), (double) thisDateObject->getDate()};

        for (size_t i = 0; i < arg_size; i++) {
            args[i] = instance->currentExecutionContext()->readArgument(i).toNumber();
            if (i >= 2)
                break;
        }

        if (std::isnan(args[0]) || std::isnan(args[1]) || std::isnan(args[2])) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        thisDateObject->setTimeValue((int) args[0], (int) args[1], (int) args[2], thisDateObject->getHours(), thisDateObject->getMinutes()
            , thisDateObject->getSeconds(), thisDateObject->getMilliseconds());

        return ESValue(thisDateObject->timeValueAsDouble());
    }, strings->setFullYear, 3));

    // $20.3.4.22 Date.prototype.setHours()
    m_datePrototype->defineDataProperty(strings->setHours, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setHours);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setHours, errorMessage_GlobalObject_ThisNotDateObject);
        }
        escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        double args[4] = {0, (double) thisDateObject->getMinutes(), (double) thisDateObject->getSeconds(), (double) thisDateObject->getMilliseconds()};

        if (arg_size < 1) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (std::isnan(thisDateObject->timeValueAsDouble())) {
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        for (size_t i = 0; i < arg_size; i++) {
            args[i] = instance->currentExecutionContext()->readArgument(i).toNumber();
            if (i >= 3)
                break;
        }

        if (std::isnan(args[0]) || std::isnan(args[1]) || std::isnan(args[2]) || std::isnan(args[3])) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        thisDateObject->setTimeValue(thisDateObject->getFullYear(), thisDateObject->getMonth(), thisDateObject->getDate()
            , (int) args[0], (int) args[1], args[2], args[3]);

        return ESValue(thisDateObject->timeValueAsDouble());
    }, strings->setHours, 4));

    // $20.3.4.23 Date.prototype.setMilliseconds()
    m_datePrototype->defineDataProperty(strings->setMilliseconds, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setMilliseconds);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setMilliseconds, errorMessage_GlobalObject_ThisNotDateObject);
        }
        escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        double args[1] = {0};

        if (arg_size < 1) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (std::isnan(thisDateObject->timeValueAsDouble())) {
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        args[0] = instance->currentExecutionContext()->readArgument(0).toNumber();

        if (std::isnan(args[0])) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        thisDateObject->setTimeValue(thisDateObject->getFullYear(), thisDateObject->getMonth(), thisDateObject->getDate()
            , thisDateObject->getHours(), thisDateObject->getMinutes(), thisDateObject->getSeconds(), args[0]);

        return ESValue(thisDateObject->timeValueAsDouble());
    }, strings->setMilliseconds, 1));

    // $20.3.4.24 Date.prototype.setMinutes()
    m_datePrototype->defineDataProperty(strings->setMinutes, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setMinutes);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setMinutes, errorMessage_GlobalObject_ThisNotDateObject);
        }
        escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        double args[3] = {0, (double) thisDateObject->getSeconds(), (double) thisDateObject->getMilliseconds()};

        if (arg_size < 1) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (std::isnan(thisDateObject->timeValueAsDouble())) {
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        for (size_t i = 0; i < arg_size; i++) {
            args[i] = instance->currentExecutionContext()->readArgument(i).toNumber();
            if (i >= 2)
                break;
        }

        if (std::isnan(args[0]) || std::isnan(args[1]) || std::isnan(args[2])) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        thisDateObject->setTimeValue(thisDateObject->getFullYear(), thisDateObject->getMonth(), thisDateObject->getDate()
            , thisDateObject->getHours(), (int) args[0], args[1], args[2]);

        return ESValue(thisDateObject->timeValueAsDouble());
    }, strings->setMinutes, 3));

    // $20.3.4.25 Date.prototype.setMonth()
    m_datePrototype->defineDataProperty(strings->setMonth, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setMonth);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setMonth, errorMessage_GlobalObject_ThisNotDateObject);
        }
        escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        double args[2] = {0, (double) thisDateObject->getDate()};

        if (arg_size < 1) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (std::isnan(thisDateObject->timeValueAsDouble())) {
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        for (size_t i = 0; i < arg_size; i++) {
            args[i] = instance->currentExecutionContext()->readArgument(i).toNumber();
            if (i >= 1)
                break;
        }

        if (std::isnan(args[0]) || std::isnan(args[1])) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        thisDateObject->setTimeValue(thisDateObject->getFullYear(), (int) args[0], (int) args[1]
            , thisDateObject->getHours(), thisDateObject->getMinutes(), thisDateObject->getSeconds(), thisDateObject->getMilliseconds());

        return ESValue(thisDateObject->timeValueAsDouble());
    }, strings->setMonth, 2));

    // $20.3.4.26 Date.prototype.setSeconds()
    m_datePrototype->defineDataProperty(strings->setSeconds, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setSeconds);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setSeconds, errorMessage_GlobalObject_ThisNotDateObject);
        }
        escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        double args[2] = {0, (double) thisDateObject->getMilliseconds()};

        if (arg_size < 1) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (std::isnan(thisDateObject->timeValueAsDouble())) {
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        for (size_t i = 0; i < arg_size; i++) {
            args[i] = instance->currentExecutionContext()->readArgument(i).toNumber();
            if (i >= 1)
                break;
        }

        if (std::isnan(args[0]) || std::isnan(args[1])) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        int64_t tmpargs[2] = {(int64_t) args[0], (int64_t) args[1]};
        for (size_t i = 0; i < arg_size; i++) {
            if (tmpargs[i] != args[i]) {
                thisDateObject->setTimeValueAsNaN();
                return ESValue(thisDateObject->timeValueAsDouble());
            }
            if (i >= 1)
                break;
        }
        thisDateObject->setTimeValue(thisDateObject->getFullYear(), thisDateObject->getMonth(), thisDateObject->getDate()
            , thisDateObject->getHours(), thisDateObject->getMinutes(), args[0], args[1]);

        return ESValue(thisDateObject->timeValueAsDouble());
    }, strings->setSeconds, 2));

    // $20.3.4.27 Date.prototype.setTime()
    m_datePrototype->defineDataProperty(strings->setTime, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setTime);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setTime, errorMessage_GlobalObject_ThisNotDateObject);
        }
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size > 0 && instance->currentExecutionContext()->arguments()[0].isNumber()) {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            thisObject->asESDateObject()->setTime(arg.toNumber());
            return ESValue(thisObject->asESDateObject()->timeValueAsDouble());
        } else {
            double value = std::numeric_limits<double>::quiet_NaN();
            thisObject->asESDateObject()->setTimeValueAsNaN();
            return ESValue(value);
        }
    }, strings->setTime, 1));

    // $20.3.4.28 Date.prototype.setUTCDate()
    m_datePrototype->defineDataProperty(strings->setUTCDate, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setUTCDate);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setUTCDate, errorMessage_GlobalObject_ThisNotDateObject);
        }
        escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        double args[1] = {0};

        if (arg_size < 1) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (std::isnan(thisDateObject->timeValueAsDouble())) {
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        args[0] = instance->currentExecutionContext()->readArgument(0).toNumber();

        if (std::isnan(args[0])) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        thisDateObject->setTimeValue(thisDateObject->getUTCFullYear(), thisDateObject->getUTCMonth(), (int) args[0]
            , thisDateObject->getUTCHours(), thisDateObject->getUTCMinutes(), thisDateObject->getUTCSeconds(), thisDateObject->getUTCMilliseconds(), false);

        return ESValue(thisDateObject->timeValueAsDouble());
    }, strings->setUTCDate, 1));

    // $20.3.4.29 Date.prototype.setUTCFullYear()
    m_datePrototype->defineDataProperty(strings->setUTCFullYear, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setUTCFullYear);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setUTCFullYear, errorMessage_GlobalObject_ThisNotDateObject);
        }
        escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        double args[3] = {0, (double) thisDateObject->getMonth(), (double) thisDateObject->getDate()};

        if (arg_size < 1) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (std::isnan(thisDateObject->timeValueAsDouble())) {
            thisDateObject->setTimeValue(0, 0, 1, 0, 0, 0, 0, true);
        }

        for (size_t i = 0; i < arg_size; i++) {
            args[i] = instance->currentExecutionContext()->readArgument(i).toNumber();
            if (i >= 2)
                break;
        }

        if (std::isnan(args[0]) || std::isnan(args[1]) || std::isnan(args[2])) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        thisDateObject->setTimeValue((int) args[0], (int) args[1], (int) args[2], thisDateObject->getUTCHours(), thisDateObject->getUTCMinutes()
            , thisDateObject->getUTCSeconds(), thisDateObject->getUTCMilliseconds(), false);

        return ESValue(thisDateObject->timeValueAsDouble());
    }, strings->setUTCFullYear, 3));

    // $20.3.4.30 Date.prototype.setUTCHours()
    m_datePrototype->defineDataProperty(strings->setUTCHours, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setUTCHours);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setUTCHours, errorMessage_GlobalObject_ThisNotDateObject);
        }
        escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        double args[4] = {0, (double) thisDateObject->getMinutes(), (double) thisDateObject->getSeconds(), (double) thisDateObject->getMilliseconds()};

        if (arg_size < 1) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (std::isnan(thisDateObject->timeValueAsDouble())) {
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        for (size_t i = 0; i < arg_size; i++) {
            args[i] = instance->currentExecutionContext()->readArgument(i).toNumber();
            if (i >= 3)
                break;
        }

        if (std::isnan(args[0]) || std::isnan(args[1]) || std::isnan(args[2]) || std::isnan(args[3])) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        thisDateObject->setTimeValue(thisDateObject->getUTCFullYear(), thisDateObject->getUTCMonth(), thisDateObject->getUTCDate()
            , (int) args[0], (int) args[1], (int) args[2], (int) args[3], false);

        return ESValue(thisDateObject->timeValueAsDouble());
    }, strings->setUTCHours, 4));

    // $20.3.4.31 Date.prototype.setUTCMilliseconds()
    m_datePrototype->defineDataProperty(strings->setUTCMilliseconds, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setUTCMilliseconds);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setUTCMilliseconds, errorMessage_GlobalObject_ThisNotDateObject);
        }
        escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        double args[1] = {0};

        if (arg_size < 1) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (std::isnan(thisDateObject->timeValueAsDouble())) {
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        args[0] = instance->currentExecutionContext()->readArgument(0).toNumber();

        if (std::isnan(args[0])) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        thisDateObject->setTimeValue(thisDateObject->getUTCFullYear(), thisDateObject->getUTCMonth(), thisDateObject->getUTCDate()
            , thisDateObject->getUTCHours(), thisDateObject->getUTCMinutes(), thisDateObject->getUTCSeconds(), (int) args[0], false);

        return ESValue(thisDateObject->timeValueAsDouble());
    }, strings->setUTCMilliseconds, 1));

    // $20.3.4.32 Date.prototype.setUTCMinutes()
    m_datePrototype->defineDataProperty(strings->setUTCMinutes, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setUTCMinutes);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setUTCMinutes, errorMessage_GlobalObject_ThisNotDateObject);
        }
        escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        double args[3] = {0, (double) thisDateObject->getSeconds(), (double) thisDateObject->getMilliseconds()};

        if (arg_size < 1) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (std::isnan(thisDateObject->timeValueAsDouble())) {
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        for (size_t i = 0; i < arg_size; i++) {
            args[i] = instance->currentExecutionContext()->readArgument(i).toNumber();
            if (i >= 2)
                break;
        }

        if (std::isnan(args[0]) || std::isnan(args[1]) || std::isnan(args[2])) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        thisDateObject->setTimeValue(thisDateObject->getUTCFullYear(), thisDateObject->getUTCMonth(), thisDateObject->getUTCDate()
            , thisDateObject->getUTCHours(), (int) args[0], (int) args[1], (int) args[2], false);

        return ESValue(thisDateObject->timeValueAsDouble());
    }, strings->setUTCMinutes, 3));

    // $20.3.4.33 Date.prototype.setUTCMonth()
    m_datePrototype->defineDataProperty(strings->setUTCMonth, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setUTCMonth);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setUTCMonth, errorMessage_GlobalObject_ThisNotDateObject);
        }
        escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        double args[2] = {0, (double) thisDateObject->getDate()};

        if (arg_size < 1) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (std::isnan(thisDateObject->timeValueAsDouble())) {
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        for (size_t i = 0; i < arg_size; i++) {
            args[i] = instance->currentExecutionContext()->readArgument(i).toNumber();
            if (i >= 1)
                break;
        }

        if (std::isnan(args[0]) || std::isnan(args[1])) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        thisDateObject->setTimeValue(thisDateObject->getUTCFullYear(), (int) args[0], (int) args[1]
            , thisDateObject->getUTCHours(), thisDateObject->getUTCMinutes(), thisDateObject->getUTCSeconds(), thisDateObject->getUTCMilliseconds(), false);

        return ESValue(thisDateObject->timeValueAsDouble());
    }, strings->setUTCMonth, 2));

    // $20.3.4.34 Date.prototype.setUTCSeconds()
    m_datePrototype->defineDataProperty(strings->setUTCSeconds, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setUTCSeconds);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setUTCSeconds, errorMessage_GlobalObject_ThisNotDateObject);
        }
        escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        double args[2] = {0, (double) thisDateObject->getMilliseconds()};

        if (arg_size < 1) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (std::isnan(thisDateObject->timeValueAsDouble())) {
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        for (size_t i = 0; i < arg_size; i++) {
            args[i] = instance->currentExecutionContext()->readArgument(i).toNumber();
            if (i >= 1)
                break;
        }

        if (std::isnan(args[0]) || std::isnan(args[1])) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }

        thisDateObject->setTimeValue(thisDateObject->getUTCFullYear(), thisDateObject->getUTCMonth(), thisDateObject->getUTCDate()
            , thisDateObject->getUTCHours(), thisDateObject->getUTCMinutes(), (int) args[0], (int) args[1], false);

        return ESValue(thisDateObject->timeValueAsDouble());
    }, strings->setUTCSeconds, 2));

    // $20.3.4.35 Date.prototype.toDateString()
    m_datePrototype->defineDataProperty(strings->toDateString, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue e = instance->currentExecutionContext()->resolveThisBinding();
        if (e.isESPointer() && e.asESPointer()->isESDateObject())
            return e.asESPointer()->asESDateObject()->toDateString();
        else
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->toDateString, errorMessage_GlobalObject_ThisNotDateObject);
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->toDateString, 0));

    // $20.3.4.36 Date.prototype.toISOString
    m_datePrototype->defineDataProperty(strings->toISOString, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, toISOString);
        if (thisObject->isESDateObject()) {
            escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();

            char buffer[512];
            if (!std::isnan(thisDateObject->timeValueAsDouble())) {
                if (thisDateObject->getUTCFullYear() >= 0 && thisDateObject->getUTCFullYear() <= 9999) {
                    snprintf(buffer, 512, "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ"
                        , thisDateObject->getUTCFullYear(), thisDateObject->getUTCMonth() + 1, thisDateObject->getUTCDate()
                        , thisDateObject->getUTCHours(), thisDateObject->getUTCMinutes(), thisDateObject->getUTCSeconds(), thisDateObject->getUTCMilliseconds());
                } else {
                    snprintf(buffer, 512, "%+07d-%02d-%02dT%02d:%02d:%02d.%03dZ"
                        , thisDateObject->getUTCFullYear(), thisDateObject->getUTCMonth() + 1, thisDateObject->getUTCDate()
                        , thisDateObject->getUTCHours(), thisDateObject->getUTCMinutes(), thisDateObject->getUTCSeconds(), thisDateObject->getUTCMilliseconds());
                }
                return ESString::create(buffer);
            } else {
                throwBuiltinError(instance, ErrorCode::RangeError, strings->Date, true, strings->toISOString, errorMessage_GlobalObject_InvalidDate);
            }
        } else {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->toISOString, errorMessage_GlobalObject_ThisNotDateObject);
        }
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->toISOString, 0));

    // $20.3.4.37 Date.prototype.toJSON()
    m_datePrototype->defineDataProperty(strings->toJSON, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        ESObject* thisObject = thisValue.toObject();
        ESValue tv = thisValue.toPrimitive(ESValue::PreferNumber);
        if (tv.isNumber() && (std::isnan(tv.asNumber()) || std::isinf(tv.asNumber()))) {
            return ESValue(ESValue::ESNull);
        }

        ESValue func = thisObject->get(strings->toISOString.string());
        if (!func.isESPointer() || !func.asESPointer()->isESFunctionObject())
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->toJSON, errorMessage_GlobalObject_ToISOStringNotCallable);
        return ESFunctionObject::call(instance, func, thisObject, NULL, 0, false);
    }, strings->toJSON, 1));

    // $20.3.4.38 Date.prototype.toLocaleDateString()
    m_datePrototype->defineDataProperty(strings->toLocaleDateString, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue e = instance->currentExecutionContext()->resolveThisBinding();
        if (e.isESPointer() && e.asESPointer()->isESDateObject())
            return e.asESPointer()->asESDateObject()->toDateString();
        else
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->toLocaleDateString, errorMessage_GlobalObject_ThisNotDateObject);
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->toLocaleDateString, 0));

    // $20.3.4.39 Date.prototype.toLocaleString()
    m_datePrototype->defineDataProperty(strings->toLocaleString, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, toLocaleString);

        icu::DateFormat *df = icu::DateFormat::createDateInstance(icu::DateFormat::FULL, instance->locale());
        icu::UnicodeString myString;
        df->format(thisObject->asESDateObject()->timeValueAsDouble(), myString);
        myString.append(u' ');
        icu::DateFormat *tf = icu::DateFormat::createTimeInstance(icu::DateFormat::FULL, instance->locale());
        tf->format(thisObject->asESDateObject()->timeValueAsDouble(), myString);

        delete df;
        delete tf;
        return ESString::create(myString);
    }, strings->toLocaleString, 0));

    // $20.3.4.40 Date.prototype.toLocaleTimeString()
    m_datePrototype->defineDataProperty(strings->toLocaleTimeString, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue e = instance->currentExecutionContext()->resolveThisBinding();
        if (e.isESPointer() && e.asESPointer()->isESDateObject())
            return e.asESPointer()->asESDateObject()->toTimeString();
        else
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->toLocaleTimeString, errorMessage_GlobalObject_ThisNotDateObject);
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->toLocaleTimeString, 0));

    // $20.3.4.42 Date.prototype.toTimeString()
    m_datePrototype->defineDataProperty(strings->toTimeString, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue e = instance->currentExecutionContext()->resolveThisBinding();
        if (e.isESPointer() && e.asESPointer()->isESDateObject())
            return e.asESPointer()->asESDateObject()->toTimeString();
        else
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->toTimeString, errorMessage_GlobalObject_ThisNotDateObject);
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->toTimeString, 0));

    // $20.3.4.43 Date.prototype.toUTCString()
    m_datePrototype->defineDataProperty(strings->toUTCString, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue e = instance->currentExecutionContext()->resolveThisBinding();
        if (e.isESPointer() && e.asESPointer()->isESDateObject()) {
            static char days[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
            static char months[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

            escargot::ESDateObject* thisDateObject = e.asESPointer()->asESDateObject();
            char buffer[512];
            if (!std::isnan(thisDateObject->timeValueAsDouble())) {
                snprintf(buffer, 512, "%s, %02d %s %d %02d:%02d:%02d GMT"
                    , days[thisDateObject->getUTCDay()], thisDateObject->getUTCDate(), months[thisDateObject->getUTCMonth()], thisDateObject->getUTCFullYear()
                    , thisDateObject->getUTCHours(), thisDateObject->getUTCMinutes(), thisDateObject->getUTCSeconds());
                return ESString::create(buffer);
            } else {
                throwBuiltinError(instance, ErrorCode::RangeError, strings->Date, true, strings->toUTCString, errorMessage_GlobalObject_InvalidDate);
            }
        } else
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->toUTCString, errorMessage_GlobalObject_ThisNotDateObject);
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->toUTCString, 0));

    // $44 Date.prototype.valueOf()
    m_datePrototype->defineDataProperty(strings->valueOf, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, valueOf);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->valueOf, errorMessage_GlobalObject_ThisNotDateObject);
        }
        double primitiveValue = thisObject->asESDateObject()->timeValueAsDouble();
        return ESValue(primitiveValue);
    }, strings->valueOf, 0));

    // $B.2.4.1 Date.prototype.getYear()
    m_datePrototype->defineDataProperty(strings->getYear, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, getYear);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->getYear, errorMessage_GlobalObject_ThisNotDateObject);
        }
        int ret = thisObject->asESDateObject()->getFullYear() - 1900;
        return ESValue(ret);
    }, strings->getYear, 0));

    // $B.2.4.2 Date.prototype.setYear()
    m_datePrototype->defineDataProperty(strings->setYear, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, setYear);
        if (!thisObject->isESDateObject()) {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->setYear, errorMessage_GlobalObject_ThisNotDateObject);
        }
        escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        double args[1];

        if (arg_size < 1) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (std::isnan(thisDateObject->timeValueAsDouble())) {
            thisDateObject->setTimeValue(1970, 0, 1, 0, 0, 0, 0, true);
        }

        args[0] = instance->currentExecutionContext()->readArgument(0).toNumber();

        if (std::isnan(args[0])) {
            thisDateObject->setTimeValueAsNaN();
            return ESValue(thisDateObject->timeValueAsDouble());
        }
        if (0 <= args[0] && args[0] <= 99) {
            args[0] += 1900;
        }

        thisDateObject->setTimeValue((int) args[0], thisDateObject->getMonth(), thisDateObject->getDate(), thisDateObject->getHours(), thisDateObject->getMinutes()
            , thisDateObject->getSeconds(), thisDateObject->getMilliseconds());

        return ESValue(thisDateObject->timeValueAsDouble());

        RELEASE_ASSERT_NOT_REACHED();
    }, strings->setYear, 1));

    // $B.2.4.3 Date.prototype.toGMTString()
    m_datePrototype->defineDataProperty(strings->toGMTString, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        RESOLVE_THIS_BINDING_TO_OBJECT(thisObject, Date, toGMTString);
        if (thisObject->isESDateObject()) {
            escargot::ESDateObject* thisDateObject = thisObject->asESDateObject();

            static char days[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
            static char months[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

            char buffer[512];
            if (!std::isnan(thisDateObject->timeValueAsDouble())) {
                snprintf(buffer, 512, "%s, %02d %s %d %02d:%02d:%02d GMT"
                    , days[thisDateObject->getUTCDay()], thisDateObject->getUTCDate(), months[thisDateObject->getUTCMonth()], thisDateObject->getUTCFullYear()
                    , thisDateObject->getUTCHours(), thisDateObject->getUTCMinutes(), thisDateObject->getUTCSeconds());
                return ESString::create(buffer);
            } else {
                throwBuiltinError(instance, ErrorCode::RangeError, strings->Date, true, strings->toISOString, errorMessage_GlobalObject_InvalidDate);
            }
        } else {
            throwBuiltinError(instance, ErrorCode::TypeError, strings->Date, true, strings->toISOString, errorMessage_GlobalObject_ThisNotDateObject);
        }
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->toGMTString, 0));
}

}
