#include "Escargot.h"
#include "GlobalObject.h"
#include "ast/AST.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#include "parser/ScriptParser.h"

#include "Yarr.h"
#include "parser/esprima.h"

namespace escargot {

GlobalObject::GlobalObject()
    : ESObject(ESPointer::Type::ESObject, ESValue())
{
    m_flags.m_isGlobalObject = true;
    m_didSomePrototypeObjectDefineIndexedProperty = false;
}

std::string char2hex(char dec )
{
    char dig1 = (dec&0xF0)>>4;
    char dig2 = (dec&0x0F);
    if (0 <= dig1 && dig1 <= 9)
        dig1 += 48; // 0, 48inascii
    if (10 <= dig1 && dig1 <=15)
        dig1 += 65-10; // a, 97inascii
    if (0 <= dig2 && dig2 <= 9)
        dig2 += 48;
    if (10 <= dig2 && dig2 <=15)
        dig2 += 65-10;

    std::string r;
    r.append(&dig1, 1);
    r.append(&dig2, 1);
    return r;
}

char hex2char(char first, char second)
{
    char dig1 = first;
    char dig2 = second;
    if (48 <= dig1 && dig1 <= 57)
        dig1 -= 48;
    if (65 <= dig1 && dig1 <= 70)
        dig1 -= 65 - 10;
    if (48 <= dig2 && dig2 <= 57)
        dig2 -= 48;
    if (65 <= dig2 && dig2 <= 70)
        dig2 -= 65 - 10;

    char dec = dig1 << 4;
    dec |= dig2;

    return dec;
}

void GlobalObject::initGlobalObject()
{
    forceNonVectorHiddenClass();
    m_objectPrototype = ESObject::create();
    m_objectPrototype->forceNonVectorHiddenClass();
    m_objectPrototype->set__proto__(ESValue(ESValue::ESNull));

    installFunction();
    installObject();
    installArray();
    installString();
    installError();
    installDate();
    installMath();
    installJSON();
    installNumber();
    installBoolean();
    installRegExp();
    installArrayBuffer();
    installTypedArray();

    // Value Properties of the Global Object
    defineDataProperty(strings->Infinity, false, false, false, ESValue(std::numeric_limits<double>::infinity()));
    defineDataProperty(strings->NaN, false, false, false, ESValue(std::numeric_limits<double>::quiet_NaN()));
    defineDataProperty(strings->undefined, false, false, false, ESValue());

    auto brkFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        printf("dbgBreak\n");
        return ESValue();
    }, ESString::create(u"dbgBreak"));
    set(ESString::create(u"dbgBreak"), brkFunction);

    auto printFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        if (instance->currentExecutionContext()->argumentCount()) {
            ESVMInstance::printValue(instance->currentExecutionContext()->arguments()[0]);
        }
        return ESValue();
    }, ESString::create(u"print"));
    set(ESString::create(u"print"), printFunction);

    auto gcFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        GC_gcollect();
        return ESValue();
    }, ESString::create(u"gc"));
    set(ESString::create(u"gc"), gcFunction);

    auto loadFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        if (instance->currentExecutionContext()->argumentCount()) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            escargot::ESString* str = val.toString();
            FILE* fp = fopen(str->utf8Data(), "r");
            if (fp) {
                fseek(fp, 0L, SEEK_END);
                size_t sz = ftell(fp);
                fseek(fp, 0L, SEEK_SET);
                std::string str;
                str.reserve(sz+2);
                static char buf[4096];
                while (fgets(buf, sizeof buf, fp) != NULL) {
                    str += buf;
                }
                fclose(fp);
                instance->runOnGlobalContext([instance, &str]() {
                    escargot::ESStringData source(str.c_str());
                    instance->evaluate(source);
                });
            }
        }
        return ESValue();
    }, ESString::create(u"load"));
    set(ESString::create(u"load"), loadFunction);
    set(ESString::create(u"run"), loadFunction);

    auto readFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        if (instance->currentExecutionContext()->argumentCount()) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            escargot::ESString* str = val.toString();
            FILE* fp = fopen(str->utf8Data(), "r");
            if (fp) {
                fseek(fp, 0L, SEEK_END);
                size_t sz = ftell(fp);
                fseek(fp, 0L, SEEK_SET);
                std::string str;
                str.reserve(sz+2);
                static char buf[4096];
                while (fgets(buf, sizeof buf, fp) != NULL) {
                    str += buf;
                }
                fclose(fp);

                escargot::ESString* data = ESString::create(str.data());
                return data;
            }
            return ESValue();
        }
        return ESValue();
    }, ESString::create(u"read"));
    set(ESString::create(u"read"), readFunction);

    // Function Properties of the Global Object
    m_eval = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue argument = instance->currentExecutionContext()->arguments()[0];
        if (!argument.isESString()) {
            return argument;
        }
        ESValue ret = instance->runOnEvalContext([instance, &argument]() {
            ESValue ret = instance->evaluate(const_cast<u16string &>(argument.asESString()->string()), true);
            return ret;
        }, false);
        return ret;
    }, ESString::create(u"eval"));
    defineDataProperty(ESString::create(u"eval"), true, false, true, m_eval);

    // $18.2.2
    defineDataProperty(ESString::create(u"isFinite"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue ret;
        int len = instance->currentExecutionContext()->argumentCount();
        if (len < 1)
            ret = ESValue(ESValue::ESFalseTag::ESFalse);
        else {
            ESValue& argument = instance->currentExecutionContext()->arguments()[0];
            double num = argument.toNumber();
            if (std::isnan(num) || num == std::numeric_limits<double>::infinity() || num == -std::numeric_limits<double>::infinity())
                ret = ESValue(ESValue::ESFalseTag::ESFalse);
            else
                ret = ESValue(ESValue::ESTrueTag::ESTrue);
        }
        return ret;
    }, ESString::create(u"isFinite")));

    // $18.2.3
    defineDataProperty(ESString::create(u"isNaN"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue ret;
        int len = instance->currentExecutionContext()->argumentCount();
        if (len < 1)
            ret = ESValue(ESValue::ESFalseTag::ESFalse);
        else {
            ESValue& argument = instance->currentExecutionContext()->arguments()[0];
            double num = argument.toNumber();
            if (std::isnan(num))
                ret = ESValue(ESValue::ESTrueTag::ESTrue);
            else
                ret = ESValue(ESValue::ESFalseTag::ESFalse);
        }
        return ret;
    }, ESString::create(u"isNaN")));

    // $18.2.4 parseFloat(string)
    defineDataProperty(ESString::create(u"parseFloat"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int len = instance->currentExecutionContext()->argumentCount();
        if (len < 1) {
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        } else {
            ESValue input = instance->currentExecutionContext()->arguments()[0];
            escargot::ESString* str = input.toString();
            double f = atof(str->utf8Data());
            return ESValue(f);
        }
    }, ESString::create(u"parseFloat")));

    // $18.2.5 parseInt(string, radix)
    defineDataProperty(ESString::create(u"parseInt"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue ret;
        int len = instance->currentExecutionContext()->argumentCount();
        if (len < 1) {
            ret = ESValue(std::numeric_limits<double>::quiet_NaN());
        } else {
            int radix = 10;
            if (len >= 2)
                radix = instance->currentExecutionContext()->arguments()[1].toInt32();
            if (radix == 0)
                radix = 10;
            if (radix < 2 || radix > 36) {
                ret = ESValue(std::numeric_limits<double>::quiet_NaN());
                return ret;
            } else {
                ESValue input = instance->currentExecutionContext()->arguments()[0];
                escargot::ESString* str = input.toString();
                // TODO remove leading space
                // TODO Let sign be 1.
                // TODO If S is not empty and the first code unit of S is 0x002D (HYPHEN-MINUS), let sign be −1.
                // TODO If S is not empty and the first code unit of S is 0x002B (PLUS SIGN) or 0x002D (HYPHEN-MINUS), remove the first code unit from S.
                bool stripPrefix = true;
                if (radix == 10) {
                } else if (radix == 16) {
                    stripPrefix = true;
                } else {
                    stripPrefix = false;
                }

                // TODO stripPrefix
                long int ll;
                str->wcharData([&ll, &radix](wchar_t* data, size_t len) {
                    ll = wcstol(data, NULL, radix);
                });
                return ESValue((double)ll);
            }
        }
        return ret;
    }, ESString::create(u"parseInt")));

    // $18.2.6.5 encodeURIComponent(uriComponent)
    defineDataProperty(ESString::create(u"encodeURIComponent"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int argLen = instance->currentExecutionContext()->argumentCount();
        if (argLen == 0)
            return ESValue();
        std::string componentString = std::string(instance->currentExecutionContext()->arguments()->asESString()->utf8Data());
        int strLen = componentString.length();

        std::string escaped="";
        for (int i = 0; i < strLen; i++) {
            if ((48 <= componentString[i] && componentString[i] <= 57) // DecimalDigit
                || (65 <= componentString[i] && componentString[i] <= 90) // uriAlpha - lower case
                || (97 <= componentString[i] && componentString[i] <= 122) // uriAlpha - lower case
                || (componentString[i] == '-' || componentString[i] == '_' || componentString[i] == '.'
                || componentString[i] == '!' || componentString[i] == '~' // uriMark
                || componentString[i] == '*' || componentString[i] == '`' || componentString[i] == '('
                || componentString[i] == ')')) {
                    escaped.append(&componentString[i], 1);
            } else {
                if ((0 <= componentString[i] && componentString[i] <= 0xD7FF)
                    || (0xDC00 <= componentString[i] && componentString[i] <= 0xFFFF)) {
                        escaped.append("%");
                        escaped.append(char2hex(componentString[i])); // converts char 255 to string "ff"
                } else {
                    throw ESValue(URIError::create(ESString::create("malformd URI")));
                }

            }
        }

        return escargot::ESString::create(escaped.c_str());
    }, ESString::create(u"encodeURIComponent")));

    // $B.2.1.2 unescape(string)
    defineDataProperty(ESString::create(u"unescape"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int argLen = instance->currentExecutionContext()->argumentCount();
        if (argLen == 0)
            return ESValue();
        std::string str = std::string(instance->currentExecutionContext()->arguments()->asESString()->utf8Data());
        int length = str.length();
        std::string R = "";
        for (int i = 0; i < length; i++) {
            if (str[i] == '%') {
                R.push_back(hex2char(str[i+1], str[i+2]));
                i = i + 2;
            } else {
                R.append(&str[i], 1);
            }
        }
        return escargot::ESString::create(R.c_str());
    }, ESString::create(u"unescape")));

}


void GlobalObject::installFunction()
{
    // $19.2.1 Function Constructor
    m_function = ESFunctionObject::create(NULL, [](ESVMInstance* instance) -> ESValue {
        int len = instance->currentExecutionContext()->argumentCount();
        CodeBlock* codeBlock = CodeBlock::create();
        ByteCodeGenerateContext context;
        if (len == 0) {
            codeBlock->pushCode(End(), context, NULL);
        } else {
            escargot::ESString* body = instance->currentExecutionContext()->arguments()[len-1].toString();
            u16string prefix = u"function anonymous(";
            for (int i = 0; i < len-1; i++) {
                escargot::ESString* arg = instance->currentExecutionContext()->arguments()[i].toString();
                prefix.append(arg->string());
                if (i != len-2)
                    prefix.append(u",");
            }
            prefix.append(u"){");
            prefix.append(body->string());
            prefix.append(u"}");
            Node* programNode = instance->scriptParser()->generateAST(instance, prefix, true);
            FunctionNode* functionDeclAST = static_cast<FunctionNode* >(static_cast<ProgramNode *>(programNode)->body()[1]);
            ByteCodeGenerateContext context;
            codeBlock->m_innerIdentifiers = std::move(functionDeclAST->innerIdentifiers());
            codeBlock->m_needsActivation = functionDeclAST->needsActivation();
            codeBlock->m_params = std::move(functionDeclAST->params());
            codeBlock->m_isStrict = functionDeclAST->isStrict();
            functionDeclAST->body()->generateStatementByteCode(codeBlock, context);
#ifdef ENABLE_ESJIT
            codeBlock->m_tempRegisterSize = context.m_currentSSARegisterCount;
#endif
            codeBlock->pushCode(ReturnFunction(), context, functionDeclAST);
#ifdef ENABLE_ESJIT
            context.cleanupSSARegisterCount();
#endif
            escargot::InternalAtomicStringVector params = functionDeclAST->params();
        }
        escargot::ESFunctionObject* function;
        LexicalEnvironment* scope = instance->globalExecutionContext()->environment();
        if (instance->currentExecutionContext()->isNewExpression() && instance->currentExecutionContext()->resolveThisBindingToObject()->isESFunctionObject()) {
            function = instance->currentExecutionContext()->resolveThisBindingToObject()->asESFunctionObject();
            function->initialize(scope, codeBlock);
        } else
            function = ESFunctionObject::create(scope, codeBlock, ESString::create(u"anonymous"));
#ifdef ENABLE_ESJIT
        context.cleanupSSARegisterCount();
#endif
        ESObject* prototype = ESObject::create();
        prototype->set__proto__(instance->globalObject()->object()->protoType());
        function->setProtoType(prototype);
        return function;
    }, strings->Function, 1); // $19.2.2.1 Function.length: This is a data property with a value of 1.
    ::escargot::ESFunctionObject* emptyFunction = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        return ESValue();
    }, strings->Function);
    m_function->forceNonVectorHiddenClass();

    m_functionPrototype = emptyFunction;
    m_functionPrototype->forceNonVectorHiddenClass();
    m_functionPrototype->set__proto__(m_objectPrototype);
    m_function->set__proto__(emptyFunction);
    m_function->setProtoType(emptyFunction);
    m_functionPrototype->defineDataProperty(strings->constructor, true, false, true, m_function);

    ESVMInstance::currentInstance()->setGlobalFunctionPrototype(m_functionPrototype);

    m_functionPrototype->defineDataProperty(strings->toString, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // FIXME
        if (instance->currentExecutionContext()->resolveThisBindingToObject()->isESFunctionObject()) {
            u16string ret;
            ret = u"function ";
            escargot::ESFunctionObject* fn = instance->currentExecutionContext()->resolveThisBindingToObject()->asESFunctionObject();
            ret.append(fn->name()->data());
            ret.append(u"() {}");
            return ESString::create(std::move(ret));
        }
        u16string ret;
        return ESString::create(std::move(ret));
    }, strings->toString));

    // $19.2.3.1 Function.prototype.apply(thisArg, argArray)
    m_functionPrototype->defineDataProperty(ESString::create(u"apply"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        auto thisVal = instance->currentExecutionContext()->resolveThisBindingToObject()->asESFunctionObject();
        int arglen = instance->currentExecutionContext()->argumentCount();
        ESValue& thisArg = instance->currentExecutionContext()->arguments()[0];
        int arrlen = 0;
        ESValue* arguments = NULL;
        if (instance->currentExecutionContext()->argumentCount() > 1) {
            if (instance->currentExecutionContext()->arguments()[1].isESPointer()) {
                if (instance->currentExecutionContext()->arguments()[1].asESPointer()->isESArrayObject()) {
                    escargot::ESArrayObject* argArray = instance->currentExecutionContext()->arguments()[1].asESPointer()->asESArrayObject();
                    arrlen = argArray->length();
                    arguments = (ESValue*)alloca(sizeof(ESValue) * arrlen);
                    for (int i = 0; i < arrlen; i++) {
                        arguments[i] = argArray->get(i);
                    }
                } else if (instance->currentExecutionContext()->arguments()[1].asESPointer()->isESObject()) {
                    escargot::ESObject* obj = instance->currentExecutionContext()->arguments()[1].asESPointer()->asESObject();
                    arrlen = obj->get(strings->length.string()).toInteger();
                    arguments = (ESValue*)alloca(sizeof(ESValue) * arrlen);
                    for (int i = 0; i < arrlen; i++) {
                        arguments[i] = obj->get(ESValue(i));
                    }
                }
            }

        }

        return ESFunctionObject::call(instance, thisVal, thisArg, arguments, arrlen, false);
    }, ESString::create(u"apply")));

    // 19.2.3.2 Function.prototype.bind (thisArg , ...args)
    m_functionPrototype->defineDataProperty(ESString::create(u"bind"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisVal = instance->currentExecutionContext()->resolveThisBinding();
        if (!thisVal.isESPointer() || !thisVal.asESPointer()->isESFunctionObject()) {
            throw ESValue(TypeError::create(ESString::create("this value should be function")));
        }
        CodeBlock* cb = CodeBlock::create();
        ByteCodeGenerateContext context;
        CallBoundFunction code;
        code.m_boundTargetFunction = thisVal.asESPointer()->asESFunctionObject();
        code.m_boundThis = instance->currentExecutionContext()->readArgument(0);
        if (instance->currentExecutionContext()->argumentCount() >= 2) {
            code.m_boundArgumentsCount = instance->currentExecutionContext()->argumentCount() - 1;
        } else
            code.m_boundArgumentsCount = 0;
        code.m_boundArguments = (ESValue *)GC_malloc(code.m_boundArgumentsCount * sizeof(ESValue));
        memcpy(code.m_boundArguments, instance->currentExecutionContext()->arguments() + 1, code.m_boundArgumentsCount * sizeof(ESValue));
        cb->pushCode(code, context, NULL);
        escargot::ESFunctionObject* function = ESFunctionObject::create(NULL, cb, strings->emptyString, std::max(code.m_boundTargetFunction->length() - (int) code.m_boundArgumentsCount, 0));
        function->set__proto__(instance->globalObject()->functionPrototype());
        ESObject* prototype = ESObject::create();
        prototype->set__proto__(instance->globalObject()->object()->protoType());
        function->setProtoType(prototype);
#ifdef ENABLE_ESJIT
        context.cleanupSSARegisterCount();
#endif
        // NOTE
        // The binded function has only one bytecode what is CallBoundFunction
        // so we should not try JIT for binded function.
        return function;
    }, ESString::create(u"bind")));

    // 19.2.3.3 Function.prototype.call (thisArg , ...args)
    m_functionPrototype->defineDataProperty(ESString::create(u"call"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        auto thisVal = instance->currentExecutionContext()->resolveThisBindingToObject()->asESFunctionObject();
        int arglen = instance->currentExecutionContext()->argumentCount()-1;
        ESValue& thisArg = instance->currentExecutionContext()->arguments()[0];
        ESValue* arguments = (ESValue*)alloca(sizeof(ESValue) * arglen);
        for (int i = 0; i < arglen; i++) {
            arguments[i] = instance->currentExecutionContext()->arguments()[i + 1];
        }

        return ESFunctionObject::call(instance, thisVal, thisArg, arguments, arglen, false);
    }, ESString::create(u"call")));

    defineDataProperty(strings->Function, true, false, true, m_function);
}

