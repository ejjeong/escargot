#include "Escargot.h"
#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ALWAYS_INLINE ESSlot* identifierNodeProcess(ESVMInstance* instance, IdentifierNode* self)
{
    if (LIKELY(self->m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
        return self->m_cachedSlot;
    } else {
        ESSlot* slot;
        if(LIKELY(self->m_canUseFastAccess && !instance->currentExecutionContext()->needsActivation())) {
            slot = instance->currentExecutionContext()->environment()->record()->toDeclarativeEnvironmentRecord()->getBindingValueForNonActivationMode(self->m_fastAccessIndex);
        }
        else
            slot = instance->currentExecutionContext()->resolveBinding(self->name());
        if(LIKELY(slot != NULL)) {
            self->m_cachedSlot = slot;
            self->m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
            return slot;
        }

        ESFunctionObject* fn = instance->globalObject()->referenceError();
        ESErrorObject* receiver = ESErrorObject::create();
        receiver->setConstructor(fn);
        receiver->set__proto__(fn);

        std::vector<ESValue> arguments;
        InternalString err_msg = self->name();
        err_msg.append(InternalString(L" is not defined"));
        //arguments.push_back(String::create(err_msg));

        ESFunctionObject::call(fn, receiver, &arguments[0], arguments.size(), instance);
        receiver->set(InternalAtomicString(L"message"), ESString::create(err_msg));

        throw ESValue(receiver);
    }
}

ESValue IdentifierNode::execute(ESVMInstance* instance)
{
    return identifierNodeProcess(instance, this)->value();
}

ESSlot* IdentifierNode::executeForWrite(ESVMInstance* instance)
{
    return identifierNodeProcess(instance, this);
}

}
