#include "Escargot.h"
#include "GlobalObject.h"
#include "ast/AST.h"
#include "parser/ESScriptParser.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

#include "Yarr.h"

namespace escargot {

GlobalObject::GlobalObject()
{

}

void GlobalObject::initGlobalObject()
{
    installFunction();
    installObject();
    installArray();
    installString();
    installError();
    installDate();
    installMath();
    installNumber();
    installBoolean();
    installRegExp();

    m_functionPrototype->set__proto__(m_objectPrototype);

    // Value Properties of the Global Object
    definePropertyOrThrow(ESString::create(u"Infinity"), false, false, false);
    definePropertyOrThrow(ESString::create(u"NaN"), false, false, false);
    definePropertyOrThrow(strings->undefined, false, false, false);
    set(ESString::create(u"Infinity"), ESValue(std::numeric_limits<double>::infinity()));
    set(ESString::create(u"NaN"), ESValue(std::numeric_limits<double>::quiet_NaN()));
    set(strings->undefined, ESValue());

    FunctionDeclarationNode* node = new FunctionDeclarationNode(InternalAtomicString(u"dbgBreak"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        printf("dbgBreak\n");
        return ESValue();
    }), false, false);
    auto brkFunction = ESFunctionObject::create(NULL, node);
    set(ESString::create(u"dbgBreak"), brkFunction);

    node = new FunctionDeclarationNode(InternalAtomicString(u"print"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        if(instance->currentExecutionContext()->argumentCount()) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            std::string str;

            auto toString = [&str](ESValue v) {
                if(v.isInt32()) {
                    str.append(v.toString()->utf8Data());
                } else if(v.isNumber()) {
                    str.append(v.toString()->utf8Data());
                } else if(v.isUndefined()) {
                    str.append(v.toString()->utf8Data());
                } else if(v.isNull()) {
                    str.append(v.toString()->utf8Data());
                } else if(v.isBoolean()) {
                    str.append(v.toString()->utf8Data());
                } else {
                    ESPointer* o = v.asESPointer();
                    if(o->isESString()) {
                        str.append(o->asESString()->utf8Data());
                    } else if(o->isESFunctionObject()) {
                        str.append(v.toString()->utf8Data());
                    } else if(o->isESArrayObject()) {
                        str.append("[");
                        str.append(v.toString()->utf8Data());
                        str.append("]");
                    } else if(o->isESErrorObject()) {
                        str.append(v.toString()->utf8Data());
                    } else if(o->isESObject()) {
                        str.append(o->asESObject()->constructor().asESPointer()->asESObject()->get(strings->name, true).toString()->utf8Data());
                        str.append(" {");
                        bool isFirst = true;
                        o->asESObject()->enumeration([&str, &isFirst, o](escargot::ESString* key, ::escargot::ESSlot* slot) {
                            if(!isFirst)
                                str.append(", ");
                                str.append(key->utf8Data());
                                str.append(": ");
                                str.append(slot->value(o->asESObject()).toString()->utf8Data());
                                isFirst = false;
                            });
                        if(o->isESStringObject()) {
                            str.append(", [[PrimitiveValue]]: \"");
                            str.append(o->asESStringObject()->getStringData()->utf8Data());
                            str.append("\"");
                        }
                        str.append("}");
                    } else {
                        RELEASE_ASSERT_NOT_REACHED();
                    }
                }
            };
            toString(val);

            printf("%s\n", str.data());
            fflush(stdout);
        }
        return ESValue();
    }), false, false);
    auto printFunction = ESFunctionObject::create(NULL, node);
    set(ESString::create(u"print"), printFunction);

    node = new FunctionDeclarationNode(InternalAtomicString(u"gc"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        GC_gcollect();
        return ESValue();
    }), false, false);
    auto gcFunction = ESFunctionObject::create(NULL, node);
    set(ESString::create(u"gc"), gcFunction);

    node = new FunctionDeclarationNode(InternalAtomicString(u"load"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        if(instance->currentExecutionContext()->argumentCount()) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            escargot::ESString* str = val.toString();
            FILE *fp = fopen(str->utf8Data(),"r");
            if(fp) {
                fseek(fp, 0L, SEEK_END);
                size_t sz = ftell(fp);
                fseek(fp, 0L, SEEK_SET);
                std::string str;
                str.reserve(sz+2);
                static char buf[4096];
                while(fgets(buf, sizeof buf, fp) != NULL) {
                    str += buf;
                }
                fclose(fp);
                instance->runOnGlobalContext([instance, &str](){
                    escargot::ESStringData source(str.c_str());
                    instance->evaluate(source);
                });
            }
        }
        return ESValue();
    }), false, false);
    auto loadFunction = ESFunctionObject::create(NULL, node);
    set(ESString::create(u"load"), loadFunction);
    set(ESString::create(u"run"), loadFunction);

    // Function Properties of the Global Object
    definePropertyOrThrow(ESString::create(u"eval"), false, false, false);
    node = new FunctionDeclarationNode(InternalAtomicString(u"eval"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESValue argument = instance->currentExecutionContext()->arguments()[0];
        if(!argument.isESString())
            instance->currentExecutionContext()->doReturn(argument);
        bool isDirectCall = true; // TODO
        ESValue ret = instance->runOnEvalContext([instance, &argument](){
            ESValue ret = instance->evaluate(const_cast<u16string &>(argument.asESString()->string()));
            return ret;
        }, isDirectCall);
        instance->currentExecutionContext()->doReturn(ret);
        return ESValue();
    }), false, false);
    set(ESString::create(u"eval"), ESFunctionObject::create(NULL, node));

    // $18.2.2
    definePropertyOrThrow(ESString::create(u"isFinite"), false, false, false);
    node = new FunctionDeclarationNode(InternalAtomicString(u"isFinite"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESValue ret;
        int len = instance->currentExecutionContext()->argumentCount();
        if (len < 1) ret = ESValue(ESValue::ESFalseTag::ESFalse);
        else {
            ESValue& argument = instance->currentExecutionContext()->arguments()[0];
            double num = argument.toNumber();
            if(std::isnan(num) || num == std::numeric_limits<double>::infinity() || num == -std::numeric_limits<double>::infinity())
                ret = ESValue(ESValue::ESFalseTag::ESFalse);
            else
                ret = ESValue(ESValue::ESTrueTag::ESTrue);
        }
        instance->currentExecutionContext()->doReturn(ret);
        return ESValue();
    }), false, false);
    set(ESString::create(u"isFinite"), ESFunctionObject::create(NULL, node));
    // $18.2.3
    definePropertyOrThrow(ESString::create(u"isNaN"), false, false, false);
    node = new FunctionDeclarationNode(InternalAtomicString(u"isNaN"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESValue ret;
        int len = instance->currentExecutionContext()->argumentCount();
        if (len < 1) ret = ESValue(ESValue::ESFalseTag::ESFalse);
        else {
            ESValue& argument = instance->currentExecutionContext()->arguments()[0];
            double num = argument.toNumber();
            if(std::isnan(num)) ret = ESValue(ESValue::ESTrueTag::ESTrue);
            else    ret = ESValue(ESValue::ESFalseTag::ESFalse);
        }
        instance->currentExecutionContext()->doReturn(ret);
        return ESValue();
    }), false, false);
    set(ESString::create(u"isNaN"), ESFunctionObject::create(NULL, node));
    // $18.2.5 parseInt(string, radix)
    definePropertyOrThrow(ESString::create(u"parseInt"), false, false, false);
    node = new FunctionDeclarationNode(InternalAtomicString(u"parseInt"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESValue ret;
        int len = instance->currentExecutionContext()->argumentCount();
        if (len < 1) {
            ret = ESValue(std::numeric_limits<double>::quiet_NaN());
        }
        else {
            int radix = 10;
            if (len >= 2) radix = instance->currentExecutionContext()->arguments()[1].toInt32();
            if (radix == 0) radix = 10;
            if (radix < 2 || radix > 36) {
                ret = ESValue(std::numeric_limits<double>::quiet_NaN());
                instance->currentExecutionContext()->doReturn(ret);
            }
            else {
                ESValue &input = instance->currentExecutionContext()->arguments()[0];
                if (radix == 10 && input.isNumber()) {
                    ret = ESValue(input.toInt32());
                    instance->currentExecutionContext()->doReturn(ret);
                }
                if (radix == 16) {
                    //TODO : stripPrefix = true
                }
                //TODO
            }
        }
        instance->currentExecutionContext()->doReturn(ret);
        return ESValue();
    }), false, false);
    set(ESString::create(u"parseInt"), ESFunctionObject::create(NULL, node));
}