inline void definePropertyWithDescriptorObject(ESObject* obj, ESValue& key, ESObject* desc)
{
    bool isEnumerable = false;
    bool isConfigurable = false;
    bool isWritable = false;
    // TODO get set
    ESValue v = desc->get(ESString::create(u"enumerable"));
    if (!v.isUndefined()) {
        isEnumerable = v.toBoolean();
    }

    v = desc->get(ESString::create(u"configurable"));
    if (!v.isUndefined()) {
        isConfigurable = v.toBoolean();
    }

    v = desc->get(ESString::create(u"writable"));
    if (!v.isUndefined()) {
        isWritable = v.toBoolean();
    }

    v = desc->get(ESString::create(u"value"));
    bool gs = false;
    ESValue get = desc->get(ESString::create(u"get"));
    ESValue set = desc->get(ESString::create(u"set"));
    if (!get.isUndefined() || !set.isUndefined()) {
        escargot::ESFunctionObject* getter = NULL;
        escargot::ESFunctionObject* setter = NULL;
        if (!get.isEmpty() && get.isESPointer() && get.asESPointer()->isESFunctionObject()) {
            getter = get.asESPointer()->asESFunctionObject();
        }
        if (!set.isEmpty() && set.isESPointer() && set.asESPointer()->isESFunctionObject()) {
            setter = set.asESPointer()->asESFunctionObject();
        }
        obj->defineAccessorProperty(key, new ESPropertyAccessorData(getter, setter), isWritable, isEnumerable, isConfigurable);
    } else {
        obj->defineDataProperty(key, isWritable, isEnumerable, isConfigurable, v);
    }
}

inline ESValue objectDefineProperties(ESValue object, ESValue& properties)
{
    if (!object.isObject())
        throw ESValue(TypeError::create(ESString::create("objectDefineProperties: first argument is not object")));
    ESObject* props = properties.toObject();
    props->enumeration([&](ESValue key) {
        ESValue propertyDesc = props->get(key);
        if (!propertyDesc.isUndefined()) {
            if (!propertyDesc.isObject())
                throw ESValue(TypeError::create(ESString::create("objectDefineProperties: descriptor is not object")));
            definePropertyWithDescriptorObject(object.asESPointer()->asESObject(), key, propertyDesc.asESPointer()->asESObject());
        }
    });
    return object;
}

void GlobalObject::installObject()
{
    ::escargot::ESFunctionObject* emptyFunction = m_functionPrototype;
    m_object = ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int len = instance->currentExecutionContext()->argumentCount();
        ESValue value;
        if (len > 0)
            value = instance->currentExecutionContext()->arguments()[0];
        if (value.isUndefined() || value.isNull()) {
            ESObject* object = ESObject::create();
            object->set__proto__(instance->globalObject()->objectPrototype());
            return object;
        } else {
            return value.toObject();
        }
    }, strings->Object);
    m_object->forceNonVectorHiddenClass();
    m_object->set__proto__(emptyFunction);
    m_object->setProtoType(m_objectPrototype);
    m_objectPrototype->defineDataProperty(strings->constructor, true, false, true, m_object);

    // Object.prototype.toString
    m_objectPrototype->defineDataProperty(strings->toString, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* thisVal = instance->currentExecutionContext()->resolveThisBindingToObject();
        if (thisVal->isESArrayObject()) {
            return ESString::create(u"[object Array]");
        } else if (thisVal->isESStringObject()) {
            return ESString::create(u"[object String]");
        } else if (thisVal->isESFunctionObject()) {
            return ESString::create(u"[object Function]");
        } else if (thisVal->isESErrorObject()) {
            return ESString::create(u"[object Error]");
        } else if (thisVal->isESBooleanObject()) {
            return ESString::create(u"[object Boolean]");
        } else if (thisVal->isESNumberObject()) {
            return ESString::create(u"[object Number]");
        } else if (thisVal->isESDateObject()) {
            return ESString::create(u"[object Date]");
        } else if (thisVal->isESRegExpObject()) {
            return ESString::create(u"[object RegExp]");
        } else if (thisVal->isESTypedArrayObject()) {
            u16string ret = u"[object ";
            ESValue ta_constructor = thisVal->get(strings->constructor.string());
            // ALWAYS created from new expression
            ASSERT(ta_constructor.isESPointer() && ta_constructor.asESPointer()->isESObject());
            ESValue ta_name = ta_constructor.asESPointer()->asESObject()->get(strings->name.string());
            ret.append(ta_name.toString()->data());
            ret.append(u"]");
            return ESString::create(std::move(ret));
        }
        return ESString::create(u"[object Object]");
    }, strings->toString));

    // $19.1.3.2 Object.prototype.hasOwnProperty(V)
    m_objectPrototype->defineDataProperty(ESString::create(u"hasOwnProperty"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue ret;
        int len = instance->currentExecutionContext()->argumentCount();
        if (len < 1) {
            ret = ESValue(ESValue::ESFalseTag::ESFalse);
            return ret;
        }
        ::escargot::ESString* key = instance->currentExecutionContext()->arguments()[0].toPrimitive(ESValue::PrimitiveTypeHint::PreferString).toString();
        auto thisVal = instance->currentExecutionContext()->resolveThisBindingToObject();
        escargot::ESString* keyString = key;
        ret = ESValue(thisVal->asESObject()->hasOwnProperty(keyString));
        return ret;
    }, ESString::create(u"hasOwnProperty")));

    // $19.1.2.3 Object.defineProperties ( O, P, Attributes )
    m_object->defineDataProperty(ESString::create(u"defineProperties"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue object = instance->currentExecutionContext()->readArgument(0);
        ESValue properties = instance->currentExecutionContext()->readArgument(0);
        return objectDefineProperties(object, properties);
    }, ESString::create(u"defineProperties")));

    // $19.1.2.4 Object.defineProperty ( O, P, Attributes )
    // http://www.ecma-international.org/ecma-262/6.0/#sec-object.defineproperty
    m_object->defineDataProperty(ESString::create(u"defineProperty"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        if (instance->currentExecutionContext()->argumentCount() >= 3) {
            if (instance->currentExecutionContext()->arguments()[0].isObject()) {
                ESObject* obj = instance->currentExecutionContext()->arguments()[0].asESPointer()->asESObject();
                // TODO toPropertyKey
                ESValue key = instance->currentExecutionContext()->arguments()[1].toString();

                if (!instance->currentExecutionContext()->arguments()[2].isObject())
                    throw ESValue(TypeError::create(ESString::create("Object.defineProperty: 3rd argument is not object")));
                ESObject* desc = instance->currentExecutionContext()->arguments()[2].toObject();
                definePropertyWithDescriptorObject(obj, key, desc);
            } else {
                throw ESValue(TypeError::create(ESString::create("Object.defineProperty: 1st argument is not object")));
            }
        } else {
            throw ESValue(TypeError::create(ESString::create("Object.defineProperty: # of arguments < 3")));
        }
        return ESValue();
    }, ESString::create(u"defineProperty")));

    // $19.1.2.2 Object.create ( O [ , Properties ] )
    // http://www.ecma-international.org/ecma-262/6.0/#sec-object.defineproperty
    m_object->defineDataProperty(ESString::create(u"create"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue proto = instance->currentExecutionContext()->readArgument(0);
        if (!proto.isObject() && !proto.isNull()) {
            throw ESValue(TypeError::create(ESString::create("first parameter is should be object")));
        }
        ESObject* obj = ESObject::create();
        if (proto.isNull())
            obj->set__proto__(ESValue());
        else
            obj->set__proto__(proto);
        if (!instance->currentExecutionContext()->readArgument(1).isUndefined()) {
            return objectDefineProperties(obj, instance->currentExecutionContext()->arguments()[1]);
        }
        return obj;
    }, ESString::create(u"create"), 2));

    // $19.1.2.7 Object.getOwnPropertyNames
    m_object->defineDataProperty(ESString::create(u"getOwnPropertyNames"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue O = instance->currentExecutionContext()->readArgument(0);
        if (!O.isObject())
            return O;
        ESObject* obj = O.asESPointer()->asESObject();
        escargot::ESArrayObject* nameList = ESArrayObject::create();
        obj->enumeration([&nameList](ESValue key) {
            if (key.isESString())
                nameList->push(key);
        });
        return nameList;
    }, ESString::create(u"getOwnPropertyNames")));

    // $19.1.2.9 Object.getPrototypeOf
    m_object->defineDataProperty(strings->getPrototypeOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue O = instance->currentExecutionContext()->readArgument(0);
        return O.toObject()->__proto__();
    }, strings->getPrototypeOf));

    // $19.1.2.14 Object.keys ( O )
    m_object->defineDataProperty(ESString::create(u"keys"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // Let obj be ToObject(O).
        ESObject* obj = instance->currentExecutionContext()->readArgument(0).toObject();
        escargot::ESArrayObject* arr = ESArrayObject::create();
        obj->enumeration([&arr](ESValue key) {
            arr->push(key);
        });
        return arr;
    }, ESString::create(u"keys")));

    // $19.1.3.7 Object.prototype.valueOf ( )
    m_objectPrototype->defineDataProperty(strings->valueOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // Return ToObject(this value).
        return instance->currentExecutionContext()->resolveThisBindingToObject();
    }, strings->valueOf));

    // $19.1.3.3 Object.prototype.isPrototypeOf ( V )
    m_objectPrototype->defineDataProperty(strings->isPrototypeOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue V = instance->currentExecutionContext()->readArgument(0);
        if (!V.isObject())
            return ESValue(false);
        ESObject* O = instance->currentExecutionContext()->resolveThisBindingToObject();
        while (true) {
            V = V.asESPointer()->asESObject()->__proto__();
            if (V.isNull())
                return ESValue(false);
            if (V.equalsTo(O))
                return ESValue(true);
        }
    }, strings->isPrototypeOf));

    // $19.1.3.4 Object.prototype.propertyIsEnumerable ( V )
    m_objectPrototype->defineDataProperty(strings->propertyIsEnumerable, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // TODO toPropertyKey
        ESValue key = instance->currentExecutionContext()->readArgument(0);
        ESObject* O = instance->currentExecutionContext()->resolveThisBindingToObject();
        if (!O->hasOwnProperty(key))
            return ESValue(false);
        if ((O->isESArrayObject() && O->asESArrayObject()->isFastmode()) || O->isESTypedArrayObject()) {
            // In fast mode, it was already checked in O->hasOwnProperty.
            return ESValue(true);
        }
        size_t t = O->hiddenClass()->findProperty(key.toString());
        if (O->hiddenClass()->m_propertyInfo[t].m_flags.m_isEnumerable)
            return ESValue(true);
        return ESValue(false);
    }, strings->propertyIsEnumerable));

    // $19.1.3.5 Object.prototype.toLocaleString
    m_objectPrototype->defineDataProperty(strings->toLocaleString, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* thisVal = instance->currentExecutionContext()->resolveThisBindingToObject();
        return ESValue(ESValue(thisVal).toString());
    }, strings->toLocaleString));

    defineDataProperty(strings->Object, true, false, true, m_object);
}

