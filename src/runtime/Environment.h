#ifndef Environment_h
#define Environment_h

#include "ESValue.h"
#include "ESValueInlines.h"

namespace escargot {

class EnvironmentRecord;
class DeclarativeEnvironmentRecord;
class GlobalEnvironmentRecord;
class ObjectEnvironmentRecord;
class JSObject;
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
    static LexicalEnvironment* newFunctionEnvironment(JSFunction* function, ESValue* newTarget);

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
    virtual JSSlot* hasBinding(const ESAtomicString& name)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void createMutableBinding(const ESAtomicString& name, bool canDelete = false)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void createImmutableBinding(const ESAtomicString& name, bool throwExecptionWhenAccessBeforeInit = false)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void initializeBinding(const ESAtomicString& name, ESValue* V)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void setMutableBinding(const ESAtomicString& name, ESValue* V, bool mustNotThrowTypeErrorExecption)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual ESValue* getBindingValue(const ESAtomicString& name, bool ignoreReferenceErrorException)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual bool deleteBinding(const ESAtomicString& name)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual bool hasSuperBinding()
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual JSObject* getThisBinding()
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

    void createMutableBindingForAST(const ESAtomicString& name,bool canDelete);

protected:
};

class ObjectEnvironmentRecord : public EnvironmentRecord {
public:
    ObjectEnvironmentRecord(JSObject* O)
        : m_bindingObject(O)
    {
    }
    ~ObjectEnvironmentRecord() { }

    //return NULL == not exist
    virtual JSSlot* hasBinding(const ESAtomicString& name)
    {
        JSSlot* slot = m_bindingObject->find(name);
        if(slot) {
            return slot;
        }
        return NULL;
    }
    void createMutableBinding(const ESAtomicString& name, bool canDelete = false);
    void createImmutableBinding(const ESAtomicString& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void initializeBinding(const ESAtomicString& name, ESValue* V);
    void setMutableBinding(const ESAtomicString& name, ESValue* V, bool mustNotThrowTypeErrorExecption);
    ESValue* getBindingValue(const ESAtomicString& name, bool ignoreReferenceErrorException)
    {
        return m_bindingObject->get(name);
    }
    bool deleteBinding(const ESAtomicString& name)
    {
        return false;
    }
    ALWAYS_INLINE JSObject* bindingObject() {
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
    JSObject* m_bindingObject;
};

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-declarative-environment-records
class DeclarativeEnvironmentRecord : public EnvironmentRecord {
public:
    DeclarativeEnvironmentRecord(bool shouldUseVector = false, size_t vectorSize = 0)
    {
        if(shouldUseVector) {
            m_innerObject = NULL;
            m_vectorData = (std::pair<ESAtomicString, JSSlot> *)malloc(sizeof (std::pair<ESAtomicString, JSSlot>) * vectorSize);
            m_usedCount = 0;
#ifndef NDEBUG
            m_vectorSize = vectorSize;
#endif
        } else {
            m_innerObject = JSObject::create();
        }
    }
    ~DeclarativeEnvironmentRecord()
    {
        if(!m_innerObject) {
            free(m_vectorData);
        }
    }

    virtual JSSlot* hasBinding(const ESAtomicString& name)
    {
        if(UNLIKELY(m_innerObject != NULL)) {
            JSSlot* slot = m_innerObject->find(name);
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
    }
    virtual void createMutableBinding(const ESAtomicString& name, bool canDelete = false)
    {
        //TODO canDelete
        if(UNLIKELY(m_innerObject != NULL)) {
            m_innerObject->set(name, esUndefined);
        } else {
            if(!hasBinding(name)) {
#ifndef NDEBUG
                ASSERT(m_usedCount != m_vectorSize);
#endif
                m_vectorData[m_usedCount].first = name;
                m_vectorData[m_usedCount].second.init(esUndefined, false, false, false);
                m_usedCount++;
            }
        }
    }

    virtual void setMutableBinding(const ESAtomicString& name, ESValue* V, bool mustNotThrowTypeErrorExecption)
    {
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
    }

    virtual ESValue* getBindingValue(const ESAtomicString& name, bool ignoreReferenceErrorException)
    {
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
    std::pair<ESAtomicString, JSSlot>* m_vectorData;
    size_t m_usedCount;
#ifndef NDEBUG
    size_t m_vectorSize;
#endif
    JSObject* m_innerObject;
};

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-global-environment-records
class GlobalEnvironmentRecord : public EnvironmentRecord {
public:
    GlobalEnvironmentRecord(JSObject* globalObject)
    {
        m_objectRecord = new ObjectEnvironmentRecord(globalObject);
        m_declarativeRecord = new DeclarativeEnvironmentRecord();
    }
    ~GlobalEnvironmentRecord() { }

    virtual JSSlot* hasBinding(const ESAtomicString& name);
    void createMutableBinding(const ESAtomicString& name, bool canDelete = false);
    void initializeBinding(const ESAtomicString& name, ESValue* V);
    void setMutableBinding(const ESAtomicString& name, ESValue* V, bool mustNotThrowTypeErrorExecption);

    JSObject* getThisBinding();
    bool hasVarDeclaration(const ESAtomicString& name);
    //bool hasLexicalDeclaration(const ESString& name);
    bool hasRestrictedGlobalProperty(const ESAtomicString& name);
    bool canDeclareGlobalVar(const ESAtomicString& name);
    bool canDeclareGlobalFunction(const ESAtomicString& name);
    void createGlobalVarBinding(const ESAtomicString& name, bool canDelete);
    void createGlobalFunctionBinding(const ESAtomicString& name, ESValue* V, bool canDelete);
    ESValue* getBindingValue(const ESAtomicString& name, bool ignoreReferenceErrorException);

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
    std::vector<ESAtomicString, gc_allocator<ESAtomicString> > m_varNames;
};



//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-function-environment-records
class FunctionEnvironmentRecord : public DeclarativeEnvironmentRecord {
    friend class LexicalEnvironment;
    friend class ESFunctionCaller;
public:
    FunctionEnvironmentRecord(bool shouldUseVector = false, size_t vectorSize = 0)
        : DeclarativeEnvironmentRecord(shouldUseVector, vectorSize)
    {
        m_thisBindingStatus = Uninitialized;
        m_thisValue = esUndefined;
        m_newTarget = esUndefined;
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
    void bindThisValue(JSObject* V);
    JSObject* getThisBinding();

protected:
    ESValue* m_thisValue;
    JSFunction* m_functionObject;
    ESValue* m_newTarget; //TODO
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
