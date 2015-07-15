#include "Escargot.h"
#include "Environment.h"
#include "GlobalObject.h"
#include "ESValue.h"

namespace escargot {

GlobalEnvironmentRecord::GlobalEnvironmentRecord(GlobalObject* G) {
    m_objectRecord = new ObjectEnvironmentRecord(G);
    m_declarativeRecord = new DeclarativeEnvironmentRecord();
}

//$8.1.1.4.12
bool GlobalEnvironmentRecord::hasVarDeclaration(const ESString& name) {
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
    JSObject::PropertyDescriptor pd = globalObj->getOwnProperty(name);
    /*
    if (pd == JSObject::m_undefined)
        return globalObj->isExtensible();
    */
    if (pd.m_isConfigurable == true)
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
//    JSObject::PropertyDescriptor existingProp = globalObj->getOwnProperty(name);
    // ReturnIfAbrupt(hasProperty)
    JSObject::PropertyDescriptor desc;
//    if (existingProp == undefined || existingProp.m_isConfigurable) {
//        desc.m_value = V;
//        desc.m_isWritable = true;
//        desc.m_isEnumerable = true;
//        desc.m_isConfigurable = canDelete;
//    } else {
        desc.m_value = V;
//    }
    globalObj->definePropertyOrThrow(name, desc);
    globalObj->setValue(name, V, false);
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
bool GlobalEnvironmentRecord::hasBinding(const ESString& name) {
    if( m_declarativeRecord->hasBinding(name) )
        return true;
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
        assert(m_objectRecord->hasBinding(name));
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
    JSObject::PropertyDescriptor pd;
    //pd.value = undefined;
    pd.m_isWritable = true;
    pd.m_isEnumerable = true;
    pd.m_isConfigurable = canDelete;
    m_bindingObject->definePropertyOrThrow(name, pd);
}

//$8.1.1.2.4
void ObjectEnvironmentRecord::initializeBinding(const ESString& name, ESValue* V) {
    return setMutableBinding(name, V, false);
}

//$8.1.1.2.5
void ObjectEnvironmentRecord::setMutableBinding(const ESString& name, ESValue* V, bool S) {
    m_bindingObject->setValue(name, V, S);
}
}
