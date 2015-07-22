#include "Escargot.h"
#include "ESValue.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

static Undefined s_undefined;
Undefined* undefined = &s_undefined;

static Null s_null;
Null* null = &s_null;


ESString ESValue::toESString()
{
    ESString ret;

    if(isSmi()) {
        ret = ESString(toSmi()->value());
    } else {
        HeapObject* o = toHeapObject();
        if(o->isUndefined()) {
            ret = strings::undefined;
        } else if(o->isNull()) {
            ret = strings::null;
        } else if(o->isNumber()) {
            ret = ESString(o->toNumber()->get());
        } else if(o->isString()) {
            ret = o->toString()->string();
        } else if(o->isJSFunction()) {
            ret = L"[Function function]";
        } else if(o->isJSArray()) {
            ret = L"[Array array]";
        } else if(o->isJSObject()) {
            ret = L"{";
            bool isFirst = true;
            o->toJSObject()->enumeration([&ret, &isFirst](const ESString& key, JSObjectSlot* slot) {
                if(!isFirst)
                    ret.append(L",");
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
            ASSERT(true);
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

}
