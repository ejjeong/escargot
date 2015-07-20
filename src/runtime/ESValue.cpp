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
            ret = L"undefined";
        } else if(o->isNull()) {
            ret = L"null";
        } else if(o->isNumber()) {
            ret = ESString(o->toNumber()->get());
        } else if(o->isString()) {
            ret = o->toString()->string();
        } else {
            //TODO array, function, object..
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
