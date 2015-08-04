#include "Escargot.h"
#include "ESValue.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#include "ast/AST.h"

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

ESString* ESValue::toESString() const
{
    return ESString::create(toInternalString());
}

InternalString ESValue::toInternalString() const
{
    InternalString ret;

    if(isInt32()) {
        ret = InternalString(asInt32());
    } else if(isNumber()) {
        double d = asNumber();
        if (std::isnan(d)) ret = L"NaN";
        else if (d == std::numeric_limits<double>::infinity()) ret = L"Infinity";
        else if (d== -std::numeric_limits<double>::infinity()) ret = L"-Infinity";
        else ret = InternalString(d);
    } else if(isUndefined()) {
        ret = strings->undefined;
    } else if(isNull()) {
        ret = strings->null;
    } else if(isBoolean()) {
        if(asBoolean())
            ret = L"true";
        else
            ret = L"false";
    } else {
        ESPointer* o = asESPointer();
        if(o->isESString()) {
            ret = o->asESString()->string();
        } else if(o->isESFunctionObject()) {
            //ret = L"[Function function]";
            ret = L"function ";
            ESFunctionObject* fn = o->asESFunctionObject();
            ret.append(fn->functionAST()->id());
            ret.append(L"() {}");
        } else if(o->isESArrayObject()) {
            bool isFirst = true;
            ret.append(L"[");
            for (int i = 0 ; i < o->asESArrayObject()->length().asInt32() ; i++) {
                if(!isFirst)
                    ret.append(L", ");
                ESValue slot = o->asESArrayObject()->get(i);
                ret.append(slot.toInternalString());
                isFirst = false;
              }
            ret.append(L"]");
        } else if(o->isESErrorObject()) {
            ret.append(o->asESObject()->get(L"name", true).toInternalString().data());
            ret.append(L": ");
            ret.append(o->asESObject()->get(L"message").toInternalString().data());
        } else if(o->isESObject()) {
            ret.append(o->asESObject()->constructor().asESPointer()->asESObject()->get(L"name", true).toInternalString().data());
            ret.append(L" {");
            bool isFirst = true;
            o->asESObject()->enumeration([&ret, &isFirst](const InternalString& key, ESSlot* slot) {
                    if(!isFirst)
                    ret.append(L", ");
                    ret.append(key);
                    ret.append(L": ");
                    ret.append(slot->value().toInternalString());
                    isFirst = false;
                    });
            if(o->isESStringObject()) {
                ret.append(L", [[PrimitiveValue]]: \"");
                ret.append(o->asESStringObject()->getStringData()->string());
                ret.append(L"\"");
            }
            ret.append(L"}");
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    return ret;
}

ESValue ESObject::defaultValue(ESVMInstance* instance, ESValue::PrimitiveTypeHint hint)
{
    if (hint == ESValue::PreferString) {
        ESValue underScoreProto = get(strings->__proto__);
        ESValue toStringMethod = underScoreProto.asESPointer()->asESObject()->get(L"toString");
        std::vector<ESValue> arguments;
        return ESFunctionObject::call(toStringMethod, this, &arguments[0], arguments.size(), instance);
    } else {
        //TODO
        RELEASE_ASSERT_NOT_REACHED();
    }
}


ESValue functionCallerInnerProcess(ESFunctionObject* fn, ESValue receiver, ESValue arguments[], size_t argumentCount, bool needsArgumentsObject, ESVMInstance* ESVMInstance)
{
    ESVMInstance->invalidateIdentifierCacheCheckCount();
    ((FunctionEnvironmentRecord *)ESVMInstance->currentExecutionContext()->environment()->record())->bindThisValue(receiver.asESPointer()->asESObject());
    DeclarativeEnvironmentRecord* functionRecord = ESVMInstance->currentExecutionContext()->environment()->record()->toDeclarativeEnvironmentRecord();

    if(needsArgumentsObject) {
        ESObject* argumentsObject = ESObject::create();
        unsigned i = 0;
        argumentsObject->set(strings->length, ESValue(argumentCount));
        for(; i < argumentCount && i < ESCARGOT_STRINGS_NUMBERS_MAX ; i ++) {
            argumentsObject->set(strings->numbers[i], arguments[i]);
        }
        for( ; i < argumentCount ; i ++) {
            argumentsObject->set(InternalAtomicString(InternalString((int)i).data()), arguments[i]);
        }

        if(!fn->functionAST()->needsActivation()) {
            for(size_t i = 0 ; i < fn->functionAST()->innerIdentifiers().size() ; i++ ) {
                if(fn->functionAST()->innerIdentifiers()[i] == strings->arguments) {
                    functionRecord->createMutableBindingForNonActivationMode(i, strings->name, argumentsObject);
                    break;
                }
            }
            ASSERT(functionRecord->hasBinding(strings->arguments));
        } else {
            functionRecord->createMutableBinding(strings->arguments, false);
            functionRecord->setMutableBinding(strings->arguments, argumentsObject, true);
        }
    }

    if(fn->functionAST()->needsActivation()) {
        const InternalAtomicStringVector& params = fn->functionAST()->params();
        for(unsigned i = 0; i < params.size() ; i ++) {
            functionRecord->createMutableBinding(params[i], false);
            if(i < argumentCount) {
                functionRecord->setMutableBinding(params[i], arguments[i], true);
            }
        }
    } else {
        for(size_t i = 0 ; i < fn->functionAST()->innerIdentifiers().size() ; i++ ) {
            if(fn->functionAST()->innerIdentifiers()[i] != strings->arguments) {
                functionRecord->createMutableBindingForNonActivationMode(i, fn->functionAST()->innerIdentifiers()[i]);
            }
        }

        const InternalAtomicStringVector& params = fn->functionAST()->params();
        for(unsigned i = 0; i < params.size() ; i ++) {
            if(i < argumentCount) {
                functionRecord->setMutableBinding(params[i], arguments[i], true);
            }
        }
    }

    int r = setjmp(ESVMInstance->currentExecutionContext()->returnPosition());
    if(r != 1) {
        fn->functionAST()->body()->execute(ESVMInstance);
    }
    return ESVMInstance->currentExecutionContext()->returnValue();
}


ESValue ESFunctionObject::call(ESValue callee, ESValue receiver, ESValue arguments[], size_t argumentCount, ESVMInstance* ESVMInstance, bool isNewExpression)
{
    ESValue result;
    if(callee.isESPointer() && callee.asESPointer()->isESFunctionObject()) {
        ExecutionContext* currentContext = ESVMInstance->currentExecutionContext();
        ESFunctionObject* fn = callee.asESPointer()->asESFunctionObject();
        if(fn->functionAST()->needsActivation()) {
            ESVMInstance->m_currentExecutionContext = new ExecutionContext(LexicalEnvironment::newFunctionEnvironment(fn, receiver), true, isNewExpression, arguments, argumentCount);
            result = functionCallerInnerProcess(fn, receiver, arguments, argumentCount, true, ESVMInstance);
            ESVMInstance->m_currentExecutionContext = currentContext;
        } else {
            FunctionEnvironmentRecord envRec(true,
                    (std::pair<InternalAtomicString, ::escargot::ESSlot>*)alloca(sizeof(std::pair<InternalAtomicString, ::escargot::ESSlot>) * fn->functionAST()->innerIdentifiers().size()),
                    fn->functionAST()->innerIdentifiers().size());

            envRec.m_functionObject = fn;
            envRec.m_newTarget = receiver;

            LexicalEnvironment env(&envRec, fn->outerEnvironment());
            ExecutionContext ec(&env, false, isNewExpression, arguments, argumentCount);
            ESVMInstance->m_currentExecutionContext = &ec;
            result = functionCallerInnerProcess(fn, receiver, arguments, argumentCount, fn->functionAST()->needsArgumentsObject(), ESVMInstance);
            ESVMInstance->m_currentExecutionContext = currentContext;
        }
    } else {
        throw TypeError();
    }

    return result;
}



}
