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

        if(val->isSmi()) {
            wprintf(L"%d\n", val->toSmi()->value());
        } else {
            HeapObject* ho = val->toHeapObject();
            if(ho->isUndefined()) {
                wprintf(L"undefined\n");
            } else if(ho->isBoolean()) {
                escargot::Boolean* b = ho->toBoolean();
                if(b->get()) {
                    wprintf(L"true\n");
                } else {
                    wprintf(L"false\n");
                }
                wprintf(L"\n");
            } else if(ho->isNull()) {
                wprintf(L"null\n");
            } else if(ho->isNumber()) {
                escargot::Number* n = ho->toNumber();
                wprintf(L"%lg\n", n->get());
            } else if(ho->isString()) {
                wprintf(L"%ls\n",ho->toString()->string().data());
            } else if(ho->isJSFunction()) {
                wprintf(L"[Function function]\n");
            } else if(ho->isJSArray()) {
                wprintf(L"[Array array]\n");
            } else if(ho->isJSObject()) {
                wprintf(L"[Object object]\n");
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
        }

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
