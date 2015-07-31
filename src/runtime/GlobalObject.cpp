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
    set(L"Infinity", ESValue(std::numeric_limits<double>::infinity()));
    set(L"NaN", ESValue(std::numeric_limits<double>::quiet_NaN()));
    set(strings->undefined, ESValue());

    FunctionDeclarationNode* node = new FunctionDeclarationNode(InternalAtomicString(L"print"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        if(instance->currentExecutionContext()->argumentCount()) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            InternalString str = val.toInternalString();
            wprintf(L"%ls\n", str.data());
        }
        return ESValue();
    }), false, false);
    auto printFunction = ESFunctionObject::create(NULL, node);
    set(L"print", printFunction);

    node = new FunctionDeclarationNode(InternalAtomicString(L"gc"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        GC_gcollect();
        return ESValue();
    }), false, false);
    auto gcFunction = ESFunctionObject::create(NULL, node);
    set(L"gc", gcFunction);

    node = new FunctionDeclarationNode(InternalAtomicString(L"load"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        if(instance->currentExecutionContext()->argumentCount()) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            InternalString str = val.toInternalString();
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
        }
        return ESValue();
    }), false, false);
    auto loadFunction = ESFunctionObject::create(NULL, node);
    set(L"load", loadFunction);
    set(L"run", loadFunction);
}


void GlobalObject::installFunction()
{
    m_function = ESFunctionObject::create(NULL, new FunctionDeclarationNode(strings->Function, InternalAtomicStringVector(), new EmptyStatementNode(), false, false));
    m_function->set(strings->constructor, m_function);
    m_function->set(strings->name, ESString::create(strings->Function));
    m_function->setConstructor(m_function);
    ::escargot::ESFunctionObject* emptyFunction = ESFunctionObject::create(NULL,new FunctionDeclarationNode(strings->Empty, InternalAtomicStringVector(), new EmptyStatementNode(), false, false));

    m_functionPrototype = emptyFunction;
    m_functionPrototype->setConstructor(m_function);

    m_function->defineAccessorProperty(strings->prototype, [](ESObject* self) -> ESValue {
        return self->asESFunctionObject()->protoType();
    },nullptr, true, false, false);
    m_function->set__proto__(emptyFunction);
    m_function->setProtoType(emptyFunction);

    set(strings->Function, m_function);
}

void GlobalObject::installObject()
{
    ::escargot::ESFunctionObject* emptyFunction = m_functionPrototype;
    m_object = ::escargot::ESFunctionObject::create(NULL,new FunctionDeclarationNode(strings->Object, InternalAtomicStringVector(), new EmptyStatementNode(), false, false));
    m_object->set(strings->name, ESString::create(strings->Object));
    m_object->setConstructor(m_function);
    m_object->set__proto__(emptyFunction);

    m_objectPrototype = ESObject::create();
    m_objectPrototype->setConstructor(m_object);
    m_object->set(strings->prototype, m_objectPrototype);

    set(strings->Object, m_object);
}

void GlobalObject::installError()
{
	  // Initialization for reference error
    ::escargot::ESFunctionObject* emptyFunction = m_functionPrototype;
    m_referenceError = ::escargot::ESFunctionObject::create(NULL,new FunctionDeclarationNode(strings->ReferenceError, InternalAtomicStringVector(), new EmptyStatementNode(), false, false));
    m_referenceError->set(strings->name, ESString::create(strings->ReferenceError));
    m_referenceError->setConstructor(m_function);
    m_referenceError->set__proto__(emptyFunction);

    m_referenceErrorPrototype = ESErrorObject::create();
    m_referenceErrorPrototype->setConstructor(m_referenceError);

    m_referenceError->set(strings->prototype, m_referenceErrorPrototype);

    set(strings->ReferenceError, m_referenceError);

    // We need initializations of other type of error objects
}

void GlobalObject::installArray()
{
    m_arrayPrototype = ESArrayObject::create(0, m_objectPrototype);

    //$22.1.1 Array Constructor
    FunctionDeclarationNode* constructor = new FunctionDeclarationNode(strings->Array, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int len = instance->currentExecutionContext()->argumentCount();
        int size = 0;
        if (len > 1) size = len;
        ESObject* proto = instance->globalObject()->arrayPrototype();
        escargot::ESArrayObject* array = ESArrayObject::create(size, proto);
        if(len) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            if (len == 1 && !val.isUndefined() && val.isInt32()) { //numberOfArgs = 1
                array->setLength( val.asInt32() );
            } else if (len >= 1) {      // numberOfArgs>=2 or (numberOfArgs==1 && val is not ESNumber)
                for (int idx = 0; idx < len; idx++) {
                    array->set(ESValue(idx), val);
                    val = instance->currentExecutionContext()->arguments()[idx + 1];
                }
            }
        } else {
        }
        instance->currentExecutionContext()->doReturn(array);
        return ESValue();
    }), false, false);

    //$22.1.3.11 Array.prototype.indexOf()
    FunctionDeclarationNode* arrayIndexOf = new FunctionDeclarationNode(L"indexOf", InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        int len = thisVal->length().asInt32();
        int ret = 0;
        if (len == 0) ret = -1;
        else {
            ESValue& fromIndex = instance->currentExecutionContext()->arguments()[1];
            int n = 0, k = 0;
            if (!fromIndex.isUndefined()) {
                n = fromIndex.asInt32();
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
                ESValue& searchElement = instance->currentExecutionContext()->arguments()[0];
                while (k < len) {
                    ESValue kPresent = thisVal->get(k);
                    RELEASE_ASSERT_NOT_REACHED();
                    /*
                    if (searchElement.equalsTo(kPresent)) {
                        ret = k;
                        break;
                    }
                    k++;
                    */
                }
            }
        }
        instance->currentExecutionContext()->doReturn(ESValue(ret));
        return ESValue();
    }), false, false);
    m_arrayPrototype->set(L"indexOf", ESFunctionObject::create(NULL, arrayIndexOf));


    //$22.1.3.17 Array.prototype.push(item)
    FunctionDeclarationNode* arrayPush = new FunctionDeclarationNode(L"push", InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int len = instance->currentExecutionContext()->argumentCount();
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        for (int i = 0; i < len; i++) {
            ESValue& val = instance->currentExecutionContext()->arguments()[i];
            thisVal->push(val);
            i++;
        }
        instance->currentExecutionContext()->doReturn(thisVal->length());

        return ESValue();
    }), false, false);
    m_arrayPrototype->set(L"push", ESFunctionObject::create(NULL, arrayPush));

    m_array = ESFunctionObject::create(NULL, constructor);
    m_arrayPrototype->setConstructor(m_array);
    m_arrayPrototype->set(strings->length, ESValue(0));
    m_array->set(strings->prototype, m_arrayPrototype);
    m_array->set(strings->length, ESValue(1));
    m_array->setConstructor(m_function);

    set(strings->Array, m_array);
}

