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

typedef std::unordered_map<InternalAtomicString, ::escargot::ESSlot,
            std::hash<InternalAtomicString>,std::equal_to<InternalAtomicString>,
            gc_allocator<std::pair<const InternalAtomicString, ::escargot::ESSlot> > > ESIdentifierMapStd;

class ESIdentifierMap : public ESIdentifierMapStd, public gc {
public:
    ESIdentifierMap(size_t siz = 0)
        : ESIdentifierMapStd(siz) { }

};

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
    static LexicalEnvironment* newFunctionEnvironment(ESFunctionObject* function, const ESValue& newTarget);

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
    virtual ESSlot* hasBinding(const InternalAtomicString& atomicName, const InternalString& name)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void createMutableBinding(const InternalAtomicString& name, const InternalString& nonAtomicName, bool canDelete = false)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void createImmutableBinding(const InternalAtomicString& name, bool throwExecptionWhenAccessBeforeInit = false)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void initializeBinding(const InternalAtomicString& name, const InternalString& nonAtomicName, const ESValue& V)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void setMutableBinding(const InternalAtomicString& name, const InternalString& nonAtomicName, const ESValue& V, bool mustNotThrowTypeErrorExecption)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    /*virtual ESValue getBindingValue(const InternalAtomicString& name, bool ignoreReferenceErrorException)
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

    void createMutableBindingForAST(const InternalAtomicString& atomicName,const InternalString& name,bool canDelete);

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
    virtual ESSlot* hasBinding(const InternalAtomicString& atomicName, const InternalString& name)
    {
        ESSlot* slot = m_bindingObject->find(name);
        if(slot) {
            return slot;
        }
        return NULL;
    }
    void createMutableBinding(const InternalAtomicString& name,const InternalString& nonAtomicName, bool canDelete = false);
    void createImmutableBinding(const InternalAtomicString& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void initializeBinding(const InternalAtomicString& name,const InternalString& nonAtomicName,  const ESValue& V);
    void setMutableBinding(const InternalAtomicString& name,const InternalString& nonAtomicName, const ESValue& V, bool mustNotThrowTypeErrorExecption);
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

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-declarative-environment-records
class DeclarativeEnvironmentRecord : public EnvironmentRecord {
public:
    DeclarativeEnvironmentRecord(bool shouldUseVector = false,std::pair<InternalAtomicString, ESSlot>* vectorBuffer = NULL, size_t vectorSize = 0)
    {
        if(shouldUseVector) {
            m_needsActivation = false;
            m_mapData = NULL;
            m_vectorData = vectorBuffer;
            m_usedCount = 0;
#ifndef NDEBUG
            m_vectorSize = vectorSize;
#endif
        } else {
            m_needsActivation = true;
            m_mapData = new ESIdentifierMap(16);
        }
    }
    ~DeclarativeEnvironmentRecord()
    {
    }

    virtual ESSlot* hasBinding(const InternalAtomicString& atomicName, const InternalString& name)
    {
        if(UNLIKELY(m_needsActivation)) {
            auto iter = m_mapData->find(atomicName);
            if(iter != m_mapData->end()) {
                return &iter->second;
            }
            return NULL;
        } else {
            for(unsigned i = 0; i < m_usedCount ; i ++) {
                if(m_vectorData[i].first == atomicName) {
                    return &m_vectorData[i].second;
                }
            }
            return NULL;
        }
    }
    void createMutableBindingForNonActivationMode(size_t index, const InternalAtomicString& name,const ESValue& val = ESValue())
    {
        ASSERT(!m_needsActivation);
        m_vectorData[index].first = name;
        m_vectorData[index].second.init(val, false, false, false);
        m_usedCount++;
    }

    virtual void createMutableBinding(const InternalAtomicString& name,const InternalString& nonAtomicName, bool canDelete = false)
    {
        //TODO canDelete
        ASSERT(m_needsActivation);
        m_mapData->insert(std::make_pair(name, ESSlot()));
    }

    virtual void setMutableBinding(const InternalAtomicString& name, const InternalString& nonAtomicName, const ESValue& V, bool mustNotThrowTypeErrorExecption)
    {
        //TODO mustNotThrowTypeErrorExecption
        if(UNLIKELY(m_needsActivation)) {
            auto iter = m_mapData->find(name);
            ASSERT(iter != m_mapData->end());
            iter->second.setValue(V);
        } else {
            for(unsigned i = 0; i < m_usedCount ; i ++) {
                if(m_vectorData[i].first == name) {
                    m_vectorData[i].second.setValue(V);
                    return;
                }
            }
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    ESSlot* getBindingValueForNonActivationMode(size_t idx)
    {
        return &m_vectorData[idx].second;
    }

    /*
    virtual ESValue getBindingValue(const InternalAtomicString& name, bool ignoreReferenceErrorException)
    {
        //TODO ignoreReferenceErrorException
        if(UNLIKELY(m_needsActivation)) {
            auto iter = m_mapData->find(name);
            ASSERT(iter != m_mapData->end());
            return iter->second.value();
        } else {
            for(unsigned i = 0; i < m_usedCount ; i ++) {
                if(m_vectorData[i].first == name) {
                    return &m_vectorData[i].second;
                }
            }
            RELEASE_ASSERT_NOT_REACHED();
        }
    }*/

    virtual bool isDeclarativeEnvironmentRecord()
    {
        return true;
    }

    virtual bool hasThisBinding()
    {
        return false;
    }

protected:
    bool m_needsActivation;

    std::pair<InternalAtomicString, ESSlot>* m_vectorData;
    size_t m_usedCount;
#ifndef NDEBUG
    size_t m_vectorSize;
#endif
    ESIdentifierMap* m_mapData;
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

    virtual ESSlot* hasBinding(const InternalAtomicString& atomicName, const InternalString& name);
    void createMutableBinding(const InternalAtomicString& name,const InternalString& nonAtomicName, bool canDelete = false);
    void initializeBinding(const InternalAtomicString& name,const InternalString& nonAtomicName,  const ESValue& V);
    void setMutableBinding(const InternalAtomicString& name, const InternalString& nonAtomicName, const ESValue& V, bool mustNotThrowTypeErrorExecption);

    ESObject* getThisBinding();
    bool hasVarDeclaration(const InternalAtomicString& name);
    //bool hasLexicalDeclaration(const InternalString& name);
    bool hasRestrictedGlobalProperty(const InternalAtomicString& name);
    bool canDeclareGlobalVar(const InternalAtomicString& name);
    bool canDeclareGlobalFunction(const InternalAtomicString& name);
    void createGlobalVarBinding(const InternalAtomicString& name,const InternalString& nonAtomicName, bool canDelete);
    void createGlobalFunctionBinding(const InternalAtomicString& name,const InternalString& nonAtomicName, const ESValue& V, bool canDelete);
    //ESValue getBindingValue(const InternalAtomicString& name, bool ignoreReferenceErrorException);

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
    void bindThisValue(const ESValue& V);
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
