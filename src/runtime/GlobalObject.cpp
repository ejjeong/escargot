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
                wprintf(L"boolean(TODO)\n");
            } else if(ho->isNull()) {
                wprintf(L"null\n");
            } else if(ho->isNumber()) {
                wprintf(L"number(TODO)\n");
            } else if(ho->isString()) {
                wprintf(L"%ls\n",ho->toString()->string().data());
            } else if(ho->isJSObject()) {
                wprintf(L"[Object object]\n");
            } else if(ho->isJSFunction()) {
                wprintf(L"[Function function]\n");
            } else if(ho->isJSArray()) {
                wprintf(L"[Array array]\n");
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
        }

        return undefined;
    }), false, false);
    auto printFunction = JSFunction::create(NULL, node);

    set(L"print", printFunction);
}

}