void GlobalObject::installError()
{
    auto errorFn = [](ESVMInstance* instance) -> ESValue {
        if (instance->currentExecutionContext()->isNewExpression()) {
            if (instance->currentExecutionContext()->argumentCount()) {
                instance->currentExecutionContext()->resolveThisBindingToObject()->asESErrorObject()->set(strings->message, instance->currentExecutionContext()->arguments()[0].toString());
            }
            return ESValue();
        } else {
            escargot::ESErrorObject* obj = ESErrorObject::create();
            if (instance->currentExecutionContext()->argumentCount()) {
                obj->set(strings->message, instance->currentExecutionContext()->arguments()[0].toString());
            }
            return obj;
        }
    };
    m_error = ::escargot::ESFunctionObject::create(NULL, errorFn, strings->Error);
    m_error->forceNonVectorHiddenClass();
    m_error->set__proto__(m_objectPrototype);
    m_errorPrototype = escargot::ESObject::create();
    m_error->setProtoType(m_errorPrototype);
    m_errorPrototype->set__proto__(m_objectPrototype);
    m_errorPrototype->defineDataProperty(strings->constructor, true, false, true, m_error);
    m_errorPrototype->forceNonVectorHiddenClass();

    escargot::ESFunctionObject* toString = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // FIXME this is wrong
        ESValue v(instance->currentExecutionContext()->resolveThisBindingToObject());
        ESPointer* o = v.asESPointer();
        u16string ret;
        ret.append(o->asESObject()->get(ESValue(ESString::create(u"name"))).toString()->data());
        ret.append(u": ");
        ret.append(o->asESObject()->get(ESValue(ESString::create(u"message"))).toString()->data());

        return ESString::create(std::move(ret));
        RELEASE_ASSERT_NOT_REACHED();
    }, strings->toString);
    m_errorPrototype->defineDataProperty(strings->toString, true, false, true, toString);

    defineDataProperty(strings->Error, true, false, true, m_error);

    // ///////////////////////////
    m_referenceError = ::escargot::ESFunctionObject::create(NULL, errorFn, strings->ReferenceError);
    m_referenceError->set__proto__(m_errorPrototype);
    m_referenceError->forceNonVectorHiddenClass();

    m_referenceErrorPrototype = ESErrorObject::create();
    m_referenceErrorPrototype->forceNonVectorHiddenClass();

    m_referenceError->setProtoType(m_referenceErrorPrototype);

    m_referenceErrorPrototype->defineDataProperty(strings->constructor, true, false, true, m_referenceError);

    defineDataProperty(strings->ReferenceError, true, false, true, m_referenceError);

    // ///////////////////////////
    m_typeError = ::escargot::ESFunctionObject::create(NULL, errorFn, strings->TypeError);
    m_typeError->set__proto__(m_errorPrototype);
    m_typeError->forceNonVectorHiddenClass();

    m_typeErrorPrototype = ESErrorObject::create();
    m_typeErrorPrototype->forceNonVectorHiddenClass();

    m_typeError->setProtoType(m_typeErrorPrototype);

    m_typeErrorPrototype->defineDataProperty(strings->constructor, true, false, true, m_typeError);

    defineDataProperty(strings->TypeError, true, false, true, m_typeError);

    // ///////////////////////////
    m_rangeError = ::escargot::ESFunctionObject::create(NULL, errorFn, strings->RangeError);
    m_rangeError->set__proto__(m_errorPrototype);
    m_rangeError->forceNonVectorHiddenClass();

    m_rangeErrorPrototype = ESErrorObject::create();
    m_rangeErrorPrototype->forceNonVectorHiddenClass();

    m_rangeError->setProtoType(m_rangeErrorPrototype);

    m_rangeErrorPrototype->defineDataProperty(strings->constructor, true, false, true, m_rangeError);

    defineDataProperty(strings->RangeError, true, false, true, m_rangeError);

    // ///////////////////////////
    m_syntaxError = ::escargot::ESFunctionObject::create(NULL, errorFn, strings->SyntaxError);
    m_syntaxError->set__proto__(m_errorPrototype);
    m_syntaxError->forceNonVectorHiddenClass();

    m_syntaxErrorPrototype = ESErrorObject::create();
    m_syntaxErrorPrototype->forceNonVectorHiddenClass();

    m_syntaxError->setProtoType(m_syntaxErrorPrototype);

    m_syntaxErrorPrototype->defineDataProperty(strings->constructor, true, false, true, m_syntaxError);

    defineDataProperty(strings->SyntaxError, true, false, true, m_syntaxError);

    // ///////////////////////////
    m_uriError = ::escargot::ESFunctionObject::create(NULL, errorFn, strings->URIError);
    m_uriError->set__proto__(m_errorPrototype);
    m_uriError->forceNonVectorHiddenClass();

    m_uriErrorPrototype = ESErrorObject::create();
    m_uriErrorPrototype->forceNonVectorHiddenClass();

    m_uriError->setProtoType(m_uriErrorPrototype);

    m_uriErrorPrototype->defineDataProperty(strings->constructor, true, false, true, m_uriError);

    defineDataProperty(strings->URIError, true, false, true, m_uriError);
}

void GlobalObject::installArray()
{
    m_arrayPrototype = ESArrayObject::create(0);
    m_arrayPrototype->set__proto__(m_objectPrototype);
    m_arrayPrototype->forceNonVectorHiddenClass();

    // $22.1.1 Array Constructor
    m_array = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int len = instance->currentExecutionContext()->argumentCount();
        int size = 0;
        if (len > 1)
            size = len;
        else if (len == 1) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            if (val.isInt32()) {
                size = val.toNumber();
            } else {
                size = 1;
            }
        }
        escargot::ESArrayObject* array;
        if (instance->currentExecutionContext()->isNewExpression() && instance->currentExecutionContext()->resolveThisBindingToObject()->isESArrayObject()) {
            array = instance->currentExecutionContext()->resolveThisBindingToObject()->asESArrayObject();
            array->setLength(size);
        } else
            array = ESArrayObject::create(size);
        if (len >= 1) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            if (len > 1 || !val.isInt32()) {
                for (int idx = 0; idx < len; idx++) {
                    array->set(idx, val);
                    val = instance->currentExecutionContext()->arguments()[idx + 1];
                }
            }
        } else {
        }
        return array;
    }, strings->Array, 1);
    m_array->forceNonVectorHiddenClass();
    m_arrayPrototype->defineDataProperty(strings->constructor, true, false, true, m_array);

    // $22.1.2.2 Array.isArray(arg)
    m_array->ESObject::defineDataProperty(strings->isArray, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        if (arglen < 1)
            return ESValue(ESValue::ESFalseTag::ESFalse);
        ESValue arg = instance->currentExecutionContext()->arguments()[0];
        if (arg.isESPointer() && arg.asESPointer()->isESArrayObject())
            return ESValue(ESValue::ESTrueTag::ESTrue);
        return ESValue(ESValue::ESFalseTag::ESFalse);
    }, strings->isArray, 1));

    // $22.1.3.1 Array.prototype.concat(...arguments)
    m_arrayPrototype->ESObject::defineDataProperty(strings->concat, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        auto thisBinded = instance->currentExecutionContext()->resolveThisBindingToObject();
        escargot::ESArrayObject* ret = ESArrayObject::create(0);
        int idx = 0;
        if (LIKELY(thisBinded->isESArrayObject())) {
            auto thisVal = thisBinded->asESArrayObject();
            for (idx = 0; idx < thisVal->length(); idx++)
                ret->set(idx, thisVal->get(idx));
        } else {
            ASSERT(thisBinded->isESObject());
            ESObject* O = thisBinded->asESObject();
            ret->set(idx++, ESValue(O));
        }
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
    }, strings->concat, 1));

    // $22.1.3.10 Array.prototype.forEach()
    m_arrayPrototype->ESObject::defineDataProperty(strings->forEach, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // Let O be ToObject(this value).
        ESObject* O = instance->currentExecutionContext()->resolveThisBindingToObject();

        // Let len be ToLength(Get(O, "length")).
        uint32_t len = O->length();

        ESValue callbackfn = instance->currentExecutionContext()->readArgument(0);

        // If IsCallable(callbackfn) is false, throw a TypeError exception.
        if (!callbackfn.isESPointer() || !callbackfn.asESPointer()->isESFunctionObject()) {
            throw ESValue(TypeError::create(ESString::create("first parameter of forEach should be function")));
        }

        // If thisArg was supplied, let T be thisArg; else let T be undefined.
        ESValue T = instance->currentExecutionContext()->readArgument(1);

        // Let k be 0.
        int32_t k = 0;
        while (k < len) {
            // Let Pk be ToString(k).
            ESValue pk(k);
            // Let kPresent be HasProperty(O, Pk).
            bool kPresent = O->hasOwnProperty(pk);
            if (kPresent) {
                ESValue kValue = O->getOwnProperty(pk);
                ESValue arguments[3] = {kValue, pk, O};
                ESFunctionObject::call(instance, callbackfn, T, arguments, 3, false);
            }
            k++;
        }
        return ESValue();
    }, strings->forEach, 1));

    // $22.1.3.11 Array.prototype.indexOf()
    m_arrayPrototype->ESObject::defineDataProperty(strings->indexOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        auto thisBinded = instance->currentExecutionContext()->resolveThisBindingToObject();
        int len = thisBinded->length();
        int ret = 0;
        if (len == 0)
            ret = -1;
        else {
            int n = 0, k = 0;
            if (instance->currentExecutionContext()->argumentCount() >= 2) {
                const ESValue& fromIndex = instance->currentExecutionContext()->arguments()[1];
                if (!fromIndex.isUndefined()) {
                    n = fromIndex.asInt32();
                    if (n >= len) {
                        ret = -1;
                    } else if (n >= 0) {
                        k = n;
                    } else {
                        k = len - n * (-1);
                        if (k < 0)
                            k = 0;
                    }
                }
            }
            if (ret != -1) {
                ret = -1;
                ESValue& searchElement = instance->currentExecutionContext()->arguments()[0];
                while (k < len) {
                    ESValue kPresent = thisBinded->get(ESValue(k));
                    if (searchElement.equalsTo(kPresent)) {
                        ret = k;
                        break;
                    }
                    k++;
                }
            }
        }
        return ESValue(ret);
    }, strings->indexOf, 1));

    // $22.1.3.12 Array.prototype.join(separator)
    m_arrayPrototype->ESObject::defineDataProperty(strings->join, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        auto thisBinded = instance->currentExecutionContext()->resolveThisBindingToObject();
        u16string ret;
        int arrlen = thisBinded->length();
        if (arrlen >= 0) {
            escargot::ESString* separator;
            if (arglen == 0) {
                separator = ESString::create(u",");
            } else {
                separator = instance->currentExecutionContext()->arguments()[0].toString();
            }
            for (int i = 0; i < arrlen; i++) {
                ESValue elemi = thisBinded->get(ESValue(i));
                if (i != 0)
                    ret.append(separator->data());
                if (!elemi.isUndefinedOrNull())
                    ret.append(elemi.toString()->data());
            }
        }
        return ESString::create(std::move(ret));
    }, strings->join, 1));

    // $22.1.3.15 Array.prototype.map(callbackfn[, thisArg])
    m_arrayPrototype->ESObject::defineDataProperty(ESString::create(u"map"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        auto thisBinded = instance->currentExecutionContext()->resolveThisBindingToObject();
        int arglen = instance->currentExecutionContext()->argumentCount();
        if (arglen < 1)
            throw ESValue(TypeError::create(ESString::create("Array.prototype.map: arglen < 1")));
        ESValue arg = instance->currentExecutionContext()->arguments()[0];
        if (!(arg.isESPointer() && arg.asESPointer()->isESFunctionObject()))
            throw ESValue(TypeError::create(ESString::create("Array.prototype.map: argument is not Function")));

        int arrlen = thisBinded->length();
        escargot::ESArrayObject* ret = ESArrayObject::create(arrlen);
        for (int idx = 0; idx < arrlen; idx++) {
            ESValue tmpValue(thisBinded->get(ESValue(idx)));
            ret->set(idx, ESFunctionObject::call(instance, arg.asESPointer()->asESFunctionObject(), instance->globalObject(), &tmpValue, 1, false));
        }
        return ret;
    }, ESString::create(u"map"), 1));

    // $22.1.3.16 Array.prototype.pop ( )
    m_arrayPrototype->ESObject::defineDataProperty(strings->pop, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        auto thisBinded = instance->currentExecutionContext()->resolveThisBindingToObject();
        return thisBinded->pop();
    }, strings->pop, 0));

    // $22.1.3.17 Array.prototype.push(item)
    m_arrayPrototype->ESObject::defineDataProperty(strings->push, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        auto thisBinded = instance->currentExecutionContext()->resolveThisBindingToObject();
        if (LIKELY(thisBinded->isESArrayObject())) {
            int len = instance->currentExecutionContext()->argumentCount();
            auto thisVal = thisBinded->asESArrayObject();
            for (int i = 0; i < len; i++) {
                ESValue& val = instance->currentExecutionContext()->arguments()[i];
                thisVal->push(val);
            }
            return ESValue(thisVal->length());
        } else {
            ASSERT(thisBinded->isESObject());
            ESObject* O = thisBinded->asESObject();
            int len = O->get(strings->length.string()).toInt32();
            int argCount = instance->currentExecutionContext()->argumentCount();
            if (len+argCount > std::pow(2, 53)-1) {
                throw ESValue(TypeError::create(ESString::create("Array.prototype.push: length is too large")));
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
    }, strings->push, 1));

    // $22.1.3.20 Array.prototype.reverse()
    m_arrayPrototype->ESObject::defineDataProperty(ESString::create(u"reverse"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* O = instance->currentExecutionContext()->resolveThisBindingToObject();
        unsigned len = O->get(strings->length.string()).toLength();
        unsigned middle = std::floor(len / 2);
        unsigned lower = 0;
        while (middle != lower) {
            unsigned upper = len - lower - 1;
            ESValue upperP = ESValue(upper);
            ESValue lowerP = ESValue(lower);

            bool lowerExists = O->hasOwnProperty(lowerP);
            ESValue lowerValue;
            if (lowerExists) {
                lowerValue = O->get(lowerP);
            }
            bool upperExists = O->hasOwnProperty(upperP);
            ESValue upperValue;
            if (upperExists) {
                upperValue = O->get(upperP);
            }

            if (lowerExists && upperExists) {
                O->set(lowerP, upperValue, true);
                O->set(upperP, lowerValue, true);
            } else if (!lowerExists && upperExists) {
                O->set(lowerP, upperValue, true);
                O->deleteProperty(upperP);
            } else if (lowerExists && !upperExists) {
                O->deleteProperty(lowerP);
                O->set(upperP, lowerValue, true);
            }
            lower++;
        }
        return O;
    }, ESString::create(u"reverse"), 0));

    // $22.1.3.21 Array.prototype.shift ( )
    m_arrayPrototype->ESObject::defineDataProperty(strings->shift, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int argumentLen = instance->currentExecutionContext()->argumentCount();
        ESObject* O = instance->currentExecutionContext()->resolveThisBindingToObject(); // 1
        int len = O->get(strings->length.string()).toLength(); // 3
        if (len == 0) { // 5
            O->set(strings->length.string(), ESValue(0), true);
            return ESValue();
        }
        ESValue first = O->get(ESValue(0)); // 6
        int k = 0; // 8

        while (k < len) { // 9
            ESValue from(k);
            ESValue to(k - 1);
            O->get(from);
            if (O->hasOwnProperty(from)) { // e
                ESValue fromVal = O->get(from);
                O->set(to, fromVal, true);
            } else {
                O->deleteProperty(to);
            }
            k++;
        }
        O->deleteProperty(ESValue(len - 1)); // 10
        O->set(strings->length, ESValue(len - 1)); // 12
        return first;
    }, strings->shift, 0));

    // $22.1.3.22 Array.prototype.slice(start, end)
    m_arrayPrototype->ESObject::defineDataProperty(strings->slice, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        auto thisBinded = instance->currentExecutionContext()->resolveThisBindingToObject();
        int arrlen = thisBinded->length();
        int start, end;
        if (arglen < 1) {
            start = 0;
        } else {
            start = instance->currentExecutionContext()->arguments()[0].toInteger();
        }
        if (start < 0) {
            start = (arrlen + start > 0) ? arrlen + start : 0;
        } else {
            start = (start < arrlen) ? start : arrlen;
        }
        if (arglen >= 2) {
            end = instance->currentExecutionContext()->arguments()[1].toInteger();
        } else {
            end = arrlen;
        }
        if (end < 0) {
            end = (arrlen + end > 0) ? arrlen + end : 0;
        } else {
            end = (end < arrlen) ? end : arrlen;
        }
        int count = (end - start > 0) ? end - start : 0;
        escargot::ESArrayObject* ret = ESArrayObject::create(count);
        for (int i = start; i < end; i++) {
            ret->set(i-start, thisBinded->get(ESValue(i)));
        }
        return ret;
    }, strings->slice, 2));

    // $22.1.3.24 Array.prototype.sort(comparefn)
    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-array.prototype.sort
    m_arrayPrototype->ESObject::defineDataProperty(strings->sort, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        auto thisBinded = instance->currentExecutionContext()->resolveThisBindingToObject();
        RELEASE_ASSERT(thisBinded->isESArrayObject());
        // TODO support sort for non-array
        auto thisVal = thisBinded->asESArrayObject();
        if (arglen == 0) {
            thisVal->sort();
        } else {
            ESValue arg0 = instance->currentExecutionContext()->arguments()[0];
            thisVal->sort([&arg0, &instance, &thisVal](const ::escargot::ESValue& a, const ::escargot::ESValue& b) -> bool {
                ESValue arg[2] = { a, b };
                ESValue ret = ESFunctionObject::call(instance, arg0, thisVal,
                    arg, 2, false);

                double v = ret.toNumber();
                if (v == 0)
                    return false;
                else if (v < 0)
                    return true;
                else
                    return false;
            });
        }

        return thisVal;
    }, strings->sort, 1));

    // $22.1.3.25 Array.prototype.splice(start, deleteCount, ...items)
    m_arrayPrototype->ESObject::defineDataProperty(strings->splice, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        auto thisBinded = instance->currentExecutionContext()->resolveThisBindingToObject();
        int arrlen = thisBinded->length();
        escargot::ESArrayObject* ret = ESArrayObject::create(0);
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
                if (dc < 0)
                    dc = 0;
                deleteCnt = dc > (arrlen-start) ? arrlen-start : dc;
            }
            for (k = 0; k < deleteCnt; k++) {
                int from = start + k;
                ret->set(k, thisBinded->get(ESValue(from)));
            }
            int argIdx = 2;
            int leftInsert = insertCnt;
            for (k = start; k < start + deleteCnt; k++) {
                if (leftInsert > 0) {
                    thisBinded->set(ESValue(k), instance->currentExecutionContext()->arguments()[argIdx]);
                    leftInsert--;
                    argIdx++;
                } else {
                    thisBinded->eraseValues(k, start + deleteCnt - k);
                    break;
                }
            }
            if (LIKELY(thisBinded->isESArrayObject() && thisBinded->asESArrayObject()->isFastmode())) {
                auto thisArr = thisBinded->asESArrayObject();
                while (leftInsert > 0) {
                    thisArr->insertValue(k, instance->currentExecutionContext()->arguments()[argIdx]);
                    leftInsert--;
                    argIdx++;
                    k++;
                }
            } else if (leftInsert > 0) {
                // Move leftInsert steps to right
                for (int i = arrlen - 1; i >= k; i--) {
                    thisBinded->set(ESValue(i + leftInsert), thisBinded->get(ESValue(i)));
                }
                for (int i = k; i < k + leftInsert; i++, argIdx++) {
                    thisBinded->set(ESValue(i), instance->currentExecutionContext()->arguments()[argIdx]);
                }
            }
            if (UNLIKELY(!thisBinded->isESArrayObject()))
                thisBinded->set(strings->length, ESValue(arrlen - deleteCnt + insertCnt));
        }
        return ret;
    }, strings->splice, 2));

    // $22.1.3.27 Array.prototype.toString()
    m_arrayPrototype->ESObject::defineDataProperty(strings->toString, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        auto thisBinded = instance->currentExecutionContext()->resolveThisBindingToObject();
        int arrlen = thisBinded->length();
        ESValue toString = thisBinded->get(strings->join.string());
        if (!toString.isESPointer() || !toString.asESPointer()->isESFunctionObject()) {
            toString = instance->globalObject()->objectPrototype()->get(strings->toString.string());
        }
        return ESFunctionObject::call(instance, toString, thisBinded, NULL, 0, false);
    }, strings->toString, 0));

    // $22.1.3.28 Array.prototype.unshift(...items)
    m_arrayPrototype->ESObject::defineDataProperty(ESString::create(u"unshift"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {

        ESObject* O = instance->currentExecutionContext()->resolveThisBindingToObject();
        int len = O->get(strings->length.string()).toLength();
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount > 0) {
            if (len+argCount > std::pow(2, 53)-1)
                throw TypeError::create(ESString::create("Array.prototype.unshift: length is too large"));
            int k = len;
            while (k > 0) {
                ESValue from(k - 1);
                ESValue to(k + argCount - 1);
                bool fromPresent = O->hasOwnProperty(from);
                if (fromPresent) {
                    ESValue fromValue = O->get(from);
                    O->set(to, fromValue, true);
                } else {
                    O->deleteProperty(to);
                }
                k--;
            }

            int j = 0;
            ESValue* items = instance->currentExecutionContext()->arguments();
            for (int i = 0; i < argCount; i++) {
                O->set(ESValue(j), *(items+i), true);
            }
        }

        O->set(strings->length.string(), ESValue(len + argCount));
        return ESValue(len + argCount);
    }, ESString::create(u"unshift"), 1));

    m_arrayPrototype->ESObject::set(strings->length, ESValue(0));
    m_arrayPrototype->set__proto__(m_objectPrototype);

    m_array->setProtoType(m_arrayPrototype);

    defineDataProperty(strings->Array, true, false, true, m_array);
}

