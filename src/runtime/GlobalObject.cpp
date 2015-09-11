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
    convertIntoMapMode();

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
    definePropertyOrThrow(strings->Infinity, false, false, false);
    definePropertyOrThrow(strings->NaN, false, false, false);
    definePropertyOrThrow(strings->undefined, false, false, false);
    set(strings->Infinity, ESValue(std::numeric_limits<double>::infinity()));
    set(strings->NaN, ESValue(std::numeric_limits<double>::quiet_NaN()));
    set(strings->undefined, ESValue());

    FunctionDeclarationNode* node = new FunctionDeclarationNode(InternalAtomicString(u"dbgBreak"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        printf("dbgBreak\n");
        return ESValue();
    }), false, false);
    auto brkFunction = ESFunctionObject::create(NULL, node);
    set(ESString::create(u"dbgBreak"), brkFunction);

    node = new FunctionDeclarationNode(InternalAtomicString(u"print"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        if(instance->currentExecutionContext()->argumentCount()) {
            ESVMInstance::printValue(instance->currentExecutionContext()->arguments()[0]);
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
        if(!argument.isESString()) {
            return argument;
        }
        bool isDirectCall = true; // TODO
        ESValue ret = instance->runOnEvalContext([instance, &argument](){
            ESValue ret = instance->evaluate(const_cast<u16string &>(argument.asESString()->string()));
            return ret;
        }, isDirectCall);
        return ret;
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
        return ret;
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
        return ret;
    }), false, false);
    set(ESString::create(u"isNaN"), ESFunctionObject::create(NULL, node));

    // $18.2.4 parseFloat(string)
    definePropertyOrThrow(ESString::create(u"parseFloat"), false, false, false);
    node = new FunctionDeclarationNode(InternalAtomicString(u"parseFloat"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int len = instance->currentExecutionContext()->argumentCount();
        if (len < 1) {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
         } else {
            ESValue input = instance->currentExecutionContext()->arguments()[0];
            escargot::ESString* str = input.toString();
            double f = atof(str->utf8Data());
            return ESValue(f);
         }
    }), false, false);
    set(ESString::create(u"parseFloat"), ESFunctionObject::create(NULL, node));

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
                return ret;
            }
            else {
                ESValue input = instance->currentExecutionContext()->arguments()[0];
                escargot::ESString* str = input.toString();
                //TODO remove leading space
                //TODO Let sign be 1.
                //TODO If S is not empty and the first code unit of S is 0x002D (HYPHEN-MINUS), let sign be −1.
                //TODO If S is not empty and the first code unit of S is 0x002B (PLUS SIGN) or 0x002D (HYPHEN-MINUS), remove the first code unit from S.
                bool stripPrefix = true;
                if (radix == 10) {
                } else if (radix == 16) {
                    stripPrefix = true;
                } else {
                    stripPrefix = false;
                }

                //TODO stripPrefix
                long int ll;
                str->wcharData([&ll, &radix](wchar_t* data, size_t len){
                    ll = wcstol(data, NULL, radix);
                });
                return ESValue((double)ll);
            }
        }
        return ret;
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
        if(instance->currentExecutionContext()->resolveThisBinding()->isESFunctionObject()) {
            u16string ret;
            ret = u"function ";
            escargot::ESFunctionObject* fn = instance->currentExecutionContext()->resolveThisBinding()->asESFunctionObject();
            ret.append(fn->functionAST()->id().data());
            ret.append(u"() {}");
            return ESString::create(std::move(ret));
        }
        u16string ret;
        return ESString::create(std::move(ret));
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
        int arrlen = 0;
        ESValue* arguments = NULL;
        if(instance->currentExecutionContext()->argumentCount() > 1) {
            if(instance->currentExecutionContext()->arguments()[1].isESPointer()) {
                if(instance->currentExecutionContext()->arguments()[1].asESPointer()->isESArrayObject()) {
                    escargot::ESArrayObject* argArray = instance->currentExecutionContext()->arguments()[1].asESPointer()->asESArrayObject();
                    arrlen = argArray->length();
                    arguments = (ESValue*)alloca(sizeof(ESValue) * arrlen);
                    for (int i = 0; i < arrlen; i++) {
                        arguments[i] = argArray->get(i);
                    }
                }
                else if(instance->currentExecutionContext()->arguments()[1].asESPointer()->isESObject()) {
                    escargot::ESObject* obj = instance->currentExecutionContext()->arguments()[1].asESPointer()->asESObject();
                    arrlen = obj->get(strings->length).toInteger();
                    arguments = (ESValue*)alloca(sizeof(ESValue) * arrlen);
                    for (int i = 0; i < arrlen; i++) {
                        arguments[i] = obj->get(ESValue(i));
                    }
                }
            }

        }
        ESValue ret = ESFunctionObject::call(instance, thisVal, thisArg, arguments, arrlen, false);
        return ret;
    }), false, false);
    m_functionPrototype->set(ESString::create(u"apply"), ESFunctionObject::create(NULL, node));

    //19.2.3.3 Function.prototype.call (thisArg , ...args)
    node = new FunctionDeclarationNode(ESString::create(u"call"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESFunctionObject();
        int arglen = instance->currentExecutionContext()->argumentCount()-1;
        ESValue& thisArg = instance->currentExecutionContext()->arguments()[0];
        ESValue* arguments = (ESValue*)alloca(sizeof(ESValue) * arglen);
        for (int i = 0; i < arglen; i++) {
            arguments[i] = instance->currentExecutionContext()->arguments()[i + 1];
        }
        return ESFunctionObject::call(instance, thisVal, thisArg, arguments, arglen, false);
    }), false, false);
    m_functionPrototype->set(ESString::create(u"call"), ESFunctionObject::create(NULL, node));

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
        return ESString::create(u"[Object object]");
    }), false, false)));

    m_object->set(strings->prototype, m_objectPrototype);

    //$19.1.3.2 Object.prototype.hasOwnProperty(V)
    FunctionDeclarationNode* node = new FunctionDeclarationNode(ESString::create(u"hasOwnProperty"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESValue ret;
        int len = instance->currentExecutionContext()->argumentCount();
        if (len < 1) {
            ret = ESValue(ESValue::ESFalseTag::ESFalse);
            return ret;
        }
        ::escargot::ESString* key = instance->currentExecutionContext()->arguments()[0].toPrimitive(ESValue::PrimitiveTypeHint::PreferString).toString();
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        escargot::ESString* keyString = key;
        ret = ESValue(thisVal->asESObject()->hasOwnProperty(keyString));
        return ret;
    }), false, false);
    m_objectPrototype->set(ESString::create(u"hasOwnProperty"), ESFunctionObject::create(NULL, node));

    //$19.1.2.4 Object.defineProperty ( O, P, Attributes )
    //http://www.ecma-international.org/ecma-262/6.0/#sec-object.defineproperty
    node = new FunctionDeclarationNode(ESString::create(u"defineProperty"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        if(instance->currentExecutionContext()->argumentCount() >= 3) {
            if(instance->currentExecutionContext()->arguments()[0].isObject()) {
                ESObject* obj = instance->currentExecutionContext()->arguments()[0].asESPointer()->asESObject();
                //TODO toPropertyKey
                ESValue key = instance->currentExecutionContext()->arguments()[1].toString();

                if(!instance->currentExecutionContext()->arguments()[2].isObject())
                    throw ESValue(TypeError::create());
                ESObject* desc = instance->currentExecutionContext()->arguments()[2].toObject();
                bool isEnumerable = true;
                bool isConfigurable = true;
                bool isWritable = true;
                //TODO get set
                ESValue v = desc->get(ESString::create(u"enumerable"), true);
                if(!v.isUndefined()) {
                    isEnumerable = v.toBoolean();
                }

                v = desc->get(ESString::create(u"configurable"), true);
                if(!v.isUndefined()) {
                    isConfigurable = v.toBoolean();
                }

                v = desc->get(ESString::create(u"writable"), true);
                if(!v.isUndefined()) {
                    isWritable = v.toBoolean();
                }

                v = desc->get(ESString::create(u"value"), true);

                obj->definePropertyOrThrow(key, isWritable, isEnumerable, isConfigurable).setValue(v);
            } else {
                throw ESValue(TypeError::create());
            }
        } else {
            throw ESValue(TypeError::create());
        }
        return ESValue();
    }), false, false);
    m_objectPrototype->set(ESString::create(u"defineProperty"), ESFunctionObject::create(NULL, node));

    set(strings->Object, m_object);
}

