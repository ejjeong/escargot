#include "Escargot.h"
#include "ESValue.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#include "ast/AST.h"

namespace escargot {

static ESUndefined s_undefined;
ESUndefined* esUndefined = &s_undefined;

static ESNull s_null;
ESNull* esESNull = &s_null;

static ESBoolean s_true(true);
ESBoolean* esTrue = &s_true;

static ESBoolean s_false(false);
ESBoolean* esFalse = &s_false;

static ESNumber s_nan(std::numeric_limits<double>::quiet_NaN());
ESNumber* esNaN = &s_nan;

static ESNumber s_infinity(std::numeric_limits<double>::infinity());
ESNumber* esInfinity = &s_infinity;
static ESNumber s_ninfinity(-std::numeric_limits<double>::infinity());
ESNumber* esNegInfinity = &s_ninfinity;

static ESNumber s_nzero(-0.0);
ESNumber* esMinusZero = &s_nzero;

// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-abstract-equality-comparison
bool ESValue::abstractEqualsTo(ESValue* val)
{
    if (isSmi() && val->isSmi()) {
        return equalsTo(val);
    } else if (isHeapObject() && val->isHeapObject()) {
        HeapObject* o = toHeapObject();
        if (val->isHeapObject()) {
            HeapObject* comp = val->toHeapObject();
            if (o->type() == comp->type())
                return equalsTo(val);
        }
    }
    //TODO
    ASSERT(false);
    return false;
}

bool ESValue::equalsTo(ESValue* val)
{
    if(isSmi()) {
        if(!val->isSmi()) return false;
        if(toSmi() == val->toSmi()) return true;
        return false;
    } else {
        HeapObject* o = toHeapObject();
        if (!val->isHeapObject()) return false;
        HeapObject* comp = val->toHeapObject();
        if (o->type() != comp->type()) return false;
        //Strict Equality Comparison: === 
        if (o->isESNumber() && o->toESNumber()->get() == comp->toESNumber()->get())
            return true;
        if (o->isESBoolean() && o->toESBoolean()->get() == comp->toESBoolean()->get())
            return true;
        if (o->isPString() && o->toPString()->string() == comp->toPString()->string())
            return true;
        //TODO
        if (o->isESFunctionObject())
            return false;
        if (o->isESArrayObject())
            return false;
        if (o->isJSObject())
            return false;
        return false;
    }
}

ESString ESValue::toESString()
{
    ESString ret;

    if(isSmi()) {
        ret = ESString(toSmi()->value());
    } else {
        HeapObject* o = toHeapObject();
        if(o->isESUndefined()) {
            ret = strings->undefined;
        } else if(o->isESNull()) {
            ret = strings->null;
        } else if(o->isESNumber()) {
            if (o == esNaN) ret = L"NaN";
            else if (o == esInfinity) ret = L"Infinity";
            else if (o == esNegInfinity) ret = L"-Infinity";
            else ret = ESString(o->toESNumber()->get());
        } else if(o->isPString()) {
            ret = o->toPString()->string();
        } else if(o->isESFunctionObject()) {
            //ret = L"[Function function]";
            ret = L"function ";
            ESFunctionObject* fn = o->toESFunctionObject();
            ret.append(fn->functionAST()->id());
            ret.append(L"() {}");
        } else if(o->isESArrayObject()) {
            bool isFirst = true;
            ret.append(L"[");
            for (int i=0; i<o->toESArrayObject()->length()->toSmi()->value(); i++) {
                if(!isFirst)
                    ret.append(L", ");
                ESValue* slot = o->toESArrayObject()->get(i);
                ret.append(slot->toESString());
                isFirst = false;
              }
            ret.append(L"]");
        } else if(o->isJSString()) {
            ret.append(o->toJSString()->getStringData()->string());
        } else if(o->isESErrorObject()) {
        	    ret.append(o->toJSObject()->get(L"name", true)->toESString().data());
        	    ret.append(L": ");
        	    ret.append(o->toJSObject()->get(L"message")->toESString().data());
        } else if(o->isJSObject()) {
          ret = L"Object {";
          bool isFirst = true;
          o->toJSObject()->enumeration([&ret, &isFirst](const ESString& key, JSSlot* slot) {
              if(!isFirst)
                  ret.append(L", ");
              ret.append(key);
              ret.append(L": ");
              ret.append(slot->value()->toESString());
              isFirst = false;
          });
          ret.append(L"}");
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    return ret;
}

ESValue* JSObject::defaultValue(ESVMInstance* instance, PrimitiveTypeHint hint)
{
    if (hint == PreferString) {
        ESValue* underScoreProto = get(strings->__proto__);
        ESValue* toStringMethod = underScoreProto->toHeapObject()->toJSObject()->get(L"toString");
        std::vector<ESValue*, gc_allocator<ESValue*>> arguments;
        return ESFunctionObject::call(toStringMethod, this, &arguments[0], arguments.size(), instance);
    } else {
        ASSERT(false); // TODO
    }
}


ESValue* functionCallerInnerProcess(ESFunctionObject* fn, ESValue* callee, ESValue* receiver, ESValue* arguments[], size_t argumentCount, bool needsArgumentsObject, ESVMInstance* ESVMInstance)
{
    ((FunctionEnvironmentRecord *)ESVMInstance->currentExecutionContext()->environment()->record())->bindThisValue(receiver->toHeapObject()->toJSObject());
    DeclarativeEnvironmentRecord* functionRecord = ESVMInstance->currentExecutionContext()->environment()->record()->toDeclarativeEnvironmentRecord();
    if(needsArgumentsObject) {
        JSObject* argumentsObject = JSObject::create();
        unsigned i = 0;
        argumentsObject->set(strings->length, Smi::fromInt(argumentCount));
        for(; i < argumentCount && i < ESCARGOT_STRINGS_NUMBERS_MAX ; i ++) {
            argumentsObject->set(strings->numbers[i], arguments[i]);
        }
        for( ; i < argumentCount ; i ++) {
            argumentsObject->set(ESAtomicString(ESString((int)i).data()), arguments[i]);
        }

        functionRecord->createMutableBinding(strings->arguments,false);
        functionRecord->setMutableBinding(strings->arguments, argumentsObject, true);
    }

    const ESAtomicStringVector& params = fn->functionAST()->params();

    for(unsigned i = 0; i < params.size() ; i ++) {
        functionRecord->createMutableBinding(params[i],false);
        if(i < argumentCount) {
            functionRecord->setMutableBinding(params[i], arguments[i], true);
        }
    }

    int r = setjmp(ESVMInstance->currentExecutionContext()->returnPosition());
    if(r != 1) {
        fn->functionAST()->body()->execute(ESVMInstance);
    }
    return ESVMInstance->currentExecutionContext()->returnValue();
}


ESValue* ESFunctionObject::call(ESValue* callee, ESValue* receiver, ESValue* arguments[], size_t argumentCount, ESVMInstance* ESVMInstance)
{
    ESValue* result = esUndefined;
    if(callee->isHeapObject() && callee->toHeapObject()->isESFunctionObject()) {
        ExecutionContext* currentContext = ESVMInstance->currentExecutionContext();
        ESFunctionObject* fn = callee->toHeapObject()->toESFunctionObject();
        if(fn->functionAST()->needsActivation()) {
            ESVMInstance->m_currentExecutionContext = new ExecutionContext(LexicalEnvironment::newFunctionEnvironment(fn, receiver));
            result = functionCallerInnerProcess(fn, callee, receiver, arguments, argumentCount, true, ESVMInstance);
            ESVMInstance->m_currentExecutionContext = currentContext;
        } else {
            bool needsArgumentsObject = false;
            ESAtomicStringVector& v = fn->functionAST()->innerIdentifiers();
            for(unsigned i = 0; i < v.size() ; i ++) {
                if(v[i] == strings->arguments) {
                    needsArgumentsObject = true;
                    break;
                }
            }

            FunctionEnvironmentRecord envRec(true,
                    (std::pair<ESAtomicString, ::escargot::JSSlot>*)alloca(sizeof(std::pair<ESAtomicString, ::escargot::JSSlot>) * fn->functionAST()->innerIdentifiers().size()),
                    fn->functionAST()->innerIdentifiers().size());

            envRec.m_functionObject = fn;
            envRec.m_newTarget = receiver;

            LexicalEnvironment env(&envRec, fn->outerEnvironment());
            ExecutionContext ec(&env);
            ESVMInstance->m_currentExecutionContext = &ec;
            result = functionCallerInnerProcess(fn, callee, receiver, arguments, argumentCount, needsArgumentsObject, ESVMInstance);
            ESVMInstance->m_currentExecutionContext = currentContext;
        }
    } else {
        throw TypeError();
    }

    return result;
}

}