void GlobalObject::installFunction()
{
    m_function = ESFunctionObject::create(NULL, new FunctionDeclarationNode(strings->Function, InternalAtomicStringVector(), new EmptyStatementNode(), false, false));
    m_function->set(strings->constructor, m_function);
    m_function->set(strings->name, strings->Function);
    m_function->setConstructor(m_function);
    ::escargot::ESFunctionObject* emptyFunction = ESFunctionObject::create(NULL,new FunctionDeclarationNode(strings->Empty, InternalAtomicStringVector(), new EmptyStatementNode(), false, false));

    m_functionPrototype = emptyFunction;
    ESVMInstance::currentInstance()->setGlobalFunctionPrototype(m_functionPrototype);
    m_functionPrototype->setConstructor(m_function);
    m_functionPrototype->set(strings->toString, ESFunctionObject::create(NULL, new FunctionDeclarationNode(strings->toString, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        //FIXME
        u16string ret;
        ret = u"function ";
        escargot::ESFunctionObject* fn = instance->currentExecutionContext()->resolveThisBinding()->asESFunctionObject();
        ret.append(fn->functionAST()->id().data());
        ret.append(u"() {}");
        instance->currentExecutionContext()->doReturn(ESString::create(std::move(ret)));
        return ESValue();
    }), false, false)));

    m_function->defineAccessorProperty(strings->prototype, [](ESObject* self) -> ESValue {
        return self->asESFunctionObject()->protoType();
    },nullptr, true, false, false);
    m_function->set__proto__(emptyFunction);
    m_function->setProtoType(emptyFunction);

    //$19.2.3.1 Function.prototype.apply(thisArg, argArray)
    FunctionDeclarationNode* node = new FunctionDeclarationNode(ESString::create(u"apply"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESFunctionObject();
        int arglen = instance->currentExecutionContext()->argumentCount();
        ESValue& thisArg = instance->currentExecutionContext()->arguments()[0];
        escargot::ESArrayObject* argArray = instance->currentExecutionContext()->arguments()[1].asESPointer()->asESArrayObject();
        int arrlen = argArray->length().toInt32();
        ESValue* arguments = (ESValue*)alloca(sizeof(ESValue) * arrlen);
        for (int i = 0; i < arrlen; i++) {
            arguments[i] = argArray->get(i);
        }
        ESValue ret = ESFunctionObject::call(thisVal, thisArg, arguments, arrlen, instance, false);
        instance->currentExecutionContext()->doReturn(ret);
        return ESValue();
    }), false, false);
    m_functionPrototype->set(ESString::create(u"apply"), ESFunctionObject::create(NULL, node));

    set(strings->Function, m_function);
}

void GlobalObject::installObject()
{
    ::escargot::ESFunctionObject* emptyFunction = m_functionPrototype;
    m_object = ::escargot::ESFunctionObject::create(NULL,new FunctionDeclarationNode(strings->Object, InternalAtomicStringVector(), new EmptyStatementNode(), false, false));
    m_object->set(strings->name, strings->Object);
    m_object->setConstructor(m_function);
    m_object->set__proto__(emptyFunction);

    m_objectPrototype = ESObject::create();
    m_objectPrototype->setConstructor(m_object);
    m_objectPrototype->set(strings->toString, ESFunctionObject::create(NULL, new FunctionDeclarationNode(strings->toString, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        //FIXME
        instance->currentExecutionContext()->doReturn(ESString::create(u"[Object object]"));
        return ESValue();
    }), false, false)));

    m_object->set(strings->prototype, m_objectPrototype);

    //$19.1.3.2 Object.prototype.hasOwnProperty(V)
    FunctionDeclarationNode* node = new FunctionDeclarationNode(ESString::create(u"hasOwnProperty"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESValue ret;
        int len = instance->currentExecutionContext()->argumentCount();
        if (len < 1) {
            ret = ESValue(ESValue::ESFalseTag::ESFalse);
            instance->currentExecutionContext()->doReturn(ret);
        }
        ::escargot::ESString* key = instance->currentExecutionContext()->arguments()[0].toPrimitive(ESValue::PrimitiveTypeHint::PreferString).toString();
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        escargot::ESString* keyString = key;
        if (thisVal->isESArrayObject() && thisVal->asESArrayObject()->hasOwnProperty(keyString))
            ret = ESValue(ESValue::ESTrueTag::ESTrue);
        else if (thisVal->asESObject()->hasOwnProperty(keyString))
            ret = ESValue(ESValue::ESTrueTag::ESTrue);
        else
            ret = ESValue(ESValue::ESFalseTag::ESFalse);
        instance->currentExecutionContext()->doReturn(ret);
        return ESValue();
    }), false, false);
    m_objectPrototype->set(ESString::create(u"hasOwnProperty"), ESFunctionObject::create(NULL, node));

    set(strings->Object, m_object);
}

