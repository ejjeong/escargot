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
    installString();
    installError();
    installDate();

    // Value Properties of the Global Object
    definePropertyOrThrow(L"Infinity", false, false, false);
    definePropertyOrThrow(L"NaN", false, false, false);
    definePropertyOrThrow(strings->undefined, false, false, false);
    set(L"Infinity", esInfinity);
    set(L"NaN", esNaN);
    set(strings->undefined, esUndefined);

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
        memset(buffer, 0, MB_CUR_MAX);
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
    set(L"run", loadFunction);
}

void GlobalObject::installFunction()
{
    m_function = JSFunction::create(NULL, new FunctionDeclarationNode(strings->Function, ESAtomicStringVector(), new EmptyStatementNode(), false, false));
    m_function->set(strings->constructor, m_function);
    m_function->set(strings->name, PString::create(strings->Function));
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
    m_object->set(strings->name, PString::create(strings->Object));
    m_object->setConstructor(m_function);
    m_object->set__proto__(emptyFunction);

    m_objectPrototype = JSObject::create();
    m_objectPrototype->setConstructor(m_object);
    m_object->set(strings->prototype, m_objectPrototype);

    set(strings->Object, m_object);
}

void GlobalObject::installError()
{
	  // Initialization for reference error
    ::escargot::JSFunction* emptyFunction = m_functionPrototype;
    m_referenceError = ::escargot::JSFunction::create(NULL,new FunctionDeclarationNode(strings->ReferenceError, ESAtomicStringVector(), new EmptyStatementNode(), false, false));
    m_referenceError->set(strings->name, PString::create(strings->ReferenceError));
    m_referenceError->setConstructor(m_function);
    m_referenceError->set__proto__(emptyFunction);

    m_referenceErrorPrototype = JSError::create();
    m_referenceErrorPrototype->setConstructor(m_referenceError);

    m_referenceError->set(strings->prototype, m_referenceErrorPrototype);

    set(strings->ReferenceError, m_referenceError);

    // We need initializations of other type of error objects
}

void GlobalObject::installArray()
{
    m_arrayPrototype = JSArray::create(0, m_objectPrototype);

    //$22.1.1 Array Constructor
    FunctionDeclarationNode* constructor = new FunctionDeclarationNode(strings->Array, ESAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        JSObject* value = instance->currentExecutionContext()->environment()->record()->getBindingValue(strings->arguments, false)->toHeapObject()->toJSObject();
        int len = value->get(strings->length)->toSmi()->value();
        int size = 0;
        if (len > 1) size = len;
        JSObject* proto = instance->globalObject()->arrayPrototype();
        escargot::JSArray* array = JSArray::create(size, proto);
        ESValue* val = value->get(strings->numbers[0]);
        if (len == 1 && val != esUndefined && val->isSmi()) { //numberOfArgs = 1
            array->setLength( val->toSmi()->value() );
        } else if (len >= 1) {      // numberOfArgs>=2 or (numberOfArgs==1 && val is not PNumber)
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
                    ESValue* kPresent = thisVal->get(k);
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
        int len = value->get(strings->length)->toSmi()->value();
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->toJSArray();
        for (int i = 0; i < len; i++) {
            ESValue* val = value->get(ESAtomicString(ESString(i).data()));
            thisVal->push(val);
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

void GlobalObject::installString()
{
    m_string = JSFunction::create(NULL, new FunctionDeclarationNode(strings->String, ESAtomicStringVector(), new EmptyStatementNode(), false, false));
    //m_string->set(strings->constructor, m_function); TODO do i need this?
    m_string->set(strings->name, PString::create(strings->String));
    m_string->setConstructor(m_function);

    m_stringPrototype = JSString::create(L"");
    m_stringPrototype->setConstructor(m_string);

    m_string->defineAccessorProperty(strings->prototype, [](JSObject* self) -> ESValue* {
        return self->toJSFunction()->protoType();
    }, nullptr, true, false, false);
    m_string->set__proto__(m_functionPrototype); // empty Function
    m_string->setProtoType(m_stringPrototype);

    set(strings->String, m_string);

    //$21.1.3.8 String.prototype.indexOf(searchString[, position])
    FunctionDeclarationNode* stringIndexOf = new FunctionDeclarationNode(L"indexOf", ESAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        JSObject* arguments = instance->currentExecutionContext()->environment()->record()->getBindingValue(L"arguments", false)->toHeapObject()->toJSObject();
        JSObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        if (thisObject->isESUndefined() || thisObject->isESNull())
            throw TypeError();
        const ESString& str = thisObject->toJSString()->getStringData()->string();
        const ESString& searchStr = arguments->get(strings->numbers[0])->toHeapObject()->toPString()->string(); // TODO converesion w&w/o test
        ESValue* val = arguments->get(strings->numbers[1]);

        int result;
        if (val == esUndefined) {
            result = str.string()->find(*searchStr.string());
        } else {
            ESValue* numPos = val->toNumber();
            int pos = numPos->toInteger()->isSmi() ? numPos->toInteger()->toSmi()->value() : numPos->toInteger()->toHeapObject()->toPNumber()->get();
            int len = str.string()->length();
            int start = std::min(std::max(pos, 0), len);
            result = str.string()->find(*searchStr.string(), start);
        }
        instance->currentExecutionContext()->doReturn(Smi::fromInt(result));
        return esUndefined;
    }), false, false);
    m_stringPrototype->set(L"indexOf", JSFunction::create(NULL, stringIndexOf));

    //$21.1.3.19 String.prototype.substring(start, end)
    FunctionDeclarationNode* stringSubstring = new FunctionDeclarationNode(L"substring", ESAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        JSObject* arguments = instance->currentExecutionContext()->environment()->record()->getBindingValue(L"arguments", false)->toHeapObject()->toJSObject();
        JSObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        if (thisObject->isESUndefined() || thisObject->isESNull())
            throw TypeError();

        const ESString& str = thisObject->toJSString()->getStringData()->string();
        int len = str.length();
        int intStart = arguments->get(strings->numbers[0])->toSmi()->value();
        ESValue* end = arguments->get(strings->numbers[1]);
        int intEnd = (end == esUndefined) ? len : end->toInteger()->toSmi()->value();
        int finalStart = std::min(std::max(intStart, 0), len);
        int finalEnd = std::min(std::max(intEnd, 0), len);
        int from = std::min(finalStart, finalEnd);
        int to = std::max(finalStart, finalEnd);
        ESString ret(str.string()->substr(from, to-from).c_str());
        instance->currentExecutionContext()->doReturn(PString::create(ret));
        return esUndefined;
    }), false, false);
    m_stringPrototype->set(L"substring", JSFunction::create(NULL, stringSubstring));
}

void GlobalObject::installDate()
{
    m_datePrototype = JSDate::create();

    //$20.3.2 The Date Constructor
    FunctionDeclarationNode* constructor = new FunctionDeclarationNode(strings->Date, ESAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue * {
        JSObject* proto = instance->globalObject()->arrayPrototype();
        escargot::JSDate* date = JSDate::create(proto);
        date->setTimeValue();
        instance->currentExecutionContext()->doReturn(date);
        return esUndefined;
    }), false, false);

      // Initialization for reference error
    ::escargot::JSFunction* emptyFunction = m_functionPrototype;
    m_date = ::escargot::JSFunction::create(NULL, constructor);
    m_date->set(strings->name, PString::create(strings->Date));
    m_date->setConstructor(m_function);
    m_date->set__proto__(emptyFunction);

    m_datePrototype->setConstructor(m_date);

    m_date->set(strings->prototype, m_datePrototype);

    set(strings->Date, m_date);
}

}
