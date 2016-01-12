#include "Escargot.h"
#include "Environment.h"
#include "GlobalObject.h"
#include "ESValue.h"
#include "ExecutionContext.h"
#include "ast/FunctionNode.h"
#include "vm/ESVMInstance.h"

namespace escargot {

void EnvironmentRecord::createMutableBindingForAST(const InternalAtomicString& atomicName, bool canDelete)
{
    if (UNLIKELY(isGlobalEnvironmentRecord())) {
        toGlobalEnvironmentRecord()->createGlobalVarBinding(atomicName, canDelete);
    } else {
        createMutableBinding(atomicName, canDelete);
    }
}

void DeclarativeEnvironmentRecord::createMutableBinding(const InternalAtomicString& name, bool canDelete)
{
    ASSERT(m_needsActivation);
    // TODO canDelete
    size_t siz = (*m_innerIdentifiers).size();
    for (unsigned i = 0; i < siz; i ++) {
        if ((*m_innerIdentifiers)[i] == name) {
            return;
        }
    }
    (*m_innerIdentifiers).push_back(name);
    m_heapAllocatedData.push_back(ESValue());
    ESVMInstance::currentInstance()->invalidateIdentifierCacheCheckCount();
}

// $8.1.1.4.12
bool GlobalEnvironmentRecord::hasVarDeclaration(const InternalAtomicString& name)
{
    // if (std::find(m_varNames.begin(), m_varNames.end(), name) != m_varNames.end())
    //    return true;
    return false;
}

// $8.1.1.4.15
bool GlobalEnvironmentRecord::canDeclareGlobalVar(const InternalAtomicString& name)
{
    RELEASE_ASSERT_NOT_REACHED();
    /*
    ESObject* globalObj = m_objectRecord->bindingObject();
    bool hasProperty = globalObj->hasOwnProperty(name);
    if (hasProperty)
        return true;
    else
        return globalObj->isExtensible();
    */
}

// $8.1.1.4.16
bool GlobalEnvironmentRecord::canDeclareGlobalFunction(const InternalAtomicString& name)
{
    RELEASE_ASSERT_NOT_REACHED();
    /*
    ESObject* globalObj = m_objectRecord->bindingObject();
    ESSlot* pd = globalObj->find(name);
    if (pd == NULL)
        return globalObj->isExtensible();

    if (pd->isConfigurable() == true)
        return true;

    // IsDataDescriptor && ..
    */

    return false;
}

// $8.1.1.4.17
void GlobalEnvironmentRecord::createGlobalVarBinding(const InternalAtomicString& name, bool canDelete)
{
    ESObject* globalObj = m_objectRecord->bindingObject();
    bool hasProperty = globalObj->hasOwnProperty(name.string());
    bool extensible = globalObj->isExtensible();
    if (!hasProperty && extensible) {
        m_objectRecord->createMutableBinding(name, canDelete);
        m_objectRecord->initializeBinding(name, ESValue());
    }
    // if (std::find(m_varNames.begin(), m_varNames.end(), name) == m_varNames.end())
    //     m_varNames.push_back(name);
}

// $8.1.1.4.18
void GlobalEnvironmentRecord::createGlobalFunctionBinding(const InternalAtomicString& name, const ESValue& V, bool canDelete)
{
    ESObject* globalObj = m_objectRecord->bindingObject();
    globalObj->defineDataProperty(name.string(), true, true, canDelete, V);
    // if (std::find(m_varNames.begin(), m_varNames.end(), name) == m_varNames.end())
    //     m_varNames.push_back(name);
}

// $8.1.1.4.11
ESValue GlobalEnvironmentRecord::getThisBinding()
{
    return m_objectRecord->bindingObject();
}

// $8.1.1.4.1
ESValue* GlobalEnvironmentRecord::hasBinding(const InternalAtomicString& atomicName)
{
    /*

    ESValue* ret = m_declarativeRecord->hasBinding(atomicName);
    if (ret)
        return ret;
        */
    return m_objectRecord->hasBinding(atomicName);
}

// $8.1.1.4.2
void GlobalEnvironmentRecord::createMutableBinding(const InternalAtomicString& name, bool canDelete)
{
    RELEASE_ASSERT_NOT_REACHED();
    // if (m_declarativeRecord->hasBinding(name))
    //    ESVMInstance::currentInstance()->throwError(TypeError::create());
    // m_declarativeRecord->createMutableBinding(name, canDelete);
}

// $8.1.1.4.4
void GlobalEnvironmentRecord::initializeBinding(const InternalAtomicString& name, const ESValue& V)
{
    /*
    if (m_declarativeRecord->hasBinding(name))
        m_declarativeRecord->initializeBinding(name, V);
    else {
        ASSERT(m_objectRecord->hasBinding(name));
        m_objectRecord->initializeBinding(name, V);
    }
    */
    m_objectRecord->initializeBinding(name, V);
}

// $8.1.1.4.6
/*
ESValue GlobalEnvironmentRecord::getBindingValue(const InternalAtomicString& name, bool ignoreReferenceErrorException) {
// FIXME
// if ( m_declarativeRecord->hasBinding(name) )
//    return m_declarativeRecord->getBindingValue(name, ignoreReferenceErrorException);
// else {
//    return m_objectRecord->getBindingValue(name, ignoreReferenceErrorException);
// }
    return m_objectRecord->getBindingValue(name, ignoreReferenceErrorException);
}*/

// $8.1.1.4.5
// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-global-environment-records-setmutablebinding-n-v-s
void GlobalEnvironmentRecord::setMutableBinding(const InternalAtomicString& name, const ESValue& V, bool S)
{
    /*
    if (m_declarativeRecord->hasBinding(name)) {
        m_declarativeRecord->setMutableBinding(name, V, S);
    } else {
        m_objectRecord->setMutableBinding(name, V, S);
    }
    */
    m_objectRecord->setMutableBinding(name, V, S);
}

// $8.1.1.2.2
void ObjectEnvironmentRecord::createMutableBinding(const InternalAtomicString& name, bool canDelete)
{
    m_bindingObject->defineDataProperty(name.string(), true, true, canDelete);
}

// $8.1.1.2.4
void ObjectEnvironmentRecord::initializeBinding(const InternalAtomicString& name, const ESValue& V)
{
    return setMutableBinding(name, V, false);
}

// $8.1.1.2.5
void ObjectEnvironmentRecord::setMutableBinding(const InternalAtomicString& name, const ESValue& V, bool S)
{
    // TODO use S
    m_bindingObject->set(name.string(), V);
}

// http://www.ecma-international.org/ecma-262/6.0/index.html#sec-bindthisvalue
/*
void FunctionEnvironmentRecord::bindThisValue(const ESValue& V)
{
#ifndef NDEBUG
    ASSERT(m_thisBindingStatus != Initialized);
    if (m_thisBindingStatus == Lexical)
        ESVMInstance::currentInstance()->throwError(ReferenceError::create());
    m_thisBindingStatus = Initialized;
#endif
    m_thisValue = V;
}

ESValue FunctionEnvironmentRecord::getThisBinding()
{
#ifndef NDEBUG
    ASSERT(m_thisBindingStatus != Lexical);
    if (m_thisBindingStatus == Uninitialized)
        ESVMInstance::currentInstance()->throwError(ReferenceError::create());
#endif
    return m_thisValue;
}
*/

ESArgumentsObject* FunctionEnvironmentRecordWithArgumentsObject::createArgumentsObject()
{
    ESArgumentsObject* argumentsObject = ESArgumentsObject::create(this);
    argumentsObject->defineDataProperty(strings->length, true, false, true, ESValue(m_argumentsCount));

    CodeBlock* codeBlock = m_callee->codeBlock();
    unsigned i = 0;
    for (; i < m_argumentsCount; i ++) {
        ESString* propertyName;
        if (i < ESCARGOT_STRINGS_NUMBERS_MAX)
            propertyName = strings->numbers[i].string();
        else
            propertyName = ESString::create((int)i);

        if (i < codeBlock->m_paramsInformation.size()) {
            ESPropertyAccessorData* argumentsObjectAccessorData = new ESPropertyAccessorData([](ESObject* self, ESObject* originalObj, ESString* propertyName) -> ESValue {
                uint32_t i = ESValue(propertyName).toIndex();
                ASSERT(i != ESValue::ESInvalidIndexValue);
                ESArgumentsObject* argumentsObject = self->asESArgumentsObject();
                FunctionEnvironmentRecordWithArgumentsObject* environment = argumentsObject->environment();
                FunctionParametersInfo& info = environment->m_callee->codeBlock()->m_paramsInformation[i];
                if (info.m_isHeapAllocated)
                    return *environment->bindingValueForHeapAllocatedData(i);
                else
                    return *environment->bindingValueForStackAllocatedData(i);
            }, [](ESObject* self, ESObject* originalObj, ESString* propertyName, const ESValue& val) -> void {
                uint32_t i = ESValue(propertyName).toIndex();
                ASSERT(i != ESValue::ESInvalidIndexValue);
                ESArgumentsObject* argumentsObject = self->asESArgumentsObject();
                FunctionEnvironmentRecordWithArgumentsObject* environment = argumentsObject->environment();
                FunctionParametersInfo& info = environment->m_callee->codeBlock()->m_paramsInformation[i];
                if (info.m_isHeapAllocated)
                    *environment->bindingValueForHeapAllocatedData(i) = val;
                else
                    *environment->bindingValueForStackAllocatedData(i) = val;
            });
            argumentsObject->defineAccessorProperty(strings->numbers[i].string(), argumentsObjectAccessorData, true, true, true);
        } else {
            argumentsObject->defineDataProperty(propertyName, true, true, true, m_arguments[i]);
        }
    }
    argumentsObject->defineDataProperty(strings->callee, true, false, true, ESValue(m_callee));

    return argumentsObject;
}

}