void GlobalObject::installError()
{
    m_error = ::escargot::ESFunctionObject::create(NULL,new FunctionDeclarationNode(strings->Error, InternalAtomicStringVector(), new EmptyStatementNode(), false, false));
    m_error->set(strings->name, strings->Error);
    m_error->setConstructor(m_function);
    m_error->set__proto__(m_objectPrototype);
    m_errorPrototype = escargot::ESObject::create();
    m_errorPrototype->setConstructor(m_error);
    m_errorPrototype->set(strings->toString, ESFunctionObject::create(NULL, new FunctionDeclarationNode(strings->toString, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        //FIXME this is wrong
        ESValue v(instance->currentExecutionContext()->resolveThisBinding());
        ESPointer* o = v.asESPointer();
        u16string ret;
        ret.append(o->asESObject()->get(ESString::create(u"name"), true).toString()->data());
        ret.append(u": ");
        ret.append(o->asESObject()->get(ESString::create(u"message")).toString()->data());

        instance->currentExecutionContext()->doReturn(ESString::create(std::move(ret)));
        RELEASE_ASSERT_NOT_REACHED();
    }), false, false)));

    m_referenceError = ::escargot::ESFunctionObject::create(NULL,new FunctionDeclarationNode(strings->ReferenceError, InternalAtomicStringVector(), new EmptyStatementNode(), false, false));
    m_referenceError->set(strings->name, strings->ReferenceError);
    m_referenceError->setConstructor(m_function);
    m_referenceError->set__proto__(m_errorPrototype);

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

    //$22.1.3.1 Array.prototype.concat(...arguments)
    FunctionDeclarationNode* arrayConcat = new FunctionDeclarationNode(strings->concat, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        int arrlen = thisVal->length().asInt32();
        escargot::ESArrayObject* ret = ESArrayObject::create(arrlen, instance->globalObject()->arrayPrototype());
        if (!thisVal->constructor().isUndefinedOrNull())
            ret->setConstructor(thisVal->constructor());
        int idx = 0;
        for (idx = 0; idx < arrlen; idx++)
            ret->set(idx, thisVal->get(idx));
        for (int i = 0; i < arglen; i++) {
            ESValue& argi = instance->currentExecutionContext()->arguments()[i];
            if (argi.isESPointer() && argi.asESPointer()->isESArrayObject()) {
                escargot::ESArrayObject* arr = argi.asESPointer()->asESArrayObject();
                int len = arr->length().asInt32();
                int st = idx;
                for (; idx < st + len; idx++)
                    ret->set(idx, arr->get(idx - st));
            } else {
                ret->set(idx++, argi);
            }
        }
        instance->currentExecutionContext()->doReturn(ESValue(ret));
        return ESValue();
    }), false, false);
    m_arrayPrototype->set(strings->concat, ESFunctionObject::create(NULL, arrayConcat));

    //$22.1.3.11 Array.prototype.indexOf()
    FunctionDeclarationNode* arrayIndexOf = new FunctionDeclarationNode(strings->indexOf, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
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
    m_arrayPrototype->set(strings->indexOf, ESFunctionObject::create(NULL, arrayIndexOf));

    //$22.1.3.12 Array.prototype.join(separator)
    FunctionDeclarationNode* arrayJoin = new FunctionDeclarationNode(strings->join, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        u16string ret;
        int arrlen = thisVal->length().asInt32();
        if (arrlen >= 0) {
            escargot::ESString* separator;
            if (arglen == 0) {
                separator = ESString::create(u",");
            } else {
                separator = instance->currentExecutionContext()->arguments()[0].toString();
            }
            for (int i = 0; i < arrlen; i++) {
                ESValue elemi = thisVal->get(i);
                if (i != 0) ret.append(separator->data());
                if (!elemi.isUndefinedOrNull())
                    ret.append(elemi.toString()->data());
            }
        }
        instance->currentExecutionContext()->doReturn(ESString::create(std::move(ret)));
        return ESValue();
    }), false, false);
    m_arrayPrototype->set(strings->join, ESFunctionObject::create(NULL, arrayJoin));

    //$22.1.3.17 Array.prototype.push(item)
    FunctionDeclarationNode* arrayPush = new FunctionDeclarationNode(strings->push, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
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
    m_arrayPrototype->set(strings->push, ESFunctionObject::create(NULL, arrayPush));

    //$22.1.3.22 Array.prototype.slice(start, end)
    FunctionDeclarationNode* arraySlice = new FunctionDeclarationNode(strings->slice, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        escargot::ESArrayObject* thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        int arrlen = thisVal->length().asInt32();
        int start, end;
        if (arglen < 1) start = 0;
        else            start = instance->currentExecutionContext()->arguments()[0].toInteger();
        if (start < 0)  start = (arrlen + start > 0) ? arrlen + start : 0;
        else            start = (start < arrlen) ? start : arrlen;
        if (arglen >= 2) end = instance->currentExecutionContext()->arguments()[1].toInteger();
        else             end = arrlen;
        if (end < 0)    end = (arrlen + end > 0) ? arrlen + end : 0;
        else            end = (end < arrlen) ? end : arrlen;
        int count = (end - start > 0) ? end - start : 0;
        escargot::ESArrayObject* ret = ESArrayObject::create(count, instance->globalObject()->arrayPrototype());
        if (!thisVal->constructor().isUndefinedOrNull())
            ret->setConstructor(thisVal->constructor());
        for (int i = start; i < end; i++) {
            ret->set(i-start, thisVal->get(i));
        }
        instance->currentExecutionContext()->doReturn(ret);

        return ESValue();
    }), false, false);
    m_arrayPrototype->set(strings->slice, ESFunctionObject::create(NULL, arraySlice));

    //$22.1.3.25 Array.prototype.sort(comparefn)
    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-array.prototype.sort
    FunctionDeclarationNode* arraySort = new FunctionDeclarationNode(strings->sort, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        escargot::ESArrayObject* thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        if(arglen == 0) {
            thisVal->sort();
        } else {
            ESValue arg0 = instance->currentExecutionContext()->arguments()[0];
            thisVal->sort([&arg0, &instance, &thisVal](const ::escargot::ESValue& a, const ::escargot::ESValue& b) -> bool {
                ESValue arg[2] = { a, b };
                ESValue ret = ESFunctionObject::call(arg0, thisVal,
                        arg, 2, instance);

                double v = ret.toNumber();
                if(v == 0)
                    return false;
                else if(v < 0)
                    return true;
                else
                    return false;
            });
        }

        instance->currentExecutionContext()->doReturn(thisVal);
        return ESValue();
    }), false, false);
    m_arrayPrototype->set(strings->sort, ESFunctionObject::create(NULL, arraySort));

    //$22.1.3.25 Array.prototype.splice(start, deleteCount, ...items)
    FunctionDeclarationNode* arraySplice = new FunctionDeclarationNode(strings->splice, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
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
    m_arrayPrototype->set(strings->splice, ESFunctionObject::create(NULL, arraySplice));
    m_arrayPrototype->set(strings->toString, ESFunctionObject::create(NULL, new FunctionDeclarationNode(strings->toString, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        //FIXME this is wrong
        u16string ret;
        escargot::ESArrayObject* fn = instance->currentExecutionContext()->resolveThisBinding()->asESArrayObject();
        bool isFirst = true;
        for (int i = 0 ; i < fn->length().asInt32() ; i++) {
            if(!isFirst)
                ret.append(u",");
            ESValue slot = fn->get(i);
            ret.append(slot.toString()->data());
            isFirst = false;
        }
        instance->currentExecutionContext()->doReturn(ESString::create(std::move(ret)));
        RELEASE_ASSERT_NOT_REACHED();
    }), false, false)));

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
            instance->currentExecutionContext()->doReturn(ESValue(value.toString()));
        }
        return ESValue();
    }), false, false);


    m_string = ESFunctionObject::create(NULL, constructor);
    m_string->set(strings->name, strings->String);
    m_string->setConstructor(m_function);

    m_stringPrototype = ESStringObject::create();
    m_stringPrototype->setConstructor(m_string);
    m_stringPrototype->set(strings->toString, ESFunctionObject::create(NULL, new FunctionDeclarationNode(strings->toString, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        //FIXME this is wrong
        ESValue v(instance->currentExecutionContext()->resolveThisBinding());
        instance->currentExecutionContext()->doReturn(instance->currentExecutionContext()->resolveThisBinding()->asESStringObject()->getStringData());
        RELEASE_ASSERT_NOT_REACHED();
    }), false, false)));

    m_string->defineAccessorProperty(strings->prototype, [](ESObject* self) -> ESValue {
        return self->asESFunctionObject()->protoType();
    }, nullptr, true, false, false);
    m_string->set__proto__(m_functionPrototype); // empty Function
    m_string->setProtoType(m_stringPrototype);

    set(strings->String, m_string);

    //$21.1.2.1 String.fromCharCode(...codeUnits)
    FunctionDeclarationNode* stringFromCharCode = new FunctionDeclarationNode(ESString::create(u"fromCharCode"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int length = instance->currentExecutionContext()->argumentCount();
        if(length == 1) {
            char16_t c = (char16_t)instance->currentExecutionContext()->arguments()[0].toInteger();
            if(c < ESCARGOT_ASCII_TABLE_MAX)
                instance->currentExecutionContext()->doReturn(strings->asciiTable[c]);
            instance->currentExecutionContext()->doReturn(ESString::create(c));
        } else {
            u16string elements;
            elements.resize(length);
            char16_t* data = const_cast<char16_t *>(elements.data());
            for(int i = 0; i < length ; i ++) {
                data[i] = {(char16_t)instance->currentExecutionContext()->arguments()[i].toInteger()};
            }
            instance->currentExecutionContext()->doReturn(ESString::create(std::move(elements)));
        }
        return ESValue();
    }), false, false);
    m_string->set(ESString::create(u"fromCharCode"), ESFunctionObject::create(NULL, stringFromCharCode));

    //$21.1.3.1 String.prototype.charAt(pos)
    FunctionDeclarationNode* stringCharAt = new FunctionDeclarationNode(ESString::create(u"charAt"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        const u16string& str = thisObject->asESStringObject()->getStringData()->string();
        if(instance->currentExecutionContext()->argumentCount() > 0) {
            int position = instance->currentExecutionContext()->arguments()[0].toInteger();
            if(LIKELY(0 <= position && position < (int)str.length())) {
                char16_t c = str[position];
                if(LIKELY(c < ESCARGOT_ASCII_TABLE_MAX)) {
                    instance->currentExecutionContext()->doReturn(strings->asciiTable[c]);
                } else {
                    instance->currentExecutionContext()->doReturn(ESString::create(c));
                }
            } else {
                instance->currentExecutionContext()->doReturn(strings->emptyESString);
            }
        }
        instance->currentExecutionContext()->doReturn(strings->emptyESString);
        return ESValue();
    }), false, false);
    m_stringPrototype->set(ESString::create(u"charAt"), ESFunctionObject::create(NULL, stringCharAt));

    //$21.1.3.2 String.prototype.charCodeAt(pos)
    FunctionDeclarationNode* stringCharCodeAt = new FunctionDeclarationNode(ESString::create(u"charCodeAt"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        const u16string& str = thisObject->asESStringObject()->getStringData()->string();
        int position = instance->currentExecutionContext()->arguments()[0].toInteger();
        ESValue ret;
        if (position < 0 || position >= (int)str.length())
            ret = ESValue(std::numeric_limits<double>::quiet_NaN());
        else
            ret = ESValue(str[position]);
        instance->currentExecutionContext()->doReturn(ret);
        return ESValue();
    }), false, false);
    m_stringPrototype->set(ESString::create(u"charCodeAt"), ESFunctionObject::create(NULL, stringCharCodeAt));

    //$21.1.3.4 String.prototype.concat(...args)
    FunctionDeclarationNode* stringConcat = new FunctionDeclarationNode(strings->concat, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        const u16string& str = thisObject->asESStringObject()->getStringData()->string();
        u16string ret;
        ret.append(str);
        int argCount = instance->currentExecutionContext()->argumentCount();
        for (int i=0; i<argCount; i++) {
            ret.append(instance->currentExecutionContext()->arguments()[i].toString()->string().data());
        }
        instance->currentExecutionContext()->doReturn(ESString::create(std::move(ret)));
        return ESValue();
    }), false, false);
    m_stringPrototype->set(strings->concat, ESFunctionObject::create(NULL, stringConcat));

    //$21.1.3.8 String.prototype.indexOf(searchString[, position])
    FunctionDeclarationNode* stringIndexOf = new FunctionDeclarationNode(strings->indexOf, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        //if (thisObject->isESUndefined() || thisObject->isESNull())
        //    throw TypeError();
        const u16string& str = thisObject->asESStringObject()->getStringData()->string();
        escargot::ESString* searchStr = instance->currentExecutionContext()->arguments()[0].toString(); // TODO converesion w&w/o test

        ESValue val;
        if(instance->currentExecutionContext()->argumentCount() > 1)
            val = instance->currentExecutionContext()->arguments()[1];

        int result;
        if (val.isUndefined()) {
            result = str.find(searchStr->string());
        } else {
            double numPos = val.toNumber();
            int pos = numPos;
            int len = str.length();
            int start = std::min(std::max(pos, 0), len);
            result = str.find(searchStr->string(), start);
        }
        instance->currentExecutionContext()->doReturn(ESValue(result));
        return ESValue();
    }), false, false);
    m_stringPrototype->set(strings->indexOf, ESFunctionObject::create(NULL, stringIndexOf));

    //$21.1.3.11 String.prototype.match(regexp)
    FunctionDeclarationNode* stringMatch = new FunctionDeclarationNode(ESString::create(u"match"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        ASSERT(thisObject->isESStringObject());
        escargot::ESArrayObject* ret = ESArrayObject::create(0, instance->globalObject()->arrayPrototype());

        int argCount = instance->currentExecutionContext()->argumentCount();
        if(argCount > 0) {
            ESPointer* esptr = instance->currentExecutionContext()->arguments()[0].asESPointer();
            ESString::RegexMatchResult result;
            thisObject->asESStringObject()->getStringData()->match(esptr, result);

            const char16_t* str = thisObject->asESStringObject()->getStringData()->data();
            int idx = 0;
            for(unsigned i = 0; i < result.m_matchResults.size() ; i ++) {
                for(unsigned j = 0; j < result.m_matchResults[i].size() ; j ++) {
                    ret->set(idx++,ESString::create(std::move(u16string(str + result.m_matchResults[i][j].m_start,str + result.m_matchResults[i][j].m_end))));
                }
            }
            if (ret->length().asInt32() == 0)
                instance->currentExecutionContext()->doReturn(ESValue(ESValue::ESNull));
        }
        instance->currentExecutionContext()->doReturn(ret);
        return ESValue();
    }), false, false);
    m_stringPrototype->set(ESString::create(u"match"), ESFunctionObject::create(NULL, stringMatch));

    //$21.1.3.14 String.prototype.replace(searchValue, replaceValue)
    FunctionDeclarationNode* stringReplace = new FunctionDeclarationNode(ESString::create(u"replace"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        ASSERT(thisObject->isESStringObject());
        escargot::ESArrayObject* ret = ESArrayObject::create(0, instance->globalObject()->arrayPrototype());
        int argCount = instance->currentExecutionContext()->argumentCount();
        if(argCount > 1) {
            ESPointer* esptr = instance->currentExecutionContext()->arguments()[0].asESPointer();

            ESValue replaceValue = instance->currentExecutionContext()->arguments()[1];
            escargot::ESString* origStr = thisObject->asESStringObject()->getStringData();
            ESString::RegexMatchResult result;
            const u16string& orgString = origStr->string();
            origStr->match(esptr, result);
            if(result.m_matchResults.size()  == 0) {
                instance->currentExecutionContext()->doReturn(origStr);
            }

            if (replaceValue.isESPointer() && replaceValue.asESPointer()->isESFunctionObject()) {
                int32_t matchCount = result.m_matchResults.size();
                ESValue callee = replaceValue.asESPointer()->asESFunctionObject();

                u16string newThis;
                newThis.reserve(orgString.size());
                newThis.append(orgString.begin(), orgString.begin() + result.m_matchResults[0][0].m_start);

                for(int32_t i = 0; i < matchCount ; i ++) {
                    int subLen = result.m_matchResults[i].size();
                    ESValue* arguments = (ESValue *)alloca((subLen+2)*sizeof (ESValue));
                    for(unsigned j = 0; j < (unsigned)subLen ; j ++) {
                        arguments[j] = ESString::create(std::move(u16string(
                                origStr->data() + result.m_matchResults[i][j].m_start
                                , origStr->data() + result.m_matchResults[i][j].m_end
                                )));
                    }
                    arguments[subLen] = ESValue((int)result.m_matchResults[i][0].m_start);
                    arguments[subLen + 1] = origStr;
                    escargot::ESString* res = ESFunctionObject::call(callee, instance->globalObject(), arguments, subLen + 2, instance).toString();

                    newThis.append(res->string());
                    if(i < matchCount - 1) {
                        newThis.append(orgString.begin() + result.m_matchResults[i][0].m_end, orgString.begin() + result.m_matchResults[i + 1][0].m_start);
                    }

                }
                newThis.append(orgString.begin() + result.m_matchResults[matchCount - 1][0].m_end, orgString.end());
                escargot::ESString* resultString = ESString::create(std::move(newThis));
                instance->currentExecutionContext()->doReturn(resultString);
            } else {
                escargot::ESString* replaceString = replaceValue.toString();
                u16string newThis;
                newThis.reserve(orgString.size());
                if(replaceString->string().find('$') == u16string::npos) {
                    //flat replace
                    int32_t matchCount = result.m_matchResults.size();
                    newThis.append(orgString.begin(), orgString.begin() + result.m_matchResults[0][0].m_start);
                    for(int32_t i = 0; i < matchCount ; i ++) {
                        escargot::ESString* res = replaceString;
                        newThis.append(res->string());
                        if(i < matchCount - 1) {
                            newThis.append(orgString.begin() + result.m_matchResults[i][0].m_end, orgString.begin() + result.m_matchResults[i + 1][0].m_start);
                        }
                    }
                    newThis.append(orgString.begin() + result.m_matchResults[matchCount - 1][0].m_end, orgString.end());
                } else {
                    //dollar replace
                    int32_t matchCount = result.m_matchResults.size();

                    const u16string& dollarString = replaceString->string();
                    newThis.append(orgString.begin(), orgString.begin() + result.m_matchResults[0][0].m_start);
                    for(int32_t i = 0; i < matchCount ; i ++) {
                        for(unsigned j = 0; j < dollarString.size() ; j ++) {
                            if(dollarString[j] == '$' && (j + 1) < dollarString.size()) {
                                char16_t c = dollarString[j + 1];
                                if(c == '$') {
                                    newThis.push_back(dollarString[j]);
                                } else if(c == '&') {
                                    newThis.append(origStr->string().begin() + result.m_matchResults[i][0].m_start,
                                            origStr->string().begin() + result.m_matchResults[i][0].m_end);
                                } else if(c == '`') {
                                    //TODO
                                    RELEASE_ASSERT_NOT_REACHED();
                                } else if(c == '`') {
                                    //TODO
                                    RELEASE_ASSERT_NOT_REACHED();
                                } else if('0' <= c && c <= '9') {
                                    //TODO support morethan 2-digits
                                    size_t idx = c - '0';
                                    if(idx < result.m_matchResults[i].size()) {
                                        newThis.append(origStr->string().begin() + result.m_matchResults[i][idx].m_start,
                                            origStr->string().begin() + result.m_matchResults[i][idx].m_end);
                                    } else {
                                        newThis.push_back('$');
                                        newThis.push_back(c);
                                    }
                                }
                                j ++;
                            } else {
                                newThis.push_back(dollarString[j]);
                            }
                        }
                        if(i < matchCount - 1) {
                            newThis.append(orgString.begin() + result.m_matchResults[i][0].m_end, orgString.begin() + result.m_matchResults[i + 1][0].m_start);
                        }
                    }
                    newThis.append(orgString.begin() + result.m_matchResults[matchCount - 1][0].m_end, orgString.end());
                }
                escargot::ESString* resultString = ESString::create(std::move(newThis));
                instance->currentExecutionContext()->doReturn(resultString);
            }
        }
        return ESValue();
    }), false, false);
    m_stringPrototype->set(ESString::create(u"replace"), ESFunctionObject::create(NULL, stringReplace));

    //$21.1.3.16 String.prototype.slice(start, end)
    FunctionDeclarationNode* stringSlice = new FunctionDeclarationNode(strings->slice, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        const u16string& str = ESValue(thisObject).toString()->string();
        int argCount = instance->currentExecutionContext()->argumentCount();
        int len = str.length();
        int intStart = instance->currentExecutionContext()->arguments()[0].toInteger();
        ESValue& end = instance->currentExecutionContext()->arguments()[1];
        int intEnd = (end.isUndefined() || argCount < 2) ? len : end.toInteger();
        int from = (intStart < 0) ? std::max(len+intStart, 0) : std::min(intStart, len);
        int to = (intEnd < 0) ? std::max(len+intEnd, 0) : std::min(intEnd, len);
        int span = std::max(to-from, 0);
        escargot::ESString* ret = ESString::create(str.substr(from, from+span-1));
        instance->currentExecutionContext()->doReturn(ESValue(ret));
        return ESValue();
    }), false, false);
    m_stringPrototype->set(strings->slice, ESFunctionObject::create(NULL, stringSlice));

    //$21.1.3.17 String.prototype.split(separator, limit)
    FunctionDeclarationNode* stringSplit = new FunctionDeclarationNode(ESString::create(u"split"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
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
        const u16string& R = separator.toString()->string();

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
        auto splitMatch = [] (const u16string& S, int q, const u16string& R) -> ESValue {
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
    m_stringPrototype->set(ESString::create(u"split"), ESFunctionObject::create(NULL, stringSplit));

    //$21.1.3.19 String.prototype.substring(start, end)
    FunctionDeclarationNode* stringSubstring = new FunctionDeclarationNode(ESString::create(u"substring"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        //if (thisObject->isESUndefined() || thisObject->isESNull())
        //    throw TypeError();
        int argCount = instance->currentExecutionContext()->argumentCount();
        escargot::ESString* str = thisObject->asESStringObject()->getStringData();
        if(argCount == 0) {
            instance->currentExecutionContext()->doReturn(str);
        } else {
            int len = str->length();
            int intStart = instance->currentExecutionContext()->arguments()[0].toInteger();
            ESValue& end = instance->currentExecutionContext()->arguments()[1];
            int intEnd = (argCount < 2 || end.isUndefined()) ? len : end.toInteger();
            int finalStart = std::min(std::max(intStart, 0), len);
            int finalEnd = std::min(std::max(intEnd, 0), len);
            int from = std::min(finalStart, finalEnd);
            int to = std::max(finalStart, finalEnd);
            instance->currentExecutionContext()->doReturn(str->substring(from, to));
        }


        return ESValue();
    }), false, false);
    m_stringPrototype->set(ESString::create(u"substring"), ESFunctionObject::create(NULL, stringSubstring));

    //$21.1.3.22 String.prototype.toLowerCase()
    FunctionDeclarationNode* stringToLowerCase = new FunctionDeclarationNode(ESString::create(u"toLowerCase"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        escargot::ESString* str = thisObject->asESStringObject()->getStringData();
        int strlen = str->string().length();
        u16string newstr(str->string());
        //TODO use ICU for this operation
        std::transform(newstr.begin(), newstr.end(), newstr.begin(), ::tolower);
        instance->currentExecutionContext()->doReturn(ESString::create(std::move(newstr)));
        return ESValue();
    }), false, false);
    m_stringPrototype->set(ESString::create(u"toLowerCase"), ESFunctionObject::create(NULL, stringToLowerCase));
    //$21.1.3.24 String.prototype.toUpperCase()
    FunctionDeclarationNode* stringToUpperCase = new FunctionDeclarationNode(ESString::create(u"toUpperCase"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        escargot::ESString* str = thisObject->asESStringObject()->getStringData();
        int strlen = str->string().length();
        u16string newstr(str->string());
        //TODO use ICU for this operation
        std::transform(newstr.begin(), newstr.end(), newstr.begin(), ::toupper);
        instance->currentExecutionContext()->doReturn(ESString::create(std::move(newstr)));
        return ESValue();
    }), false, false);
    m_stringPrototype->set(ESString::create(u"toUpperCase"), ESFunctionObject::create(NULL, stringToUpperCase));

    m_stringObjectProxy = ESStringObject::create();
    m_stringObjectProxy->setConstructor(m_string);
    m_stringObjectProxy->set__proto__(m_string->protoType());
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
        instance->currentExecutionContext()->doReturn(ESString::create(u"FixMe: We have to return string with date and time data"));
        return ESValue();
    }), false, false);

      // Initialization for reference error
    m_date = ::escargot::ESFunctionObject::create(NULL, constructor);
    m_date->set(strings->name, strings->Date);
    m_date->setConstructor(m_function);

    m_datePrototype->setConstructor(m_date);
    m_datePrototype->set(strings->toString, ESFunctionObject::create(NULL, new FunctionDeclarationNode(strings->toString, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        //FIXME this is wrong
        ESValue v(instance->currentExecutionContext()->resolveThisBinding());
        instance->currentExecutionContext()->doReturn(v.toString());
        RELEASE_ASSERT_NOT_REACHED();
    }), false, false)));

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
    m_math->set(strings->name, strings->Math);
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

    // initialize math object: $20.2.2.16 Math.ceil()
    FunctionDeclarationNode* ceilNode = new FunctionDeclarationNode(strings->ceil, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            instance->currentExecutionContext()->doReturn(ESValue(value));
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            if (arg.isInt32()) {
                instance->currentExecutionContext()->doReturn(arg);
            } else if (arg.isDouble()) {
                int value = ceil(arg.asDouble());
                instance->currentExecutionContext()->doReturn(ESValue(value));
             }
        }

       return ESValue();
    }), false, false);
    m_math->set(strings->ceil, ::escargot::ESFunctionObject::create(NULL, ceilNode));

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

    // initialize math object: $20.2.2.20 Math.log()
    FunctionDeclarationNode* logNode = new FunctionDeclarationNode(strings->log, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            instance->currentExecutionContext()->doReturn(ESValue(value));
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            double value = arg.toNumber();
            double ret;
            if(isnan(value))
                ret = std::numeric_limits<double>::quiet_NaN();
            else if(value < 0)
                ret = std::numeric_limits<double>::quiet_NaN();
            else if(value == 0.0 && value == -0.0)
                ret = -std::numeric_limits<double>::infinity();
            else if(value == 1)
                ret = 0;
            else if(isinf(value))
                ret = std::numeric_limits<double>::infinity();
            else
                ret = log(value);
            instance->currentExecutionContext()->doReturn(ESValue(ret));
        }
        return ESValue();
    }), false, false);
    m_math->set(strings->log, ::escargot::ESFunctionObject::create(NULL, logNode));

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

