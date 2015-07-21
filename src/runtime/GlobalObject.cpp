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
        return undefined;
    }), false, false);
    auto printFunction = JSFunction::create(NULL, node);

    set(L"print", printFunction);
    set(L"Array", installArray());
}

ESValue* GlobalObject::installArray() {
    m_arrayPrototype = JSArray::create(0);
    //FIXME : implement array push
    m_arrayPrototype->set(L"push", JSFunction::create(NULL, NULL));

    //$22.1.1
    FunctionDeclarationNode* constructor = new FunctionDeclarationNode(L"Array", ESStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        return JSArray::create(0);
    }), false, false);

    auto function = JSFunction::create(NULL, constructor);
    function->set(L"prototype", arrayPrototype());
    return function;
}

}
