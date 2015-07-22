#include "Escargot.h"
#include "GlobalObject.h"
#include "ast/AST.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"


namespace escargot {

GlobalObject::GlobalObject()
{
    FunctionDeclarationNode* node = new FunctionDeclarationNode(L"print", ESStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        JSObject* value = instance->currentExecutionContext()->environment()->record()->getBindingValue(L"arguments", false)->toHeapObject()->toJSObject();
        ESValue* val = value->get(L"0");
        ESString str = val->toESString();
        wprintf(L"%ls\n", str.data());
        return esUndefined;
    }), false, false);
    auto printFunction = JSFunction::create(NULL, node);

    set(L"print", printFunction);
    set(L"Array", installArray());
}

ESValue* GlobalObject::installArray() {
    m_arrayPrototype = JSArray::create(0, NULL); //FIXME: %ObjectPrototype%

    //$22.1.3.17 Array.prototype.push(item)
    FunctionDeclarationNode* arrayPush = new FunctionDeclarationNode(L"push", ESStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        JSObject* value = instance->currentExecutionContext()->environment()->record()->getBindingValue(L"arguments", false)->toHeapObject()->toJSObject();
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->toJSArray();
        int i = 0;
        while(true) {
            ESValue* val = value->get(ESString(i));
            if (val == esUndefined) break;
            thisVal->set( thisVal->length(), val );
            i++;
        }
        instance->currentExecutionContext()->doReturn(thisVal->length());

        return esUndefined;
    }), false, false);
    m_arrayPrototype->set(L"push", JSFunction::create(NULL, arrayPush));

    //$22.1.1
    FunctionDeclarationNode* constructor = new FunctionDeclarationNode(L"Array", ESStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        auto r = JSArray::create(0, instance->globalObject()->arrayPrototype());
        instance->currentExecutionContext()->doReturn(r);
        return esUndefined;
    }), false, false);

    auto function = JSFunction::create(NULL, constructor);
    function->set(L"prototype", arrayPrototype());
    return function;
}

}