static int itoa(int value, char *sp, int radix)
{
    char tmp[16];// be careful with the length of the buffer
    char *tp = tmp;
    int i;
    unsigned v;

    int sign = (radix == 10 && value < 0);
    if (sign)
        v = -value;
    else
        v = (unsigned)value;

    while (v || tp == tmp)
    {
        i = v % radix;
        v /= radix; // v/=radix uses less CPU clocks than v=v/radix does
        if (i < 10)
          *tp++ = i+'0';
        else
          *tp++ = i + 'a' - 10;
    }

    int len = tp - tmp;

    if (sign)
    {
        *sp++ = '-';
        len++;
    }

    while (tp > tmp) {
        *sp++ = *--tp;
    }
    *sp++ = 0;

    return len;
}

void GlobalObject::installNumber()
{
    // create number object: $20.1.1 The Number Constructor
    FunctionDeclarationNode* constructor = new FunctionDeclarationNode(strings->Number, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {

        return ESValue();
    }), false, false);
    m_number = ::escargot::ESFunctionObject::create(NULL, constructor);

    // create numberPrototype object
    m_numberPrototype = ESNumberObject::create(0.0);

    // initialize number object
    m_number->set(strings->name, strings->Number);
    m_number->setConstructor(m_function);
    m_number->set(strings->prototype, m_numberPrototype);

    // initialize numberPrototype object
    m_numberPrototype->setConstructor(m_number);

    // initialize numberPrototype object: $20.1.3.3 Number.prototype.toFixed(fractionDigits)
    FunctionDeclarationNode* toFixedNode = new FunctionDeclarationNode(strings->toFixed, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        escargot::ESNumberObject* thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESNumberObject();
        int arglen = instance->currentExecutionContext()->argumentCount();
        if (arglen == 0) {
            instance->currentExecutionContext()->doReturn(ESValue(round(thisVal->numberData())).toString());
        } else if (arglen == 1) {
             int digit = instance->currentExecutionContext()->arguments()[0].toInteger();
             int shift = pow(10, digit);
             instance->currentExecutionContext()->doReturn(ESValue(round(thisVal->numberData()*shift)/shift).toString());
         }

        return ESValue();
    }), false, false);
    m_numberPrototype->set(strings->toFixed, ::escargot::ESFunctionObject::create(NULL, toFixedNode));

    // initialize numberPrototype object: $20.1.3.6 Number.prototype.toString()
    FunctionDeclarationNode* toStringNode = new FunctionDeclarationNode(strings->toString, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        escargot::ESNumberObject* thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESNumberObject();
        int arglen = instance->currentExecutionContext()->argumentCount();
        double radix = 10;
        if (arglen >= 1) {
            radix = instance->currentExecutionContext()->arguments()[0].toInteger();
            if (radix < 2 || radix > 36)
                throw RangeError(ESString::create(u"String.prototype.toString() radix is not in valid range"));
        }
        if (radix == 10)
            instance->currentExecutionContext()->doReturn(ESValue(thisVal->numberData()).toString());
        else {
            char buffer[256];
            int len = itoa((int)thisVal->numberData(), buffer, radix);
            instance->currentExecutionContext()->doReturn(ESString::create(buffer));
        }
        return ESValue();
    }), false, false);
    m_numberPrototype->set(strings->toString, ::escargot::ESFunctionObject::create(NULL, toStringNode));

    // add number to global object
    set(strings->Number, m_number);

    m_numberObjectProxy = ESNumberObject::create(0);
    m_numberObjectProxy->setConstructor(m_number);
    m_numberObjectProxy->set__proto__(m_numberPrototype);
}

