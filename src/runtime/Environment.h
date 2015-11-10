#ifndef Environment_h
#define Environment_h

#include "ESValue.h"
#include "ESValueInlines.h"

namespace escargot {

class FunctionNode;
class EnvironmentRecord;
class DeclarativeEnvironmentRecord;
class GlobalEnvironmentRecord;
class ObjectEnvironmentRecord;
class ESObject;
class GlobalObject;

typedef std::pair<InternalAtomicString, ::escargot::ESValue> ESIdentifierVectorStdItem;
typedef std::vector<ESIdentifierVectorStdItem,
    gc_allocator<ESIdentifierVectorStdItem> > ESIdentifierVectorStd;

class ESIdentifierVector : public ESIdentifierVectorStd {
public:
    ESIdentifierVector()
        : ESIdentifierVectorStd() { }

#ifdef ENABLE_ESJIT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
    static size_t offsetofData() { return offsetof(ESIdentifierVector, _M_impl._M_start); }
#pragma GCC diagnostic pop
#endif
};

class PropertyDescriptor {
public:
    static bool IsAccessorDescriptor(ESValue desc)
    {
        if (desc.isUndefined())
            return false;
        ASSERT(desc.isObject());
        ESObject* obj = desc.asESPointer()->asESObject();
        if (!obj->hasProperty(ESString::create(u"get")) && !obj->hasProperty(ESString::create(u"set")))
            return false;
        return true;
    }

    static bool IsDataDescriptor(ESValue desc)
    {
        if (desc.isUndefined())
            return false;
        ASSERT(desc.isObject());
        ESObject* obj = desc.asESPointer()->asESObject();
        if (!obj->hasOwnProperty(ESString::create(u"value")) && !obj->hasOwnProperty(ESString::create(u"writable")))
            return false;
        return true;
    }

    static bool IsGenericDescriptor(ESValue desc)
    {
        if (desc.isUndefined())
            return false;
        if (!IsAccessorDescriptor(desc) && !IsDataDescriptor(desc))
            return true;
        return false;
    }

    // ES5.1 8.10.4 FromPropertyDescriptor
    static ESValue FromPropertyDescriptor(ESObject* descSrc, size_t idx)
    {
        bool isActualDataProperty = false;
        if (descSrc->isESArrayObject() && idx == 1) {
            isActualDataProperty = true;
        }
        const ESHiddenClassPropertyInfo& propertyInfo = descSrc->hiddenClass()->propertyInfo(idx);
        ESObject* obj = ESObject::create();
        if (propertyInfo.m_flags.m_isDataProperty || isActualDataProperty) {
            obj->set(ESString::create(u"value"), descSrc->hiddenClass()->read(descSrc, descSrc, idx));
            obj->set(ESString::create(u"writable"), ESValue(propertyInfo.m_flags.m_isWritable));
        } else {
            descSrc->accessorData(idx)->setGetterAndSetterTo(obj);
//            if (descSrc->accessorData(idx)->getJSGetter() || descSrc->accessorData(idx)->getJSSetter()) {
//                obj->defineDataProperty(ESString::create(u"get"), true, true, true, ESValue(2));
//                obj->defineDataProperty(ESString::create(u"set"), true, true, true, ESValue(2));
//            } else {
//                ESObject* getDesc = ESObject::create();
//                getDesc->set(ESString::create(u"value"), ESValue(2));
//                getDesc->set(ESString::create(u"writable"), ESValue(true));
//                getDesc->set(ESString::create(u"enumerable"), ESValue(true));
//                getDesc->set(ESString::create(u"configurable"), ESValue(true));
//                ESValue getStr = ESString::create(u"get");
//                obj->DefineOwnProperty(getStr, getDesc, false);
//
//                ESObject* setDesc = ESObject::create();
//                setDesc->set(ESString::create(u"value"), ESValue(2));
//                setDesc->set(ESString::create(u"writable"), ESValue(true));
//                setDesc->set(ESString::create(u"enumerable"), ESValue(true));
//                setDesc->set(ESString::create(u"configurable"), ESValue(true));
//                ESValue setStr = ESString::create(u"set");
//                obj->DefineOwnProperty(setStr, setDesc, false);
//            }
        }
        obj->set(ESString::create(u"enumerable"), ESValue(propertyInfo.m_flags.m_isEnumerable));
        obj->set(ESString::create(u"configurable"), ESValue(propertyInfo.m_flags.m_isConfigurable));
        return obj;
    }

