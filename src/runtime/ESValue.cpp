#include "Escargot.h"
#include "ESValue.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#include "ast/AST.h"

namespace escargot {

static Undefined s_undefined;
Undefined* esUndefined = &s_undefined;

static Null s_null;
Null* esNull = &s_null;

static Boolean s_true(true);
Boolean* esTrue = &s_true;

static Boolean s_false(false);
Boolean* esFalse = &s_false;

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
        if (o->type() != comp->type())
            return false;
        if (o->isNumber() && o->toNumber()->get() == comp->toNumber()->get())
            return true;
        if (o->isBoolean() && o->toBoolean()->get() == comp->toBoolean()->get())
            return true;
        if (o->isJSString() && o->toJSString()->string() == comp->toJSString()->string())
            return true;
        //TODO
        if (o->isJSFunction())
            return false;
        if (o->isJSArray())
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
        if(o->isUndefined()) {
            ret = strings->undefined;
        } else if(o->isNull()) {
            ret = strings->null;
        } else if(o->isNumber()) {
            ret = ESString(o->toNumber()->get());
        } else if(o->isJSString()) {
            ret = o->toJSString()->string();
        } else if(o->isJSFunction()) {
            //ret = L"[Function function]";
            ret = L"function ";
            JSFunction* fn = o->toJSFunction();
            ret.append(fn->functionAST()->id());
            ret.append(L"() {}");
        } else if(o->isJSArray()) {
            bool isFirst = true;
            for (int i=0; i<o->toJSArray()->length()->toSmi()->value(); i++) {
                ESString key = ESString(i);
                if(!isFirst)
                    ret.append(L", ");
                ESValue* slot = o->toJSObject()->get(ESAtomicString(key.data()));
                ret.append(slot->toESString());
                isFirst = false;
            }
        } else if(o->isJSStringObject()) {
            ret.append(o->toJSStringObject()->getStringData()->string());
        } else if(o->isJSError()) {
        	    JSObject* jso = o->toJSObject();
        	    ESValue* name = jso->get(L"name");
        	    ESValue* msg = jso->get(L"message");
        	    wprintf(L"%ls: %ls\n", name->toESString().data(), msg->toESString().data());
        } else if(o->isJSObject()) {
          ret = L"{";
          bool isFirst = true;
          o->toJSObject()->enumeration([&ret, &isFirst](const ESString& key, JSSlot* slot) {
              if(!isFirst)
                  ret.append(L", ");
              ret.append(key);
              ret.append(L":");
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

// http://www.ecma-international.org/ecma-262/5.1/#sec-9.1
ESValue* ESValue::toPrimitive()
{
    ESValue* ret = this;
    if(isSmi()) {
    } else {
        HeapObject* o = toHeapObject();
        if (o->isJSObject()) {
            // TODO
#ifndef NDEBUG
            ASSERT(false);
#endif
        }
    }
    return ret;
}

// http://www.ecma-international.org/ecma-262/5.1/#sec-9.3
ESValue* ESValue::toNumber()
{
    // TODO
    ESValue* ret = this;
    if(isSmi()) {
    } else {
    }
    return ret;
}

// http://www.ecma-international.org/ecma-262/5.1/#sec-9.5
ESValue* ESValue::toInt32()
{
    // TODO
    ESValue* ret = this->toNumber();
    if(isSmi()) {
    } else {
        HeapObject* o = this->toHeapObject();
        if (o->isNumber()) {
            double d = o->toNumber()->get();
            long long int posInt = d<0?-1:1 * std::floor(std::abs(d));
            long long int int32bit = posInt % 0x100000000;
            int res;
            if (int32bit >= 0x80000000)
                res = int32bit - 0x100000000;
            else
                res = int32bit;
            if (res >= 0x40000000)
                ret = Number::create(res);
            else
                ret = Smi::fromInt(res);
        } else { // TODO
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
    return ret;
}

// http://www.ecma-international.org/ecma-262/5.1/#sec-9.4
ESValue* ESValue::toInteger()
{
    // TODO
    ESValue* ret = this->toNumber();
    if (isSmi()) {
    } else {
        // TODO
    }
    return ret;
}

JSString* ESValue::toString()
{
    return JSString::create(toESString());
}

ESValue* JSObject::defaultValue(ESVMInstance* instance, PrimitiveTypeHint hint)
{
    if (hint == PreferString) {
        ESValue* underScoreProto = get(strings->__proto__);
        ESValue* toStringMethod = underScoreProto->toHeapObject()->toJSObject()->get(L"toString");
        std::vector<ESValue*, gc_allocator<ESValue*>> arguments;
        return ESFunctionCaller::call(toStringMethod, this, &arguments[0], arguments.size(), instance);
    } else {
        ASSERT(false); // TODO
    }
}

}