void GlobalObject::installString()
{
    m_string = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        if (instance->currentExecutionContext()->isNewExpression()) {
            // called as constructor
            ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
            escargot::ESStringObject* stringObject = thisObject->asESStringObject();
            if (instance->currentExecutionContext()->argumentCount() == 0) {
                stringObject->setStringData(strings->emptyString.string());
            } else {
                ESValue value = instance->currentExecutionContext()->readArgument(0);
                stringObject->setStringData(value.toString());
            }
            return stringObject;
        } else {
            // called as function
            if (instance->currentExecutionContext()->argumentCount() == 0)
                return strings->emptyString.string();
            ESValue value = instance->currentExecutionContext()->arguments()[0];
            return value.toString();
        }
        return ESValue();
    }, strings->String);
    m_string->forceNonVectorHiddenClass();

    m_stringPrototype = ESStringObject::create();
    m_stringPrototype->forceNonVectorHiddenClass();

    m_stringPrototype->set__proto__(m_objectPrototype);
    m_stringPrototype->defineDataProperty(strings->constructor, true, false, true, m_string);
    m_stringPrototype->defineDataProperty(strings->toString, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        if (instance->currentExecutionContext()->resolveThisBinding().isObject()) {
            if (instance->currentExecutionContext()->resolveThisBindingToObject()->isESStringObject()) {
                return instance->currentExecutionContext()->resolveThisBindingToObject()->asESStringObject()->stringData();
            }
        }
        return instance->currentExecutionContext()->resolveThisBinding().toString();
    }, strings->toString));

    m_string->set__proto__(m_functionPrototype); // empty Function
    m_string->setProtoType(m_stringPrototype);

    defineDataProperty(strings->String, true, false, true, m_string);

    // $21.1.2.1 String.fromCharCode(...codeUnits)
    m_string->defineDataProperty(ESString::create(u"fromCharCode"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int length = instance->currentExecutionContext()->argumentCount();
        if (length == 1) {
            char16_t c = (char16_t)instance->currentExecutionContext()->arguments()[0].toInteger();
            if (c >= 0 && c < ESCARGOT_ASCII_TABLE_MAX)
                return strings->asciiTable[c].string();
            return ESString::create(c);
        } else {
            u16string elements;
            elements.resize(length);
            char16_t* data = const_cast<char16_t *>(elements.data());
            for (int i = 0; i < length ; i ++) {
                data[i] = {(char16_t)instance->currentExecutionContext()->arguments()[i].toInteger()};
            }
            return ESString::create(std::move(elements));
        }
        return ESValue();
    }, ESString::create(u"fromCharCode")));

    // $21.1.3.1 String.prototype.charAt(pos)
    m_stringPrototype->defineDataProperty(ESString::create(u"charAt"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        const u16string& str = instance->currentExecutionContext()->resolveThisBinding().toString()->string();
        int position;
        if (instance->currentExecutionContext()->argumentCount() == 0) {
            position = 0;
        } else if (instance->currentExecutionContext()->argumentCount() > 0) {
            position = instance->currentExecutionContext()->arguments()[0].toInteger();
        } else {
            return ESValue(strings->emptyString.string());
        }

        if (LIKELY(0 <= position && position < (int)str.length())) {
            char16_t c = str[position];
            if (LIKELY(c < ESCARGOT_ASCII_TABLE_MAX)) {
                return strings->asciiTable[c].string();
            } else {
                return ESString::create(c);
            }
        } else {
            return strings->emptyString.string();
        }
        RELEASE_ASSERT_NOT_REACHED();
    }, ESString::create(u"charAt")));

    // $21.1.3.2 String.prototype.charCodeAt(pos)
    m_stringPrototype->defineDataProperty(ESString::create(u"charCodeAt"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        const u16string& str = instance->currentExecutionContext()->resolveThisBinding().toString()->string();
        int position = instance->currentExecutionContext()->arguments()[0].toInteger();
        ESValue ret;
        if (position < 0 || position >= (int)str.length())
            ret = ESValue(std::numeric_limits<double>::quiet_NaN());
        else
            ret = ESValue(str[position]);
        return ret;
    }, ESString::create(u"charCodeAt")));

    // $21.1.3.4 String.prototype.concat(...args)
    m_stringPrototype->defineDataProperty(strings->concat, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESString* ret = instance->currentExecutionContext()->resolveThisBinding().toString();
        int argCount = instance->currentExecutionContext()->argumentCount();
        for (int i = 0; i < argCount; i++) {
            escargot::ESString* arg = instance->currentExecutionContext()->arguments()[i].toString();
            ret = ESString::concatTwoStrings(ret, arg);
        }
        return ret;
    }, strings->concat));

    // $21.1.3.8 String.prototype.indexOf(searchString[, position])
    m_stringPrototype->defineDataProperty(strings->indexOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisObject = instance->currentExecutionContext()->resolveThisBinding();
        if (thisObject.isUndefinedOrNull())
            throw ESValue(TypeError::create(ESString::create("String.prototype.indexOf: this is undefined or null")));
        const u16string& str = instance->currentExecutionContext()->resolveThisBinding().toString()->string();
        escargot::ESString* searchStr = instance->currentExecutionContext()->readArgument(0).toString();

        ESValue val;
        if (instance->currentExecutionContext()->argumentCount() > 1)
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
    }, strings->indexOf));

    // $21.1.3.9 String.prototype.lastIndexOf ( searchString [ , position ] )
    m_stringPrototype->defineDataProperty(strings->lastIndexOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // Let O be RequireObjectCoercible(this value).
        ESValue O = instance->currentExecutionContext()->resolveThisBinding();
        if (O.isUndefinedOrNull())
            throw ESValue(TypeError::create(ESString::create("String.prototype.lastIndexOf: this is undefined or null")));
        // Let S be ToString(O).
        escargot::ESString* S = O.toString();
        escargot::ESString* searchStr = instance->currentExecutionContext()->readArgument(0).toString();

        double numPos = instance->currentExecutionContext()->readArgument(1).toNumber();
        double pos;
        // If numPos is NaN, let pos be +∞; otherwise, let pos be ToInteger(numPos).
        if (isnan(numPos))
            pos = std::numeric_limits<double>::infinity();
        else
            pos = numPos;

        double len = S->length();
        double start = std::min(std::max(pos, 0.0), len);
        int result = S->string().find_last_of(searchStr->string(), start);
        if (result != -1) {
            result -= (searchStr->length() - 1);
        }

        return ESValue(result);
    }, strings->lastIndexOf));

    // $21.1.3.11 String.prototype.match(regexp)
    m_stringPrototype->defineDataProperty(ESString::create(u"match"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESString* thisObject = instance->currentExecutionContext()->resolveThisBinding().toString();
        escargot::ESArrayObject* ret = ESArrayObject::create(0);

        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount > 0) {
            ESPointer* esptr = instance->currentExecutionContext()->arguments()[0].asESPointer();
            ESString::RegexMatchResult result;
            thisObject->match(esptr, result);

            const char16_t* str = thisObject->data();
            int idx = 0;
            for (unsigned i = 0; i < result.m_matchResults.size() ; i ++) {
                for (unsigned j = 0; j < result.m_matchResults[i].size() ; j ++) {
                    if (std::numeric_limits<unsigned>::max() == result.m_matchResults[i][j].m_start)
                        ret->set(idx++, ESValue(strings->emptyString.string()));
                    else
                        ret->set(idx++, ESString::create(std::move(u16string(str + result.m_matchResults[i][j].m_start, str + result.m_matchResults[i][j].m_end))));
                }
            }
            if (ret->length() == 0)
                return ESValue(ESValue::ESNull);
        }
        return ret;
    }, ESString::create(u"match")));

    // $21.1.3.14 String.prototype.replace(searchValue, replaceValue)
    m_stringPrototype->defineDataProperty(ESString::create(u"replace"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESString* thisObject = instance->currentExecutionContext()->resolveThisBinding().toString();
        escargot::ESArrayObject* ret = ESArrayObject::create(0);
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount > 1) {
            ESPointer* esptr = instance->currentExecutionContext()->arguments()[0].asESPointer();
            escargot::ESString* origStr = thisObject;
            ESString::RegexMatchResult result;
            origStr->match(esptr, result);
            if (result.m_matchResults.size() == 0) {
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

                for (int32_t i = 0; i < matchCount ; i ++) {
                    int subLen = result.m_matchResults[i].size();
                    ESValue* arguments = (ESValue *)alloca((subLen+2)*sizeof(ESValue));
                    for (unsigned j = 0; j < (unsigned)subLen ; j ++) {
                        if (result.m_matchResults[i][j].m_start == std::numeric_limits<unsigned>::max())
                            RELEASE_ASSERT_NOT_REACHED(); // implement this case
                        arguments[j] = ESString::create(std::move(u16string(
                            origStr->data() + result.m_matchResults[i][j].m_start
                            , origStr->data() + result.m_matchResults[i][j].m_end
                            )));
                    }
                    arguments[subLen] = ESValue((int)result.m_matchResults[i][0].m_start);
                    arguments[subLen + 1] = origStr;
                    escargot::ESString* res = ESFunctionObject::call(instance, callee, instance->globalObject(), arguments, subLen + 2, false).toString();

                    newThis.append(res->string());
                    if (i < matchCount - 1) {
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
                if (replaceString->string().find('$') == u16string::npos) {
                    // flat replace
                    int32_t matchCount = result.m_matchResults.size();
                    if ((unsigned)replaceString->length() > ESRopeString::ESRopeStringCreateMinLimit) {
                        // create Rope string
                        u16string append(orgString, 0, result.m_matchResults[0][0].m_start);
                        escargot::ESString* newStr = ESString::create(std::move(append));
                        escargot::ESString* appendStr = nullptr;
                        for (int32_t i = 0; i < matchCount ; i ++) {
                            newStr = escargot::ESString::concatTwoStrings(newStr, replaceString);
                            if (i < matchCount - 1) {
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
                    for (int32_t i = 0; i < matchCount ; i ++) {
                        escargot::ESString* res = replaceString;
                        newThis.append(res->string());
                        if (i < matchCount - 1) {
                            newThis.append(orgString.begin() + result.m_matchResults[i][0].m_end, orgString.begin() + result.m_matchResults[i + 1][0].m_start);
                        }
                    }
                    newThis.append(orgString.begin() + result.m_matchResults[matchCount - 1][0].m_end, orgString.end());
                } else {
                    // dollar replace
                    int32_t matchCount = result.m_matchResults.size();

                    const u16string& dollarString = replaceString->string();
                    newThis.append(orgString.begin(), orgString.begin() + result.m_matchResults[0][0].m_start);
                    for (int32_t i = 0; i < matchCount ; i ++) {
                        for (unsigned j = 0; j < dollarString.size() ; j ++) {
                            if (dollarString[j] == '$' && (j + 1) < dollarString.size()) {
                                char16_t c = dollarString[j + 1];
                                if (c == '$') {
                                    newThis.push_back(dollarString[j]);
                                } else if (c == '&') {
                                    newThis.append(origStr->string().begin() + result.m_matchResults[i][0].m_start,
                                        origStr->string().begin() + result.m_matchResults[i][0].m_end);
                                } else if (c == '`') {
                                    // TODO
                                    RELEASE_ASSERT_NOT_REACHED();
                                } else if (c == '`') {
                                    // TODO
                                    RELEASE_ASSERT_NOT_REACHED();
                                } else if ('0' <= c && c <= '9') {
                                    // TODO support morethan 2-digits
                                    size_t idx = c - '0';
                                    if (idx < result.m_matchResults[i].size()) {
                                        newThis.append(origStr->string().begin() + result.m_matchResults[i][idx].m_start,
                                            origStr->string().begin() + result.m_matchResults[i][idx].m_end);
                                    } else {
                                        newThis.push_back('$');
                                        newThis.push_back(c);
                                    }
                                }
                                j++;
                            } else {
                                newThis.push_back(dollarString[j]);
                            }
                        }
                        if (i < matchCount - 1) {
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
    }, ESString::create(u"replace")));

    // $21.1.3.16 String.prototype.slice(start, end)
    m_stringPrototype->defineDataProperty(strings->slice, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        const u16string& str = instance->currentExecutionContext()->resolveThisBinding().toString()->string();
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
    }, strings->slice));

    // $21.1.3.17 String.prototype.split(separator, limit)
    m_stringPrototype->defineDataProperty(ESString::create(u"split"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // 1, 2

        // 3
        int argCount = instance->currentExecutionContext()->argumentCount();
        ESValue separator = argCount>0 ? instance->currentExecutionContext()->arguments()[0] : ESValue();
        /*
        if (!separator.isUndefinedOrNull()) {
            ESValue splitter = separator.toObject()
            RELEASE_ASSERT_NOT_REACHED(); // TODO
        }
        */
        if (separator.isESPointer() && separator.asESPointer()->isESRegExpObject()) {
            // 4, 5
            escargot::ESString* str = instance->currentExecutionContext()->resolveThisBinding().toString();

            // 6
            escargot::ESArrayObject* arr = ESArrayObject::create(0);

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
            if (lim == 0)
                return arr;

            // 15
            if (separator.isUndefined()) {
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
                if (!ret)
                    return ESValue(false);
                return ESValue(result.m_matchResults[0][0].m_end);
            };
            // 16
            if (s == 0) {
                ESValue z = splitMatch(str, 0, R);
                if (z != ESValue(false))
                    return arr;
                arr->set(0, str);
                return arr;
            }

            // 17
            int q = p;

            // 18
            while (q != s) {
                escargot::ESString::RegexMatchResult result;
                ESValue e = splitMatch(str, q, R);
                auto prev = R->option();
                R->setOption((escargot::ESRegExpObject::Option)(prev & ~escargot::ESRegExpObject::Option::Global));
                bool ret = str->match(R, result, false, (size_t)q);
                R->setOption(prev);
                if (e == ESValue(ESValue::ESFalseTag::ESFalse)) {
                    if ((double)lengthA == lim)
                        return arr;
                    escargot::ESString* T = str->substring(q, str->length());
                    arr->set(lengthA, ESValue(T));
                    return arr;
                } else {
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
            escargot::ESString* str = instance->currentExecutionContext()->resolveThisBinding().toString();

            // 6
            escargot::ESArrayObject* arr = ESArrayObject::create(0);

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
            if (lim == 0)
                return arr;

            // 15
            if (separator.isUndefined()) {
                arr->set(0, str);
                return arr;
            }

            // 16
            auto splitMatch = [] (const u16string& S, int q, const u16string& R) -> ESValue {
                int s = S.length();
                int r = R.length();
                if (q + r > s)
                    return ESValue(false);
                for (int i = 0; i < r; i++)
                    if (S.data()[q+i] != R.data()[i])
                        return ESValue(false);
                return ESValue(q+r);
            };
            // 16
            if (s == 0) {
                ESValue z = splitMatch(str->string(), 0, R);
                if (z != ESValue(false))
                    return arr;
                arr->set(0, str);
                return arr;
            }

            // 17
            int q = p;

            // 18
            while (q != s) {
                ESValue e = splitMatch(str->string(), q, R);
                if (e == ESValue(ESValue::ESFalseTag::ESFalse))
                    q++;
                else {
                    if (e.asInt32() == p)
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
    }, ESString::create(u"split")));

    // $21.1.3.19 String.prototype.substring(start, end)
    m_stringPrototype->defineDataProperty(ESString::create(u"substring"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisObject = instance->currentExecutionContext()->resolveThisBinding();
        if (thisObject.isUndefinedOrNull())
            throw TypeError::create(ESString::create("String.prototype.substring: this is undefined or null"));
        int argCount = instance->currentExecutionContext()->argumentCount();
        escargot::ESString* str = thisObject.toString();
        if (argCount == 0) {
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
    }, ESString::create(u"substring")));

    // $21.1.3.22 String.prototype.toLowerCase()
    m_stringPrototype->defineDataProperty(ESString::create(u"toLowerCase"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESString* str = instance->currentExecutionContext()->resolveThisBinding().toString();
        int strlen = str->string().length();
        u16string newstr(str->string());
        // TODO use ICU for this operation
        std::transform(newstr.begin(), newstr.end(), newstr.begin(), ::tolower);
        return ESString::create(std::move(newstr));
    }, ESString::create(u"toLowerCase")));
    m_stringPrototype->defineDataProperty(ESString::create(u"toLocalLowerCase"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESString* str = instance->currentExecutionContext()->resolveThisBinding().toString();
        int strlen = str->string().length();
        u16string newstr(str->string());
        // TODO use ICU for this operation
        std::transform(newstr.begin(), newstr.end(), newstr.begin(), ::tolower);
        return ESString::create(std::move(newstr));
    }, ESString::create(u"toLocalLowerCase")));

    // $21.1.3.24 String.prototype.toUpperCase()
    m_stringPrototype->defineDataProperty(ESString::create(u"toUpperCase"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESString* str = instance->currentExecutionContext()->resolveThisBinding().toString();
        int strlen = str->string().length();
        u16string newstr(str->string());
        // TODO use ICU for this operation
        std::transform(newstr.begin(), newstr.end(), newstr.begin(), ::toupper);
        return ESString::create(std::move(newstr));
    }, ESString::create(u"toUpperCase")));
    m_stringPrototype->defineDataProperty(ESString::create(u"toLocaleUpperCase"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESString* str = instance->currentExecutionContext()->resolveThisBinding().toString();
        int strlen = str->string().length();
        u16string newstr(str->string());
        // TODO use ICU for this operation
        std::transform(newstr.begin(), newstr.end(), newstr.begin(), ::toupper);
        return ESString::create(std::move(newstr));
    }, ESString::create(u"toLocaleUpperCase")));

    // $21.1.3.25 String.prototype.trim()
    m_stringPrototype->defineDataProperty(ESString::create(u"trim"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESString* str = instance->currentExecutionContext()->resolveThisBinding().toString();
        u16string newstr(str->string());

        // trim left
        while (newstr.length()) {
            if (esprima::isWhiteSpace(newstr[0]) || esprima::isLineTerminator(newstr[0])) {
                newstr.erase(newstr.begin());
            }
            break;
        }

        // trim right
        while (newstr.length()) {
            if (esprima::isWhiteSpace(newstr[newstr.length()-1]) || esprima::isLineTerminator(newstr[newstr.length()-1])) {
                newstr.erase(newstr.end()-1);
            }
            break;
        }

        return ESString::create(std::move(newstr));
    }, ESString::create(u"trim")));

    // $21.1.3.26 String.prototype.valueOf ( )
    m_stringPrototype->defineDataProperty(strings->valueOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // Let s be thisStringValue(this value).
        // Return s.
        // The abstract operation thisStringValue(value) performs the following steps:
        // If Type(value) is String, return value.
        // If Type(value) is Object and value has a [[StringData]] internal slot, then
        // Assert: value’s [[StringData]] internal slot is a String value.
        // Return the value of value’s [[StringData]] internal slot.
        // Throw a TypeError exception.
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        if (thisValue.isESString()) {
            return thisValue.toString();
        } else if (thisValue.isESPointer() && thisValue.asESPointer()->isESStringObject()) {
            return thisValue.asESPointer()->asESStringObject()->stringData();
        }
        throw ESValue(TypeError::create(ESString::create("String.prototyep.valueOf: this is not String")));
    }, strings->valueOf));


    // $B.2.3.1 String.prototype.substr (start, length)
    m_stringPrototype->defineDataProperty(ESString::create(u"substr"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESString* str = instance->currentExecutionContext()->resolveThisBinding().toString();
        if (instance->currentExecutionContext()->argumentCount() < 1) {
            return str;
        }
        double intStart = instance->currentExecutionContext()->arguments()[0].toInteger();
        double end;
        if (instance->currentExecutionContext()->argumentCount() > 1) {
            if (instance->currentExecutionContext()->arguments()[1].isUndefined()) {
                end = std::numeric_limits<double>::infinity();
            } else
                end = instance->currentExecutionContext()->arguments()[1].toInteger();
        } else {
            end = std::numeric_limits<double>::infinity();
        }
        double size = str->length();
        if (intStart < 0)
            intStart = std::max(size + intStart, 0.0);
        double resultLength = std::min(std::max(end, 0.0), size - intStart);
        if (resultLength <= 0)
            return strings->emptyString.string();
        return str->substring(intStart, intStart + resultLength);
    }, ESString::create(u"substr")));

    m_stringObjectProxy = ESStringObject::create();
    m_stringObjectProxy->set__proto__(m_string->protoType());
}

void GlobalObject::installDate()
{
    m_datePrototype = ESDateObject::create();
    m_datePrototype->forceNonVectorHiddenClass();
    m_datePrototype->set__proto__(m_objectPrototype);

    // $20.3.2 The Date Constructor
    m_date = ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* proto = instance->globalObject()->datePrototype();
        if (instance->currentExecutionContext()->isNewExpression()) {
            escargot::ESDateObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject()->asESDateObject();

            size_t arg_size = instance->currentExecutionContext()->argumentCount();
            if (arg_size == 0) {
                thisObject->setTimeValue();
            } else if (arg_size == 1) {
                ESValue str = instance->currentExecutionContext()->arguments()[0];
                thisObject->setTimeValue(str);
            } else {
                int year = instance->currentExecutionContext()->readArgument(0).toNumber();
                if (year >= 0 && year <= 99) {
                    year += 1900;
                }
                int month = instance->currentExecutionContext()->readArgument(1).toNumber();
                int date;
                if (instance->currentExecutionContext()->readArgument(2).isUndefined()) {
                    date = 1;
                } else {
                    date = instance->currentExecutionContext()->readArgument(2).toNumber();
                }
                int hour = instance->currentExecutionContext()->readArgument(3).toNumber();
                int minute = instance->currentExecutionContext()->readArgument(4).toNumber();
                int second = instance->currentExecutionContext()->readArgument(5).toNumber();
                // TODO : have to implement millisecond
                int millisecond = instance->currentExecutionContext()->readArgument(6).toNumber();

                thisObject->setTimeValue(year, month, date, hour, minute, second, millisecond);
            }
        }
        return ESString::create(u"FixMe: We have to return string with date and time data");
    }, strings->Date, 7); // $20.3.3 Properties of the Date Constructor: the length property is 7.
    m_date->forceNonVectorHiddenClass();

    m_datePrototype->defineDataProperty(strings->toString, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // http://www.ecma-international.org/ecma-262/5.1/#sec-15.9.5.2
        // TODO
        ESValue e = instance->currentExecutionContext()->resolveThisBinding();
        if (e.isESPointer() && e.asESPointer()->isESDateObject()) {
            escargot::ESDateObject* obj = e.asESPointer()->asESDateObject();
            char buffer[512]; // TODO consider buffer-overflow
            sprintf(buffer, "%d-%02d-%02d %02d:%02d:%02d"
                , obj->getFullYear(), obj->getMonth() + 1, obj->getDate(), obj->getHours(), obj->getMinutes(), obj->getSeconds());
            return ESString::create(buffer);
        } else {
            return strings->emptyString.string();
        }
    }, strings->toString));

    m_date->setProtoType(m_datePrototype);

    m_datePrototype->defineDataProperty(strings->constructor, true, false, true, m_date);

    defineDataProperty(strings->Date, true, false, true, m_date);

    // $20.3.3.1 Date.now()
    m_date->defineDataProperty(ESString::create(u"now"), true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        struct timespec nowTime;
        clock_gettime(CLOCK_REALTIME, &nowTime);
        double ret = nowTime.tv_sec*1000 + floor(nowTime.tv_nsec / 1000000);
        return ESValue(ret);
    }, ESString::create(u"now")));

    // $20.3.4.2 Date.prototype.getDate()
    m_datePrototype->defineDataProperty(strings->getDate, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
        int ret = thisObject->asESDateObject()->getDate();
        return ESValue(ret);
    }, strings->getDate));

    // $20.3.4.3 Date.prototype.getDay()
    m_datePrototype->defineDataProperty(strings->getDay, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
        int ret = thisObject->asESDateObject()->getDay();
        return ESValue(ret);
    }, strings->getDay));

    // $20.3.4.4 Date.prototype.getFullYear()
    m_datePrototype->defineDataProperty(strings->getFullYear, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
        int ret = thisObject->asESDateObject()->getFullYear();
        return ESValue(ret);
    }, strings->getFullYear));

    // $20.3.4.5 Date.prototype.getHours()
    m_datePrototype->defineDataProperty(strings->getHours, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
        int ret = thisObject->asESDateObject()->getHours();
        return ESValue(ret);
    }, strings->getHours));

    // $20.3.4.7 Date.prototype.getMinutes()
    m_datePrototype->defineDataProperty(strings->getMinutes, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
        int ret = thisObject->asESDateObject()->getMinutes();
        return ESValue(ret);
    }, strings->getMinutes));

    // $20.3.4.8 Date.prototype.getMonth()
    m_datePrototype->defineDataProperty(strings->getMonth, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
        int ret = thisObject->asESDateObject()->getMonth();
        return ESValue(ret);
    }, strings->getMonth));

    // $20.3.4.9 Date.prototype.getSeconds()
    m_datePrototype->defineDataProperty(strings->getSeconds, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
        int ret = thisObject->asESDateObject()->getSeconds();
        return ESValue(ret);
    }, strings->getSeconds));

    // $20.3.4.10 Date.prototype.getTime()
    m_datePrototype->defineDataProperty(strings->getTime, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
        double ret = thisObject->asESDateObject()->getTimeAsMilisec();
        return ESValue(ret);
    }, strings->getTime));

    // $20.3.4.11 Date.prototype.getTimezoneOffset()
    m_datePrototype->defineDataProperty(strings->getTimezoneOffset, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
        int ret = thisObject->asESDateObject()->getTimezoneOffset();
        return ESValue(ret);
    }, strings->getTimezoneOffset));

    // $20.3.4.27 Date.prototype.setTime()
    m_datePrototype->defineDataProperty(strings->setTime, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();

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
    }, strings->setTime));

    // $44 Date.prototype.valueOf()
    m_datePrototype->defineDataProperty(strings->valueOf, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
        double ret = thisObject->asESDateObject()->getTimeAsMilisec();
        return ESValue(ret);
    }, strings->getTime));
}

