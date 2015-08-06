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
    installMath();
    installNumber();

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

    // Function Properties of the Global Object
    definePropertyOrThrow(L"eval", false, false, false);
    node = new FunctionDeclarationNode(InternalAtomicString(L"eval"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESValue argument = instance->currentExecutionContext()->arguments()[0];
        if(!argument.isESString())
            instance->currentExecutionContext()->doReturn(argument);
        std::string scriptStr = argument.asESString()->string().toStdString();
        bool isDirectCall = true; // TODO
        ESValue ret = instance->runOnEvalContext([instance, &scriptStr](){
            ESValue ret = instance->evaluate(scriptStr);
            return ret;
        }, isDirectCall);
        instance->currentExecutionContext()->doReturn(ret);
        return ESValue();
    }), false, false);
    set(L"eval", ESFunctionObject::create(NULL, node));
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
        else if(len == 1) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            if(val.isInt32()) {
                size = val.toNumber();
            } else {
                size = 1;
            }
        }
        ESObject* proto = instance->globalObject()->arrayPrototype();
        escargot::ESArrayObject* array;
        if(instance->currentExecutionContext()->isNewExpression() && instance->currentExecutionContext()->resolveThisBinding()->isESArrayObject()) {
            array = instance->currentExecutionContext()->resolveThisBinding()->asESArrayObject();
            array->setLength(size);
        } else
            array = ESArrayObject::create(size, proto);
        if(len >= 1) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            if(len > 1 || !val.isInt32()) {
                for (int idx = 0; idx < len; idx++) {
                    array->set(idx, val);
                    val = instance->currentExecutionContext()->arguments()[idx + 1];
                }
            }
        } else {
        }
        instance->currentExecutionContext()->doReturn(array);
        return ESValue();
    }), false, false);

    //$22.1.3.11 Array.prototype.indexOf()
    FunctionDeclarationNode* arrayIndexOf = new FunctionDeclarationNode(InternalString(L"indexOf"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        int len = thisVal->length().asInt32();
        int ret = 0;
        if (len == 0) ret = -1;
        else {
            int n = 0, k = 0;
            if(instance->currentExecutionContext()->argumentCount() >= 2) {
                const ESValue& fromIndex = instance->currentExecutionContext()->arguments()[1];
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
            }
            if (ret != -1) {
                ret = -1;
                ESValue& searchElement = instance->currentExecutionContext()->arguments()[0];
                while (k < len) {
                    ESValue kPresent = thisVal->get(k);
                    if (searchElement.equalsTo(kPresent)) {
                        ret = k;
                        break;
                    }
                    k++;
                }
            }
        }
        instance->currentExecutionContext()->doReturn(ESValue(ret));
        return ESValue();
    }), false, false);
    m_arrayPrototype->set(L"indexOf", ESFunctionObject::create(NULL, arrayIndexOf));

    //$22.1.3.12 Array.prototype.join(separator)
    FunctionDeclarationNode* arrayJoin = new FunctionDeclarationNode(InternalString(L"join"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        InternalString ret;
        int arrlen = thisVal->length().asInt32();
        if (arrlen >= 0) {
            InternalString separator;
            if (arglen == 0) {
                separator = L",";
            } else {
                separator = instance->currentExecutionContext()->arguments()[0].toInternalString();
            }
            for (int i = 0; i < arrlen; i++) {
                ESValue elemi = thisVal->get(i);
                if (i != 0) ret.append(separator);
                if (!elemi.isUndefinedOrNull())
                    ret.append(elemi.toInternalString());
            }
        }
        instance->currentExecutionContext()->doReturn(ESString::create(ret));
        return ESValue();
    }), false, false);
    m_arrayPrototype->set(L"join", ESFunctionObject::create(NULL, arrayJoin));

    //$22.1.3.17 Array.prototype.push(item)
    FunctionDeclarationNode* arrayPush = new FunctionDeclarationNode(InternalString(L"push"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
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

    //$22.1.3.25 Array.prototype.splice(start, deleteCount, ...items)
    FunctionDeclarationNode* arraySplice = new FunctionDeclarationNode(InternalString(L"splice"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        int arrlen = thisVal->length().asInt32();
        escargot::ESArrayObject* ret = ESArrayObject::create(0, instance->globalObject()->arrayPrototype());
        if (!thisVal->constructor().isUndefinedOrNull())
            ret->setConstructor(thisVal->constructor());
        if (arglen == 0) {
        } else if (arglen >= 1) {
            int start = instance->currentExecutionContext()->arguments()[0].toNumber();
            int deleteCnt = 0, insertCnt = 0;
            int k;
            if (start < 0)
                start = arrlen+start > 0 ? arrlen+start : 0;
            else
                start = start > arrlen ? arrlen : start;
            if (arglen == 1) {
                deleteCnt = arrlen - start;
            } else {
                insertCnt = arglen - 2;
                int dc = instance->currentExecutionContext()->arguments()[1].toNumber();
                if (dc < 0) dc = 0;
                deleteCnt = dc > (arrlen-start) ? arrlen-start : dc;
            }
            for (k = 0; k < deleteCnt; k++) {
                int from = start + k;
                ret->set(k, thisVal->get(from));
            }
            int argIdx = 2;
            int leftInsert = insertCnt;
            for (k = start; k < start + deleteCnt; k++) {
                if (leftInsert > 0) {
                    thisVal->set(k, instance->currentExecutionContext()->arguments()[argIdx]);
                    leftInsert--; argIdx++;
                } else {
                    thisVal->eraseValues(k, start + deleteCnt - k);
                    break;
                }
            }
            if (thisVal->isFastmode()) {
                while (leftInsert > 0) {
                    thisVal->insertValue(k, instance->currentExecutionContext()->arguments()[argIdx]);
                    leftInsert--; argIdx++; k++;
                }
            } else if (leftInsert > 0) {
                // Move leftInsert steps to right
                for (int i = arrlen - 1; i >= k; i--) {
                    thisVal->set(i + leftInsert, thisVal->get(i));
                }
                for (int i = k; i < k + leftInsert; i++, argIdx++) {
                    thisVal->set(i, instance->currentExecutionContext()->arguments()[argIdx]);
                }
            }
        }
        instance->currentExecutionContext()->doReturn(ret);

        return ESValue();
    }), false, false);
    m_arrayPrototype->set(L"splice", ESFunctionObject::create(NULL, arraySplice));

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
    FunctionDeclarationNode* constructor = new FunctionDeclarationNode(strings->String, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        if (thisObject->isESStringObject()) {
            // called as constructor
            escargot::ESStringObject* stringObject = thisObject->asESStringObject();
            ESValue value = instance->currentExecutionContext()->arguments()[0];
            stringObject->setString(value.toString());
            instance->currentExecutionContext()->doReturn(stringObject);
        } else {
            // called as function
            ESValue value = instance->currentExecutionContext()->arguments()[0];
            instance->currentExecutionContext()->doReturn(ESValue(ESString::create(value.toString()->string())));
        }
        return ESValue();
    }), false, false);


    m_string = ESFunctionObject::create(NULL, constructor);
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

    //$21.1.2.1 String.fromCharCode(...codeUnits)
    FunctionDeclarationNode* stringFromCharCode = new FunctionDeclarationNode(InternalString(L"fromCharCode"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int length = instance->currentExecutionContext()->argumentCount();
        wchar_t* elements = (wchar_t*)alloca(sizeof(wchar_t) * (length+1));
        int i;
        for (i=0; i<length; i++)
            elements[i] = instance->currentExecutionContext()->arguments()[i].toInteger();
        elements[i] = L'\0';
        instance->currentExecutionContext()->doReturn(ESString::create(InternalString(elements)));
        return ESValue();
    }), false, false);
    m_string->set(L"fromCharCode", ESFunctionObject::create(NULL, stringFromCharCode));

    //$21.1.3.1 String.prototype.charAt(pos)
    FunctionDeclarationNode* stringCharAt = new FunctionDeclarationNode(InternalString(L"charAt"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        const InternalString& str = thisObject->asESStringObject()->getStringData()->string();
        int position = instance->currentExecutionContext()->arguments()[0].toInteger();
        instance->currentExecutionContext()->doReturn(ESString::create(InternalString((*str.string())[position])));
        return ESValue();
    }), false, false);
    m_stringPrototype->set(L"charAt", ESFunctionObject::create(NULL, stringCharAt));

    //$21.1.3.2 String.prototype.charCodeAt(pos)
    FunctionDeclarationNode* stringCharCodeAt = new FunctionDeclarationNode(InternalString(L"charCodeAt"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        const InternalString& str = thisObject->asESStringObject()->getStringData()->string();
        int position = instance->currentExecutionContext()->arguments()[0].toInteger();
        instance->currentExecutionContext()->doReturn(ESValue((*str.string())[position]));
        return ESValue();
    }), false, false);
    m_stringPrototype->set(L"charCodeAt", ESFunctionObject::create(NULL, stringCharCodeAt));

    //$21.1.3.4 String.prototype.concat(...args)
    FunctionDeclarationNode* stringConcat = new FunctionDeclarationNode(InternalString(L"concat"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        const InternalString& str = thisObject->asESStringObject()->getStringData()->string();
        InternalString ret;
        ret.append(str);
        int argCount = instance->currentExecutionContext()->argumentCount();
        for (int i=0; i<argCount; i++) {
            ret.append(instance->currentExecutionContext()->arguments()[i].toString()->string());
        }
        instance->currentExecutionContext()->doReturn(ESString::create(ret));
        return ESValue();
    }), false, false);
    m_stringPrototype->set(L"concat", ESFunctionObject::create(NULL, stringConcat));

    //$21.1.3.8 String.prototype.indexOf(searchString[, position])
    FunctionDeclarationNode* stringIndexOf = new FunctionDeclarationNode(InternalString(L"indexOf"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
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

    //$21.1.3.16 String.prototype.slice(start, end)
    FunctionDeclarationNode* stringSlice = new FunctionDeclarationNode(InternalString(L"slice"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        const InternalString& str = ESValue(thisObject).toString()->string();
        int argCount = instance->currentExecutionContext()->argumentCount();
        int len = str.length();
        int intStart = instance->currentExecutionContext()->arguments()[0].toInteger();
        ESValue& end = instance->currentExecutionContext()->arguments()[1];
        int intEnd = (end.isUndefined() || argCount < 2) ? len : end.toInteger();
        int from = (intStart < 0) ? std::max(len+intStart, 0) : std::min(intStart, len);
        int to = (intEnd < 0) ? std::max(len+intEnd, 0) : std::min(intEnd, len);
        int span = std::max(to-from, 0);
        escargot::ESString* ret = ESString::create(InternalString(str.string()->substr(from, from+span-1).c_str()));
        instance->currentExecutionContext()->doReturn(ESValue(ret));
        return ESValue();
    }), false, false);
    m_stringPrototype->set(L"slice", ESFunctionObject::create(NULL, stringSlice));

    //$21.1.3.17 String.prototype.split(separator, limit)
    FunctionDeclarationNode* stringSplit = new FunctionDeclarationNode(InternalString(L"split"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        // 1, 2
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();

        // 3
        int argCount = instance->currentExecutionContext()->argumentCount();
        ESValue separator = argCount>0 ? instance->currentExecutionContext()->arguments()[0] : ESValue();
        /*
        if(!separator.isUndefinedOrNull()) {
            ESValue splitter = separator.toObject()
            RELEASE_ASSERT_NOT_REACHED(); // TODO
        }
        */

        // 4, 5
        escargot::ESString* str = ESValue(thisObject).toString();

        // 6
        escargot::ESArrayObject* arr = ESArrayObject::create(0, instance->globalObject()->arrayPrototype());

        // 7
        int lengthA = 0;

        // 8, 9
        double lim = argCount>1 ? instance->currentExecutionContext()->arguments()[1].toLength() : std::pow(2, 53)-1;

        // 10
        int s = str->length();

        // 11
        int p = 0;

        // 12, 13
        const InternalString& R = separator.toString()->string();

        // 14
        if(lim == 0)
            return arr;

        // 15
        if(separator.isUndefined()) {
            arr->set(0, str);
            instance->currentExecutionContext()->doReturn(ESValue(arr));
        }

        // 16
        if(s == 0)
            RELEASE_ASSERT_NOT_REACHED(); // TODO

        // 17
        int q = p;

        // 18
        auto splitMatch = [] (const InternalString& S, int q, const InternalString& R) -> ESValue {
            int s = S.length();
            int r = R.length();
            if (q+r > s)
                return ESValue(false);
            for (int i=0; i<r; i++)
                if (S.data()[q+i] != R.data()[i])
                    return ESValue(false);
            return ESValue(q+r);
        };
        while(q != s) {
            ESValue e = splitMatch(str->string(), q, R);
            if(e == ESValue(ESValue::ESFalseTag::ESFalse))
                q++;
            else {
                if(e.asInt32() == p)
                    q++;
                else {
                    escargot::ESString* T = str->substring(p, q);
                    arr->set(lengthA, ESValue(T));
                    lengthA++;
                    if ((double)lengthA == lim)
                        instance->currentExecutionContext()->doReturn(ESValue(arr));
                    p = e.asInt32();
                    q = p;
                }
            }
        }

        // 19
        escargot::ESString* T = str->substring(p, s);

        // 20
        arr->set(lengthA, ESValue(T));

        // 21, 22
        instance->currentExecutionContext()->doReturn(ESValue(arr));
        return ESValue();
    }), false, false);
    m_stringPrototype->set(L"split", ESFunctionObject::create(NULL, stringSplit));

    //$21.1.3.19 String.prototype.substring(start, end)
    FunctionDeclarationNode* stringSubstring = new FunctionDeclarationNode(InternalString(L"substring"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        //if (thisObject->isESUndefined() || thisObject->isESNull())
        //    throw TypeError();

        escargot::ESString* str = thisObject->asESStringObject()->getStringData();
        int argCount = instance->currentExecutionContext()->argumentCount();
        int len = str->length();
        int intStart = instance->currentExecutionContext()->arguments()[0].toInteger();
        ESValue& end = instance->currentExecutionContext()->arguments()[1];
        int intEnd = (end.isUndefined() || argCount < 2) ? len : end.toInteger();
        int finalStart = std::min(std::max(intStart, 0), len);
        int finalEnd = std::min(std::max(intEnd, 0), len);
        int from = std::min(finalStart, finalEnd);
        int to = std::max(finalStart, finalEnd);
        instance->currentExecutionContext()->doReturn(str->substring(from, to));
        return ESValue();
    }), false, false);
    m_stringPrototype->set(L"substring", ESFunctionObject::create(NULL, stringSubstring));
}

