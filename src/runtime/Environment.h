#ifndef Environment_h
#define Environment_h

#include "ESValue.h"
#include "ESValueInlines.h"

namespace escargot {

class EnvironmentRecord;
class DeclarativeEnvironmentRecord;
class GlobalEnvironmentRecord;
class ObjectEnvironmentRecord;
class ESObject;
class GlobalObject;

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-lexical-environments
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

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-newfunctionenvironment
    static LexicalEnvironment* newFunctionEnvironment(ESFunctionObject* function, ESValue* newTarget);

protected:
    EnvironmentRecord* m_record;
    LexicalEnvironment* m_outerEnvironment;
};

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-environment-records
class EnvironmentRecord : public gc {
protected:
    EnvironmentRecord()
    {
    }
public:
    virtual ~EnvironmentRecord() { }

    //return NULL == not exist
    virtual ESSlot* hasBinding(const InternalAtomicString& name)
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

    virtual void initializeBinding(const InternalAtomicString& name, ESValue* V)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void setMutableBinding(const InternalAtomicString& name, ESValue* V, bool mustNotThrowTypeErrorExecption)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual ESValue* getBindingValue(const InternalAtomicString& name, bool ignoreReferenceErrorException)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual bool deleteBinding(const InternalAtomicString& name)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual bool hasSuperBinding()
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual ESObject* getThisBinding()
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual bool hasThisBinding()
    {
        RELEASE_ASSERT_NOT_REACHED();
    }
    //WithBaseObject ()

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

    void createMutableBindingForAST(const InternalAtomicString& name,bool canDelete);

protected:
};

class ObjectEnvironmentRecord : public EnvironmentRecord {
public:
    ObjectEnvironmentRecord(ESObject* O)
        : m_bindingObject(O)
    {
    }
    ~ObjectEnvironmentRecord() { }

