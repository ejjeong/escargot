#include "Escargot.h"
#include "GlobalObject.h"
#include "ast/AST.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"


namespace escargot {

GlobalObject::GlobalObject()
{
    installFunction();
    installObject();
    installArray();
    installError();

    FunctionDeclarationNode* node = new FunctionDeclarationNode(ESAtomicString(L"print"), ESAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        JSObject* value = instance->currentExecutionContext()->environment()->record()->getBindingValue(strings->arguments, false)->toHeapObject()->toJSObject();
        ESValue* val = value->get(strings->numbers[0]);
        ESString str = val->toESString();
        wprintf(L"%ls\n", str.data());
        return esUndefined;
    }), false, false);
    auto printFunction = JSFunction::create(NULL, node);
    set(L"print", printFunction);

    node = new FunctionDeclarationNode(ESAtomicString(L"gc"), ESAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        GC_gcollect();
        return esUndefined;
    }), false, false);
    auto gcFunction = JSFunction::create(NULL, node);
    set(L"gc", gcFunction);

    node = new FunctionDeclarationNode(ESAtomicString(L"load"), ESAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        JSObject* value = instance->currentExecutionContext()->environment()->record()->getBindingValue(strings->arguments, false)->toHeapObject()->toJSObject();
        ESValue* val = value->get(strings->numbers[0]);
        ESString str = val->toESString();
        const wchar_t* pt = str.data();
        std::string path;
        char buffer [MB_CUR_MAX];
        while(*pt) {
            int length = std::wctomb(buffer,*pt);
            if (length<1)
                break;
            path.append(buffer);
            pt++;
        }
        FILE *fp = fopen(path.c_str(),"r");
        if(fp) {
            std::string str;
            char buf[512];
            while(fgets(buf, sizeof buf, fp) != NULL) {
                str += buf;
            }
            fclose(fp);
            instance->runOnGlobalContext([instance, &str](){
                instance->evaluate(str);
            });

        }
        return esUndefined;
    }), false, false);
    auto loadFunction = JSFunction::create(NULL, node);
    set(L"load", loadFunction);
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

void GlobalObject::installError()
{
    ::escargot::JSFunction* emptyFunction = m_functionPrototype;
    m_error = ::escargot::JSFunction::create(NULL,new FunctionDeclarationNode(strings->Error, ESAtomicStringVector(), new EmptyStatementNode(), false, false));
    m_error->set(strings->name, String::create(strings->Error));
    m_error->setConstructor(m_function);
    m_error->set__proto__(emptyFunction);

    m_errorPrototype = JSObject::create();
    m_errorPrototype->setConstructor(m_error);
    m_error->set(strings->prototype, m_errorPrototype);

    set(strings->Error, m_error);
}

void GlobalObject::installArray()
{
    m_arrayPrototype = JSArray::create(0, m_objectPrototype);

    //$22.1.1 Array Constructor
    FunctionDeclarationNode* constructor = new FunctionDeclarationNode(strings->Array, ESAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        JSObject* value = instance->currentExecutionContext()->environment()->record()->getBindingValue(L"arguments", false)->toHeapObject()->toJSObject();
        int len = instance->currentExecutionContext()->environment()->record()->getBindingValue(strings->length, false)->toSmi()->value();
        int size = 0;
        if (len > 1) size = len;
        JSObject* proto = instance->globalObject()->arrayPrototype();
        escargot::JSArray* array = JSArray::create(size, proto);
        ESValue* val = value->get(strings->numbers[0]);
        if (len == 1 && val != esUndefined && val->isSmi()) { //numberOfArgs = 1
            array->setLength( val->toSmi()->value() );
        } else if (len >= 1) {      // numberOfArgs>=2 or (numberOfArgs==1 && val is not Number)
            for (int idx = 0; idx < len; idx++) {
                array->set(Smi::fromInt(idx), val);
                val = value->get(ESAtomicString(ESString(idx + 1).data()));
            }
        }
        instance->currentExecutionContext()->doReturn(array);
        return esUndefined;
    }), false, false);

    //$22.1.3.11 Array.prototype.indexOf()
    FunctionDeclarationNode* arrayIndexOf = new FunctionDeclarationNode(L"indexOf", ESAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        JSObject* value = instance->currentExecutionContext()->environment()->record()->getBindingValue(L"arguments", false)->toHeapObject()->toJSObject();
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->toJSArray();
        int len = thisVal->length()->toSmi()->value();
        int ret = 0;
        if (len == 0) ret = -1;
        else {
            ESValue* fromIndex = value->get(L"1");
            int n = 0, k = 0;
            if (fromIndex != esUndefined) {
                n = fromIndex->toSmi()->value();
                if (n >= len) {
                    ret = -1;
                } else if (n >= 0) {
                    k = n;
                } else {
                    k = len - n * (-1);
                    if(k < 0) k = 0;
                }
            }
            if (ret != -1) {
                ret = -1;
                ESValue* searchElement = value->get(L"0");
                while (k < len) {
                    ESValue* kPresent = thisVal->get(ESString(k).data());
                    if (searchElement->equalsTo(kPresent)) {
                        ret = k;
                        break;
                    }
                    k++;
                }
            }
        }
        instance->currentExecutionContext()->doReturn(Smi::fromInt(ret));
        return esUndefined;
    }), false, false);
    m_arrayPrototype->set(L"indexOf", JSFunction::create(NULL, arrayIndexOf));


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

    m_array = JSFunction::create(NULL, constructor);
    m_arrayPrototype->setConstructor(m_array);
    m_arrayPrototype->set(strings->length, Smi::fromInt(0));
    m_array->set(strings->prototype, m_arrayPrototype);
    m_array->set(strings->length, Smi::fromInt(1));
    m_array->setConstructor(m_function);

    set(strings->Array, m_array);

}

}