void GlobalObject::installJSON()
{
    // create JSON object
    m_json = ESObject::create();
    m_json->forceNonVectorHiddenClass();
    m_json->set__proto__(m_objectPrototype);
    defineDataProperty(strings->JSON, true, false, true, m_json);

    // $24.3.1 JSON.parse(text[, reviver])
    m_json->defineDataProperty(strings->parse, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        if (!instance->currentExecutionContext()->readArgument(1).isUndefined())
            RELEASE_ASSERT_NOT_REACHED(); // implement reviver
        escargot::ESString* str = instance->currentExecutionContext()->readArgument(0).toString();
        // FIXME spec says we should we ECMAScript parser instead of json parser
        /*
        // FIXME json parser can not parse this form
        u16string src;
        src.append(u"(");
        src.append(str->string());
        src.append(u") ;");
        */

        rapidjson::GenericDocument<rapidjson::UTF16<char16_t>> jsonDocument;

        // FIXME(ksh8281) javascript string is not null-terminated string
        rapidjson::GenericStringStream<rapidjson::UTF16<char16_t>> stringStream(str->data());
        jsonDocument.ParseStream(stringStream);
        if (jsonDocument.HasParseError()) {
            throw ESValue(SyntaxError::create(ESString::create(u"occur error while parse json")));
        }
        std::function<ESValue(rapidjson::GenericValue<rapidjson::UTF16<char16_t>>& value)> fn;
        fn = [&](rapidjson::GenericValue<rapidjson::UTF16<char16_t>>& value) -> ESValue {
            if (value.IsBool()) {
                return ESValue(value.GetBool());
            } else if (value.IsInt()) {
                return ESValue(value.GetInt());
            } else if (value.IsDouble()) {
                return ESValue(value.GetDouble());
            } else if (value.IsNull()) {
                return ESValue(ESValue::ESNull);
            } else if (value.IsString()) {
                return ESString::create(value.GetString());
            } else if (value.IsArray()) {
                escargot::ESArrayObject* arr = ESArrayObject::create();
                auto iter = value.Begin();
                while (iter != value.End()) {
                    arr->push(fn(*iter));
                    iter++;
                }
                return arr;
            } else if (value.IsObject()) {
                escargot::ESObject* obj = ESObject::create();
                auto iter = value.MemberBegin();
                while (iter != value.MemberEnd()) {
                    obj->defineDataProperty(ESString::create(iter->name.GetString()), true, false, true, fn(iter->value));
                    iter++;
                }
                return obj;
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
        };
        return fn(jsonDocument);
    }, strings->parse));

    // $24.3.2 JSON.stringify(value[, replacer[, space ]])
    m_json->defineDataProperty(strings->stringify, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount < 1) {
            return ESValue();
        } else {
            ESValue value = instance->currentExecutionContext()->arguments()[0];
            ESValue replacer;
            ESValue space;
            if (argCount >= 2)
                replacer = instance->currentExecutionContext()->arguments()[1];
            if (argCount >= 3)
                space = instance->currentExecutionContext()->arguments()[2];
            if (value.isObject()) {
                ESObject* valObj = value.asESPointer()->asESObject();
                ESValue toJson = valObj->get(strings->toJSON.string());
                if (toJson.isESPointer() && toJson.asESPointer()->isESFunctionObject()) {
                    value = ESFunctionObject::call(instance, toJson, value, NULL, 0, false);
                }
            }
            if (!replacer.isUndefined()) {
                RELEASE_ASSERT_NOT_REACHED(); // TODO
            }

            typedef rapidjson::GenericStringBuffer<rapidjson::UTF16<char16_t>> StringBuffer;
            typedef rapidjson::GenericValue<rapidjson::UTF16<char16_t>> Value;
            typedef rapidjson::GenericDocument<rapidjson::UTF16<char16_t>> Document;
            StringBuffer buf;
            rapidjson::Writer<StringBuffer, rapidjson::UTF16<char16_t>> writer(buf);
            Document jsonDocument;
            Document::AllocatorType& allocator = jsonDocument.GetAllocator();

            std::function<Value(ESValue value)> sfn;
            sfn = [&](ESValue value) -> Value {
                if (value.isNumber()) {
                    Value s;
                    if (value.isInt32()) {
                        s.SetInt(value.toInt32());
                    } else {
                        double valNum = value.toNumber();
                        if (std::isnan(valNum) || valNum == std::numeric_limits<double>::infinity() || valNum == -std::numeric_limits<double>::infinity())
                            return s;
                        s.SetDouble(valNum);
                    }
                    return s;
                } else if (value.isBoolean()) {
                    Value s(value.toBoolean());
                    return s;
                } else if (value.isNull()) {
                    return Value();
                } else if (value.isESString()) {
                    Value s;
                    escargot::ESString* str = value.toString();
                    s.SetString(str->data(), str->length(), allocator);
                    return s;
                } else if (value.isObject() && value.asESPointer()->asESObject()->isESArrayObject()) {
                    escargot::ESArrayObject* arr = value.toObject()->asESArrayObject();
                    Value s;
                    s.SetArray();
                    for (int i = 0; i < arr->length(); i++) {
                        s.PushBack(sfn(arr->get(i)), allocator);
                    }
                    return s;
                } else if (value.isObject()) {
                    Value s;
                    s.SetObject();
                    ESObject* obj = value.toObject();
                    obj->enumeration([&](ESValue key) {
                        ESValue res = obj->get(key);
                        if (!res.isUndefined())
                            s.AddMember(sfn(key), sfn(res), allocator);
                    });
                    return s;
                }
                return Value();
            };
            Value root = sfn(value);
            root.Accept(writer);
            return ESString::create(buf.GetString());
        }
    }, strings->stringify));
}

