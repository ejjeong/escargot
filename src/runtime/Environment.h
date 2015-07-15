#ifndef Environment_h
#define Environment_h

#include "ESValue.h"
#include "ESValueInlines.h"
#include <vector>
#include <algorithm>

namespace escargot {

class EnvironmentRecord;
class ObjectEnvironmentRecord;
class DeclarativeEnvironmentRecord;
class JSObject;
class GlobalObject;

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-lexical-environments
class LexicalEnvironment : public gc_cleanup {
public:
    LexicalEnvironment(EnvironmentRecord* record, LexicalEnvironment* env)
        : m_record(record)
        , m_outerEnvironment(env)
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
protected:
    EnvironmentRecord* m_record;
    LexicalEnvironment* m_outerEnvironment;
};

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-environment-records
class EnvironmentRecord : public gc_cleanup {
protected:
    struct EnvironmentRecordValue {
        ESValue* m_value;
        bool m_isMutable:1;
        bool m_canDelete:1;
        //6-bits remain
    };
    EnvironmentRecord() { }
public:
    virtual ~EnvironmentRecord() { }
    virtual bool hasBinding(const ESString& name) = 0;
    virtual void createMutableBinding(const ESString& name, bool canDelete = false) = 0;
    virtual void createImmutableBinding(const ESString& name, bool throwExecptionWhenAccessBeforeInit = false) = 0;
    virtual void initializeBinding(const ESString& name, ESValue* V) = 0;
    virtual void setMutableBinding(const ESString& name, ESValue* V, bool mustNotThrowTypeErrorExecption) = 0;
    virtual ESValue* getBindingValue(const ESString& name, bool ignoreReferenceErrorException) = 0;
    virtual bool deleteBinding(const ESString& name) = 0;
    //HasThisBinding()
    //HasSuperBinding()
    //WithBaseObject ()

protected:
    std::unordered_map<std::wstring, EnvironmentRecordValue> m_values;
};

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-global-environment-records
class GlobalEnvironmentRecord : public EnvironmentRecord {
public:
    ~GlobalEnvironmentRecord() { }
    GlobalEnvironmentRecord(GlobalObject* G);
    bool hasBinding(const ESString& name);
    void createMutableBinding(const ESString& name, bool canDelete = false);
    void createImmutableBinding(const ESString& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void initializeBinding(const ESString& name, ESValue* V);
    void setMutableBinding(const ESString& name, ESValue* V, bool mustNotThrowTypeErrorExecption) {}
    ESValue* getBindingValue(const ESString& name, bool ignoreReferenceErrorException);
    bool deleteBinding(const ESString& name)
    {
        return false;
    }

    ESValue* getThisBinding();
    bool hasVarDeclaration(const ESString& name);
    //bool hasLexicalDeclaration(const ESString& name);
    bool hasRestrictedGlobalProperty(const ESString& name);
    bool canDeclareGlobalVar(const ESString& name);
    bool canDeclareGlobalFunction(const ESString& name);
    void createGlobalVarBinding(const ESString& name, bool canDelete);
    void createGlobalFunctionBinding(const ESString& name, ESValue* V, bool canDelete);

protected:
    ObjectEnvironmentRecord* m_objectRecord;
    DeclarativeEnvironmentRecord* m_declarativeRecord;
    std::vector<ESString> m_varNames;
};

class ObjectEnvironmentRecord : public EnvironmentRecord {
public:
    ObjectEnvironmentRecord(JSObject* O)
        : m_bindingObject(O)
    {
    }
    ~ObjectEnvironmentRecord() { }
    bool hasBinding(const ESString& name)
    {
        return false;
    }
    void createMutableBinding(const ESString& name, bool canDelete = false);
    void createImmutableBinding(const ESString& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void initializeBinding(const ESString& name, ESValue* V);
    void setMutableBinding(const ESString& name, ESValue* V, bool mustNotThrowTypeErrorExecption);
    ESValue* getBindingValue(const ESString& name, bool ignoreReferenceErrorException)
    {
        return NULL;//ESValue();
    }
    bool deleteBinding(const ESString& name)
    {
        return false;
    }
    ALWAYS_INLINE JSObject* bindingObject() {
        return m_bindingObject;
    }

protected:
    JSObject* m_bindingObject;
};

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-declarative-environment-records
class DeclarativeEnvironmentRecord : public EnvironmentRecord {
public:
    ~DeclarativeEnvironmentRecord() { }
    bool hasBinding(const ESString& name)
    {
        return false;
    }
    void createMutableBinding(const ESString& name, bool canDelete = false) {}
    void createImmutableBinding(const ESString& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void initializeBinding(const ESString& name, ESValue* V) {}
    void setMutableBinding(const ESString& name, ESValue* V, bool mustNotThrowTypeErrorExecption) {}
    ESValue* getBindingValue(const ESString& name, bool ignoreReferenceErrorException)
    {
        return NULL;//ESValue();
    }
    bool deleteBinding(const ESString& name)
    {
        return false;
    }
};

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-function-environment-records
class FunctionEnvironmentRecord : public DeclarativeEnvironmentRecord {
    bool hasBinding(const ESString& name)
    {
        return false;
    }
    void createMutableBinding(const ESString& name, bool canDelete = false) {}
    void createImmutableBinding(const ESString& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void initializeBinding(const ESString& name, ESValue* V) {}
    void setMutableBinding(const ESString& name, ESValue* V, bool mustNotThrowTypeErrorExecption) {}
    ESValue* getBindingValue(const ESString& name, bool ignoreReferenceErrorException)
    {
        return NULL;//ESValue();
    }
    bool deleteBinding(const ESString& name)
    {
        return false;
    }
protected:
    ESValue* m_thisValue;
};

/*
//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-module-environment-records
class ModuleEnvironmentRecord : public DeclarativeEnvironmentRecord {
protected:
};
*/

}
#endif