    // For ArrayFastMode
    static ESValue FromPropertyDescriptorForIndexedProperties(ESObject* obj, uint32_t index)
    {
        if (obj->isESArrayObject() && obj->asESArrayObject()->isFastmode()) {
            if (index != ESValue::ESInvalidIndexValue) {
                if (LIKELY((int)index < obj->asESArrayObject()->length())) {
                    ESValue e = obj->asESArrayObject()->data()[index];
                    if (LIKELY(!e.isEmpty())) {
                        ESObject* ret = ESObject::create();
                        ret->set(ESString::create(u"value"), e);
                        ret->set(ESString::create(u"writable"), ESValue(true));
                        ret->set(ESString::create(u"enumerable"), ESValue(true));
                        ret->set(ESString::create(u"configurable"), ESValue(true));
                        return ret;
                    }
                }
            }
        }
        if (obj->isESStringObject()) {
            if (index != ESValue::ESInvalidIndexValue) {
                if (LIKELY((int)index < obj->asESStringObject()->length())) {
                    ESValue e = obj->asESStringObject()->getCharacterAsString(index);
                    ESObject* ret = ESObject::create();
                    ret->set(ESString::create(u"value"), e);
                    ret->set(ESString::create(u"writable"), ESValue(false));
                    ret->set(ESString::create(u"enumerable"), ESValue(true));
                    ret->set(ESString::create(u"configurable"), ESValue(false));
                    return ret;
                }
            }
        }
        return ESValue();
    }
};

// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-lexical-environments
class LexicalEnvironment : public gc {
public:
    LexicalEnvironment(EnvironmentRecord* record, LexicalEnvironment* outerEnv)
        : m_record(record)
        , m_outerEnvironment(outerEnv)
    {

    }
    ALWAYS_INLINE EnvironmentRecord* record()
    {
        return m_record;
    }

    ALWAYS_INLINE LexicalEnvironment* outerEnvironment()
    {
        return m_outerEnvironment;
    }

    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-newfunctionenvironment
    static LexicalEnvironment* newFunctionEnvironment(ESValue arguments[], const size_t& argumentCount, ESFunctionObject* function);

#ifdef ENABLE_ESJIT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
    static size_t offsetofOuterEnvironment() { return offsetof(LexicalEnvironment, m_outerEnvironment); }
    static size_t offsetofRecord() { return offsetof(LexicalEnvironment, m_record); }
#pragma GCC diagnostic pop
#endif

protected:
    EnvironmentRecord* m_record;
    LexicalEnvironment* m_outerEnvironment;
};

// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-environment-records
class EnvironmentRecord : public gc {
protected:
    EnvironmentRecord()
    {
    }
public:
    virtual ~EnvironmentRecord() { }

    // return NULL == not exist
    virtual ESValue* hasBinding(const InternalAtomicString& atomicName)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }
    virtual ESValue* hasBindingForArgumentsObject()
    {
        RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void createMutableBinding(const InternalAtomicString& name, bool canDelete = false)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void createImmutableBinding(const InternalAtomicString& name, bool throwExecptionWhenAccessBeforeInit = false)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void initializeBinding(const InternalAtomicString& name, const ESValue& V)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void setMutableBinding(const InternalAtomicString& name, const ESValue& V, bool mustNotThrowTypeErrorExecption)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    /* virtual ESValue getBindingValue(const InternalAtomicString& name, bool ignoreReferenceErrorException)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }*/

    virtual bool deleteBinding(const InternalAtomicString& name)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual bool hasSuperBinding()
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual ESValue getThisBinding()
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual bool hasThisBinding()
    {
        RELEASE_ASSERT_NOT_REACHED();
    }
    // WithBaseObject ()

    virtual bool isGlobalEnvironmentRecord()
    {
        return false;
    }

    virtual bool isObjectEnvironmentRecord()
    {
        return false;
    }

    virtual bool isDeclarativeEnvironmentRecord()
    {
        return false;
    }

    GlobalEnvironmentRecord* toGlobalEnvironmentRecord()
    {
        ASSERT(isGlobalEnvironmentRecord());
        return reinterpret_cast<GlobalEnvironmentRecord*>(this);
    }

    DeclarativeEnvironmentRecord* toDeclarativeEnvironmentRecord()
    {
        ASSERT(isDeclarativeEnvironmentRecord());
        return reinterpret_cast<DeclarativeEnvironmentRecord*>(this);
    }

    void createMutableBindingForAST(const InternalAtomicString& atomicName, bool canDelete);

protected:
};

class ObjectEnvironmentRecord : public EnvironmentRecord {
public:
    ObjectEnvironmentRecord(ESObject* O)
        : m_bindingObject(O)
    {
    }
    ~ObjectEnvironmentRecord() { }