void GlobalObject::installMath()
{
    // create math object
    m_math = ::escargot::ESObject::create();
    m_math->forceNonVectorHiddenClass();

    // initialize math object: $20.2.1.6 Math.PI
    m_math->defineDataProperty(strings->PI, false, false, false, ESValue(3.1415926535897932));
    // TODO(add reference)
    m_math->defineDataProperty(strings->E, false, false, false, ESValue(2.718281828459045));
    // http://www.ecma-international.org/ecma-262/5.1/#sec-15.8.1.3
    m_math->defineDataProperty(escargot::ESString::create(u"LN2"), false, false, false, ESValue(0.6931471805599453));
    // http://www.ecma-international.org/ecma-262/5.1/#sec-15.8.1.2
    m_math->defineDataProperty(escargot::ESString::create(u"LN10"), false, false, false, ESValue(2.302585092994046));
    // http://www.ecma-international.org/ecma-262/5.1/#sec-15.8.1.4
    m_math->defineDataProperty(escargot::ESString::create(u"LOG2E"), false, false, false, ESValue(1.4426950408889634));
    // http://www.ecma-international.org/ecma-262/5.1/#sec-15.8.1.5
    m_math->defineDataProperty(escargot::ESString::create(u"LOG10E"), false, false, false, ESValue(0.4342944819032518));
    // http://www.ecma-international.org/ecma-262/5.1/#sec-15.8.1.7
    m_math->defineDataProperty(escargot::ESString::create(u"SQRT1_2"), false, false, false, ESValue(0.7071067811865476));
    // http://www.ecma-international.org/ecma-262/5.1/#sec-15.8.1.8
    m_math->defineDataProperty(escargot::ESString::create(u"SQRT2"), false, false, false, ESValue(1.4142135623730951));

    // initialize math object: $20.2.2.1 Math.abs()
    m_math->defineDataProperty(strings->abs, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            return ESValue(value);
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            double value = std::abs(arg.toNumber());
            return ESValue(value);
        }
        return ESValue();
    }, strings->abs));

    // initialize math object: $20.2.2.16 Math.ceil()
    m_math->defineDataProperty(strings->ceil, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            return ESValue(value);
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            double value = ceil(arg.toNumber());
            return ESValue(value);
        }

        return ESValue();
    }, strings->ceil));

    // initialize math object: $20.2.2.12 Math.cos()
    m_math->defineDataProperty(strings->cos, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
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
    }, strings->cos));

    // initialize math object: $20.2.2.16 Math.floor()
    m_math->defineDataProperty(strings->floor, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            return ESValue(value);
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            double value = floor(arg.toNumber());
            return ESValue(value);
        }

        return ESValue();
    }, strings->floor));

    // initialize math object: $20.2.2.20 Math.log()
    m_math->defineDataProperty(strings->log, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double value = std::numeric_limits<double>::quiet_NaN();
            return ESValue(value);
        } else {
            ESValue arg = instance->currentExecutionContext()->arguments()[0];
            double value = log(arg.toNumber());
            return ESValue(value);
        }
        return ESValue();
    }, strings->log));

    // initialize math object: $20.2.2.24 Math.max()
    m_math->defineDataProperty(strings->max, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            double n_inf = -1 * std::numeric_limits<double>::infinity();
            return ESValue(n_inf);
        } else  {
            double max_value = instance->currentExecutionContext()->arguments()[0].toNumber();
            for (unsigned i = 1; i < arg_size; i++) {
                double value = instance->currentExecutionContext()->arguments()[i].toNumber();
                double qnan = std::numeric_limits<double>::quiet_NaN();
                if (std::isnan(value))
                    return ESValue(qnan);
                if (value > max_value)
                    max_value = value;
            }
            return ESValue(max_value);
        }
        return ESValue();
    }, strings->max, 2));

    // initialize math object: $20.2.2.25 Math.min()
    m_math->defineDataProperty(strings->min, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size == 0) {
            return ESValue(std::numeric_limits<double>::infinity());
        } else {
            double min_value = instance->currentExecutionContext()->arguments()[0].toNumber();
            for (unsigned i = 1; i < arg_size; i++) {
                double value = instance->currentExecutionContext()->arguments()[i].toNumber();
                double qnan = std::numeric_limits<double>::quiet_NaN();
                if (std::isnan(value))
                    return ESValue(qnan);
                if (value < min_value)
                min_value = value;
            }
            return ESValue(min_value);
        }
        return ESValue();
    }, strings->min, 2));

    // initialize math object: $20.2.2.26 Math.pow()
    m_math->defineDataProperty(strings->pow, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
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
    }, strings->pow));

    // initialize math object: $20.2.2.27 Math.random()
    m_math->defineDataProperty(strings->random, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        double rand = (double) std::rand() / RAND_MAX;
        return ESValue(rand);
    }, strings->random));

    // initialize math object: $20.2.2.28 Math.round()
    m_math->defineDataProperty(strings->round, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
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
    }, strings->round));

    // initialize math object: $20.2.2.30 Math.sin()
    m_math->defineDataProperty(strings->sin, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
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
    }, strings->sin));

    // initialize math object: $20.2.2.32 Math.sqrt()
    m_math->defineDataProperty(strings->sqrt, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
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
    }, strings->sqrt));

    // initialize math object: $20.2.2.33 Math.tan()
    m_math->defineDataProperty(strings->tan, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        double x = instance->currentExecutionContext()->readArgument(0).toNumber();
        /*
        If x is NaN, the result is NaN.
        If x is +0, the result is +0.
        If x is −0, the result is −0.
        If x is +∞ or −∞, the result is NaN.
        */
        if (isnan(x))
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        else if (x == 0.0) {
            if (std::signbit(x)) {
                return ESValue(ESValue::EncodeAsDouble, -0.0);
            } else {
                return ESValue(0);
            }
        } else if (std::isinf(x))
            return ESValue(std::numeric_limits<double>::quiet_NaN());
        return ESValue(tan(x));
    }, strings->tan));

    // add math to global object
    defineDataProperty(strings->Math, true, false, true, m_math);
}

