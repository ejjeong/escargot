#include "Escargot.h"
#include "GlobalObject.h"
#include "ast/AST.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"


namespace escargot {

GlobalObject::GlobalObject()
{
    FunctionDeclarationNode* node = new FunctionDeclarationNode(ESAtomicString(L"print"), ESAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        JSObject* value = instance->currentExecutionContext()->environment()->record()->getBindingValue(strings->arguments, false)->toHeapObject()->toJSObject();
        ESValue* val = value->get(strings->numbers[0]);
        ESString str = val->toESString();
        wprintf(L"%ls\n", str.data());
        return esUndefined;
    }), false, false);
    auto printFunction = JSFunction::create(NULL, node);

    installFunction();
    installObject();
    installArray();

    set(L"print", printFunction);
}

void GlobalObject::installFunction()
{
    m_function = JSFunction::create(NULL, new FunctionDeclarationNode(strings->Function, ESAtomicStringVector(), new EmptyStatementNode(), false, false));
    m_function->set(strings->constructor, m_function);
    m_function->set(strings->name, String::create(strings->Function));
    m_function->setConstructor(m_function);
    ::escargot::JSFunction* emptyFunction = JSFunction::create(NULL,new FunctionDeclarationNode(strings->Empty, ESAtomicStringVector(), new EmptyStatementNode(), false, false));

    m_functionPrototype = emptyFunction;
    m_functionPrototype->setConstructor(m_function);

    m_function->defineAccessorProperty(strings->prototype, [](JSObject* self) -> ESValue* {
        return self->toJSFunction()->protoType();
    },nullptr, true, false, false);
    m_function->set__proto__(emptyFunction);
    m_function->setProtoType(emptyFunction);

    set(strings->Function, m_function);
}

void GlobalObject::installObject()
{
    ::escargot::JSFunction* emptyFunction = m_functionPrototype;
    m_object = ::escargot::JSFunction::create(NULL,new FunctionDeclarationNode(strings->Object, ESAtomicStringVector(), new EmptyStatementNode(), false, false));
    m_object->set(strings->name, String::create(strings->Object));
    m_object->setConstructor(m_function);
    m_object->set__proto__(emptyFunction);

    m_objectPrototype = JSObject::create();
    m_objectPrototype->setConstructor(m_object);
    m_object->set(strings->prototype, m_objectPrototype);

    set(strings->Object, m_object);
}

void GlobalObject::installArray()
{
    m_arrayPrototype = JSArray::create(0, NULL); //FIXME: %ObjectPrototype%

    //$22.1.3.17 Array.prototype.push(item)
    FunctionDeclarationNode* arrayPush = new FunctionDeclarationNode(L"push", ESAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        JSObject* value = instance->currentExecutionContext()->environment()->record()->getBindingValue(L"arguments", false)->toHeapObject()->toJSObject();
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->toJSArray();
        int i = 0;
        while(true) {
            ESValue* val = value->get(ESAtomicString(ESString(i).data()));
            if (val == esUndefined) break;
            thisVal->set( thisVal->length(), val );
            i++;
        }
        instance->currentExecutionContext()->doReturn(thisVal->length());

        return esUndefined;
    }), false, false);
    m_arrayPrototype->set(L"push", JSFunction::create(NULL, arrayPush));

    //$22.1.1
    FunctionDeclarationNode* constructor = new FunctionDeclarationNode(strings->Array, ESAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        auto r = JSArray::create(0, instance->globalObject()->arrayPrototype());
        instance->currentExecutionContext()->doReturn(r);
        return esUndefined;
    }), false, false);

    auto function = JSFunction::create(NULL, constructor);
    function->set(strings->prototype, arrayPrototype());

    set(strings->Array, function);

}


}
