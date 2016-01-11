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

struct InnerIdentifierInfo {
    InternalAtomicString m_name;
    struct {
        bool m_isHeapAllocated:1;
    } m_flags;

    InnerIdentifierInfo(InternalAtomicString name)
        : m_name(name)
    {
        m_flags.m_isHeapAllocated = false;
    }
};

struct FunctionParametersInfo {
    bool m_isHeapAllocated;
    size_t m_index;
};

typedef std::vector<InnerIdentifierInfo, gc_allocator<InnerIdentifierInfo> > InnerIdentifierInfoVector;
typedef std::vector<FunctionParametersInfo, gc_allocator<FunctionParametersInfo> > FunctionParametersInfoVector;

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
    ALWAYS_INLINE LexicalEnvironment(EnvironmentRecord* record, LexicalEnvironment* outerEnv)
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
    static LexicalEnvironment* newFunctionEnvironment(bool needsToPrepareGenerateArgumentsObject, ESValue* stackAllocatedStorage, size_t stackAllocatedStorageSize, const InternalAtomicStringVector& innerIdentifiers, ESValue arguments[], const size_t& argumentCount, ESFunctionObject* function, bool needsActivation);

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
        return NULL;
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
    ALWAYS_INLINE DeclarativeEnvironmentRecord(ESValue* stackAllocatedData = 0, size_t stackAllocatedSize = 0, const InternalAtomicStringVector& innerIdentifiers = InternalAtomicStringVector(), bool needsActivation = false)
        : m_heapAllocatedData(innerIdentifiers.size())
    {
        if (UNLIKELY(needsActivation)) {
            m_innerIdentifiers = new InternalAtomicStringVector(innerIdentifiers);
        } else {
            m_stackAllocatedData = stackAllocatedData;
            std::fill(stackAllocatedData, &stackAllocatedData[stackAllocatedSize], ESValue());
        }
        m_needsActivation = needsActivation;
    }

    ~DeclarativeEnvironmentRecord()
    {
    }

    virtual ESValue* hasBinding(const InternalAtomicString& atomicName)
    {
        if (!m_needsActivation)
            return NULL;
        for (unsigned i = 0; i < m_innerIdentifiers->size(); i ++) {
            if ((*m_innerIdentifiers)[i] == atomicName) {
                return &m_heapAllocatedData[i];
            }
        }
        return NULL;
    }

    virtual void createMutableBinding(const InternalAtomicString& name, bool canDelete = false);
    virtual void setMutableBinding(const InternalAtomicString& name, const ESValue& V, bool mustNotThrowTypeErrorExecption)
    {
        ASSERT(m_needsActivation);
        // TODO mustNotThrowTypeErrorExecption
        size_t siz = (*m_innerIdentifiers).size();
        for (unsigned i = 0; i < siz; i ++) {
            if ((*m_innerIdentifiers)[i] == name) {
                m_heapAllocatedData[i] = V;
                return;
            }
        }
        RELEASE_ASSERT_NOT_REACHED();
    }

    ESValue* bindingValueForStackAllocatedData(size_t idx)
    {
        ASSERT(!m_needsActivation);
        return &m_stackAllocatedData[idx];
    }

    ESValue* bindingValueForHeapAllocatedData(size_t idx)
    {
        return &m_heapAllocatedData[idx];
    }

    ESValueVector& heapAllocatedData()
    {
        return m_heapAllocatedData;
    }

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
    static size_t offsetofActivationData() { return offsetof(DeclarativeEnvironmentRecord, m_heapAllocatedData); }
#pragma GCC diagnostic pop
#endif

protected:
    union {
        ESValue* m_stackAllocatedData;
        InternalAtomicStringVector* m_innerIdentifiers;
    };
    ESValueVector m_heapAllocatedData;
    bool m_needsActivation;
};

// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-global-environment-records
class GlobalEnvironmentRecord : public EnvironmentRecord {
public:
    GlobalEnvironmentRecord(ESObject* globalObject)
    {
        m_objectRecord = new ObjectEnvironmentRecord(globalObject);
        // m_declarativeRecord = new DeclarativeEnvironmentRecord();
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
    // DeclarativeEnvironmentRecord* m_declarativeRecord;
    // std::vector<InternalAtomicString, gc_allocator<InternalAtomicString> > m_varNames;
};



// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-function-environment-records
class FunctionEnvironmentRecord : public DeclarativeEnvironmentRecord {
    friend class LexicalEnvironment;
    friend class ESFunctionObject;
public:
    ALWAYS_INLINE FunctionEnvironmentRecord(ESValue* stackAllocatedData = 0, size_t stackAllocatedSize = 0, const InternalAtomicStringVector& innerIdentifiers = InternalAtomicStringVector(), bool needsActivation = false)
        : DeclarativeEnvironmentRecord(stackAllocatedData, stackAllocatedSize, innerIdentifiers, needsActivation)
    {
    }