    //return NULL == not exist
    virtual ESSlot* hasBinding(const InternalAtomicString& name)
    {
#if 0
        ESSlot* slot = m_bindingObject->find(name);
        if(slot) {
            return slot;
        }
        return NULL;
#else
        return NULL;
#endif
    }
    void createMutableBinding(const InternalAtomicString& name, bool canDelete = false);
    void createImmutableBinding(const InternalAtomicString& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void initializeBinding(const InternalAtomicString& name, ESValue* V);
    void setMutableBinding(const InternalAtomicString& name, ESValue* V, bool mustNotThrowTypeErrorExecption);
    ESValue* getBindingValue(const InternalAtomicString& name, bool ignoreReferenceErrorException)
    {
#if 0
        return m_bindingObject->get(name);
#else
        return NULL;
#endif
    }
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

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-declarative-environment-records
class DeclarativeEnvironmentRecord : public EnvironmentRecord {
public:
    DeclarativeEnvironmentRecord(bool shouldUseVector = false,std::pair<InternalAtomicString, ESSlot>* vectorBuffer = NULL, size_t vectorSize = 0)
    {
#if 0
        if(shouldUseVector) {
            m_innerObject = NULL;
            m_vectorData = vectorBuffer;
            m_usedCount = 0;
#ifndef NDEBUG
            m_vectorSize = vectorSize;
#endif
        } else {
            m_innerObject = ESObject::create();
        }
#endif
    }
    ~DeclarativeEnvironmentRecord()
    {
    }

    virtual ESSlot* hasBinding(const InternalAtomicString& name)
    {
#if 0
        if(UNLIKELY(m_innerObject != NULL)) {
            ESSlot* slot = m_innerObject->find(name);
            if(slot) {
                return slot;
            }
            return NULL;
        } else {
            for(unsigned i = 0; i < m_usedCount ; i ++) {
                if(m_vectorData[i].first == name) {
                    return &m_vectorData[i].second;
                }
            }
            return NULL;
        }
#else
        return NULL;
#endif
    }
    void createMutableBindingForNonActivationMode(size_t index, const InternalAtomicString& name,const ESValue& val = ESValue())
    {
#if 0
        ASSERT(!m_innerObject);
        m_vectorData[index].first = name;
        m_vectorData[index].second.init(val, false, false, false);
        m_usedCount++;
#endif
    }

    virtual void createMutableBinding(const InternalAtomicString& name, bool canDelete = false)
    {
#if 0
        //TODO canDelete
        ASSERT(m_innerObject);
        m_innerObject->set(name, esUndefined);

#endif
    }

    virtual void setMutableBinding(const InternalAtomicString& name, ESValue* V, bool mustNotThrowTypeErrorExecption)
    {
#if 0
        //TODO mustNotThrowTypeErrorExecption
        if(UNLIKELY(m_innerObject != NULL)) {
            m_innerObject->set(name, V);
        } else {
            for(unsigned i = 0; i < m_usedCount ; i ++) {
                if(m_vectorData[i].first == name) {
                    m_vectorData[i].second.setValue(V);
                    return;
                }
            }
            RELEASE_ASSERT_NOT_REACHED();
        }
#endif
    }

    ESSlot* getBindingValueForNonActivationMode(size_t idx)
    {
        return &m_vectorData[idx].second;
    }

    virtual ESValue* getBindingValue(const InternalAtomicString& name, bool ignoreReferenceErrorException)
    {
#if 0
        //TODO ignoreReferenceErrorException
        if(UNLIKELY(m_innerObject != NULL)) {
            return m_innerObject->get(name);
        } else {
            for(unsigned i = 0; i < m_usedCount ; i ++) {
                if(m_vectorData[i].first == name) {
                    return &m_vectorData[i].second;
                }
            }
            RELEASE_ASSERT_NOT_REACHED();
        }
#else
        return NULL;
#endif
    }

    virtual bool isDeclarativeEnvironmentRecord()
    {
        return true;
    }

    virtual bool hasThisBinding()
    {
        return false;
    }

protected:
    std::pair<InternalAtomicString, ESSlot>* m_vectorData;
    size_t m_usedCount;
#ifndef NDEBUG
    size_t m_vectorSize;
#endif
    ESObject* m_innerObject;
};

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-global-environment-records
class GlobalEnvironmentRecord : public EnvironmentRecord {
public:
    GlobalEnvironmentRecord(ESObject* globalObject)
    {
        m_objectRecord = new ObjectEnvironmentRecord(globalObject);
        m_declarativeRecord = new DeclarativeEnvironmentRecord();
    }
    ~GlobalEnvironmentRecord() { }

    virtual ESSlot* hasBinding(const InternalAtomicString& name);
    void createMutableBinding(const InternalAtomicString& name, bool canDelete = false);
    void initializeBinding(const InternalAtomicString& name, ESValue* V);
    void setMutableBinding(const InternalAtomicString& name, ESValue* V, bool mustNotThrowTypeErrorExecption);

    ESObject* getThisBinding();
    bool hasVarDeclaration(const InternalAtomicString& name);
    //bool hasLexicalDeclaration(const InternalString& name);
    bool hasRestrictedGlobalProperty(const InternalAtomicString& name);
    bool canDeclareGlobalVar(const InternalAtomicString& name);
    bool canDeclareGlobalFunction(const InternalAtomicString& name);
    void createGlobalVarBinding(const InternalAtomicString& name, bool canDelete);
    void createGlobalFunctionBinding(const InternalAtomicString& name, ESValue* V, bool canDelete);
    ESValue* getBindingValue(const InternalAtomicString& name, bool ignoreReferenceErrorException);

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



//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-function-environment-records
class FunctionEnvironmentRecord : public DeclarativeEnvironmentRecord {
    friend class LexicalEnvironment;
    friend class ESFunctionObject;
public:
    FunctionEnvironmentRecord(bool shouldUseVector = false,std::pair<InternalAtomicString, ESSlot>* vectorBuffer = NULL, size_t vectorSize = 0)
        : DeclarativeEnvironmentRecord(shouldUseVector, vectorBuffer, vectorSize)
    {
        m_thisBindingStatus = Uninitialized;
    }
    enum ThisBindingStatus {
        Lexical, Initialized, Uninitialized
    };
    virtual bool hasThisBinding()
    {
        //we dont use arrow function now. so binding status is alwalys not lexical.
        return true;
    }

    //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-bindthisvalue
    void bindThisValue(ESObject* V);
    ESObject* getThisBinding();

protected:
    ESValue m_thisValue;
    ESFunctionObject* m_functionObject;
    ESValue m_newTarget; //TODO
    ThisBindingStatus m_thisBindingStatus;
};

/*
//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-module-environment-records
class ModuleEnvironmentRecord : public DeclarativeEnvironmentRecord {
protected:
};
*/


}
#endif
