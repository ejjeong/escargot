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
        if (o->isPString() && o->toPString()->string() == comp->toPString()->string())
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
        } else if(o->isPString()) {
            ret = o->toPString()->string();
        } else if(o->isJSFunction()) {
            //ret = L"[Function function]";
            ret = L"function ";
            JSFunction* fn = o->toJSFunction();
            ret.append(fn->functionAST()->id());
            ret.append(L"() {}");
        } else if(o->isJSArray()) {
            bool isFirst = true;
            for (int i=0; i<o->toJSArray()->length()->toSmi()->value(); i++) {
                if(!isFirst)
                    ret.append(L", ");
                ESValue* slot = o->toJSArray()->get(i);
                ret.append(slot->toESString());
                isFirst = false;
            }
        } else if(o->isJSString()) {
            ret.append(o->toJSString()->getStringData()->string());
        } else if(o->isJSError()) {
        	    JSObject* jso = o->toJSObject();
        	    ESValue* name = jso->get(L"name", true);
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