    virtual bool hasThisBinding()
    {
        // we dont use arrow function now. so binding status is alwalys not lexical.
        return true;
    }
    // http://www.ecma-international.org/ecma-262/6.0/index.html#sec-bindthisvalue
    // void bindThisValue(const ESValue& V);
    // ESValue getThisBinding();

protected:
    // ESValue m_thisValue;
    // ESFunctionObject* m_functionObject; //TODO
    // ESValue m_newTarget; //TODO
};

class FunctionEnvironmentRecordWithArgumentsObject : public FunctionEnvironmentRecord {
public:
    ALWAYS_INLINE FunctionEnvironmentRecordWithArgumentsObject(ESValue arguments[], const size_t& argumentCount, ESFunctionObject* callee, ESValue* stackAllocatedData = 0, size_t stackAllocatedSize = 0, const InternalAtomicStringVector& innerIdentifiers = InternalAtomicStringVector(), bool needsActivation = false)
        : FunctionEnvironmentRecord(stackAllocatedData, stackAllocatedSize, innerIdentifiers, needsActivation)
        , m_argumentsObject(ESValue::ESEmptyValue)
    {
        m_arguments = arguments;
        m_argumentsCount = argumentCount;
        m_callee = callee;
    }

    virtual ESValue* hasBindingForArgumentsObject()
    {
        if (LIKELY(!m_argumentsObject.isEmpty()))
            return &m_argumentsObject;

        ESObject* argumentsObject = ESArgumentsObject::create();
        m_argumentsObject = argumentsObject;
        unsigned i = 0;
        argumentsObject->defineDataProperty(strings->length, true, false, true, ESValue(m_argumentsCount));
        for (; i < m_argumentsCount && i < ESCARGOT_STRINGS_NUMBERS_MAX; i ++) {
            argumentsObject->set(strings->numbers[i].string(), m_arguments[i]);
        }
        for (; i < m_argumentsCount; i ++) {
            argumentsObject->set(ESString::create((int)i), m_arguments[i]);
        }
        argumentsObject->defineDataProperty(strings->callee, true, false, true, ESValue(m_callee));

        return &m_argumentsObject;
    }

protected:
    ESValue* m_arguments;
    size_t m_argumentsCount;
    ESValue m_argumentsObject;
    ESFunctionObject* m_callee;
};

/*
// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-module-environment-records
class ModuleEnvironmentRecord : public DeclarativeEnvironmentRecord {
protected:
};
*/


// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-newfunctionenvironment
// $8.1.2.4
ALWAYS_INLINE LexicalEnvironment* LexicalEnvironment::newFunctionEnvironment(bool needsToPrepareGenerateArgumentsObject, ESValue* stackAllocatedStorage, size_t stackAllocatedStorageSize, const InternalAtomicStringVector& innerIdentifiers, ESValue arguments[], const size_t& argumentCount, ESFunctionObject* function, bool needsActivation)
{
    FunctionEnvironmentRecord* envRec;
    if (UNLIKELY(!needsToPrepareGenerateArgumentsObject)) {
        envRec = new FunctionEnvironmentRecord(stackAllocatedStorage, stackAllocatedStorageSize, innerIdentifiers, needsActivation);
    } else {
        envRec = new FunctionEnvironmentRecordWithArgumentsObject(arguments, argumentCount, function, stackAllocatedStorage, stackAllocatedStorageSize, innerIdentifiers, needsActivation);
    }

    // envRec->m_functionObject = function;
    // envRec->m_newTarget = newTarget;
    LexicalEnvironment* env = new LexicalEnvironment(envRec, function->outerEnvironment());
    // TODO
    // If F’s [[ThisMode]] internal slot is lexical, set envRec.[[thisBindingStatus]] to "lexical".
    // [[ThisMode]] internal slot is lexical, set envRec.[[thisBindingStatus]] to "lexical".
    // Let home be the value of F’s [[HomeObject]] internal slot.
    // Set envRec.[[HomeObject]] to home.
    // Set envRec.[[NewTarget]] to newTarget.
    return env;
}

}
#endif
