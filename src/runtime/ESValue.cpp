#include "Escargot.h"
#include "ESValue.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

static Undefined s_undefined;
Undefined* esUndefined = &s_undefined;

static Null s_null;
Null* esNull = &s_null;

static Boolean s_true(true);
Boolean* esTrue = &s_true;

static Boolean s_false(false);
Boolean* esFalse = &s_false;


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

}
