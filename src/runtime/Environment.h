#ifndef Environment_h
#define Environment_h

#include "ESValue.h"
#include "ESValueInlines.h"
#include <vector>
#include <algorithm>

namespace escargot {

class EnvironmentRecord;
class DeclarativeEnvironmentRecord;
class GlobalEnvironmentRecord;
class ObjectEnvironmentRecord;
class JSObject;
class GlobalObject;

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-lexical-environments
class LexicalEnvironment : public gc_cleanup {
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
class EnvironmentRecord : public gc_cleanup {
protected:
    EnvironmentRecord() { }
public:
    virtual ~EnvironmentRecord() { }

    //return NULL == not exist
    virtual JSObjectSlot* hasBinding(const ESString& name)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }
    virtual void createMutableBinding(const ESString& name, bool canDelete = false)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void createImmutableBinding(const ESString& name, bool throwExecptionWhenAccessBeforeInit = false)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void initializeBinding(const ESString& name, ESValue* V)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void setMutableBinding(const ESString& name, ESValue* V, bool mustNotThrowTypeErrorExecption)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual ESValue* getBindingValue(const ESString& name, bool ignoreReferenceErrorException)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual bool deleteBinding(const ESString& name)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    //HasSuperBinding()
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

    void createMutableBindingForAST(const ESString& name,bool canDelete);

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
    virtual JSObjectSlot* hasBinding(const ESString& name)
    {
        JSObjectSlot* slot = m_bindingObject->find(name);
        if(slot) {
            return slot;
        }
        return NULL;
    }
    void createMutableBinding(const ESString& name, bool canDelete = false);
    void createImmutableBinding(const ESString& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void initializeBinding(const ESString& name, ESValue* V);
    void setMutableBinding(const ESString& name, ESValue* V, bool mustNotThrowTypeErrorExecption);
    ESValue* getBindingValue(const ESString& name, bool ignoreReferenceErrorException)
    {
        return m_bindingObject->get(name);
    }
    bool deleteBinding(const ESString& name)
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

protected:
    JSObject* m_bindingObject;
};

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-declarative-environment-records
class DeclarativeEnvironmentRecord : public EnvironmentRecord {
public:
    DeclarativeEnvironmentRecord(JSObject* innerObject = NULL)
    {
        m_innerObject = innerObject;
        if(m_innerObject == NULL) {
            m_innerObject = JSObject::create();
        }
    }
    ~DeclarativeEnvironmentRecord() { }

    virtual JSObjectSlot* hasBinding(const ESString& name)
    {
        JSObjectSlot* slot = m_innerObject->find(name);
        if(slot) {
            return slot;
        }
        return NULL;
    }
    virtual void createMutableBinding(const ESString& name, bool canDelete = false)
    {
        //TODO canDelete
        m_innerObject->set(name, undefined);
    }

    virtual void setMutableBinding(const ESString& name, ESValue* V, bool mustNotThrowTypeErrorExecption)
    {
        //TODO mustNotThrowTypeErrorExecption
        m_innerObject->set(name, V);
    }

    virtual ESValue* getBindingValue(const ESString& name, bool ignoreReferenceErrorException)
    {
        //TODO ignoreReferenceErrorException
        return m_innerObject->get(name);
    }

    virtual bool isDeclarativeEnvironmentRecord()
    {
        return true;
    }

    JSObject* innerObject() { return m_innerObject; }

protected:
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

    virtual JSObjectSlot* hasBinding(const ESString& name);
    void createMutableBinding(const ESString& name, bool canDelete = false);
    void initializeBinding(const ESString& name, ESValue* V);
    void setMutableBinding(const ESString& name, ESValue* V, bool mustNotThrowTypeErrorExecption);

    ESValue* getThisBinding();
    bool hasVarDeclaration(const ESString& name);
    //bool hasLexicalDeclaration(const ESString& name);
    bool hasRestrictedGlobalProperty(const ESString& name);
    bool canDeclareGlobalVar(const ESString& name);
    bool canDeclareGlobalFunction(const ESString& name);
    void createGlobalVarBinding(const ESString& name, bool canDelete);
    void createGlobalFunctionBinding(const ESString& name, ESValue* V, bool canDelete);
    ESValue* getBindingValue(const ESString& name, bool ignoreReferenceErrorException);

    virtual bool isGlobalEnvironmentRecord()
    {
        return true;
    }

protected:
    ObjectEnvironmentRecord* m_objectRecord;
    DeclarativeEnvironmentRecord* m_declarativeRecord;
    std::vector<ESString, gc_allocator<ESString>> m_varNames;
};



//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-function-environment-records
class FunctionEnvironmentRecord : public DeclarativeEnvironmentRecord {
    friend class LexicalEnvironment;
public:
protected:
    ESValue* m_thisValue;
    JSFunction* m_functionObject;
};

/*
//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-module-environment-records
class ModuleEnvironmentRecord : public DeclarativeEnvironmentRecord {
protected:
};
*/


}
#endif
