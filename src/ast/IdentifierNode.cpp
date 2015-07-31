#include "Escargot.h"
#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue IdentifierNode::execute(ESVMInstance* instance)
{
    /*
    if (LIKELY(m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
        return m_cachedSlot;
    } else {
        ESSlot* slot;
        if(LIKELY(m_canUseFastAccess && !instance->currentExecutionContext()->needsActivation())) {
            slot = instance->currentExecutionContext()->environment()->record()->toDeclarativeEnvironmentRecord()->getBindingValueForNonActivationMode(m_fastAccessIndex);
        }
        else
            slot = instance->currentExecutionContext()->resolveBinding(name());
        if(LIKELY(slot != NULL)) {
            m_cachedSlot = slot;
            m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
            return slot;
        }

        ESValue* fn = instance->globalObject()->referenceError();
        ESErrorObject* receiver = ESErrorObject::create();
        receiver->setConstructor(fn);
        receiver->set__proto__(fn->toHeapObject()->toESFunctionObject());

        std::vector<ESValue*, gc_allocator<ESValue*>> arguments;
        InternalString err_msg = m_name;
        err_msg.append(InternalString(L" is not defined"));
        //arguments.push_back(String::create(err_msg));

        ESFunctionObject::call(fn, receiver, &arguments[0], arguments.size(), instance);
        receiver->set(InternalAtomicString(L"message"), ESString::create(err_msg));

        throw (ESValue*) receiver;
        return esUndefined;
    }

    ESSlot* slot = instance->currentExecutionContext()->resolveBinding(name());
    if(LIKELY(slot != NULL)) {
        m_cachedExecutionContext = instance->currentExecutionContext();
        m_cachedSlot = slot;
        m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
        return slot;
    }

    ESValue* fn = instance->globalObject()->referenceError();
    ESErrorObject* receiver = ESErrorObject::create();
    receiver->setConstructor(fn);
    receiver->set__proto__(fn->toHeapObject()->toESFunctionObject());

    std::vector<ESValue*, gc_allocator<ESValue*>> arguments;
    InternalString err_msg = m_name;
    err_msg.append(InternalString(L" is not defined"));
    //arguments.push_back(String::create(err_msg));

    ESFunctionObject::call(fn, receiver, &arguments[0], arguments.size(), instance);
    receiver->set(InternalAtomicString(L"message"), ESString::create(err_msg));

    throw (ESValue*) receiver;
    return esUndefined;
    */
    return ESValue();
}

}
