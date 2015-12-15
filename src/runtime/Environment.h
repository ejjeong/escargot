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
        if (!obj->hasProperty(strings->get.string()) && !obj->hasProperty(strings->set.string()))
            return false;
        return true;
    }

    static bool IsDataDescriptor(ESValue desc)
    {
        if (desc.isUndefined())
            return false;
        if (desc.isPrimitive())
            return true;
        ESObject* obj = desc.asESPointer()->asESObject();
        if (!obj->hasOwnProperty(strings->value.string()) && !obj->hasOwnProperty(strings->writable.string()))
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
            obj->set(strings->value.string(), descSrc->hiddenClass()->read(descSrc, descSrc, idx));
            obj->set(strings->writable.string(), ESValue(propertyInfo.m_flags.m_isWritable));
        } else if (descSrc->accessorData(idx)->getJSGetter()
            || descSrc->accessorData(idx)->getJSSetter()
            || (!descSrc->accessorData(idx)->getNativeGetter() && !descSrc->accessorData(idx)->getNativeSetter())) {
            ESObject* getDesc = ESObject::create();
            getDesc->set(strings->value.string(), descSrc->accessorData(idx)->getJSGetter() ? descSrc->accessorData(idx)->getJSGetter() : ESValue());
            getDesc->set(strings->writable.string(), ESValue(true));
            getDesc->set(strings->enumerable.string(), ESValue(true));
            getDesc->set(strings->configurable.string(), ESValue(true));
            ESValue getStr = strings->get.string();
            obj->defineOwnProperty(getStr, getDesc, false);

            ESObject* setDesc = ESObject::create();
            setDesc->set(strings->value.string(), descSrc->accessorData(idx)->getJSSetter() ? descSrc->accessorData(idx)->getJSSetter() : ESValue());
            setDesc->set(strings->writable.string(), ESValue(true));
            setDesc->set(strings->enumerable.string(), ESValue(true));
            setDesc->set(strings->configurable.string(), ESValue(true));
            ESValue setStr = strings->set.string();
            obj->defineOwnProperty(setStr, setDesc, false);
            obj->set(strings->writable.string(), ESValue(false));
        } else {
            descSrc->accessorData(idx)->setGetterAndSetterTo(obj);
        }
        obj->set(strings->enumerable.string(), ESValue(propertyInfo.m_flags.m_isEnumerable));
        obj->set(strings->configurable.string(), ESValue(propertyInfo.m_flags.m_isConfigurable));
        return obj;
    }

    // For ArrayFastMode
    static ESValue FromPropertyDescriptorForIndexedProperties(ESObject* obj, uint32_t index)
    {
        if (obj->isESArrayObject() && obj->asESArrayObject()->isFastmode()) {
            if (index != ESValue::ESInvalidIndexValue) {
                if (LIKELY(index < obj->asESArrayObject()->length())) {
                    ESValue e = obj->asESArrayObject()->data()[index];
                    if (LIKELY(!e.isEmpty())) {
                        ESObject* ret = ESObject::create();
                        ret->set(strings->value.string(), e);
                        ret->set(strings->writable.string(), ESValue(true));
                        ret->set(strings->enumerable.string(), ESValue(true));
                        ret->set(strings->configurable.string(), ESValue(true));
                        return ret;
                    }
                }
            }
        }
        if (obj->isESStringObject()) {
            if (index != ESValue::ESInvalidIndexValue) {
                if (LIKELY(index < obj->asESStringObject()->length())) {
                    ESValue e = obj->asESStringObject()->getCharacterAsString(index);
                    ESObject* ret = ESObject::create();
                    ret->set(strings->value.string(), e);
                    ret->set(strings->writable.string(), ESValue(false));
                    ret->set(strings->enumerable.string(), ESValue(true));
                    ret->set(strings->configurable.string(), ESValue(false));
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
    static LexicalEnvironment* newFunctionEnvironment(bool needsToPrepareGenerateArgumentsObject, ESValue arguments[], const size_t& argumentCount, ESFunctionObject* function);

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
    DeclarativeEnvironmentRecord(ESValue* stackAllocatedData, size_t innerIdentifierSize)
    {
        m_stackAllocatedData = stackAllocatedData;
        std::fill(stackAllocatedData, &stackAllocatedData[innerIdentifierSize], ESValue());
    }

    DeclarativeEnvironmentRecord(const InternalAtomicStringVector& innerIdentifiers = InternalAtomicStringVector())
    {
        m_stackAllocatedData = NULL;
        m_heapAllocatedData.reserve(innerIdentifiers.size());
        for (unsigned i = 0; i < innerIdentifiers.size(); i ++) {
            m_heapAllocatedData.push_back(std::make_pair(innerIdentifiers[i], ESValue()));
        }
    }

    ~DeclarativeEnvironmentRecord()
    {
    }

    virtual ESValue* hasBinding(const InternalAtomicString& atomicName)
    {
        if (m_stackAllocatedData) {
            return NULL;
        } else {
            for (unsigned i = 0; i < m_heapAllocatedData.size(); i ++) {
                if (m_heapAllocatedData[i].first == atomicName) {
                    return &m_heapAllocatedData[i].second;
                }
            }
            return NULL;
        }
    }

    virtual void createMutableBinding(const InternalAtomicString& name, bool canDelete = false);
    virtual void setMutableBinding(const InternalAtomicString& name, const ESValue& V, bool mustNotThrowTypeErrorExecption)
    {
        // TODO mustNotThrowTypeErrorExecption
        if (!m_stackAllocatedData) {
            size_t siz = m_heapAllocatedData.size();
            for (unsigned i = 0; i < siz; i ++) {
                if (m_heapAllocatedData[i].first == name) {
                    m_heapAllocatedData[i].second = V;
                }
            }
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    ESValue bindingValue(size_t t)
    {
        if (m_stackAllocatedData) {
            return m_stackAllocatedData[t];
        } else {
            return m_heapAllocatedData[t].second;
        }
    }

    ESValue* bindingValueForStackAllocatedData(size_t idx)
    {
        return &m_stackAllocatedData[idx];
    }

    ESValue* bindingValueForHeapAllocatedData(size_t idx)
    {
        return &m_heapAllocatedData[idx].second;
    }

    virtual bool isDeclarativeEnvironmentRecord()
    {
        return true;
    }

    virtual bool hasThisBinding()
    {
        return false;
    }

    bool useHeapAllocatedStorage()
    {
        return !m_stackAllocatedData;
    }

#ifdef ENABLE_ESJIT
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Winvalid-offsetof"
    static size_t offsetofActivationData() { return offsetof(DeclarativeEnvironmentRecord, m_heapAllocatedData); }
#pragma GCC diagnostic pop
#endif

protected:
    ESValue* m_stackAllocatedData;
    ESIdentifierVector m_heapAllocatedData;
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
    struct ArgumentsObjectData : public gc {
        ESValue* m_arguments;
        size_t m_argumentsCount;
        ESValue m_argumentsObject;
    };
public:

    // stack storage
    FunctionEnvironmentRecord(bool needsToPrepareGenerateArgumentsObject, ESValue arguments[], const size_t& argumentCount, ESValue* buf, size_t idLen)
        : DeclarativeEnvironmentRecord(buf, idLen)
    {
#ifndef NDEBUG
        m_needsToPrepareGenerateArgumentsObject = needsToPrepareGenerateArgumentsObject;
#endif
        if (needsToPrepareGenerateArgumentsObject) {
            prepareToGenereateArgumentsObject(arguments, argumentCount);
        }
    }

    // heap storage
    FunctionEnvironmentRecord(bool needsToPrepareGenerateArgumentsObject, ESValue arguments[], const size_t& argumentCount, const InternalAtomicStringVector& innerIdentifiers = InternalAtomicStringVector())
        : DeclarativeEnvironmentRecord(innerIdentifiers)
    {
#ifndef NDEBUG
        m_needsToPrepareGenerateArgumentsObject = needsToPrepareGenerateArgumentsObject;
#endif
        if (needsToPrepareGenerateArgumentsObject)
            prepareToGenereateArgumentsObject(arguments, argumentCount);
    }

    void prepareToGenereateArgumentsObject(ESValue arguments[], const size_t& argumentCount)
    {
        m_argumentsObjectData = new ArgumentsObjectData;
        m_argumentsObjectData->m_arguments = arguments;
        m_argumentsObjectData->m_argumentsCount = argumentCount;
        m_argumentsObjectData->m_argumentsObject = ESValue(ESValue::ESEmptyValue);
    }

    ESValue* argumentsObject()
    {
        if (!m_argumentsObjectData->m_argumentsObject.isEmpty())
            return &m_argumentsObjectData->m_argumentsObject;
        ESObject* argumentsObject = ESArgumentsObject::create();
        m_argumentsObjectData->m_argumentsObject = argumentsObject;
        unsigned i = 0;
        argumentsObject->defineDataProperty(strings->length, true, false, true, ESValue(m_argumentsObjectData->m_argumentsCount));
        for (; i < m_argumentsObjectData->m_argumentsCount && i < ESCARGOT_STRINGS_NUMBERS_MAX; i ++) {
            argumentsObject->set(strings->numbers[i].string(), m_argumentsObjectData->m_arguments[i]);
        }
        for (; i < m_argumentsObjectData->m_argumentsCount; i ++) {
            argumentsObject->set(ESString::create((int)i), m_argumentsObjectData->m_arguments[i]);
        }

        return &m_argumentsObjectData->m_argumentsObject;
    }

    virtual bool hasThisBinding()
    {
        // we dont use arrow function now. so binding status is alwalys not lexical.
        return true;
    }

    virtual ESValue* hasBindingForArgumentsObject()
    {
        ASSERT(m_needsToPrepareGenerateArgumentsObject);
        return argumentsObject();
    }
    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-bindthisvalue
    // void bindThisValue(const ESValue& V);
    //  ESValue getThisBinding();

protected:
    // ESValue m_thisValue;
    // ESFunctionObject* m_functionObject; //TODO
    // ESValue m_newTarget; //TODO
#ifndef NDEBUG
    bool m_needsToPrepareGenerateArgumentsObject;
#endif
    ArgumentsObjectData* m_argumentsObjectData;
};

/*
// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-module-environment-records
class ModuleEnvironmentRecord : public DeclarativeEnvironmentRecord {
protected:
};
*/


}
#endif