void GlobalObject::installBoolean()
{
    // create number object: $19.3.1 The Boolean Constructor
    FunctionDeclarationNode* constructor = new FunctionDeclarationNode(strings->Boolean, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        bool val = false;
        if (arglen >= 1) {
            val = instance->currentExecutionContext()->arguments()[0].toBoolean();
        }
        ESValue ret;
        if (val)
            ret = ESValue(ESValue::ESTrueTag::ESTrue);
        else
            ret = ESValue(ESValue::ESFalseTag::ESFalse);

        if(instance->currentExecutionContext()->isNewExpression() && instance->currentExecutionContext()->resolveThisBinding()->isESBooleanObject()) {
            ::escargot::ESBooleanObject* o = instance->currentExecutionContext()->resolveThisBinding()->asESBooleanObject();
            o->setBooleanData(ret);
            instance->currentExecutionContext()->doReturn(o);
        } else // If NewTarget is undefined, return b
            instance->currentExecutionContext()->doReturn(ret);
        return ESValue();
    }), false, false);
    m_boolean = ::escargot::ESFunctionObject::create(NULL, constructor);

    // create booleanPrototype object
    m_booleanPrototype = ESBooleanObject::create(ESValue(ESValue::ESFalseTag::ESFalse));

    // initialize boolean object
    m_boolean->set(strings->name, strings->Boolean);
    m_boolean->setConstructor(m_function);
    m_boolean->set(strings->prototype, m_booleanPrototype);

    // initialize booleanPrototype object
    m_booleanPrototype->setConstructor(m_boolean);
    m_booleanPrototype->set__proto__(m_objectPrototype);

    // initialize booleanPrototype object: $19.3.3.2 Boolean.prototype.toString()
    FunctionDeclarationNode* toStringNode = new FunctionDeclarationNode(strings->toString, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        escargot::ESBooleanObject* thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESBooleanObject();
        instance->currentExecutionContext()->doReturn(thisVal->booleanData().toString());
        return ESValue();
    }), false, false);
    m_booleanPrototype->set(strings->toString, ::escargot::ESFunctionObject::create(NULL, toStringNode));

    // initialize booleanPrototype object: $19.3.3.3 Boolean.prototype.valueOf()
    FunctionDeclarationNode* valueOfNode = new FunctionDeclarationNode(strings->valueOf, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        escargot::ESBooleanObject* thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESBooleanObject();
        instance->currentExecutionContext()->doReturn(thisVal->booleanData());
        return ESValue();
    }), false, false);
    m_booleanPrototype->set(strings->valueOf, ::escargot::ESFunctionObject::create(NULL, valueOfNode));

    // add number to global object
    set(strings->Boolean, m_boolean);
}