void GlobalObject::installString()
{
    m_string = ESFunctionObject::create(NULL, new FunctionDeclarationNode(strings->String, InternalAtomicStringVector(), new EmptyStatementNode(), false, false));
    //m_string->set(strings->constructor, m_function); TODO do i need this?
    m_string->set(strings->name, ESString::create(strings->String));
    m_string->setConstructor(m_function);

    m_stringPrototype = ESStringObject::create(L"");
    m_stringPrototype->setConstructor(m_string);

    m_string->defineAccessorProperty(strings->prototype, [](ESObject* self) -> ESValue {
        return self->asESFunctionObject()->protoType();
    }, nullptr, true, false, false);
    m_string->set__proto__(m_functionPrototype); // empty Function
    m_string->setProtoType(m_stringPrototype);

    set(strings->String, m_string);

    //$21.1.3.8 String.prototype.indexOf(searchString[, position])
    FunctionDeclarationNode* stringIndexOf = new FunctionDeclarationNode(L"indexOf", InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        //if (thisObject->isESUndefined() || thisObject->isESNull())
        //    throw TypeError();
        const InternalString& str = thisObject->asESStringObject()->getStringData()->string();
        const InternalString& searchStr = instance->currentExecutionContext()->arguments()[0].toInternalString(); // TODO converesion w&w/o test

        ESValue val;
        if(instance->currentExecutionContext()->argumentCount() > 1)
            val = instance->currentExecutionContext()->arguments()[1];

        int result;
        if (val.isUndefined()) {
            result = str.string()->find(*searchStr.string());
        } else {
            double numPos = val.toNumber();
            int pos = numPos;
            int len = str.string()->length();
            int start = std::min(std::max(pos, 0), len);
            result = str.string()->find(*searchStr.string(), start);
        }
        instance->currentExecutionContext()->doReturn(ESValue(result));
        return ESValue();
    }), false, false);
    m_stringPrototype->set(L"indexOf", ESFunctionObject::create(NULL, stringIndexOf));

    //$21.1.3.19 String.prototype.substring(start, end)
    FunctionDeclarationNode* stringSubstring = new FunctionDeclarationNode(L"substring", InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        //if (thisObject->isESUndefined() || thisObject->isESNull())
        //    throw TypeError();

        const InternalString& str = thisObject->asESStringObject()->getStringData()->string();
        int len = str.length();
        int intStart = instance->currentExecutionContext()->arguments()[0].asInt32();
        ESValue& end = instance->currentExecutionContext()->arguments()[1];
        //int intEnd = (end.isUndefined()) ? len : end->toInteger().asInt32();
        int intEnd = (end.isUndefined()) ? len : end.asInt32();
        int finalStart = std::min(std::max(intStart, 0), len);
        int finalEnd = std::min(std::max(intEnd, 0), len);
        int from = std::min(finalStart, finalEnd);
        int to = std::max(finalStart, finalEnd);
        InternalString ret(str.string()->substr(from, to-from).c_str());
        instance->currentExecutionContext()->doReturn(ESString::create(ret));
        return ESValue();
    }), false, false);
    m_stringPrototype->set(L"substring", ESFunctionObject::create(NULL, stringSubstring));
}

void GlobalObject::installDate()
{
    m_datePrototype = ESDateObject::create();

    //$20.3.2 The Date Constructor
    FunctionDeclarationNode* constructor = new FunctionDeclarationNode(strings->Date, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* proto = instance->globalObject()->arrayPrototype();
        escargot::ESDateObject* date = ESDateObject::create(proto);
        date->setTimeValue();
        instance->currentExecutionContext()->doReturn(date);
        return ESValue();
    }), false, false);

      // Initialization for reference error
    ::escargot::ESFunctionObject* emptyFunction = m_functionPrototype;
    m_date = ::escargot::ESFunctionObject::create(NULL, constructor);
    m_date->set(strings->name, ESString::create(strings->Date));
    m_date->setConstructor(m_function);
    m_date->set__proto__(emptyFunction);

    m_datePrototype->setConstructor(m_date);

    m_date->set(strings->prototype, m_datePrototype);

    set(strings->Date, m_date);
}


}
