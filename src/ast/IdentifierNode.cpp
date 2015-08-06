#include "Escargot.h"
#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue IdentifierNode::execute(ESVMInstance* instance)
{
    return executeForWrite(instance)->readDataProperty();
}

ESSlot* IdentifierNode::executeForWrite(ESVMInstance* instance)
{
    if (LIKELY(m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
        return m_cachedSlot;
    } else {
        ESSlot* slot;
        ExecutionContext* ec = instance->currentExecutionContext();

        if(LIKELY(m_canUseFastAccess && !ec->needsActivation())) {
            slot = ec->environment()->record()->toDeclarativeEnvironmentRecord()->getBindingValueForNonActivationMode(m_fastAccessIndex);
        }
        else
            slot = ec->resolveBinding(name(), nonAtomicName());

        if(LIKELY(slot != NULL)) {
            m_cachedSlot = slot;
            m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
            return slot;
        }

        ESFunctionObject* fn = instance->globalObject()->referenceError();
        ESErrorObject* receiver = ESErrorObject::create();
        receiver->setConstructor(fn);
        receiver->set__proto__(fn);

        std::vector<ESValue> arguments;
        InternalString err_msg = name().data();
        err_msg.append(InternalString(L" is not defined"));
        //arguments.push_back(String::create(err_msg));

        ESFunctionObject::call(fn, receiver, &arguments[0], arguments.size(), instance);
        receiver->set(InternalString(L"message"), ESString::create(err_msg));

        throw ESValue(receiver);
    }
}

}