void GlobalObject::installDate()
{
    m_datePrototype = ESDateObject::create();

    //$20.3.2 The Date Constructor
    FunctionDeclarationNode* constructor = new FunctionDeclarationNode(strings->Date, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* proto = instance->globalObject()->datePrototype();
        if (instance->currentExecutionContext()->isNewExpression()) {
            escargot::ESDateObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESDateObject();

            size_t arg_size = instance->currentExecutionContext()->argumentCount();
            if (arg_size == 0) {
                thisObject->setTimeValue(ESValue());
            } else {
                ESValue arg = instance->currentExecutionContext()->arguments()[0];
                thisObject->setTimeValue(arg);
             }
         }
        instance->currentExecutionContext()->doReturn(ESString::create(L"FixMe: We have to return string with date and time data"));
        return ESValue();
    }), false, false);

      // Initialization for reference error
    m_date = ::escargot::ESFunctionObject::create(NULL, constructor);
    m_date->set(strings->name, ESString::create(strings->Date));
    m_date->setConstructor(m_function);

    m_datePrototype->setConstructor(m_date);

    m_date->set(strings->prototype, m_datePrototype);

    set(strings->Date, m_date);

    //$20.3.4.2 Date.prototype.getDate()
    FunctionDeclarationNode* getDateNode = new FunctionDeclarationNode(strings->getDate, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getDate();
       instance->currentExecutionContext()->doReturn(ESValue(ret));
       return ESValue();
    }), false, false);
    m_datePrototype->set(strings->getDate, ::escargot::ESFunctionObject::create(NULL, getDateNode));

    //$20.3.4.3 Date.prototype.getDay()
    FunctionDeclarationNode* getDayNode = new FunctionDeclarationNode(strings->getDay, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getDay();
       instance->currentExecutionContext()->doReturn(ESValue(ret));
       return ESValue();
    }), false, false);
    m_datePrototype->set(strings->getDay, ::escargot::ESFunctionObject::create(NULL, getDayNode));

    //$20.3.4.4 Date.prototype.getFullYear()
    FunctionDeclarationNode* getFullYearNode = new FunctionDeclarationNode(strings->getFullYear, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getFullYear();
       instance->currentExecutionContext()->doReturn(ESValue(ret));
       return ESValue();
    }), false, false);
    m_datePrototype->set(strings->getFullYear, ::escargot::ESFunctionObject::create(NULL, getFullYearNode));


    //$20.3.4.5 Date.prototype.getHours()
    FunctionDeclarationNode* getHoursNode = new FunctionDeclarationNode(strings->getHours, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getHours();
       instance->currentExecutionContext()->doReturn(ESValue(ret));
       return ESValue();
    }), false, false);
    m_datePrototype->set(strings->getHours, ::escargot::ESFunctionObject::create(NULL, getHoursNode));


    //$20.3.4.7 Date.prototype.getMinutes()
    FunctionDeclarationNode* getMinutesNode = new FunctionDeclarationNode(strings->getMinutes, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getMinutes();
       instance->currentExecutionContext()->doReturn(ESValue(ret));
       return ESValue();
    }), false, false);
    m_datePrototype->set(strings->getMinutes, ::escargot::ESFunctionObject::create(NULL, getMinutesNode));


    //$20.3.4.8 Date.prototype.getMonth()
    FunctionDeclarationNode* getMonthNode = new FunctionDeclarationNode(strings->getMonth, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getMonth();
       instance->currentExecutionContext()->doReturn(ESValue(ret));
       return ESValue();
    }), false, false);
    m_datePrototype->set(strings->getMonth, ::escargot::ESFunctionObject::create(NULL, getMonthNode));


    //$20.3.4.9 Date.prototype.getSeconds()
    FunctionDeclarationNode* getSecondsNode = new FunctionDeclarationNode(strings->getSeconds, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getSeconds();
       instance->currentExecutionContext()->doReturn(ESValue(ret));
       return ESValue();
    }), false, false);
    m_datePrototype->set(strings->getSeconds, ::escargot::ESFunctionObject::create(NULL, getSecondsNode));

    //$20.3.4.10 Date.prototype.getTime()
    FunctionDeclarationNode* getTimeNode = new FunctionDeclarationNode(strings->getTime, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        double ret = thisObject->asESDateObject()->getTimeAsMilisec();
        instance->currentExecutionContext()->doReturn(ESValue(ret));
        return ESValue();
    }), false, false);
    m_datePrototype->set(strings->getTime, ::escargot::ESFunctionObject::create(NULL, getTimeNode));

    //$20.3.4.11 Date.prototype.getTimezoneOffset()
    FunctionDeclarationNode* getTimezoneOffsetNode = new FunctionDeclarationNode(strings->getTimezoneOffset, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getTimezoneOffset();
       instance->currentExecutionContext()->doReturn(ESValue(ret));
       return ESValue();
    }), false, false);
    m_datePrototype->set(strings->getTimezoneOffset, ::escargot::ESFunctionObject::create(NULL, getTimezoneOffsetNode));

    //$20.3.4.27 Date.prototype.setTime()
    FunctionDeclarationNode* setTimeNode = new FunctionDeclarationNode(strings->setTime, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();

       size_t arg_size = instance->currentExecutionContext()->argumentCount();
       if (arg_size > 0 && instance->currentExecutionContext()->arguments()[0].isNumber()) {
           ESValue arg = instance->currentExecutionContext()->arguments()[0];
           thisObject->asESDateObject()->setTime(arg.toNumber());
           instance->currentExecutionContext()->doReturn(ESValue());
       } else {
           double value = std::numeric_limits<double>::quiet_NaN();
           instance->currentExecutionContext()->doReturn(ESValue(value));
        }
       return ESValue();
    }), false, false);
    m_datePrototype->set(strings->setTime, ::escargot::ESFunctionObject::create(NULL, setTimeNode));
}

