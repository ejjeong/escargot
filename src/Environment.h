#ifndef Environment_h
#define Environment_h

#include "ESValue.h"
#include <unordered_map>

class EnvironmentRecord;
class LexicalEnvironment {
public:
    EnvironmentRecord* getRecord();
    EnvironmentRecord* record;
    LexicalEnvironment* outerEnv;
};

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-environment-records
class EnvironmentRecord {
    struct EnvironmentRecordValue {
        ESValue value;
        bool isMutable;
        bool canDelete;
    };
public:
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
    std::unordered_map<std::wstring, EnvironmentRecordValue> values;
};

//http://www.ecma-international.org/ecma-262/6.0/index.html#sec-global-environment-records
class GlobalEnvironmentRecord : public EnvironmentRecord {
    bool HasBinding(const std::wstring& name) {}
    void CreateMutableBinding(const std::wstring& name, bool canDelete = false) {}
    void CreateImmutableBinding(const std::wstring& name, bool throwExecptionWhenAccessBeforeInit = false) {}
    void InitializeBinding(const std::wstring& name, ESValue V) {}
    void SetMutableBinding(const std::wstring& name, ESValue V, bool mustNotThrowTypeErrorExecption) {}
    ESValue GetBindingValue(const std::wstring& name, bool ignoreReferenceErrorException) {}
    bool DeleteBinding(const std::wstring& name) {}
};

class ObjectEnvironmentRecord : public EnvironmentRecord {

};

class DeclarativeEnvironmentRecord : public EnvironmentRecord {
};


class FunctionEnvironmentRecord : public DeclarativeEnvironmentRecord {
protected:
    ESValue thisValue;
};

class ModuleEnvironmentRecord : public DeclarativeEnvironmentRecord {
protected:
};

#endif
