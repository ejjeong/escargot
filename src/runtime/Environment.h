/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

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
    enum Origin { Parameter, FunctionDeclaration, FunctionExpression, VariableDeclarator };
    struct {
        bool m_isHeapAllocated:1;
        bool m_bindingIsImmutable:1;
        Origin m_origin:2;
    } m_flags;

    InnerIdentifierInfo(InternalAtomicString name, Origin origin)
        : m_name(name)
    {
        m_flags.m_isHeapAllocated = false;
        m_flags.m_bindingIsImmutable = false;
        m_flags.m_origin = origin;
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
    static LexicalEnvironment* newFunctionEnvironment(bool needsToPrepareGenerateArgumentsObject, ESValue* stackAllocatedStorage, size_t stackAllocatedStorageSize, const InternalAtomicStringVector& innerIdentifiers, ESValue arguments[], const size_t& argumentCount, ESFunctionObject* function, bool needsActivation, size_t mutableIndex);

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
    virtual ESBindingSlot hasBinding(const InternalAtomicString& atomicName)
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

    GlobalEnvironmentRecord* asGlobalEnvironmentRecord()
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
    virtual ESBindingSlot hasBinding(const InternalAtomicString& atomicName)
    {
        ESBindingSlot addressOfProperty = ((GlobalObject *)m_bindingObject)->addressOfProperty(atomicName.string());
        if (addressOfProperty)
            return addressOfProperty;
        else
            return nullptr;
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
    ALWAYS_INLINE DeclarativeEnvironmentRecord(ESValue* stackAllocatedData, size_t stackAllocatedSize, const InternalAtomicStringVector& innerIdentifiers, bool needsActivation, size_t mutableIndex)
        : m_heapAllocatedData(innerIdentifiers.size())
    {
        if (UNLIKELY(needsActivation)) {
            m_innerIdentifiers = new (GC) InternalAtomicStringVector(innerIdentifiers);
            if (innerIdentifiers.size() >= std::pow(2, 15))
                ESVMInstance::currentInstance()->throwError(RangeError::create(ESString::create("Number of variables in single function should be less than 2^15")));
            m_numVariableDeclarations = innerIdentifiers.size();
        } else {
            m_stackAllocatedData = stackAllocatedData;
            std::fill(stackAllocatedData, &stackAllocatedData[stackAllocatedSize], ESValue());
            m_numVariableDeclarations = stackAllocatedSize;
        }
        m_needsActivation = needsActivation;
        m_mutableIndex = mutableIndex;
    }

    ~DeclarativeEnvironmentRecord()
    {
    }

    virtual ESBindingSlot hasBinding(const InternalAtomicString& atomicName);
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

    bool needsActivation()
    {
        return m_needsActivation;
    }

    size_t numVariableDeclarations()
    {
        return m_numVariableDeclarations;
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
    bool m_needsActivation:1;
#ifdef ESCARGOT_32
    size_t m_mutableIndex:15;
    size_t m_numVariableDeclarations:15;
#elif ESCARGOT_64
    size_t m_mutableIndex:31;
    size_t m_numVariableDeclarations:31;
#endif
};

class DeclarativeEnvironmentRecordForCatchClause : public DeclarativeEnvironmentRecord {
public:
    ALWAYS_INLINE DeclarativeEnvironmentRecordForCatchClause(ESValue* stackAllocatedData, size_t stackAllocatedSize, const InternalAtomicStringVector& innerIdentifiers, bool needsActivation, size_t mutableIndex, const InternalAtomicString& errorName, EnvironmentRecord* parentRecord)
        : DeclarativeEnvironmentRecord(stackAllocatedData, stackAllocatedSize, innerIdentifiers, needsActivation, mutableIndex)
        , m_parentRecord(parentRecord)
        , m_errorName(errorName)
    {
        DeclarativeEnvironmentRecord::createMutableBinding(m_errorName);
    }

    virtual ESBindingSlot hasBinding(const InternalAtomicString& atomicName)
    {
        if (atomicName == m_errorName)
            return DeclarativeEnvironmentRecord::hasBinding(atomicName);
        else
            return m_parentRecord->hasBinding(atomicName);
    }

    void createMutableBinding(const InternalAtomicString& name, bool canDelete = false)
    {
        m_parentRecord->createMutableBindingForAST(name, canDelete);
    }

    void setMutableBinding(const InternalAtomicString& name, const ESValue& V, bool mustNotThrowTypeErrorExecption)
    {
        if (name == m_errorName)
            DeclarativeEnvironmentRecord::setMutableBinding(m_errorName, V, mustNotThrowTypeErrorExecption);
        else
            m_parentRecord->setMutableBinding(name, V, mustNotThrowTypeErrorExecption);
    }

private:
    EnvironmentRecord* m_parentRecord;
    InternalAtomicString m_errorName;
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

    virtual ESBindingSlot hasBinding(const InternalAtomicString& atomicName);
    virtual ESBindingSlot hasBinding(const InternalAtomicString& atomicName, bool& isBindingMutable, bool& isBindingConfigurable) { return hasBinding(atomicName); }
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

    ESObject* bindingObject() { return m_objectRecord->bindingObject(); }

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
    ALWAYS_INLINE FunctionEnvironmentRecord(ESValue* stackAllocatedData, size_t stackAllocatedSize, const InternalAtomicStringVector& innerIdentifiers, bool needsActivation, size_t mutableIndex)
        : DeclarativeEnvironmentRecord(stackAllocatedData, stackAllocatedSize, innerIdentifiers, needsActivation, mutableIndex)
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
    ALWAYS_INLINE FunctionEnvironmentRecordWithArgumentsObject(ESValue arguments[], const size_t& argumentCount, ESFunctionObject* callee, ESValue* stackAllocatedData, size_t stackAllocatedSize, const InternalAtomicStringVector& innerIdentifiers, bool needsActivation, size_t mutableIndex)
        : FunctionEnvironmentRecord(stackAllocatedData, stackAllocatedSize, innerIdentifiers, needsActivation, mutableIndex)
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

        m_argumentsObject = createArgumentsObject();
        return &m_argumentsObject;
    }

    ESFunctionObject* callee()
    {
        return m_callee;
    }

protected:
    ESValue* m_arguments;
    size_t m_argumentsCount;
    ESValue m_argumentsObject;
    ESFunctionObject* m_callee;

private:
    // http://www.ecma-international.org/ecma-262/5.1/#sec-10.6
    ESArgumentsObject* createArgumentsObject();
};

/*
// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-module-environment-records
class ModuleEnvironmentRecord : public DeclarativeEnvironmentRecord {
protected:
};
*/


// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-newfunctionenvironment
// $8.1.2.4
ALWAYS_INLINE LexicalEnvironment* LexicalEnvironment::newFunctionEnvironment(bool needsToPrepareGenerateArgumentsObject, ESValue* stackAllocatedStorage, size_t stackAllocatedStorageSize, const InternalAtomicStringVector& innerIdentifiers, ESValue arguments[], const size_t& argumentCount, ESFunctionObject* function, bool needsActivation, size_t mutableIndex)
{
    FunctionEnvironmentRecord* envRec;
    if (UNLIKELY(!needsToPrepareGenerateArgumentsObject)) {
        envRec = new FunctionEnvironmentRecord(stackAllocatedStorage, stackAllocatedStorageSize, innerIdentifiers, needsActivation, mutableIndex);
    } else {
        envRec = new FunctionEnvironmentRecordWithArgumentsObject(arguments, argumentCount, function, stackAllocatedStorage, stackAllocatedStorageSize, innerIdentifiers, needsActivation, mutableIndex);
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