    // return NULL == not exist
    virtual ESValue* hasBinding(const InternalAtomicString& atomicName)
    {
        return ((GlobalObject *)m_bindingObject)->addressOfProperty(atomicName.string());
    }
    void createMutableBinding(const InternalAtomicString& name, bool canDelete = false);
    void createImmutableBinding(const InternalAtomicString& name, bool throwExecptionWhenAccessBeforeInit = false) { }
    void initializeBinding(const InternalAtomicString& name,  const ESValue& V);
    void setMutableBinding(const InternalAtomicString& name, const ESValue& V, bool mustNotThrowTypeErrorExecption);
    /*
    ESValue getBindingValue(const InternalAtomicString& name, bool ignoreReferenceErrorException)
    {
        return m_bindingObject->get(name);
    }
    */
    bool deleteBinding(const InternalAtomicString& name)
    {
        return false;
    }
    ALWAYS_INLINE ESObject* bindingObject() {
        return m_bindingObject;
    }

    virtual bool isObjectEnvironmentRecord()
    {
        return true;
    }

    virtual bool hasThisBinding()
    {
        return false;
    }

protected:
    ESObject* m_bindingObject;
};

// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-declarative-environment-records
class DeclarativeEnvironmentRecord : public EnvironmentRecord {
public:
    DeclarativeEnvironmentRecord(ESValue* vectorBuffer, InternalAtomicStringVector* innerIdentifiers)
    {
        m_needsActivation = false;
        m_vectorData = vectorBuffer;
        m_innerIdentifiers = innerIdentifiers;
        std::fill(vectorBuffer, &vectorBuffer[innerIdentifiers->size()], ESValue());
    }

    DeclarativeEnvironmentRecord(const InternalAtomicStringVector& innerIdentifiers = InternalAtomicStringVector())
    {
        m_vectorData = NULL;
        m_innerIdentifiers = NULL;
        m_needsActivation = true;
        m_activationData.reserve(innerIdentifiers.size());
        for (unsigned i = 0; i < innerIdentifiers.size(); i ++) {
            m_activationData.push_back(std::make_pair(innerIdentifiers[i], ESValue()));
        }
    }

    ~DeclarativeEnvironmentRecord()
    {
    }

    InternalAtomicStringVector* innerIdentifiers() { return m_innerIdentifiers; }

    virtual ESValue* hasBinding(const InternalAtomicString& atomicName)
    {
        if (m_needsActivation) {
            size_t siz = m_activationData.size();
            for (unsigned i = 0; i < siz; i ++) {
                if (m_activationData[i].first == atomicName) {
                    return &m_activationData[i].second;
                }
            }

            return NULL;
        } else {
            for (unsigned i = 0; i < m_innerIdentifiers->size(); i ++) {
                if ((*m_innerIdentifiers)[i] == atomicName) {
                    return &m_vectorData[i];
                }
            }
            return NULL;
        }
    }