static int itoa(int value, char *sp, int radix)
{
    char tmp[16]; // be careful with the length of the buffer
    char* tp = tmp;
    int i;
    unsigned v;

    int sign = (radix == 10 && value < 0);
    if (sign)
        v = -value;
    else
        v = (unsigned)value;

    while (v || tp == tmp) {
        i = v % radix;
        v /= radix; // v/=radix uses less CPU clocks than v=v/radix does
        if (i < 10)
            *tp++ = i+'0';
        else
            *tp++ = i + 'a' - 10;
    }

    int len = tp - tmp;

    if (sign) {
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
    m_number = ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        if (instance->currentExecutionContext()->isNewExpression()) {
            if (instance->currentExecutionContext()->argumentCount())
                instance->currentExecutionContext()->resolveThisBindingToObject()->asESNumberObject()->setNumberData(instance->currentExecutionContext()->readArgument(0).toNumber());
            return instance->currentExecutionContext()->resolveThisBinding();
        } else {
            return ESValue(instance->currentExecutionContext()->readArgument(0).toNumber());
        }
    }, strings->Number);
    m_number->forceNonVectorHiddenClass();

    // create numberPrototype object
    m_numberPrototype = ESNumberObject::create(0.0);
    m_numberPrototype->forceNonVectorHiddenClass();

    // initialize number object
    m_number->setProtoType(m_numberPrototype);

    m_numberPrototype->defineDataProperty(strings->constructor, true, false, true, m_number);

    // $ 20.1.2.6 Number.MAX_SAFE_INTEGER
    m_number->defineDataProperty(ESString::create(u"MAX_SAFE_INTEGER"), false, false, false, ESValue(9007199254740991.0));
    // $ 20.1.2.7 Number.MAX_VALUE
    m_number->defineDataProperty(strings->MAX_VALUE, false, false, false, ESValue(1.7976931348623157E+308));
    // $ 20.1.2.8 Number.MIN_SAFE_INTEGER
    m_number->defineDataProperty(ESString::create(u"MIN_SAFE_INTEGER"), false, false, false, ESValue(ESValue::EncodeAsDouble, -9007199254740991.0));
    // $ 20.1.2.9 Number.MIN_VALUE
    m_number->defineDataProperty(strings->MIN_VALUE, false, false, false, ESValue(5E-324));
    // $ 20.1.2.10 Number.NaN
    m_number->defineDataProperty(strings->NaN, false, false, false, ESValue(std::numeric_limits<double>::quiet_NaN()));
    // $ 20.1.2.11 Number.NEGATIVE_INFINITY
    m_number->defineDataProperty(strings->NEGATIVE_INFINITY, false, false, false, ESValue(-std::numeric_limits<double>::infinity()));
    // $ 20.1.2.14 Number.POSITIVE_INFINITY
    m_number->defineDataProperty(strings->POSITIVE_INFINITY, false, false, false, ESValue(std::numeric_limits<double>::infinity()));

    // initialize numberPrototype object
    m_numberPrototype->set__proto__(m_objectPrototype);

    // initialize numberPrototype object: $20.1.3.3 Number.prototype.toFixed(fractionDigits)
    m_numberPrototype->defineDataProperty(strings->toFixed, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        double number = instance->currentExecutionContext()->resolveThisBinding().toNumber();
        int arglen = instance->currentExecutionContext()->argumentCount();
        if (arglen == 0) {
            return ESValue(round(number)).toString();
        } else if (arglen == 1) {
            double digit_d = instance->currentExecutionContext()->arguments()[0].toNumber();
            if (digit_d == 0 || isnan(digit_d)) {
                return ESValue(round(number)).toString();
            }
            int digit = (int) trunc(digit_d);
            if (digit < 0 || digit > 20) {
                throw ESValue(RangeError::create());
            }
            if (isnan(number)) {
                return strings->NaN.string();
            } else if (number >= pow(10, 21)) {
                return ESValue(round(number)).toString();
            }

            std::basic_ostringstream<char> stream;
            stream << "%." << digit << "lf";
            std::string fstr = stream.str();
            char buf[512];
            sprintf(buf, fstr.c_str(), number);
            return ESValue(ESString::create(buf));
        }
        return ESValue();
    }, strings->toFixed));

    m_numberPrototype->defineDataProperty(strings->toPrecision, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        double number = instance->currentExecutionContext()->resolveThisBinding().toNumber();
        int arglen = instance->currentExecutionContext()->argumentCount();
        if (arglen == 0 || instance->currentExecutionContext()->arguments()[0].isUndefined()) {
            return ESValue(number).toString();
        } else if (arglen == 1) {
            double x = number;
            int p = instance->currentExecutionContext()->arguments()[0].toInteger();
            if (isnan(x)) {
                return strings->NaN.string();
            }
            u16string s;
            if (x < 0) {
                s = u"-";
                x = -x;
            }
            if (std::isinf(x)) {
                s += u"Infinity";
                return escargot::ESString::create(std::move(s));
            }

            if (p < 1 && p > 21) {
                throw ESValue(RangeError::create());
            }

            x = number;
            std::basic_ostringstream<char> stream;
            stream << "%." << p << "lf";
            std::string fstr = stream.str();
            char buf[512];
            sprintf(buf, fstr.c_str(), x);
            return ESValue(ESString::create(buf));
        }

        return ESValue();
    }, strings->toPrecision));

    // initialize numberPrototype object: $20.1.3.6 Number.prototype.toString()
    m_numberPrototype->defineDataProperty(strings->toString, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        double number = instance->currentExecutionContext()->resolveThisBinding().toNumber();
        int arglen = instance->currentExecutionContext()->argumentCount();
        double radix = 10;
        if (arglen >= 1) {
            radix = instance->currentExecutionContext()->arguments()[0].toInteger();
            if (radix < 2 || radix > 36)
                throw ESValue(RangeError::create(ESString::create(u"String.prototype.toString() radix is not in valid range")));
        }
        if (radix == 10)
            return (ESValue(number).toString());
        else {
            char buffer[256];
            int len = itoa((int)number, buffer, radix);
            return (ESString::create(buffer));
        }
        return ESValue();
    }, strings->toString));

    // $20.1.3.26 Number.prototype.valueOf ( )
    m_numberPrototype->defineDataProperty(strings->valueOf, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // Let s be thisNumberValue(this value).
        // Return s.
        // The abstract operation thisNumberValue(value) performs the following steps:
        // If Type(value) is Number, return value.
        // If Type(value) is Object and value has a [[NumberData]] internal slot, then
        // Assert: value’s [[NumberData]] internal slot is a Number value.
        // Return the value of value’s [[NumberData]] internal slot.
        // Throw a TypeError exception.
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        if (thisValue.isNumber()) {
            return ESValue(thisValue.asNumber());
        } else if (thisValue.isESPointer() && thisValue.asESPointer()->isESNumberObject()) {
            return ESValue(thisValue.asESPointer()->asESNumberObject()->numberData());
        }
        throw ESValue(TypeError::create(ESString::create("Number.prototype.valueOf: this is not number")));
    }, strings->valueOf));

    // add number to global object
    defineDataProperty(strings->Number, true, false, true, m_number);

    m_numberObjectProxy = ESNumberObject::create(0);
    m_numberObjectProxy->set__proto__(m_numberPrototype);
}

void GlobalObject::installBoolean()
{
    // create number object: $19.3.1 The Boolean Constructor
    m_boolean = ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
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

        if (instance->currentExecutionContext()->isNewExpression() && instance->currentExecutionContext()->resolveThisBindingToObject()->isESBooleanObject()) {
            ::escargot::ESBooleanObject* o = instance->currentExecutionContext()->resolveThisBindingToObject()->asESBooleanObject();
            o->setBooleanData(ret.toBoolean());
            return (o);
        } else // If NewTarget is undefined, return b
            return (ret);
        return ESValue();
    }, strings->Boolean);
    m_boolean->forceNonVectorHiddenClass();

    // create booleanPrototype object
    m_booleanPrototype = ESBooleanObject::create(false);
    m_booleanPrototype->forceNonVectorHiddenClass();
    m_booleanPrototype->set__proto__(m_objectPrototype);

    // initialize boolean object
    m_boolean->setProtoType(m_booleanPrototype);

    // initialize booleanPrototype object
    m_booleanPrototype->set__proto__(m_objectPrototype);

    m_booleanPrototype->defineDataProperty(strings->constructor, true, false, true, m_boolean);

    // initialize booleanPrototype object: $19.3.3.2 Boolean.prototype.toString()
    m_booleanPrototype->defineDataProperty(strings->toString, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        escargot::ESBooleanObject* thisVal = instance->currentExecutionContext()->resolveThisBindingToObject()->asESBooleanObject();
        return (ESValue(thisVal->booleanData()).toString());
    }, strings->toString));

    // initialize booleanPrototype object: $19.3.3.3 Boolean.prototype.valueOf()
    m_booleanPrototype->defineDataProperty(strings->valueOf, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        if (thisValue.isBoolean()) {
            return ESValue(thisValue.asNumber());
        } else if (thisValue.isESPointer() && thisValue.asESPointer()->isESBooleanObject()) {
            return ESValue(thisValue.asESPointer()->asESBooleanObject()->booleanData());
        }
        throw ESValue(TypeError::create(strings->emptyString));
    }, strings->valueOf));

    // add number to global object
    defineDataProperty(strings->Boolean, true, false, true, m_boolean);
}

void GlobalObject::installRegExp()
{
    // create regexp object: $21.2.3 The RegExp Constructor
    m_regexp = ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESValue thisValue = instance->currentExecutionContext()->resolveThisBinding();
        escargot::ESRegExpObject* thisVal;
        if (thisValue.isUndefined()) {
            thisVal = ESRegExpObject::create(strings->emptyString.string(), ESRegExpObject::Option::None);
            thisVal->set__proto__(instance->globalObject()->regexpPrototype());
        } else {
            thisVal = thisValue.toObject()->asESRegExpObject();
        }
        size_t arg_size = instance->currentExecutionContext()->argumentCount();
        if (arg_size > 0) {
            ESValue pattern = instance->currentExecutionContext()->arguments()[0];
            if (pattern.isESPointer() && pattern.asESPointer()->isESRegExpObject()) {
                if (instance->currentExecutionContext()->readArgument(1).isUndefined())
                    return pattern;
                else
                    throw ESValue(TypeError::create(ESString::create(u"Cannot supply flags when constructing one RegExp from another")));
            }
            thisVal->setSource(pattern.toString());
        }

        if (arg_size > 1) {
            escargot::ESString* is = instance->currentExecutionContext()->arguments()[1].toString();
            const u16string& str = static_cast<const u16string&>(is->string());
            ESRegExpObject::Option option = ESRegExpObject::Option::None;
            if (str.find('g') != u16string::npos) {
                option = (ESRegExpObject::Option) (option | ESRegExpObject::Option::Global);
            }
            if (str.find('i') != u16string::npos) {
                option = (ESRegExpObject::Option) (option | ESRegExpObject::Option::IgnoreCase);
            }
            if (str.find('m') != u16string::npos) {
                option = (ESRegExpObject::Option) (option | ESRegExpObject::Option::MultiLine);
            }
            if (str.find('y') != u16string::npos) {
                option = (ESRegExpObject::Option) (option | ESRegExpObject::Option::Sticky);
            }
            thisVal->setOption(option);
        }
        return ESValue(thisVal);
    }, strings->RegExp);
    m_regexp->forceNonVectorHiddenClass();

    // create regexpPrototype object
    m_regexpPrototype = ESRegExpObject::create(strings->emptyString, ESRegExpObject::Option::None);
    m_regexpPrototype->forceNonVectorHiddenClass();
    m_regexpPrototype->set__proto__(m_objectPrototype);

    m_regexpPrototype->defineDataProperty(strings->constructor, true, false, true, m_regexp);

    // initialize regexp object
    m_regexp->setProtoType(m_regexpPrototype);

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
    m_regexpPrototype->defineDataProperty(strings->test, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
        ::escargot::ESRegExpObject* regexp = thisObject->asESRegExpObject();
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount > 0) {
            escargot::ESString* sourceStr = instance->currentExecutionContext()->arguments()[0].toString();
            ESString::RegexMatchResult result;
            bool testResult = sourceStr->match(thisObject, result, true);
            return (ESValue(testResult));
        }
        return ESValue(false);
    }, strings->test));

    // 21.2.5.2 RegExp.prototype.exec( string )
    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-regexp.prototype.test
    m_regexpPrototype->defineDataProperty(strings->exec, true, false, true, ::escargot::ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        ESObject* thisObject = instance->currentExecutionContext()->resolveThisBindingToObject();
        if (!thisObject->isESRegExpObject())
            throw TypeError::create();
        ::escargot::ESRegExpObject* regexp = thisObject->asESRegExpObject();
        int argCount = instance->currentExecutionContext()->argumentCount();
        if (argCount > 0) {
            escargot::ESString* sourceStr = instance->currentExecutionContext()->arguments()[0].toString();
            if (sourceStr == regexp->m_lastExecutedString || (regexp->m_lastExecutedString && sourceStr->string() == regexp->m_lastExecutedString->string())) {

            } else {
                regexp->m_lastIndex = 0;
            }
            regexp->m_lastExecutedString = sourceStr;
            ESString::RegexMatchResult result;
            bool isGlobal = regexp->option() & ESRegExpObject::Option::Global;
            regexp->setOption((ESRegExpObject::Option)(regexp->option() & ~ESRegExpObject::Option::Global));
            bool testResult = sourceStr->match(thisObject, result, false, regexp->m_lastIndex);
            if (isGlobal) {
                regexp->setOption((ESRegExpObject::Option)(regexp->option() | ESRegExpObject::Option::Global));
            }

            if (!testResult) {
                regexp->m_lastIndex = 0;
                return ESValue(ESValue::ESNull);
            }

            if (isGlobal) {
                // update lastIndex
                regexp->m_lastIndex = result.m_matchResults[0][0].m_end;
            }
            ::escargot::ESArrayObject* arr = ::escargot::ESArrayObject::create();
            ((ESObject *)arr)->set(ESValue(strings->input), ESValue(sourceStr));
            ((ESObject *)arr)->set(ESValue(strings->index), ESValue(result.m_matchResults[0][0].m_start));
            const char16_t* str = sourceStr->string().data();

            int idx = 0;
            for (unsigned i = 0; i < result.m_matchResults.size() ; i ++) {
                for (unsigned j = 0; j < result.m_matchResults[i].size() ; j ++) {
                    if (result.m_matchResults[i][j].m_start == std::numeric_limits<unsigned>::max())
                        arr->set(idx++, ESValue(strings->emptyString));
                    else
                        arr->set(idx++, ESString::create(std::move(u16string(str + result.m_matchResults[i][j].m_start, str + result.m_matchResults[i][j].m_end))));
                }
            }
            return arr;
        } else {
            return ESValue(ESValue::ESNull);
        }
    }, strings->exec));

    // add regexp to global object
    defineDataProperty(strings->RegExp, true, false, true, m_regexp);
}

void GlobalObject::installArrayBuffer()
{
    m_arrayBufferPrototype = ESArrayBufferObject::create();
    m_arrayBufferPrototype->forceNonVectorHiddenClass();
    m_arrayBufferPrototype->set__proto__(m_objectPrototype);
    m_arrayBufferPrototype->defineDataProperty(strings->constructor, true, false, true, m_arrayBuffer);

    // $24.1.2.1 ArrayBuffer(length)
    m_arrayBuffer = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // if NewTarget is undefined, throw a TypeError
        if (!instance->currentExecutionContext()->isNewExpression())
            throw ESValue(TypeError::create(ESString::create(u"Constructor ArrayBuffer requires \'new\'")));
        ASSERT(instance->currentExecutionContext()->resolveThisBindingToObject()->isESArrayBufferObject());
        escargot::ESArrayBufferObject* obj = instance->currentExecutionContext()->resolveThisBindingToObject()->asESArrayBufferObject();
        int len = instance->currentExecutionContext()->argumentCount();
        if (len == 0)
            obj->allocateArrayBuffer(0);
        else if (len >= 1) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            int numlen = val.toNumber();
            int elemlen = val.toLength();
            if (numlen != elemlen)
                throw ESValue(TypeError::create(ESString::create(u"Constructor ArrayBuffer : 1st argument is error")));
            obj->allocateArrayBuffer(elemlen);
        }
        return obj;
    }, strings->ArrayBuffer);
    m_arrayBuffer->forceNonVectorHiddenClass();

    // $22.2.3.2
    m_arrayBufferPrototype->defineAccessorProperty(strings->byteLength, [](ESObject* self) -> ESValue {
        return ESValue(self->asESArrayBufferObject()->bytelength());
    }, nullptr, true, false, false);

    m_arrayBuffer->set__proto__(m_functionPrototype); // empty Function
    m_arrayBuffer->setProtoType(m_arrayBufferPrototype);
    defineDataProperty(strings->ArrayBuffer, true, false, true, m_arrayBuffer);
}