void GlobalObject::installRegExp()
{
    // create regexp object: $21.2.3 The RegExp Constructor
    FunctionDeclarationNode* constructor = new FunctionDeclarationNode(strings->RegExp, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        escargot::ESRegExpObject* thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESRegExpObject();
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size > 0) {
            thisVal->setSource(instance->currentExecutionContext()->arguments()[0].toString());
        }

        if (arg_size > 1) {
            escargot::ESString* is = instance->currentExecutionContext()->arguments()[1].toString();
            const u16string& str = static_cast<const u16string&>(is->string());
            ESRegExpObject::Option option = ESRegExpObject::Option::None;
            if(str.find('g') != u16string::npos) {
                option = (ESRegExpObject::Option) (option | ESRegExpObject::Option::Global);
            }
            if(str.find('i') != u16string::npos) {
                option = (ESRegExpObject::Option) (option | ESRegExpObject::Option::IgnoreCase);
            }
            if(str.find('m') != u16string::npos) {
                option = (ESRegExpObject::Option) (option | ESRegExpObject::Option::MultiLine);
            }
            if(str.find('y') != u16string::npos) {
                option = (ESRegExpObject::Option) (option | ESRegExpObject::Option::Sticky);
            }
            thisVal->setOption(option);
        }
        return ESValue();
    }), false, false);
    m_regexp = ::escargot::ESFunctionObject::create(NULL, constructor);

    // create regexpPrototype object
    m_regexpPrototype = ESRegExpObject::create(strings->emptyESString,ESRegExpObject::Option::None, m_objectPrototype);

    // initialize regexp object
    m_regexp->set(strings->name, strings->RegExp);
    m_regexp->setConstructor(m_function);
    m_regexp->set(strings->prototype, m_regexpPrototype);

    // initialize regexpPrototype object
    m_regexpPrototype->setConstructor(m_regexp);

    m_regexpPrototype->defineAccessorProperty(strings->source, [](ESObject* self) -> ESValue {
        return self->asESRegExpObject()->source();
    }, nullptr, true, false, false);


    // 21.2.5.13 RegExp.prototype.test( S )
    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-regexp.prototype.test
    FunctionDeclarationNode* testNode = new FunctionDeclarationNode(strings->test, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        ::escargot::ESRegExpObject* regexp = thisObject->asESRegExpObject();
        int argCount = instance->currentExecutionContext()->argumentCount();
        if(argCount > 0) {
            escargot::ESString* sourceStr = instance->currentExecutionContext()->arguments()[0].toString();
            ESString::RegexMatchResult result;
            bool testResult = sourceStr->match(thisObject, result, true);
            instance->currentExecutionContext()->doReturn(ESValue(testResult));
            if(result.m_matchResults.size()) {
                instance->currentExecutionContext()->doReturn(ESValue(true));
            } else {
                instance->currentExecutionContext()->doReturn(ESValue(false));
            }
        }
        return ESValue(false);
    }), false, false);
    m_regexpPrototype->set(strings->test, ::escargot::ESFunctionObject::create(NULL, testNode));

    // add regexp to global object
    set(strings->RegExp, m_regexp);
}

}
