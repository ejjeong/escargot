#include "Escargot.h"
#include "Environment.h"
#include "GlobalObject.h"
#include "ESValue.h"

namespace escargot {

//$8.1.1.4.12
bool GlobalEnvironmentRecord::hasVarDeclaration(const ESString& name)
{
    if( std::find(m_varNames.begin(), m_varNames.end(), name) != m_varNames.end() )
        return true;
    return false;
}

//$8.1.1.4.15
bool GlobalEnvironmentRecord::canDeclareGlobalVar(const ESString& name) {
    JSObject* globalObj = m_objectRecord->bindingObject();
    bool hasProperty = globalObj->hasOwnProperty(name);
    // ReturnIfAbrupt(hasProperty)
    if (hasProperty)
        return true;
    else
        return globalObj->isExtensible();
}

//$8.1.1.4.16
bool GlobalEnvironmentRecord::canDeclareGlobalFunction(const ESString& name) {
    JSObject* globalObj = m_objectRecord->bindingObject();
    JSObjectSlot* pd = globalObj->find(name);
    if(pd == NULL)
        return globalObj->isExtensible();

    if (pd->isConfigurable() == true)
        return true;

    // IsDataDescriptor && ..
    
    return false;
}

//$8.1.1.4.17
void GlobalEnvironmentRecord::createGlobalVarBinding(const ESString& name, bool canDelete) {
    JSObject* globalObj = m_objectRecord->bindingObject();
    bool hasProperty = globalObj->hasOwnProperty(name);
    // ReturnIfAbrupt(hasProperty)
    bool extensible = globalObj->isExtensible();
    // ReturnIfAbrupt(extensible)
    if (!hasProperty && extensible) {
        m_objectRecord->createMutableBinding(name, canDelete);
//        m_objectRecord->initializeBinding(name, undefined);
    }
    if( std::find(m_varNames.begin(), m_varNames.end(), name) == m_varNames.end() )
        m_varNames.push_back(name);
}

//$8.1.1.4.18
void GlobalEnvironmentRecord::createGlobalFunctionBinding(const ESString& name, ESValue* V, bool canDelete) {
    JSObject* globalObj = m_objectRecord->bindingObject();
    globalObj->definePropertyOrThrow(name, true, true, canDelete);
    globalObj->set(name, V, false);
    if( std::find(m_varNames.begin(), m_varNames.end(), name) == m_varNames.end() )
        m_varNames.push_back(name);
}

//$8.1.1.4.11
ESValue* GlobalEnvironmentRecord::getThisBinding() {
//    JSObject* globalObj = m_objectRecord->bindingObject();
//    return new ESValue(sizeof(globalObj), globalObj);
    return NULL;
}

//$8.1.1.4.1
JSObjectSlot* GlobalEnvironmentRecord::hasBinding(const ESString& name) {
    JSObjectSlot* ret = m_declarativeRecord->hasBinding(name);
    if(ret)
        return ret;
    return m_objectRecord->hasBinding(name);
}

//$8.1.1.4.2
void GlobalEnvironmentRecord::createMutableBinding(const ESString& name, bool canDelete) {
    if( m_declarativeRecord->hasBinding(name) )
        throw "TypeError";
    m_declarativeRecord->createMutableBinding(name, canDelete);
}

//$8.1.1.4.4
void GlobalEnvironmentRecord::initializeBinding(const ESString& name, ESValue* V) {
    if( m_declarativeRecord->hasBinding(name) )
        m_declarativeRecord->initializeBinding(name, V);
    else {
        ASSERT(m_objectRecord->hasBinding(name));
        m_objectRecord->initializeBinding(name, V);
    }
}

//$8.1.1.4.6
ESValue* GlobalEnvironmentRecord::getBindingValue(const ESString& name, bool ignoreReferenceErrorException) {
    if( m_declarativeRecord->hasBinding(name) )
        return m_declarativeRecord->getBindingValue(name, ignoreReferenceErrorException);
    else {
        return m_objectRecord->getBindingValue(name, ignoreReferenceErrorException);
    }
}

//$8.1.1.2.2
void ObjectEnvironmentRecord::createMutableBinding(const ESString& name, bool canDelete) {
    m_bindingObject->definePropertyOrThrow(name, true, true, canDelete);
}

//$8.1.1.2.4
void ObjectEnvironmentRecord::initializeBinding(const ESString& name, ESValue* V) {
    return setMutableBinding(name, V, false);
}

//$8.1.1.2.5
void ObjectEnvironmentRecord::setMutableBinding(const ESString& name, ESValue* V, bool S) {
    m_bindingObject->set(name, V, S);
}
}
