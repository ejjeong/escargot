/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

#include "Escargot.h"
#include "parser/esprima.h"
#include "parser/ScriptParser.h"
#include "ESVMInstance.h"
#include "runtime/Environment.h"
#include "runtime/ExecutionContext.h"
#include "runtime/GlobalObject.h"
#include "runtime/JobQueue.h"
#include "bytecode/ByteCode.h"
#ifdef ENABLE_ESJIT
#include "nanojit.h"
#endif

#include "BumpPointerAllocator.h"

namespace escargot {


size_t ESVMInstance::m_nativeHeapUsage = 0;

#ifndef ANDROID
__thread ESVMInstance* currentInstance;
#else
ESVMInstance* currentInstance;
#endif

ESVMInstance::ESVMInstance()
{
    GC_set_oom_fn([](size_t bytes) -> void* {
        ESVMInstanceCurrentInstance()->throwOOMError();
        RELEASE_ASSERT_NOT_REACHED();
    });

    /*
    GC_set_on_collection_event([](GC_EventType evtType) {
        if (evtType == GC_EVENT_END) {
            ESVMInstance::currentInstance()->regexpCache()->clear();
        }
    });
    */
    m_profile = false;

#ifndef NDEBUG
    m_debug = false;
    m_dumpByteCode = false;
    m_dumpExecuteByteCode = false;
    m_verboseJIT = false;
    m_reportUnsupportedOpcode = false;
    m_reportCompiledFunction = false;
    m_reportOSRExitedFunction = false;
    m_useVerboseWriter = false;
    m_compiledFunctions = 0;
    m_osrExitedFunctions = 0;
#endif
    m_useExprFilter = false;
    m_useCseFilter = false;
    m_jitThreshold = 2;
    m_osrExitThreshold = 1;
    enter();

    m_scriptParser = new(GC) ScriptParser();
    std::srand(std::time(0));

    m_opcodeTable = new(PointerFreeGC) OpcodeTable();
    // init goto table
    interpret(this, NULL, 0);

    // if you don't use localtime() or timegm() or etc. you should call tzset() to use tzname[]
    tzset();
    m_timezone = NULL;

    m_error = ESValue(ESValue::ESEmptyValueTag::ESEmptyValue);

#ifdef ENABLE_ESJIT
    m_JITConfig = new nanojit::Config();
#endif

    m_catchDepth = 0;

    m_identifierCacheInvalidationCheckCount = 0;

    std::setlocale(LC_ALL, "en_US.utf8");
    m_strings.initStaticStrings(this);

    // TODO call destructor
    m_bumpPointerAllocator = new(GC) WTF::BumpPointerAllocator();

    m_globalFunctionPrototype = NULL;

    // TODO: Object.prototype.__proto__ should be configurable.
    //       This is defined in ES6. ($B.2.2.1)
    m_object__proto__AccessorData.setGetter([](ESObject* obj, ESObject* originalObj, ESString* propertyName) -> ESValue {
        return obj->__proto__();
    });

    m_object__proto__AccessorData.setSetter([](::escargot::ESObject* self, ESObject* originalObj, ESString* propertyName, const ESValue& value) -> void {
        if (value.isESPointer() && value.asESPointer()->isESObject()) {
            // https://developer.mozilla.org/ko/docs/Web/JavaScript/Reference/Global_Objects/Object/preventExtensions
            // Calling Object.preventExtensions() on an object will also prevent extensions on its __proto__
            if (!self->isExtensible())
                ESVMInstanceCurrentInstance()->throwError(ESValue(TypeError::create(ESString::create(u"Attempted to assign to readonly property."))));
            self->set__proto__(value.asESPointer()->asESObject());
        } else if (value.isUndefined()) {
            self->set__proto__(ESValue());
        } else if (value.isNull()) {
            self->set__proto__(ESValue(ESValue::ESNull));
        }
    });

    // m_initialHiddenClassForObject.m_propertyIndexHashMapInfo.insert(std::make_pair(
    //     m_strings.__proto__,
    //     0
    //     ));
    m_initialHiddenClassForObject.m_propertyInfo.push_back(ESHiddenClassPropertyInfo(m_strings.__proto__.string(), NativeAccessor | Writable));

    // $19.2.4 Function Instances
    m_initialHiddenClassForFunctionObject = m_initialHiddenClassForObject.defineProperty(m_strings.length, Data);
    m_initialHiddenClassForFunctionObject = m_initialHiddenClassForFunctionObject->defineProperty(m_strings.prototype, NativeAccessor | Writable);
    m_initialHiddenClassForFunctionObject = m_initialHiddenClassForFunctionObject->defineProperty(m_strings.name, Data | Configurable);

    m_initialHiddenClassForFunctionObjectWithoutPrototype = m_initialHiddenClassForObject.defineProperty(m_strings.length, Data);
    m_initialHiddenClassForFunctionObjectWithoutPrototype = m_initialHiddenClassForFunctionObjectWithoutPrototype->defineProperty(m_strings.name, Data | Configurable);

    m_initialHiddenClassForPrototypeObject = m_initialHiddenClassForObject.defineProperty(m_strings.constructor, Data | Writable | Configurable);

    m_initialHiddenClassForArrayObject = m_initialHiddenClassForObject.defineProperty(m_strings.length, NativeAccessor | Writable);

    m_initialHiddenClassForRegExpObject = m_initialHiddenClassForObject.defineProperty(m_strings.source, NativeAccessor);
    m_initialHiddenClassForRegExpObject = m_initialHiddenClassForRegExpObject->defineProperty(m_strings.ignoreCase, NativeAccessor);
    m_initialHiddenClassForRegExpObject = m_initialHiddenClassForRegExpObject->defineProperty(m_strings.global, NativeAccessor);
    m_initialHiddenClassForRegExpObject = m_initialHiddenClassForRegExpObject->defineProperty(m_strings.multiline, NativeAccessor);
    m_initialHiddenClassForRegExpObject = m_initialHiddenClassForRegExpObject->defineProperty(m_strings.lastIndex, NativeAccessor | Writable);

    m_functionPrototypeAccessorData.setGetter([](ESObject* self, ESObject* originalObj, ESString* propertyName) -> ESValue {
        return self->asESFunctionObject()->protoType();
    });

    m_functionPrototypeAccessorData.setSetter([](::escargot::ESObject* self, ESObject* originalObj, ESString* propertyName, const ESValue& value) {
        self->asESFunctionObject()->setProtoType(value);
    });

    m_arrayLengthAccessorData.setGetter([](ESObject* self, ESObject* originalObj, ESString* propertyName) -> ESValue {
        return ESValue(self->asESArrayObject()->length());
    });

    m_arrayLengthAccessorData.setSetter([](::escargot::ESObject* self, ESObject* originalObj, ESString* propertyName, const ESValue& value) {
        uint32_t newlen = value.toUint32();
        if (value.toNumber() != newlen)
            ESVMInstanceCurrentInstance()->throwError((RangeError::create(ESString::create(u"invalid array length"))));
        self->asESArrayObject()->setLength(newlen);
    });

    m_stringObjectLengthAccessorData.setGetter([](ESObject* self, ESObject* originalObj, ESString* propertyName) -> ESValue {
        return ESValue(self->asESStringObject()->stringData()->length());
    });

    // regexp.source
    m_regexpAccessorData[0].setGetter([](ESObject* self, ESObject* originalObj, ::escargot::ESString* propertyName) -> ESValue {
        return self->asESRegExpObject()->source();
    });

    // regexp.ignoreCase
    m_regexpAccessorData[1].setGetter([](ESObject* self, ESObject* originalObj, ::escargot::ESString* propertyName) -> ESValue {
        return ESValue((bool)(self->asESRegExpObject()->option() & ESRegExpObject::Option::IgnoreCase));
    });

    // regexp.global
    m_regexpAccessorData[2].setGetter([](ESObject* self, ESObject* originalObj, ::escargot::ESString* propertyName) -> ESValue {
        return ESValue((bool)(self->asESRegExpObject()->option() & ESRegExpObject::Option::Global));
    });

    // regexp.multiline
    m_regexpAccessorData[3].setGetter([](ESObject* self, ESObject* originalObj, ::escargot::ESString* propertyName) -> ESValue {
        return ESValue((bool)(self->asESRegExpObject()->option() & ESRegExpObject::Option::MultiLine));
    });

    // regexp.lastIndex
    m_regexpAccessorData[4].setGetter([](ESObject* self, ESObject* originalObj, ::escargot::ESString* propertyName) -> ESValue {
        return self->asESRegExpObject()->lastIndex();
    });
    m_regexpAccessorData[4].setSetter([](ESObject* self, ESObject* originalObj, ::escargot::ESString* propertyName, const ESValue& index) {
        self->asESRegExpObject()->setLastIndex(index);
    });

#ifdef USE_ES6_FEATURE
    m_jobQueue = JobQueue::create();
#endif

    m_globalObject = new GlobalObject();
    m_globalObject->initGlobalObject();

    ESFunctionObject* thrower = ESFunctionObject::create(NULL, [](ESVMInstance* instance)->ESValue {
        instance->throwError(ErrorCode::TypeError, errorMessage_ArgumentsOrCaller_InStrictMode);
        RELEASE_ASSERT_NOT_REACHED();
    }, strings().emptyString, 1);
    thrower->defineDataProperty(m_strings.prototype.string(), false, false, false, ESObject::create());
    thrower->setExtensible(false);
    m_throwerAccessorData.setJSSetter(thrower);
    m_throwerAccessorData.setJSGetter(thrower);

    LexicalEnvironment* a = new LexicalEnvironment(new GlobalEnvironmentRecord(m_globalObject), NULL);

    m_globalExecutionContext = new ExecutionContext(a, false, false, (ESValue*)GC_MALLOC(sizeof(ESValue*) * 2));
    m_globalExecutionContext->setThisBinding(m_globalObject);
    m_currentExecutionContext = m_globalExecutionContext;
    GC_gcollect();
    exit();
}

ESVMInstance::~ESVMInstance()
{
    m_globalObject->finalize();
#ifdef ENABLE_ESJIT
    delete m_JITConfig;
#endif
}

ESValue ESVMInstance::evaluate(ESString* source)
{
    // unsigned long start = ESVMInstance::currentInstance()->tickCount();

    ExecutionContext* oldContext = m_currentExecutionContext;
    m_currentExecutionContext = m_globalExecutionContext;
    bool oldContextIsStrictMode = oldContext->isStrictMode();

    ParserContextInformation parserContextInformation(false, true, false, false, true);
    CodeBlock* block = m_scriptParser->parseScript(this, source, ExecutableType::GlobalCode, parserContextInformation);
    if (block->shouldUseStrictMode())
        m_currentExecutionContext->setStrictMode(true);

    m_lastExpressionStatementValue = ESValue();
    interpret(this, block);
    if (!block->m_isCached)
        block->finalize(this, true);

    // unsigned long end = ESVMInstance::currentInstance()->tickCount();
    // printf("ESVMInstance::evaluate takes %lfms\n", (end-start)/1000.0);

    m_currentExecutionContext = oldContext;
    m_currentExecutionContext->setStrictMode(oldContextIsStrictMode);
    return m_lastExpressionStatementValue;
}

ESValue ESVMInstance::evaluateEval(ESString* source, bool isDirectCall, CodeBlock* outerCodeBlock)
{
    ExecutionContext* oldContext = m_currentExecutionContext;
    bool oldContextIsStrictMode = oldContext->isStrictMode();
    bool oldGlobalContextIsStrictMode = m_globalExecutionContext->isStrictMode();

    bool strictFromOutside = m_currentExecutionContext->isStrictMode() && isDirectCall;
    bool shouldWorkAroundIdentifier = !isInCatchClause();
    bool hasArgumentsBinding = outerCodeBlock ? outerCodeBlock->m_hasArgumentsBinding : false;
    bool isForGlobalScope = !isDirectCall;
    ParserContextInformation parserContextInformation(strictFromOutside, shouldWorkAroundIdentifier, hasArgumentsBinding, true, isForGlobalScope);
    CodeBlock* block = m_scriptParser->parseScript(this, source, ExecutableType::EvalCode, parserContextInformation);
    bool isStrictCode = block->shouldUseStrictMode();
    if (!isDirectCall) {
        // $ES5 10.4.2.1. Use global execution context
        m_currentExecutionContext = m_globalExecutionContext;
        m_currentExecutionContext->setStrictMode(isStrictCode);
    } else {
        // $ES5 10.4.2.2. Use calling execution context
    }

    m_lastExpressionStatementValue = ESValue();
    if (isStrictCode) {
        // $ES5 10.4.2.3. Use new environment
        block->m_hasCode = true;
        block->m_needsActivation = true; // FIXME modify parser to generate fastindex codes for evals
        block->m_needsHeapAllocatedExecutionContext = true;
        ESFunctionObject* callee = ESFunctionObject::create(m_currentExecutionContext->environment(), block, strings().emptyString);
        ESFunctionObject::call(this, callee, m_currentExecutionContext->resolveThisBinding(), nullptr, 0, false);
    } else {
        interpret(this, block);
    }
    if (!block->m_isCached)
        block->finalize(this, true);
    m_globalExecutionContext->setStrictMode(oldGlobalContextIsStrictMode);
    m_currentExecutionContext = oldContext;
    m_currentExecutionContext->setStrictMode(oldContextIsStrictMode);
    return m_lastExpressionStatementValue;
}

void ESVMInstance::exit()
{
    escargot::currentInstance = NULL;
    escargot::strings = NULL;
}

void ESVMInstance::printValue(ESValue val, bool newLine)
{
    UTF16String str;
    std::function<void(ESValue v)> toString = [&str, &toString](ESValue v)
    {
        if (v.isEmpty()) {
            str.append(u"[Empty Value]");
        } else if (v.isInt32()) {
            str.append((v.toString()->toUTF16String()));
        } else if (v.isNumber()) {
            str.append((v.toString()->toUTF16String()));
        } else if (v.isUndefined()) {
            str.append((v.toString()->toUTF16String()));
        } else if (v.isNull()) {
            str.append((v.toString()->toUTF16String()));
        } else if (v.isBoolean()) {
            str.append((v.toString()->toUTF16String()));
        } else if (v.isESPointer()) {
            ESPointer* o = v.asESPointer();
            if (o->isESString()) {
                str.append((o->asESString()->toUTF16String()));
            } else if (o->isESFunctionObject()) {
                str.append((v.toString()->toUTF16String()));
            } else if (o->isESArrayObject()) {
                str.append((v.toString()->toUTF16String()));
            } else if (o->isESErrorObject()) {
                str.append((v.toString()->toUTF16String()));
            } else if (o->isESObject()) {
                if (o->asESObject()->get(ESValue(currentInstance()->strings().constructor)).isESPointer() && o->asESObject()->get(ESValue(currentInstance()->strings().constructor)).asESPointer()->isESObject())
                    str.append((o->asESObject()->get(ESValue(currentInstance()->strings().constructor)).asESPointer()->asESObject()->get(ESValue(currentInstance()->strings().name)).toString()->toUTF16String()));
                str.append(u" {");
                bool isFirst = true;
                o->asESObject()->enumeration([&str, &isFirst, o, &toString](escargot::ESValue key) {
                    if (!isFirst)
                        str.append(u", ");
                    str.append((key.toString()->toUTF16String()));
                    str.append(u": ");
                    str.append((o->asESObject()->getOwnProperty(key).toString()->toUTF16String()));
                    // toString(slot.value(o->asESObject()));
                    isFirst = false;
                });
                if (o->isESStringObject()) {
                    str.append(u", [[PrimitiveValue]]: \"");
                    str.append((o->asESStringObject()->stringData()->toUTF16String()));
                    str.append(u"\"");
                }
                str.append(u"}");
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
        } else {
#ifdef ESCARGOT_64
            printf("Invalid ESValue Format : 0x%lx\n", v.asRawData());
#else
            printf("Invalid ESValue Format : 0x%llx\n", v.asRawData());
#endif
            ASSERT(false);
        }
    };
    toString(val);

    // print one by one character just in case for nullable string such as "\u0002\u0000\u0001"
    for (char16_t c : str) {
        printf("%c", c);
    }
    if (newLine)
        printf("\n");
    fflush(stdout);
}

void ESSimpleAllocator::allocSlow()
{
    std::vector<ESSimpleAllocatorMemoryFragment, pointer_free_allocator<ESSimpleAllocatorMemoryFragment> >& allocatedMemorys = ESVMInstance::currentInstance()->m_allocatedMemorys;
    ESSimpleAllocatorMemoryFragment f;
    f.m_buffer = malloc(s_fragmentBufferSize);
    f.m_currentUsage = 0;
    f.m_totalSize = s_fragmentBufferSize;
    allocatedMemorys.push_back(f);
}

void ESSimpleAllocator::freeAll()
{
    std::vector<ESSimpleAllocatorMemoryFragment, pointer_free_allocator<ESSimpleAllocatorMemoryFragment> >& allocatedMemorys = ESVMInstance::currentInstance()->m_allocatedMemorys;
    for (unsigned i = 0 ; i < allocatedMemorys.size() ; i ++) {
        free(allocatedMemorys[i].m_buffer);
    }
    allocatedMemorys.clear();
}


}
