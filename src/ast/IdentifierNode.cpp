#include "Escargot.h"
#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#include "runtime/ESFunctionCaller.h"

namespace escargot {

ESValue* IdentifierNode::execute(ESVMInstance* instance)
{
    if(m_cachedExecutionContext == instance->currentExecutionContext() && m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount()) {
        return m_cachedSlot;
    }

    JSSlot* slot = instance->currentExecutionContext()->resolveBinding(name());
    if(LIKELY(slot != NULL)) {
        m_cachedExecutionContext = instance->currentExecutionContext();
        m_cachedSlot = slot;
        m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
        return slot;
    }

    ESValue* fn = instance->globalObject()->referenceError();
    JSError* receiver = JSError::create();
    receiver->setConstructor(fn);
    receiver->set__proto__(fn->toHeapObject()->toJSFunction());

    std::vector<ESValue*, gc_allocator<ESValue*>> arguments;
    ESString err_msg = m_name;
    err_msg.append(ESString(L" is not defined"));
    //arguments.push_back(String::create(err_msg));

    ESFunctionCaller::call(fn, receiver, &arguments[0], arguments.size(), instance);
    receiver->set(ESAtomicString(L"message"), PString::create(err_msg));

    throw (ESValue*) receiver;
    return esUndefined;
}

}