void GlobalObject::installTypedArray()
{
    m_Int8Array = installTypedArray<Int8Adaptor> (strings->Int8Array);
    m_Int16Array = installTypedArray<Int16Adaptor>(strings->Int16Array);
    m_Int32Array = installTypedArray<Int32Adaptor>(strings->Int32Array);
    m_Uint8Array = installTypedArray<Uint8Adaptor>(strings->Uint8Array);
    m_Uint16Array = installTypedArray<Uint16Adaptor>(strings->Uint16Array);
    m_Uint32Array = installTypedArray<Uint32Adaptor>(strings->Uint32Array);
    m_Uint8ClampedArray = installTypedArray<Uint8Adaptor> (strings->Uint8ClampedArray);
    m_Float32Array = installTypedArray<Float32Adaptor> (strings->Float32Array);
    m_Float64Array = installTypedArray<Float64Adaptor>(strings->Float64Array);
    m_Int8ArrayPrototype = m_Int8Array->protoType().asESPointer()->asESObject();
    m_Int16ArrayPrototype = m_Int16Array->protoType().asESPointer()->asESObject();
    m_Int32ArrayPrototype = m_Int32Array->protoType().asESPointer()->asESObject();
    m_Uint8ArrayPrototype = m_Uint8Array->protoType().asESPointer()->asESObject();
    m_Uint16ArrayPrototype = m_Uint16Array->protoType().asESPointer()->asESObject();
    m_Uint32ArrayPrototype = m_Uint32Array->protoType().asESPointer()->asESObject();
    m_Float32ArrayPrototype = m_Float32Array->protoType().asESPointer()->asESObject();
    m_Float64ArrayPrototype = m_Float64Array->protoType().asESPointer()->asESObject();
}

template <typename T>
ESFunctionObject* GlobalObject::installTypedArray(escargot::ESString* ta_name)
{
    escargot::ESObject* ta_prototype = escargot::ESTypedArrayObject<T>::create();
    ta_prototype->forceNonVectorHiddenClass();
    ta_prototype->set__proto__(m_objectPrototype);

    // $22.2.1.1~
    escargot::ESFunctionObject* ta_constructor = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        // if NewTarget is undefined, throw a TypeError
        if (!instance->currentExecutionContext()->isNewExpression())
            throw ESValue(TypeError::create(ESString::create(u"Constructor TypedArray requires \'new\'")));
        ASSERT(instance->currentExecutionContext()->resolveThisBindingToObject()->isESTypedArrayObject());
        escargot::ESTypedArrayObject<T>* obj = instance->currentExecutionContext()->resolveThisBindingToObject()->asESTypedArrayObject<T>();
        int len = instance->currentExecutionContext()->argumentCount();
        if (len == 0) {
            obj->allocateTypedArray(0);
        } else if (len >= 1) {
            ESValue& val = instance->currentExecutionContext()->arguments()[0];
            if (!val.isObject()) {
                // $22.2.1.2 %TypedArray%(length)
                int numlen = val.toNumber();
                int elemlen = val.toLength();
                if (numlen != elemlen)
                    throw ESValue(RangeError::create(ESString::create(u"Constructor TypedArray : 1st argument is error")));
                obj->allocateTypedArray(elemlen);
            } else if (val.isESPointer() && val.asESPointer()->isESArrayBufferObject()) {
                // $22.2.1.5 %TypedArray%(buffer [, byteOffset [, length] ] )
                escargot::ESString* msg = ESString::create(u"ArrayBuffer length minus the byteOffset is not a multiple of the element size");
                unsigned elementSize = obj->elementSize();
                int offset = 0;
                ESValue lenVal;
                if (len >= 2)
                    offset = instance->currentExecutionContext()->arguments()[1].toInt32();
                if (offset < 0) {
                    throw ESValue(RangeError::create(msg));
                }
                if (offset % elementSize != 0) {
                    throw ESValue(RangeError::create(msg));
                }
                escargot::ESArrayBufferObject* buffer = val.asESPointer()->asESArrayBufferObject();
                unsigned bufferByteLength = buffer->bytelength();
                if (len >= 3) {
                    lenVal = instance->currentExecutionContext()->arguments()[2];
                }
                unsigned newByteLength;
                if (lenVal.isUndefined()) {
                    if (bufferByteLength % elementSize != 0)
                        throw ESValue(RangeError::create());
                    newByteLength = bufferByteLength - offset;
                    if (newByteLength < 0)
                        throw ESValue(RangeError::create(msg));
                } else {
                    int length = lenVal.toLength();
                    newByteLength = length * elementSize;
                    if (offset + newByteLength > bufferByteLength)
                        throw ESValue(RangeError::create(msg));
                }
                obj->setBuffer(buffer);
                obj->setBytelength(newByteLength);
                obj->setByteoffset(offset);
                obj->setArraylength(newByteLength / elementSize);
            } else if (val.isESPointer() && val.asESPointer()->isESObject()) {
                // TODO implement 22.2.1.4
                ESObject* inputObj = val.asESPointer()->asESObject();
                uint32_t length = inputObj->length();
                ASSERT(length >= 0);
                unsigned elementSize = obj->elementSize();
                escargot::ESArrayBufferObject *buffer = ESArrayBufferObject::createAndAllocate(length * elementSize);
                obj->setBuffer(buffer);
                obj->setBytelength(length * elementSize);
                obj->setByteoffset(0);
                obj->setArraylength(length);
                for (int32_t i = 0; i < length ; i ++) {
                    obj->set(i, inputObj->get(ESValue(i)));
                }
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
            // TODO
            ASSERT(obj->arraylength() < 210000000);
        }
        return obj;
    }, ta_name);

    ta_constructor->forceNonVectorHiddenClass();

    // $22.2.3.2
    ta_prototype->defineAccessorProperty(strings->byteLength, [](ESObject* self) -> ESValue {
        return ESValue(self->asESTypedArrayObject<T>()->bytelength());
    }, nullptr, true, false, false);
    // $22.2.3.2
    ta_prototype->defineAccessorProperty(strings->length, [](ESObject* self) -> ESValue {
        return ESValue(self->asESTypedArrayObject<T>()->arraylength());
    }, nullptr, true, false, false);

    // TODO add reference
    ta_prototype->defineAccessorProperty(strings->buffer, [](ESObject* self) -> ESValue {
        return ESValue(self->asESTypedArrayObject<T>()->buffer());
    }, nullptr, true, false, false);
    // $22.2.3.22 %TypedArray%.prototype.set(overloaded[, offset])
    ta_prototype->ESObject::defineDataProperty(strings->set, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        auto thisBinded = instance->currentExecutionContext()->resolveThisBindingToObject();
        if (!thisBinded->isESTypedArrayObject() || arglen < 1) {
            throw TypeError::create();
        }
        auto thisVal = thisBinded->asESTypedArrayObjectWrapper();
        int offset = 0;
        if (arglen >= 2)
            offset = instance->currentExecutionContext()->arguments()[1].toInt32();
        if (offset < 0)
            throw TypeError::create();
        auto arg0 = instance->currentExecutionContext()->arguments()[0].asESPointer();
        escargot::ESArrayBufferObject* targetBuffer = thisVal->buffer();
        unsigned targetLength = thisVal->arraylength();
        int targetByteOffset = thisVal->byteoffset();
        int targetElementSize = thisVal->elementSize();
        if (!arg0->isESTypedArrayObject()) {
            ESObject* src = arg0->asESObject();
            escargot::ESArrayObject* tmp = src->asESArrayObject();
            int32_t srcLength = src->get(strings->length.string()).asInt32();
            if (srcLength + offset > targetLength)
                throw RangeError::create();

            int targetByteIndex = offset * targetElementSize + targetByteOffset;
            int k = 0;
            int limit = targetByteIndex + targetElementSize * srcLength;

            while (targetByteIndex < limit) {
                escargot::ESString* Pk = ESString::create(k);
                double kNumber = src->get(Pk).toNumber();
                thisVal->set(targetByteIndex / targetElementSize, ESValue(kNumber));
                k++;
                targetByteIndex += targetElementSize;
            }
            return ESValue();
        } else {
            auto arg0Wrapper = arg0->asESTypedArrayObjectWrapper();
            escargot::ESArrayBufferObject* srcBuffer = arg0Wrapper->buffer();
            unsigned srcLength = arg0Wrapper->arraylength();
            int srcByteOffset = arg0Wrapper->byteoffset();
            int srcElementSize = arg0Wrapper->elementSize();
            if (srcLength + offset > targetLength)
                throw RangeError::create();
            int srcByteIndex = 0;
            if (srcBuffer == targetBuffer) {
                // TODO: 24) should clone targetBuffer
                RELEASE_ASSERT_NOT_REACHED();
            } else {
                srcByteIndex = srcByteOffset;
            }
            int targetIndex = offset, srcIndex = 0;
            int targetByteIndex = offset * targetElementSize + targetByteOffset;
            int limit = targetByteIndex + targetElementSize * srcLength;
            if (thisVal->arraytype() != arg0Wrapper->arraytype()) {
                while (targetIndex < offset + srcLength) {
                    ESValue value = arg0Wrapper->get(srcIndex);
                    thisVal->set(targetIndex, value);
                    srcIndex++;
                    targetIndex++;
                }
            } else {
                while (targetByteIndex < limit) {
                    ESValue value = srcBuffer->getValueFromBuffer<uint8_t>(srcByteIndex, escargot::Uint8Array);
                    targetBuffer->setValueInBuffer<Uint8Adaptor>(targetByteIndex, escargot::Uint8Array, value);
                    srcByteIndex++;
                    targetByteIndex++;
                }
            }
            return ESValue();
        }
    }, strings->set));
    // $22.2.3.26 %TypedArray%.prototype.subarray([begin [, end]])
    ta_prototype->ESObject::defineDataProperty(strings->subarray, true, false, true, ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        int arglen = instance->currentExecutionContext()->argumentCount();
        auto thisBinded = instance->currentExecutionContext()->resolveThisBindingToObject();
        if (!thisBinded->isESTypedArrayObject())
            throw TypeError::create();
        auto thisVal = thisBinded->asESTypedArrayObjectWrapper();
        escargot::ESArrayBufferObject* buffer = thisVal->buffer();
        unsigned srcLength = thisVal->arraylength();
        int relativeBegin = 0, beginIndex = 0;
        if (arglen >= 1)
            relativeBegin = instance->currentExecutionContext()->arguments()[0].toInt32();
        if (relativeBegin < 0)
            beginIndex = (srcLength + relativeBegin) > 0 ? (srcLength + relativeBegin) : 0;
        else
            beginIndex = relativeBegin < srcLength ? relativeBegin : srcLength;
        int relativeEnd = srcLength, endIndex;
        if (arglen >= 2)
            relativeEnd = instance->currentExecutionContext()->arguments()[1].toInt32();
        if (relativeEnd < 0)
            endIndex = (srcLength + relativeEnd) > 0 ? (srcLength + relativeEnd) : 0;
        else
            endIndex = relativeEnd < srcLength ? relativeEnd : srcLength;
        int newLength = 0;
        if (endIndex - beginIndex > 0)
            newLength = endIndex - beginIndex;
        int srcByteOffset = thisVal->byteoffset();

        ESValue arg[3] = {buffer, ESValue(srcByteOffset + beginIndex * thisVal->elementSize()), ESValue(newLength)};
        escargot::ESTypedArrayObject<T>* newobj = escargot::ESTypedArrayObject<T>::create();
        ESValue ret = ESFunctionObject::call(instance, thisBinded->get(strings->constructor.string()), newobj, arg, 3, instance);
        return ret;
    }, strings->subarray));

    ta_constructor->set__proto__(m_functionPrototype); // empty Function
    ta_constructor->setProtoType(ta_prototype);
    ta_prototype->set__proto__(m_objectPrototype);
    ta_prototype->defineDataProperty(strings->constructor, true, false, true, ta_constructor);
    defineDataProperty(ta_name, true, false, true, ta_constructor);
    return ta_constructor;
}

void GlobalObject::registerCodeBlock(CodeBlock* cb)
{
    m_codeBlocks.push_back(cb);
}

void GlobalObject::unregisterCodeBlock(CodeBlock* cb)
{
    auto iter = std::find(m_codeBlocks.begin(), m_codeBlocks.end(), cb);
    ASSERT(iter != m_codeBlocks.end());
    m_codeBlocks.erase(iter);

}

void GlobalObject::somePrototypeObjectDefineIndexedProperty()
{
    if (!m_didSomePrototypeObjectDefineIndexedProperty) {
        fprintf(stderr, "some prototype object define indexed property.....\n");
        m_didSomePrototypeObjectDefineIndexedProperty = true;
        for (unsigned i = 0; i < m_codeBlocks.size() ; i ++) {
            // printf("%p..\n", m_codeBlocks[i]);
            iterateByteCode(m_codeBlocks[i], [](CodeBlock* block, unsigned idx, ByteCode* code, Opcode opcode) {
                switch (opcode) {
                case GetObjectOpcode:
                    {
                        GetObjectSlowMode n;
                        n.assignOpcodeInAddress();
                        block->m_extraData[idx].m_opcode = GetObjectSlowModeOpcode;
                        memcpy(code, &n, sizeof(GetObjectSlowMode));
                        break;
                    }
                case GetObjectAndPushObjectSlowModeOpcode:
                    {
                        GetObjectAndPushObjectSlowMode n;
                        n.assignOpcodeInAddress();
                        block->m_extraData[idx].m_opcode = GetObjectAndPushObjectSlowModeOpcode;
                        memcpy(code, &n, sizeof(GetObjectAndPushObjectSlowMode));
                        break;
                    }
                case GetObjectWithPeekingOpcode:
                    {
                        GetObjectWithPeekingSlowMode n;
                        n.assignOpcodeInAddress();
                        block->m_extraData[idx].m_opcode = GetObjectWithPeekingSlowModeOpcode;
                        memcpy(code, &n, sizeof(GetObjectWithPeekingSlowMode));
                        break;
                    }
                case GetObjectPreComputedCaseOpcode:
                    {
                        GetObjectPreComputedCaseSlowMode n(((GetObjectPreComputedCase *)code)->m_propertyValue);
                        n.assignOpcodeInAddress();
                        block->m_extraData[idx].m_opcode = GetObjectPreComputedCaseSlowModeOpcode;
                        memcpy(code, &n, sizeof(GetObjectPreComputedCaseSlowMode));
                        break;
                    }
                case GetObjectPreComputedCaseAndPushObjectOpcode:
                    {
                        GetObjectPreComputedCaseAndPushObjectSlowMode n(((GetObjectPreComputedCaseAndPushObject *)code)->m_propertyValue);
                        n.assignOpcodeInAddress();
                        block->m_extraData[idx].m_opcode = GetObjectPreComputedCaseAndPushObjectSlowModeOpcode;
                        memcpy(code, &n, sizeof(GetObjectPreComputedCaseAndPushObjectSlowMode));
                        break;
                    }
                case GetObjectWithPeekingPreComputedCaseOpcode:
                    {
                        GetObjectWithPeekingPreComputedCaseSlowMode n(((GetObjectWithPeekingPreComputedCase *)code)->m_propertyValue);
                        n.assignOpcodeInAddress();
                        block->m_extraData[idx].m_opcode = GetObjectWithPeekingPreComputedCaseSlowModeOpcode;
                        memcpy(code, &n, sizeof(GetObjectWithPeekingPreComputedCaseSlowMode));
                        break;
                    }
                case SetObjectOpcode:
                    {
                        SetObjectSlowMode n;
                        n.assignOpcodeInAddress();
                        block->m_extraData[idx].m_opcode = SetObjectSlowModeOpcode;
                        memcpy(code, &n, sizeof(SetObjectSlowMode));
                        break;
                    }
                case SetObjectPreComputedCaseOpcode:
                    {
                        SetObjectPreComputedCaseSlowMode n(((SetObjectPreComputedCase *)code)->m_propertyValue);
                        n.assignOpcodeInAddress();
                        block->m_extraData[idx].m_opcode = SetObjectPreComputedCaseSlowModeOpcode;
                        memcpy(code, &n, sizeof(SetObjectPreComputedCaseSlowMode));
                        break;
                    }
                default: { }
                }
            });
        }
    }
}

}
