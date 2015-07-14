#ifndef Environment_h
#define Environment_h

#include "ESValue.h"
#include "ESValueInlines.h"
#include <vector>

namespace escargot {

class EnvironmentRecord;
class ObjectEnvironmentRecord;
class DeclarativeEnvironmentRecord;

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
        ESValue m_value;
        bool m_isMutable:1;
        bool m_canDelete:1;
        //6-bits remain
    };
    EnvironmentRecord() { }
public:
    virtual ~EnvironmentRecord() { }
    virtual bool hasBinding(const std::wstring& name) = 0;
    virtual void createMutableBinding(const std::wstring& name, bool canDelete = false) = 0;
    virtual void createImmutableBinding(const std::wstring& name, bool throwExecptionWhenAccessBeforeInit = false) = 0;
    virtual void initializeBinding(const std::wstring& name, ESValue V) = 0;
    virtual void setMutableBinding(const std::wstring& name, ESValue V, bool mustNotThrowTypeErrorExecption) = 0;
    virtual ESValue getBindingValue(const std::wstring& name, bool ignoreReferenceErrorException) = 0;
    virtual bool deleteBinding(const std::wstring& name) = 0;
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
    bool hasBinding(const std::wstring& name)
    {
        return false;
    }
    void createMutableBinding(const std::wstring& name, bool canDelete = false) {}
    void createImmutableBinding(const std::wstring& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void initializeBinding(const std::wstring& name, ESValue V) {}
    void setMutableBinding(const std::wstring& name, ESValue V, bool mustNotThrowTypeErrorExecption) {}
    ESValue getBindingValue(const std::wstring& name, bool ignoreReferenceErrorException)
    {
        return ESValue();
    }
    bool deleteBinding(const std::wstring& name)
    {
        return false;
    }

    ESValue getThisBinding();
    bool hasVarDeclaration(const std::wstring& name);
    //bool hasLexicalDeclaration(const std::wstring& name);
    bool hasRestrictedGlobalProperty(const std::wstring& name);
    bool canDeclareGlobalVar(const std::wstring& name);
    bool canDeclareGlobalFunction(const std::wstring& name);
    void createGlobalVarBinding(const std::wstring& name, bool canDelete);
    void createGlobalFunctionBinding(const std::wstring& name, ESValue V, bool canDelete);

protected:
    ObjectEnvironmentRecord* m_objectRecord;
    DeclarativeEnvironmentRecord* m_declarativeRecord;
    std::vector<std::wstring> varNames;
};

class ObjectEnvironmentRecord : public EnvironmentRecord {
public:
    ~ObjectEnvironmentRecord() { }
    bool hasBinding(const std::wstring& name)
    {
        return false;
    }
    void createMutableBinding(const std::wstring& name, bool canDelete = false) {}
    void createImmutableBinding(const std::wstring& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void initializeBinding(const std::wstring& name, ESValue V) {}
    void setMutableBinding(const std::wstring& name, ESValue V, bool mustNotThrowTypeErrorExecption) {}
    ESValue getBindingValue(const std::wstring& name, bool ignoreReferenceErrorException)
    {
        return ESValue();
    }
    bool deleteBinding(const std::wstring& name)
    {
        return false;
    }
};

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-declarative-environment-records
class DeclarativeEnvironmentRecord : public EnvironmentRecord {
public:
    ~DeclarativeEnvironmentRecord() { }
    bool hasBinding(const std::wstring& name)
    {
        return false;
    }
    void createMutableBinding(const std::wstring& name, bool canDelete = false) {}
    void createImmutableBinding(const std::wstring& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void initializeBinding(const std::wstring& name, ESValue V) {}
    void setMutableBinding(const std::wstring& name, ESValue V, bool mustNotThrowTypeErrorExecption) {}
    ESValue getBindingValue(const std::wstring& name, bool ignoreReferenceErrorException)
    {
        return ESValue();
    }
    bool deleteBinding(const std::wstring& name)
    {
        return false;
    }
};

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-function-environment-records
class FunctionEnvironmentRecord : public DeclarativeEnvironmentRecord {
    bool hasBinding(const std::wstring& name)
    {
        return false;
    }
    void createMutableBinding(const std::wstring& name, bool canDelete = false) {}
    void createImmutableBinding(const std::wstring& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void initializeBinding(const std::wstring& name, ESValue V) {}
    void setMutableBinding(const std::wstring& name, ESValue V, bool mustNotThrowTypeErrorExecption) {}
    ESValue getBindingValue(const std::wstring& name, bool ignoreReferenceErrorException)
    {
        return ESValue();
    }
    bool deleteBinding(const std::wstring& name)
    {
        return false;
    }
protected:
    ESValue m_thisValue;
};

/*
//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-module-environment-records
class ModuleEnvironmentRecord : public DeclarativeEnvironmentRecord {
protected:
};
*/

}
#endif