    virtual void createMutableBinding(const InternalAtomicString& name, bool canDelete = false);
    virtual void setMutableBinding(const InternalAtomicString& name, const ESValue& V, bool mustNotThrowTypeErrorExecption)
    {
        // TODO mustNotThrowTypeErrorExecption
        if (m_needsActivation) {
            size_t siz = m_activationData.size();
            for (unsigned i = 0; i < siz; i ++) {
                if (m_activationData[i].first == name) {
                    m_activationData[i].second = V;
                }
            }
        } else {
            for (unsigned i = 0; i < m_innerIdentifiers->size(); i ++) {
                if ((*m_innerIdentifiers)[i] == name) {
                    m_vectorData[i] = V;
                    return;
                }
            }
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    ESValue* bindingValueForNonActivationMode(size_t idx)
    {
        return &m_vectorData[idx];
    }

    ESValue* bindingValueForActivationMode(size_t idx)
    {
        return &m_activationData[idx].second;
    }

    /*
    virtual ESValue getBindingValue(const InternalAtomicString& name, bool ignoreReferenceErrorException)
    {
        // TODO ignoreReferenceErrorException
        if (UNLIKELY(m_needsActivation)) {
            auto iter = m_mapData->find(name);
            ASSERT(iter != m_mapData->end());
            return iter->second.value();
        } else {
            for (unsigned i = 0; i < m_usedCount; i ++) {
                if (m_vectorData[i].first == name) {
                    return &m_vectorData[i].second;
                }
            }
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
    */

    virtual bool isDeclarativeEnvironmentRecord()
    {
        return true;
    }

    virtual bool hasThisBinding()
    {
        return false;
    }

#ifdef ENABLE_ESJIT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
    static size_t offsetofActivationData() { return offsetof(DeclarativeEnvironmentRecord, m_activationData); }
#pragma GCC diagnostic pop
#endif

protected:
    bool m_needsActivation;

    ESValue* m_vectorData;
    InternalAtomicStringVector* m_innerIdentifiers;

    ESIdentifierVector m_activationData;
};

// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-global-environment-records
class GlobalEnvironmentRecord : public EnvironmentRecord {
public:
    GlobalEnvironmentRecord(ESObject* globalObject)
    {
        m_objectRecord = new ObjectEnvironmentRecord(globalObject);
        m_declarativeRecord = new DeclarativeEnvironmentRecord();
    }
    ~GlobalEnvironmentRecord() { }

    virtual ESValue* hasBinding(const InternalAtomicString& atomicName);
    void createMutableBinding(const InternalAtomicString& name, bool canDelete = false);
    void initializeBinding(const InternalAtomicString& name,  const ESValue& V);
    void setMutableBinding(const InternalAtomicString& name, const ESValue& V, bool mustNotThrowTypeErrorExecption);

    ESValue getThisBinding();
    bool hasVarDeclaration(const InternalAtomicString& name);
    // bool hasLexicalDeclaration(ESString* name);
    bool hasRestrictedGlobalProperty(const InternalAtomicString& name);
    bool canDeclareGlobalVar(const InternalAtomicString& name);
    bool canDeclareGlobalFunction(const InternalAtomicString& name);
    void createGlobalVarBinding(const InternalAtomicString& name, bool canDelete);
    void createGlobalFunctionBinding(const InternalAtomicString& name, const ESValue& V, bool canDelete);
    // ESValue getBindingValue(const InternalAtomicString& name, bool ignoreReferenceErrorException);

    virtual bool isGlobalEnvironmentRecord()
    {
        return true;
    }

    virtual bool hasThisBinding()
    {
        return true;
    }

protected:
    ObjectEnvironmentRecord* m_objectRecord;
    DeclarativeEnvironmentRecord* m_declarativeRecord;
    std::vector<InternalAtomicString, gc_allocator<InternalAtomicString> > m_varNames;
};



// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-function-environment-records
class FunctionEnvironmentRecord : public DeclarativeEnvironmentRecord {
    friend class LexicalEnvironment;
    friend class ESFunctionObject;
public:

    // m_needsActivation = false
    FunctionEnvironmentRecord(ESValue arguments[], const size_t& argumentCount, ESValue* vectorBuffer, InternalAtomicStringVector* innerIdentifiers)
        : DeclarativeEnvironmentRecord(vectorBuffer, innerIdentifiers)
        , m_argumentsObject(ESValue::ESEmptyValue)
    {
#ifndef NDEBUG
        m_thisBindingStatus = Uninitialized;
#endif
        m_arguments = arguments;
        m_argumentCount = argumentCount;
    }

    // m_needsActivation = true
    FunctionEnvironmentRecord(ESValue arguments[], const size_t& argumentCount, const InternalAtomicStringVector& innerIdentifiers = InternalAtomicStringVector())
        : DeclarativeEnvironmentRecord(innerIdentifiers)
        , m_argumentsObject(ESValue::ESEmptyValue)
    {
#ifndef NDEBUG
        m_thisBindingStatus = Uninitialized;
#endif
        m_arguments = (ESValue *)GC_MALLOC(sizeof(ESValue) * argumentCount);
        memcpy(m_arguments, arguments, sizeof(ESValue) * argumentCount);
        m_argumentCount = argumentCount;
    }
    enum ThisBindingStatus {
        Lexical, Initialized, Uninitialized
    };
    virtual bool hasThisBinding()
    {
        // we dont use arrow function now. so binding status is alwalys not lexical.
        return true;
    }

    virtual ESValue* hasBindingForArgumentsObject()
    {
        if (m_argumentsObject.isEmpty()) {
            ESObject* argumentsObject = ESArgumentsObject::create();
            m_argumentsObject = argumentsObject;
            unsigned i = 0;
            argumentsObject->defineDataProperty(strings->length, true, false, true, ESValue(m_argumentCount));
            for (; i < m_argumentCount && i < ESCARGOT_STRINGS_NUMBERS_MAX; i ++) {
                argumentsObject->set(strings->numbers[i].string(), m_arguments[i]);
            }
            for (; i < m_argumentCount; i ++) {
                argumentsObject->set(ESString::create((int)i), m_arguments[i]);
            }
        }
        return &m_argumentsObject;
    }
    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-bindthisvalue
    void bindThisValue(const ESValue& V);
    ESValue getThisBinding();

protected:
    ESValue m_thisValue;
    // ESFunctionObject* m_functionObject; //TODO
    // ESValue m_newTarget; //TODO
    ESValue* m_arguments;
    size_t m_argumentCount;
    ESValue m_argumentsObject;
#ifndef NDEBUG
    ThisBindingStatus m_thisBindingStatus;
#endif
};

/*
// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-module-environment-records
class ModuleEnvironmentRecord : public DeclarativeEnvironmentRecord {
protected:
};
*/


}
#endif