void GlobalObject::installError()
{
    NativeFunctionNode* errorFn = new NativeFunctionNode([](ESVMInstance* instance) -> ESValue {
        if(instance->currentExecutionContext()->isNewExpression()) {
            if(instance->currentExecutionContext()->argumentCount()) {
                instance->currentExecutionContext()->resolveThisBinding()->asESErrorObject()->set(strings->message, instance->currentExecutionContext()->arguments()[0].toString());
            }
            return ESValue();
        } else {
            escargot::ESErrorObject* obj = ESErrorObject::create();
            if(instance->currentExecutionContext()->argumentCount()) {
                obj->set(strings->message, instance->currentExecutionContext()->arguments()[0].toString());
            }
            return obj;
        }
    });
    m_error = ::escargot::ESFunctionObject::create(NULL,new FunctionDeclarationNode(strings->Error, InternalAtomicStringVector(),
            errorFn, false, false));
    m_error->set(strings->name, strings->Error);
    m_error->setConstructor(m_function);
    m_error->set__proto__(m_objectPrototype);
    m_errorPrototype = escargot::ESObject::create();
    m_error->setProtoType(m_errorPrototype);
    m_errorPrototype->setConstructor(m_error);
    escargot::Node* toStringNode = new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        //FIXME this is wrong
        ESValue v(instance->currentExecutionContext()->resolveThisBinding());
        ESPointer* o = v.asESPointer();
        u16string ret;
        ret.append(o->asESObject()->get(ESValue(ESString::create(u"name")), true).toString()->data());
        ret.append(u": ");
        ret.append(o->asESObject()->get(ESValue(ESString::create(u"message"))).toString()->data());

        return ESString::create(std::move(ret));
        RELEASE_ASSERT_NOT_REACHED();
    });
    m_errorPrototype->set(strings->toString, ESFunctionObject::create(NULL, new FunctionDeclarationNode(strings->toString, InternalAtomicStringVector(), toStringNode, false, false)));

    m_error->set(strings->toString, ESFunctionObject::create(NULL, new FunctionDeclarationNode(strings->toString, InternalAtomicStringVector(), toStringNode, false, false)));

    set(strings->Error, m_error);

    /////////////////////////////
    m_referenceError = ::escargot::ESFunctionObject::create(NULL,new FunctionDeclarationNode(strings->ReferenceError, InternalAtomicStringVector(), errorFn, false, false));
    m_referenceError->set(strings->name, strings->ReferenceError);
    m_referenceError->setConstructor(m_function);
    m_referenceError->set__proto__(m_errorPrototype);

    m_referenceErrorPrototype = ESErrorObject::create();
    m_referenceErrorPrototype->setConstructor(m_referenceError);

    m_referenceError->set(strings->prototype, m_referenceErrorPrototype);

    set(strings->ReferenceError, m_referenceError);

    /////////////////////////////
    m_typeError = ::escargot::ESFunctionObject::create(NULL,new FunctionDeclarationNode(strings->TypeError, InternalAtomicStringVector(), errorFn, false, false));
    m_typeError->set(strings->name, strings->TypeError);
    m_typeError->setConstructor(m_function);
    m_typeError->set__proto__(m_errorPrototype);

    m_typeErrorPrototype = ESErrorObject::create();
    m_typeErrorPrototype->setConstructor(m_typeError);

    m_typeError->set(strings->prototype, m_typeErrorPrototype);

    set(strings->TypeError, m_typeError);

    /////////////////////////////
    m_rangeError = ::escargot::ESFunctionObject::create(NULL,new FunctionDeclarationNode(strings->RangeError, InternalAtomicStringVector(), errorFn, false, false));
    m_rangeError->set(strings->name, strings->RangeError);
    m_rangeError->setConstructor(m_function);
    m_rangeError->set__proto__(m_errorPrototype);

    m_rangeErrorPrototype = ESErrorObject::create();
    m_rangeErrorPrototype->setConstructor(m_rangeError);

    m_rangeError->set(strings->prototype, m_rangeErrorPrototype);

    set(strings->RangeError, m_rangeError);

    /////////////////////////////
    m_syntaxError = ::escargot::ESFunctionObject::create(NULL,new FunctionDeclarationNode(strings->SyntaxError, InternalAtomicStringVector(), errorFn, false, false));
    m_syntaxError->set(strings->name, strings->SyntaxError);
    m_syntaxError->setConstructor(m_function);
    m_syntaxError->set__proto__(m_errorPrototype);

    m_syntaxErrorPrototype = ESErrorObject::create();
    m_syntaxErrorPrototype->setConstructor(m_syntaxError);

    m_syntaxError->set(strings->prototype, m_syntaxErrorPrototype);

    set(strings->SyntaxError, m_syntaxError);

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
        return array;
    }), false, false);

    //$22.1.3.1 Array.prototype.concat(...arguments)
    FunctionDeclarationNode* arrayConcat = new FunctionDeclarationNode(strings->concat, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        int arrlen = thisVal->length();
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
                int len = arr->length();
                int st = idx;
                for (; idx < st + len; idx++)
                    ret->set(idx, arr->get(idx - st));
            } else {
                ret->set(idx++, argi);
            }
        }
        return ret;
    }), false, false);
    m_arrayPrototype->ESObject::set(strings->concat, ESFunctionObject::create(NULL, arrayConcat));

    //$22.1.3.11 Array.prototype.indexOf()
    FunctionDeclarationNode* arrayIndexOf = new FunctionDeclarationNode(strings->indexOf, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        int len = thisVal->length();
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
        return ESValue(ret);
    }), false, false);
    m_arrayPrototype->ESObject::set(strings->indexOf, ESFunctionObject::create(NULL, arrayIndexOf));

    //$22.1.3.12 Array.prototype.join(separator)
    FunctionDeclarationNode* arrayJoin = new FunctionDeclarationNode(strings->join, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        u16string ret;
        int arrlen = thisVal->length();
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
        return ESString::create(std::move(ret));
    }), false, false);
    m_arrayPrototype->ESObject::set(strings->join, ESFunctionObject::create(NULL, arrayJoin));

    //$22.1.3.15 Array.prototype.map(callbackfn[, thisArg])
    FunctionDeclarationNode* arrayMap = new FunctionDeclarationNode(ESString::create(u"map"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        if(arglen < 1)
            throw ESValue(TypeError::create());
        ESValue arg = instance->currentExecutionContext()->arguments()[0];
        if (!(arg.isESPointer() && arg.asESPointer()->isESFunctionObject()))
            throw ESValue(TypeError::create());

        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        int arrlen = thisVal->length();
        escargot::ESArrayObject* ret = ESArrayObject::create(arrlen, instance->globalObject()->arrayPrototype());
        if (!thisVal->constructor().isUndefinedOrNull())
            ret->setConstructor(thisVal->constructor());

        ESValue tmpValue(thisVal);
        for (int idx = 0; idx < arrlen; idx++)
            ret->set(idx, ESFunctionObject::call(instance, arg.asESPointer()->asESFunctionObject(), instance->globalObject(), &tmpValue, 1, instance));
        return ret;
    }), false, false);
    m_arrayPrototype->ESObject::set(ESString::create(u"map"), ESFunctionObject::create(NULL, arrayMap));

    //$22.1.3.16 Array.prototype.pop ( )
    FunctionDeclarationNode* arrayPop = new FunctionDeclarationNode(strings->pop, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        return thisVal->pop();
    }), false, false);
    m_arrayPrototype->ESObject::set(strings->pop, ESFunctionObject::create(NULL, arrayPop));

    //$22.1.3.17 Array.prototype.push(item)
    FunctionDeclarationNode* arrayPush = new FunctionDeclarationNode(strings->push, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        auto thisBinded = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        if (thisBinded->isESArrayObject()) {
            int len = instance->currentExecutionContext()->argumentCount();
            auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
            for (int i = 0; i < len; i++) {
                ESValue& val = instance->currentExecutionContext()->arguments()[i];
                thisVal->push(val);
                i++;
             }
            return ESValue(thisVal->length());
        } else {
            ASSERT(instance->currentExecutionContext()->environment()->record()->getThisBinding()->isESObject());
            ESObject* O = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESObject();
            int len = O->get(strings->length).toInt32();
            int argCount = instance->currentExecutionContext()->argumentCount();
            if (len+argCount > std::pow(2, 53)-1) {
                throw ESValue(TypeError::create());
            } else {
                for (int i = 0; i < argCount; i++) {
                    ESValue& val = instance->currentExecutionContext()->arguments()[i];
                    O->set(ESString::create(len + i), val);
                  }
                ESValue ret = ESValue(len + argCount);
                O->set(strings->length, ret);
                return ret;
             }
         }
    }), false, false);
    m_arrayPrototype->ESObject::set(strings->push, ESFunctionObject::create(NULL, arrayPush));

    //$22.1.3.21 Array.prototype.shift ( )
    FunctionDeclarationNode* arrayShift = new FunctionDeclarationNode(strings->shift, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int argumentLen = instance->currentExecutionContext()->argumentCount();
        ESObject* O = instance->currentExecutionContext()->environment()->record()->getThisBinding(); //1
        int len = O->get(strings->length, true).toLength(); //3
        if(len == 0) { //5
            O->set(strings->length, ESValue(0),true);
            return ESValue();
        }
        ESValue first = O->get(ESValue(0));//6
        int k = 0; //8

        while(k < len) { //9
            ESValue from(k);
            ESValue to(k - 1);
            ESSlotAccessor fromPresentAccessor = O->findOwnProperty(from);
            if(fromPresentAccessor.hasData()) { //e
                ESValue fromVal = fromPresentAccessor.value();
                O->set(to, fromVal, true);
            } else {
                O->deletePropety(to);
            }
            k ++;
        }
        O->deletePropety(ESValue(len - 1)); //10
        O->set(strings->length, ESValue(len - 1)); //12
        return first;
    }), false, false);
    m_arrayPrototype->ESObject::set(strings->shift, ESFunctionObject::create(NULL, arrayShift));

    //$22.1.3.22 Array.prototype.slice(start, end)
    FunctionDeclarationNode* arraySlice = new FunctionDeclarationNode(strings->slice, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        escargot::ESArrayObject* thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        int arrlen = thisVal->length();
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
        return ret;
    }), false, false);
    m_arrayPrototype->ESObject::set(strings->slice, ESFunctionObject::create(NULL, arraySlice));

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
                ESValue ret = ESFunctionObject::call(instance, arg0, thisVal,
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

        return thisVal;
    }), false, false);
    m_arrayPrototype->ESObject::set(strings->sort, ESFunctionObject::create(NULL, arraySort));

    //$22.1.3.25 Array.prototype.splice(start, deleteCount, ...items)
    FunctionDeclarationNode* arraySplice = new FunctionDeclarationNode(strings->splice, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        auto thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESArrayObject();
        int arrlen = thisVal->length();
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
        return ret;
    }), false, false);
    m_arrayPrototype->ESObject::set(strings->splice, ESFunctionObject::create(NULL, arraySplice));
    m_arrayPrototype->ESObject::set(strings->toString, ESFunctionObject::create(NULL, new FunctionDeclarationNode(strings->toString, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        //FIXME this is wrong
        u16string ret;
        escargot::ESArrayObject* fn = instance->currentExecutionContext()->resolveThisBinding()->asESArrayObject();
        bool isFirst = true;
        for (int i = 0 ; i < fn->length() ; i++) {
            if(!isFirst)
                ret.append(u",");
            ESValue slot = fn->get(i);
            ret.append(slot.toString()->data());
            isFirst = false;
        }
        return ESString::create(std::move(ret));
    }), false, false)));

    m_array = ESFunctionObject::create(NULL, constructor);
    m_arrayPrototype->setConstructor(m_array);
    m_arrayPrototype->ESObject::set(strings->length, ESValue(0));
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
            return stringObject;
        } else {
            // called as function
            ESValue value = instance->currentExecutionContext()->arguments()[0];
            return value.toString();
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
        return instance->currentExecutionContext()->resolveThisBinding()->asESStringObject()->getStringData();
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
                return strings->asciiTable[c];
            return ESString::create(c);
        } else {
            u16string elements;
            elements.resize(length);
            char16_t* data = const_cast<char16_t *>(elements.data());
            for(int i = 0; i < length ; i ++) {
                data[i] = {(char16_t)instance->currentExecutionContext()->arguments()[i].toInteger()};
            }
            return ESString::create(std::move(elements));
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
                    return strings->asciiTable[c];
                } else {
                    return ESString::create(c);
                }
            } else {
                return strings->emptyESString;
            }
        }
        return ESValue(strings->emptyESString);
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
        return ret;
    }), false, false);
    m_stringPrototype->set(ESString::create(u"charCodeAt"), ESFunctionObject::create(NULL, stringCharCodeAt));

    //$21.1.3.4 String.prototype.concat(...args)
    FunctionDeclarationNode* stringConcat = new FunctionDeclarationNode(strings->concat, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        escargot::ESString* ret = thisObject->asESStringObject()->getStringData();
        int argCount = instance->currentExecutionContext()->argumentCount();
        for (int i=0; i<argCount; i++) {
            escargot::ESString* arg = instance->currentExecutionContext()->arguments()[i].toString();
            ret = ESString::concatTwoStrings(ret, arg);
        }
        return ret;
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
        return ESValue(result);
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
                    if(std::numeric_limits<unsigned>::max() == result.m_matchResults[i][j].m_start)
                        ret->set(idx++,ESValue(strings->emptyESString));
                    else
                        ret->set(idx++,ESString::create(std::move(u16string(str + result.m_matchResults[i][j].m_start,str + result.m_matchResults[i][j].m_end))));
                }
            }
            if (ret->length() == 0)
                return ESValue(ESValue::ESNull);
        }
        return ret;
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
            escargot::ESString* origStr = thisObject->asESStringObject()->getStringData();
            ESString::RegexMatchResult result;
            origStr->match(esptr, result);
            if(result.m_matchResults.size() == 0) {
                return origStr;
            }

            ESValue replaceValue = instance->currentExecutionContext()->arguments()[1];
            const u16string& orgString = origStr->string();
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
                        if(result.m_matchResults[i][j].m_start == std::numeric_limits<unsigned>::max())
                            RELEASE_ASSERT_NOT_REACHED(); //implement this case
                        arguments[j] = ESString::create(std::move(u16string(
                                origStr->data() + result.m_matchResults[i][j].m_start
                                , origStr->data() + result.m_matchResults[i][j].m_end
                                )));
                    }
                    arguments[subLen] = ESValue((int)result.m_matchResults[i][0].m_start);
                    arguments[subLen + 1] = origStr;
                    escargot::ESString* res = ESFunctionObject::call(instance, callee, instance->globalObject(), arguments, subLen + 2, instance).toString();

                    newThis.append(res->string());
                    if(i < matchCount - 1) {
                        newThis.append(orgString.begin() + result.m_matchResults[i][0].m_end, orgString.begin() + result.m_matchResults[i + 1][0].m_start);
                    }

                }
                newThis.append(orgString.begin() + result.m_matchResults[matchCount - 1][0].m_end, orgString.end());
                escargot::ESString* resultString = ESString::create(std::move(newThis));
                return resultString;
            } else {
                escargot::ESString* replaceString = replaceValue.toString();
                u16string newThis;
                newThis.reserve(orgString.size());
                if(replaceString->string().find('$') == u16string::npos) {
                    //flat replace
                    int32_t matchCount = result.m_matchResults.size();
                    if ((unsigned)replaceString->length() > ESRopeString::ESRopeStringCreateMinLimit) {
                        //create Rope string
                        u16string append(orgString, 0, result.m_matchResults[0][0].m_start);
                        escargot::ESString* newStr = ESString::create(std::move(append));
                        escargot::ESString* appendStr = nullptr;
                        for(int32_t i = 0; i < matchCount ; i ++) {
                            newStr = escargot::ESString::concatTwoStrings(newStr, replaceString);
                            if(i < matchCount - 1) {
                                u16string append2(orgString, result.m_matchResults[i][0].m_end, result.m_matchResults[i + 1][0].m_start - result.m_matchResults[i][0].m_end);
                                appendStr = ESString::create(std::move(append2));
                                newStr = escargot::ESString::concatTwoStrings(newStr, appendStr);
                            }
                        }
                        u16string append2(orgString, result.m_matchResults[matchCount - 1][0].m_end);
                        appendStr = ESString::create(std::move(append2));
                        newStr = escargot::ESString::concatTwoStrings(newStr, appendStr);
                        return newStr;
                    }
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
                return resultString;
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
        return ret;
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
        if(separator.isESPointer() && separator.asESPointer()->isESRegExpObject()) {
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
            escargot::ESRegExpObject* R = separator.asESPointer()->asESRegExpObject();

            // 14
            if(lim == 0)
                return arr;

            // 15
            if(separator.isUndefined()) {
                arr->set(0, str);
                return arr;
            }

            // 16
            auto splitMatch = [] (escargot::ESString* S, int q, escargot::ESRegExpObject* R) -> ESValue {
                escargot::ESString::RegexMatchResult result;
                auto prev = R->option();
                R->setOption((escargot::ESRegExpObject::Option)(prev & ~escargot::ESRegExpObject::Option::Global));
                bool ret = S->match(R, result, false, (size_t)q);
                R->setOption(prev);
                if(!ret)
                    return ESValue(false);
                return ESValue(result.m_matchResults[0][0].m_end);
            };
            // 16
            if(s == 0) {
                ESValue z = splitMatch(str, 0, R);
                if(z != ESValue(false))
                    return arr;
                arr->set(0, str);
                return arr;
            }

            // 17
            int q = p;

            // 18
            while(q != s) {
                escargot::ESString::RegexMatchResult result;
                ESValue e = splitMatch(str, q, R);
                auto prev = R->option();
                R->setOption((escargot::ESRegExpObject::Option)(prev & ~escargot::ESRegExpObject::Option::Global));
                bool ret = str->match(R, result, false, (size_t)q);
                R->setOption(prev);
                if(e == ESValue(ESValue::ESFalseTag::ESFalse)) {
                    if ((double)lengthA == lim)
                        return arr;
                    escargot::ESString* T = str->substring(q, str->length());
                    arr->set(lengthA, ESValue(T));
                    return arr;
                }
                else {
                    escargot::ESString* T = str->substring(p, result.m_matchResults[0][0].m_start);
                    arr->set(lengthA, ESValue(T));
                    lengthA++;
                    if ((double)lengthA == lim)
                        return arr;
                    p = result.m_matchResults[0][0].m_end;
                    q = result.m_matchResults[0][0].m_end;
                }
            }

            // 19
            escargot::ESString* T = str->substring(p, s);

            // 20
            arr->set(lengthA, ESValue(T));

            // 21, 22
            return arr;
        } else {
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
                return arr;
            }

            // 16
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
            // 16
            if(s == 0) {
                ESValue z = splitMatch(str->string(), 0, R);
                if(z != ESValue(false))
                    return arr;
                arr->set(0, str);
                return arr;
            }

            // 17
            int q = p;

            // 18
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
                            return arr;
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
            return arr;
        }
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
            return str;
        } else {
            int len = str->length();
            int intStart = instance->currentExecutionContext()->arguments()[0].toInteger();
            ESValue& end = instance->currentExecutionContext()->arguments()[1];
            int intEnd = (argCount < 2 || end.isUndefined()) ? len : end.toInteger();
            int finalStart = std::min(std::max(intStart, 0), len);
            int finalEnd = std::min(std::max(intEnd, 0), len);
            int from = std::min(finalStart, finalEnd);
            int to = std::max(finalStart, finalEnd);
            return str->substring(from, to);
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
        return ESString::create(std::move(newstr));
    }), false, false);
    m_stringPrototype->set(ESString::create(u"toLowerCase"), ESFunctionObject::create(NULL, stringToLowerCase));
    m_stringPrototype->set(ESString::create(u"toLocalLowerCase"), ESFunctionObject::create(NULL, stringToLowerCase));

    //$21.1.3.24 String.prototype.toUpperCase()
    FunctionDeclarationNode* stringToUpperCase = new FunctionDeclarationNode(ESString::create(u"toUpperCase"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        escargot::ESString* str = thisObject->asESStringObject()->getStringData();
        int strlen = str->string().length();
        u16string newstr(str->string());
        //TODO use ICU for this operation
        std::transform(newstr.begin(), newstr.end(), newstr.begin(), ::toupper);
        return ESString::create(std::move(newstr));
    }), false, false);
    m_stringPrototype->set(ESString::create(u"toUpperCase"), ESFunctionObject::create(NULL, stringToUpperCase));
    m_stringPrototype->set(ESString::create(u"toLocaleUpperCase"), ESFunctionObject::create(NULL, stringToUpperCase));

    //$B.2.3.1 String.prototype.substr (start, length)
    FunctionDeclarationNode* stringSubstr = new FunctionDeclarationNode(ESString::create(u"substr"), InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        escargot::ESString* str = thisObject->asESStringObject()->getStringData();
        if(instance->currentExecutionContext()->argumentCount() < 1) {
            return str;
        }
        double intStart = instance->currentExecutionContext()->arguments()[0].toInteger();
        double end;
        if(instance->currentExecutionContext()->argumentCount() > 1) {
            if(instance->currentExecutionContext()->arguments()[1].isUndefined()) {
                end = std::numeric_limits<double>::infinity();
            } else
                end = instance->currentExecutionContext()->arguments()[1].toInteger();
        } else {
            end = std::numeric_limits<double>::infinity();
        }
        double size = str->length();
        if(intStart < 0)
            intStart = std::max(size + intStart,0.0);
        double resultLength = std::min(std::max(end,0.0), size - intStart);
        if(resultLength <= 0)
            return strings->emptyESString;
        return str->substring(intStart, intStart + resultLength);
    }), false, false);
    m_stringPrototype->set(ESString::create(u"substr"), ESFunctionObject::create(NULL, stringSubstr));

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
        return ESString::create(u"FixMe: We have to return string with date and time data");
    }), false, false);

      // Initialization for reference error
    m_date = ::escargot::ESFunctionObject::create(NULL, constructor);
    m_date->set(strings->name, strings->Date);
    m_date->setConstructor(m_function);

    m_datePrototype->setConstructor(m_date);
    m_datePrototype->set(strings->toString, ESFunctionObject::create(NULL, new FunctionDeclarationNode(strings->toString, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        //FIXME this is wrong
        ESValue v(instance->currentExecutionContext()->resolveThisBinding());
        return v.toString();
    }), false, false)));

    m_date->set(strings->prototype, m_datePrototype);

    set(strings->Date, m_date);

    //$20.3.3.1 Date.now()
    FunctionDeclarationNode* nowNode = new FunctionDeclarationNode(ESString::create(u"now"), InternalAtomicStringVector(), new NativeFunctionNode([&](ESVMInstance* instance)->ESValue {
        struct timespec nowTime;
        clock_gettime(CLOCK_REALTIME,&nowTime);
        double ret = nowTime.tv_sec*1000 + floor(nowTime.tv_nsec/1000000);
        return ESValue(ret);
    }), false, false);
    m_date->set(ESString::create(u"now"), ESFunctionObject::create(NULL, nowNode));

    //$20.3.4.2 Date.prototype.getDate()
    FunctionDeclarationNode* getDateNode = new FunctionDeclarationNode(strings->getDate, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getDate();
       return ESValue(ret);
    }), false, false);
    m_datePrototype->set(strings->getDate, ::escargot::ESFunctionObject::create(NULL, getDateNode));

    //$20.3.4.3 Date.prototype.getDay()
    FunctionDeclarationNode* getDayNode = new FunctionDeclarationNode(strings->getDay, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getDay();
       return ESValue(ret);
    }), false, false);
    m_datePrototype->set(strings->getDay, ::escargot::ESFunctionObject::create(NULL, getDayNode));

    //$20.3.4.4 Date.prototype.getFullYear()
    FunctionDeclarationNode* getFullYearNode = new FunctionDeclarationNode(strings->getFullYear, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getFullYear();
       return ESValue(ret);
    }), false, false);
    m_datePrototype->set(strings->getFullYear, ::escargot::ESFunctionObject::create(NULL, getFullYearNode));


    //$20.3.4.5 Date.prototype.getHours()
    FunctionDeclarationNode* getHoursNode = new FunctionDeclarationNode(strings->getHours, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getHours();
       return ESValue(ret);
    }), false, false);
    m_datePrototype->set(strings->getHours, ::escargot::ESFunctionObject::create(NULL, getHoursNode));


    //$20.3.4.7 Date.prototype.getMinutes()
    FunctionDeclarationNode* getMinutesNode = new FunctionDeclarationNode(strings->getMinutes, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getMinutes();
       return ESValue(ret);
    }), false, false);
    m_datePrototype->set(strings->getMinutes, ::escargot::ESFunctionObject::create(NULL, getMinutesNode));


    //$20.3.4.8 Date.prototype.getMonth()
    FunctionDeclarationNode* getMonthNode = new FunctionDeclarationNode(strings->getMonth, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getMonth();
       return ESValue(ret);
    }), false, false);
    m_datePrototype->set(strings->getMonth, ::escargot::ESFunctionObject::create(NULL, getMonthNode));


    //$20.3.4.9 Date.prototype.getSeconds()
    FunctionDeclarationNode* getSecondsNode = new FunctionDeclarationNode(strings->getSeconds, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getSeconds();
       return ESValue(ret);
    }), false, false);
    m_datePrototype->set(strings->getSeconds, ::escargot::ESFunctionObject::create(NULL, getSecondsNode));

    //$20.3.4.10 Date.prototype.getTime()
    FunctionDeclarationNode* getTimeNode = new FunctionDeclarationNode(strings->getTime, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        double ret = thisObject->asESDateObject()->getTimeAsMilisec();
        return ESValue(ret);
    }), false, false);
    m_datePrototype->set(strings->getTime, ::escargot::ESFunctionObject::create(NULL, getTimeNode));

    //$20.3.4.11 Date.prototype.getTimezoneOffset()
    FunctionDeclarationNode* getTimezoneOffsetNode = new FunctionDeclarationNode(strings->getTimezoneOffset, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
       int ret = thisObject->asESDateObject()->getTimezoneOffset();
       return ESValue(ret);
    }), false, false);
    m_datePrototype->set(strings->getTimezoneOffset, ::escargot::ESFunctionObject::create(NULL, getTimezoneOffsetNode));

    //$20.3.4.27 Date.prototype.setTime()
    FunctionDeclarationNode* setTimeNode = new FunctionDeclarationNode(strings->setTime, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
       ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();

       size_t arg_size = instance->currentExecutionContext()->argumentCount();
       if (arg_size > 0 && instance->currentExecutionContext()->arguments()[0].isNumber()) {
           ESValue arg = instance->currentExecutionContext()->arguments()[0];
           thisObject->asESDateObject()->setTime(arg.toNumber());
           return ESValue();
       } else {
           double value = std::numeric_limits<double>::quiet_NaN();
           return ESValue(value);
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
    // TODO(add reference)
    m_math->set(strings->E, ESValue(2.718281828459045));
    // TODO(add reference)
    m_math->set(escargot::ESString::create(u"LN2"), ESValue(0.6931471805599453));
    m_math->set(escargot::ESString::create(u"LN10"), ESValue(2.302585092994046));

    // initialize math object: $20.2.2.1 Math.abs()
    FunctionDeclarationNode* absNode = new FunctionDeclarationNode(strings->abs, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            return ESValue(value);
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            double value = abs(arg.toNumber());
            return ESValue(value);
         }
        return ESValue();
    }), false, false);
    m_math->set(strings->abs, ::escargot::ESFunctionObject::create(NULL, absNode));

    // initialize math object: $20.2.2.16 Math.ceil()
    FunctionDeclarationNode* ceilNode = new FunctionDeclarationNode(strings->ceil, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            return ESValue(value);
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            if (arg.isInt32()) {
                return arg;
            } else if (arg.isDouble()) {
                int value = ceil(arg.asDouble());
                return ESValue(value);
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
            return ESValue(value);
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            double value = cos(arg.toNumber());
            return ESValue(value);
         }
        return ESValue();
    }), false, false);
    m_math->set(strings->cos, ::escargot::ESFunctionObject::create(NULL, cosNode));

    // initialize math object: $20.2.2.16 Math.floor()
    FunctionDeclarationNode* floorNode = new FunctionDeclarationNode(strings->floor, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            return ESValue(value);
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            if (arg.isInt32()) {
                return ESValue(arg);
            } else if (arg.isDouble()) {
                int value = floor(arg.asDouble());
                return ESValue(value);
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
            return ESValue(value);
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
            return ESValue(ret);
        }
        return ESValue();
    }), false, false);
    m_math->set(strings->log, ::escargot::ESFunctionObject::create(NULL, logNode));

    // initialize math object: $20.2.2.24 Math.max()
    FunctionDeclarationNode* maxNode = new FunctionDeclarationNode(strings->max, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double n_inf = -1 * std::numeric_limits<double>::infinity();
            return ESValue(n_inf);
        } else{
            double max_value = instance->currentExecutionContext()->arguments()[0].toNumber();
            for (unsigned i = 1; i < arg_size; i++) {
                double value = instance->currentExecutionContext()->arguments()[i].toNumber();
                if (value > max_value)
                    max_value = value;
             }
            return ESValue(max_value);
         }
        return ESValue();
    }), false, false);
    m_math->set(strings->max, ::escargot::ESFunctionObject::create(NULL, maxNode));

    // initialize math object: $20.2.2.25 Math.min()
    FunctionDeclarationNode* minNode = new FunctionDeclarationNode(strings->min, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            return ESValue(std::numeric_limits<double>::infinity());
        } else{
            double min_value = instance->currentExecutionContext()->arguments()[0].toNumber();
            for (unsigned i = 1; i < arg_size; i++) {
                double value = instance->currentExecutionContext()->arguments()[i].toNumber();
                if (value < min_value)
                    min_value = value;
             }
            return ESValue(min_value);
         }
        return ESValue();
    }), false, false);
    m_math->set(strings->min, ::escargot::ESFunctionObject::create(NULL, minNode));

    // initialize math object: $20.2.2.26 Math.pow()
    FunctionDeclarationNode* powNode = new FunctionDeclarationNode(strings->pow, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size < 2) {
            double value = std::numeric_limits<double>::quiet_NaN();
            return ESValue(value);
        } else {
            ESValue arg1 = instance->currentExecutionContext()->arguments()[0];
            ESValue arg2 = instance->currentExecutionContext()->arguments()[1];
            double value = pow(arg1.toNumber(), arg2.toNumber());
            return ESValue(value);
         }

        return ESValue();
    }), false, false);
    m_math->set(strings->pow, ::escargot::ESFunctionObject::create(NULL, powNode));

    // initialize math object: $20.2.2.27 Math.random()
    FunctionDeclarationNode* randomNode = new FunctionDeclarationNode(strings->random, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        double rand = (double) std::rand()/RAND_MAX;
        return ESValue(rand);
    }), false, false);
    m_math->set(strings->random, ::escargot::ESFunctionObject::create(NULL, randomNode));

    // initialize math object: $20.2.2.28 Math.round()
    FunctionDeclarationNode* roundNode = new FunctionDeclarationNode(strings->round, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            return ESValue(value);
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            double value = round(arg.toNumber());
            return ESValue(value);
         }

        return ESValue();
    }), false, false);
    m_math->set(strings->round, ::escargot::ESFunctionObject::create(NULL, roundNode));

    // initialize math object: $20.2.2.30 Math.sin()
    FunctionDeclarationNode* sinNode = new FunctionDeclarationNode(strings->sin, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            return ESValue(value);
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            double value = sin(arg.toNumber());
            return ESValue(value);
         }
        return ESValue();
    }), false, false);
    m_math->set(strings->sin, ::escargot::ESFunctionObject::create(NULL, sinNode));

    // initialize math object: $20.2.2.32 Math.sqrt()
    FunctionDeclarationNode* sqrtNode = new FunctionDeclarationNode(strings->sqrt, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            return ESValue(value);
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            double value = sqrt(arg.toNumber());
            return ESValue(value);
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

    // $ 20.1.2.6 Number.MAX_SAFE_INTEGER
    m_number->set(ESString::create(u"MAX_SAFE_INTEGER"), ESValue(9007199254740991.0));
    // $ 20.1.2.7 Number.MAX_VALUE
    m_number->set(strings->MAX_VALUE, ESValue(1.7976931348623157E+308));
    // $ 20.1.2.8 Number.MIN_SAFE_INTEGER
    m_number->set(ESString::create(u"MIN_SAFE_INTEGER"), ESValue(ESValue::EncodeAsDouble, -9007199254740991.0));
    // $ 20.1.2.9 Number.MIN_VALUE
    m_number->set(strings->MIN_VALUE, ESValue(5E-324));
    // $ 20.1.2.10 Number.NaN
    m_number->set(strings->NaN, ESValue(std::numeric_limits<double>::quiet_NaN()));
    // $ 20.1.2.11 Number.NEGATIVE_INFINITY
    m_number->set(strings->NEGATIVE_INFINITY, ESValue(-std::numeric_limits<double>::infinity()));
    // $ 20.1.2.14 Number.POSITIVE_INFINITY
    m_number->set(strings->POSITIVE_INFINITY, ESValue(std::numeric_limits<double>::infinity()));

    // initialize numberPrototype object
    m_numberPrototype->setConstructor(m_number);

    // initialize numberPrototype object: $20.1.3.3 Number.prototype.toFixed(fractionDigits)
    FunctionDeclarationNode* toFixedNode = new FunctionDeclarationNode(strings->toFixed, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        escargot::ESNumberObject* thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESNumberObject();
        int arglen = instance->currentExecutionContext()->argumentCount();
        if (arglen == 0) {
            return ESValue(round(thisVal->numberData())).toString();
        } else if (arglen == 1) {
             int digit = instance->currentExecutionContext()->arguments()[0].toInteger();
             int shift = pow(10, digit);
             return ESValue(round(thisVal->numberData()*shift)/shift).toString();
        }

        return ESValue();
    }), false, false);
    m_numberPrototype->set(strings->toFixed, ::escargot::ESFunctionObject::create(NULL, toFixedNode));

    FunctionDeclarationNode* toPrecisionNode = new FunctionDeclarationNode(strings->toPrecision, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        escargot::ESNumberObject* thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESNumberObject();
        int arglen = instance->currentExecutionContext()->argumentCount();
        if (arglen == 0 || instance->currentExecutionContext()->arguments()[0].isUndefined()) {
            return ESValue(thisVal->numberData()).toString();
        } else if (arglen == 1) {
            double x = thisVal->numberData();
            int p = instance->currentExecutionContext()->arguments()[0].toInteger();
            if(isnan(x)) {
                return strings->NaN;
            }
            u16string s;
            if(x < 0) {
                s = u"-";
                x = -x;
            }
            if(isinf(x)) {
                s += u"Infinity";
                return escargot::ESString::create(std::move(s));
            }

            if(p < 1 && p > 21) {
                throw ESValue(RangeError::create());
            }

            x = thisVal->numberData();
            std::basic_ostringstream<char> stream;
            stream << "%." << p << "lf";
            std::string fstr = stream.str();
            char buf[512];
            sprintf(buf, fstr.c_str(), x);
            return ESValue(ESString::create(buf));
        }

        return ESValue();
    }), false, false);
    m_numberPrototype->set(strings->toPrecision, ::escargot::ESFunctionObject::create(NULL, toPrecisionNode));

    // initialize numberPrototype object: $20.1.3.6 Number.prototype.toString()
    FunctionDeclarationNode* toStringNode = new FunctionDeclarationNode(strings->toString, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        escargot::ESNumberObject* thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESNumberObject();
        int arglen = instance->currentExecutionContext()->argumentCount();
        double radix = 10;
        if (arglen >= 1) {
            radix = instance->currentExecutionContext()->arguments()[0].toInteger();
            if (radix < 2 || radix > 36)
                throw ESValue(RangeError::create(ESString::create(u"String.prototype.toString() radix is not in valid range")));
        }
        if (radix == 10)
            return (ESValue(thisVal->numberData()).toString());
        else {
            char buffer[256];
            int len = itoa((int)thisVal->numberData(), buffer, radix);
            return (ESString::create(buffer));
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
            return (o);
        } else // If NewTarget is undefined, return b
            return (ret);
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
        return (thisVal->booleanData().toString());
    }), false, false);
    m_booleanPrototype->set(strings->toString, ::escargot::ESFunctionObject::create(NULL, toStringNode));

    // initialize booleanPrototype object: $19.3.3.3 Boolean.prototype.valueOf()
    FunctionDeclarationNode* valueOfNode = new FunctionDeclarationNode(strings->valueOf, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        escargot::ESBooleanObject* thisVal = instance->currentExecutionContext()->environment()->record()->getThisBinding()->asESBooleanObject();
        return (thisVal->booleanData());
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

    m_regexpPrototype->defineAccessorProperty(escargot::ESString::create(u"lastIndex"), [](ESObject* self) -> ESValue {
        return ESValue(self->asESRegExpObject()->lastIndex());
    }, nullptr, true, false, false);

    m_regexpPrototype->defineAccessorProperty(escargot::ESString::create(u"ignoreCase"), [](ESObject* self) -> ESValue {
        return ESValue((bool)(self->asESRegExpObject()->option() & ESRegExpObject::Option::IgnoreCase));
    }, nullptr, true, false, false);

    m_regexpPrototype->defineAccessorProperty(escargot::ESString::create(u"global"), [](ESObject* self) -> ESValue {
        return ESValue((bool)(self->asESRegExpObject()->option() & ESRegExpObject::Option::Global));
    }, nullptr, true, false, false);

    m_regexpPrototype->defineAccessorProperty(escargot::ESString::create(u"multiline"), [](ESObject* self) -> ESValue {
        return ESValue((bool)(self->asESRegExpObject()->option() & ESRegExpObject::Option::MultiLine));
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
            return (ESValue(testResult));
        }
        return ESValue(false);
    }), false, false);
    m_regexpPrototype->set(strings->test, ::escargot::ESFunctionObject::create(NULL, testNode));

    // 21.2.5.2 RegExp.prototype.exec( string )
    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-regexp.prototype.test
    FunctionDeclarationNode* execNode = new FunctionDeclarationNode(strings->exec, InternalAtomicStringVector(), new NativeFunctionNode([](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->environment()->record()->getThisBinding();
        if(!thisObject->isESRegExpObject())
            throw TypeError::create();
        ::escargot::ESRegExpObject* regexp = thisObject->asESRegExpObject();
        int argCount = instance->currentExecutionContext()->argumentCount();
        if(argCount > 0) {
            escargot::ESString* sourceStr = instance->currentExecutionContext()->arguments()[0].toString();
            if(sourceStr == regexp->m_lastExecutedString || (regexp->m_lastExecutedString && sourceStr->string() == regexp->m_lastExecutedString->string())) {

            } else {
                regexp->m_lastIndex = 0;
            }
            regexp->m_lastExecutedString = sourceStr;
            ESString::RegexMatchResult result;
            bool isGlobal = regexp->option() & ESRegExpObject::Option::Global;
            regexp->setOption((ESRegExpObject::Option)(regexp->option() & ~ESRegExpObject::Option::Global));
            bool testResult = sourceStr->match(thisObject, result, false, regexp->m_lastIndex);
            if(isGlobal) {
                regexp->setOption((ESRegExpObject::Option)(regexp->option() | ESRegExpObject::Option::Global));
            }

            if(!testResult) {
                regexp->m_lastIndex = 0;
                return ESValue(ESValue::ESNull);
            }

            if(isGlobal) {
                //update lastIndex
                regexp->m_lastIndex = result.m_matchResults[0][0].m_end;
            }
            ::escargot::ESArrayObject* arr = ::escargot::ESArrayObject::create();
            ((ESObject *)arr)->set(ESValue(strings->input), ESValue(sourceStr));
            ((ESObject *)arr)->set(ESValue(strings->index), ESValue(result.m_matchResults[0][0].m_start));
            const char16_t* str = sourceStr->string().data();

            int idx = 0;
            for(unsigned i = 0; i < result.m_matchResults.size() ; i ++) {
                for(unsigned j = 0; j < result.m_matchResults[i].size() ; j ++) {
                    if(result.m_matchResults[i][j].m_start == std::numeric_limits<unsigned>::max())
                        arr->set(idx++,ESValue(strings->emptyESString));
                    else
                        arr->set(idx++,ESString::create(std::move(u16string(str + result.m_matchResults[i][j].m_start,str + result.m_matchResults[i][j].m_end))));
                }
            }
            return arr;
        } else {
            return ESValue(ESValue::ESNull);
        }
    }), false, false);
    m_regexpPrototype->set(strings->exec, ::escargot::ESFunctionObject::create(NULL, execNode));

    // add regexp to global object
    set(strings->RegExp, m_regexp);
}

}