void GlobalObject::installMath()
{
    // create math object
    FunctionDeclarationNode* constructor = new FunctionDeclarationNode(strings->Math, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        std::srand(std::time(0));
        return ESValue();
    }), false, false);
    m_math = ::escargot::ESFunctionObject::create(NULL, constructor);

    // create mathPrototype object
    m_mathPrototype = ESObject::create();

    // initialize math object
    m_math->set(strings->name, ESString::create(strings->Math));
    m_math->setConstructor(m_function);
    m_math->set(strings->prototype, m_mathPrototype);

    // initialize math object: $20.2.1.6 Math.PI
    m_math->set(strings->PI, ESValue(3.1415926535897932));

    // initialize math object: $20.2.2.1 Math.abs()
    FunctionDeclarationNode* absNode = new FunctionDeclarationNode(strings->abs, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            instance->currentExecutionContext()->doReturn(ESValue(value));
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            double value = abs(arg.toNumber());
            if (value == (int) value) {
                instance->currentExecutionContext()->doReturn(ESValue((int) value));
            } else {
                instance->currentExecutionContext()->doReturn(ESValue(value));
              }
         }
        return ESValue();
    }), false, false);
    m_math->set(strings->abs, ::escargot::ESFunctionObject::create(NULL, absNode));

    // initialize math object: $20.2.2.12 Math.cos()
    FunctionDeclarationNode* cosNode = new FunctionDeclarationNode(strings->cos, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            instance->currentExecutionContext()->doReturn(ESValue(value));
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            double value = cos(arg.toNumber());
            if (value == (int) value) {
                instance->currentExecutionContext()->doReturn(ESValue((int) value));
            } else {
                instance->currentExecutionContext()->doReturn(ESValue(value));
              }
         }
        return ESValue();
    }), false, false);
    m_math->set(strings->cos, ::escargot::ESFunctionObject::create(NULL, cosNode));

    // initialize math object: $20.2.2.16 Math.floor()
    FunctionDeclarationNode* floorNode = new FunctionDeclarationNode(strings->floor, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            instance->currentExecutionContext()->doReturn(ESValue(value));
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            if (arg.isInt32()) {
                instance->currentExecutionContext()->doReturn(arg);
            } else if (arg.isDouble()) {
                int value = floor(arg.asDouble());
                instance->currentExecutionContext()->doReturn(ESValue(value));
             }
        }

       return ESValue();
    }), false, false);
    m_math->set(strings->floor, ::escargot::ESFunctionObject::create(NULL, floorNode));

    // initialize math object: $20.2.2.24 Math.max()
    FunctionDeclarationNode* maxNode = new FunctionDeclarationNode(strings->max, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double n_inf = -1 * std::numeric_limits<double>::infinity();
            instance->currentExecutionContext()->doReturn(ESValue(n_inf));
        } else{
            double max_value = instance->currentExecutionContext()->arguments()[0].toNumber();
            for (unsigned i = 1; i < arg_size; i++) {
                double value = instance->currentExecutionContext()->arguments()[i].toNumber();
                if (value > max_value)
                    max_value = value;
             }
           if (max_value == (int) max_value) {
               instance->currentExecutionContext()->doReturn(ESValue(max_value));
           } else {
               instance->currentExecutionContext()->doReturn(ESValue((int) max_value));
            }
         }
        return ESValue();
    }), false, false);
    m_math->set(strings->max, ::escargot::ESFunctionObject::create(NULL, maxNode));

    // initialize math object: $20.2.2.26 Math.pow()
    FunctionDeclarationNode* powNode = new FunctionDeclarationNode(strings->pow, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size < 2) {
            double value = std::numeric_limits<double>::quiet_NaN();
            instance->currentExecutionContext()->doReturn(ESValue(value));
        } else {
            ESValue arg1 = instance->currentExecutionContext()->arguments()[0];
            ESValue arg2 = instance->currentExecutionContext()->arguments()[1];
            double value = pow(arg1.toNumber(), arg2.toNumber());
            if (value == (int) value) {
                instance->currentExecutionContext()->doReturn(ESValue((int) value));
            } else {
                instance->currentExecutionContext()->doReturn(ESValue(value));
              }
         }

        return ESValue();
    }), false, false);
    m_math->set(strings->pow, ::escargot::ESFunctionObject::create(NULL, powNode));

    // initialize math object: $20.2.2.27 Math.random()
    FunctionDeclarationNode* randomNode = new FunctionDeclarationNode(strings->random, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        double rand = (double) std::rand()/RAND_MAX;
        instance->currentExecutionContext()->doReturn(ESValue(rand));
        return ESValue();
    }), false, false);
    m_math->set(strings->random, ::escargot::ESFunctionObject::create(NULL, randomNode));

    // initialize math object: $20.2.2.28 Math.round()
    FunctionDeclarationNode* roundNode = new FunctionDeclarationNode(strings->round, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            instance->currentExecutionContext()->doReturn(ESValue(value));
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            double value = round(arg.toNumber());
            if (value == (int) value) {
                instance->currentExecutionContext()->doReturn(ESValue((int) value));
            } else {
                instance->currentExecutionContext()->doReturn(ESValue(value));
              }
         }

        return ESValue();
    }), false, false);
    m_math->set(strings->round, ::escargot::ESFunctionObject::create(NULL, roundNode));

    // initialize math object: $20.2.2.30 Math.sin()
    FunctionDeclarationNode* sinNode = new FunctionDeclarationNode(strings->sin, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            instance->currentExecutionContext()->doReturn(ESValue(value));
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            double value = sin(arg.toNumber());
            if (value == (int) value) {
                instance->currentExecutionContext()->doReturn(ESValue((int) value));
            } else {
                instance->currentExecutionContext()->doReturn(ESValue(value));
              }
         }
        return ESValue();
    }), false, false);
    m_math->set(strings->sin, ::escargot::ESFunctionObject::create(NULL, sinNode));

    // initialize math object: $20.2.2.32 Math.sqrt()
    FunctionDeclarationNode* sqrtNode = new FunctionDeclarationNode(strings->sqrt, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            instance->currentExecutionContext()->doReturn(ESValue(value));
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            double value = sqrt(arg.toNumber());
            if (value == (int) value) {
                instance->currentExecutionContext()->doReturn(ESValue((int) value));
            } else {
                instance->currentExecutionContext()->doReturn(ESValue(value));
              }
         }
        return ESValue();
    }), false, false);
    m_math->set(strings->sqrt, ::escargot::ESFunctionObject::create(NULL, sqrtNode));

    // initialize mathPrototype object
    m_mathPrototype->setConstructor(m_math);

    // add math to global object
    set(strings->Math, m_math);
}

void GlobalObject::installNumber()
{
    // create number object: $20.1.1 The Number Constructor
    FunctionDeclarationNode* constructor = new FunctionDeclarationNode(strings->Number, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {

        return ESValue();
    }), false, false);
    m_number = ::escargot::ESFunctionObject::create(NULL, constructor);

    // create numberPrototype object
    m_numberPrototype = ESNumberObject::create(ESValue(0.0));

    // initialize number object
    m_number->set(strings->name, ESString::create(strings->Number));
    m_number->setConstructor(m_function);
    m_number->set(strings->prototype, m_numberPrototype);

    // initialize numberPrototype object
    m_numberPrototype->setConstructor(m_number);


    // initialize numberPrototype object: $20.1.3.6 Number.prototype.toString()
    FunctionDeclarationNode* toStringNode = new FunctionDeclarationNode(strings->toString, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        escargot::ESNumberObject* thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESNumberObject();
        instance->currentExecutionContext()->doReturn(ESString::create(thisVal->numberData().toInternalString()));
        return ESValue();
    }), false, false);
    m_numberPrototype->set(strings->toString, ::escargot::ESFunctionObject::create(NULL, toStringNode));

    // add number to global object
    set(strings->Number, m_number);
}

}
