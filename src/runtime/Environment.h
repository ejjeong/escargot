#ifndef Environment_h
#define Environment_h

#include "ESValue.h"

namespace escargot {

class EnvironmentRecord;

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-lexical-environments
class LexicalEnvironment : public gc_cleanup {
public:
    LexicalEnvironment(EnvironmentRecord* record,LexicalEnvironment* env)
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
    virtual bool HasBinding(const std::wstring& name) = 0;
    virtual void CreateMutableBinding(const std::wstring& name, bool canDelete = false) = 0;
    virtual void CreateImmutableBinding(const std::wstring& name, bool throwExecptionWhenAccessBeforeInit = false) = 0;
    virtual void InitializeBinding(const std::wstring& name, ESValue V) = 0;
    virtual void SetMutableBinding(const std::wstring& name, ESValue V, bool mustNotThrowTypeErrorExecption) = 0;
    virtual ESValue GetBindingValue(const std::wstring& name, bool ignoreReferenceErrorException) = 0;
    virtual bool DeleteBinding(const std::wstring& name) = 0;
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
    bool HasBinding(const std::wstring& name)
    {
        return false;
    }
    void CreateMutableBinding(const std::wstring& name, bool canDelete = false) {}
    void CreateImmutableBinding(const std::wstring& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void InitializeBinding(const std::wstring& name, ESValue V) {}
    void SetMutableBinding(const std::wstring& name, ESValue V, bool mustNotThrowTypeErrorExecption) {}
    ESValue GetBindingValue(const std::wstring& name, bool ignoreReferenceErrorException)
    {
        return ESValue();
    }
    bool DeleteBinding(const std::wstring& name)
    {
        return false;
    }
};

class ObjectEnvironmentRecord : public EnvironmentRecord {
public:
    ~ObjectEnvironmentRecord() { }
    bool HasBinding(const std::wstring& name)
    {
        return false;
    }
    void CreateMutableBinding(const std::wstring& name, bool canDelete = false) {}
    void CreateImmutableBinding(const std::wstring& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void InitializeBinding(const std::wstring& name, ESValue V) {}
    void SetMutableBinding(const std::wstring& name, ESValue V, bool mustNotThrowTypeErrorExecption) {}
    ESValue GetBindingValue(const std::wstring& name, bool ignoreReferenceErrorException)
    {
        return ESValue();
    }
    bool DeleteBinding(const std::wstring& name)
    {
        return false;
    }
};

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-declarative-environment-records
class DeclarativeEnvironmentRecord : public EnvironmentRecord {
public:
    ~DeclarativeEnvironmentRecord() { }
    bool HasBinding(const std::wstring& name)
    {
        return false;
    }
    void CreateMutableBinding(const std::wstring& name, bool canDelete = false) {}
    void CreateImmutableBinding(const std::wstring& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void InitializeBinding(const std::wstring& name, ESValue V) {}
    void SetMutableBinding(const std::wstring& name, ESValue V, bool mustNotThrowTypeErrorExecption) {}
    ESValue GetBindingValue(const std::wstring& name, bool ignoreReferenceErrorException)
    {
        return ESValue();
    }
    bool DeleteBinding(const std::wstring& name)
    {
        return false;
    }
};

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-function-environment-records
class FunctionEnvironmentRecord : public DeclarativeEnvironmentRecord {
    bool HasBinding(const std::wstring& name)
    {
        return false;
    }
    void CreateMutableBinding(const std::wstring& name, bool canDelete = false) {}
    void CreateImmutableBinding(const std::wstring& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void InitializeBinding(const std::wstring& name, ESValue V) {}
    void SetMutableBinding(const std::wstring& name, ESValue V, bool mustNotThrowTypeErrorExecption) {}
    ESValue GetBindingValue(const std::wstring& name, bool ignoreReferenceErrorException)
    {
        return ESValue();
    }
    bool DeleteBinding(const std::wstring& name)
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
