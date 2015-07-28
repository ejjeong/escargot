#include "Escargot.h"
#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

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

    ESString err_msg = m_name;
    err_msg.append(ESString(L" is not defined"));
    instance->globalObject()->error()->set(ESAtomicString(L"name"), String::create(ESString(L"ReferenceError")));
    instance->globalObject()->error()->set(ESAtomicString(L"message"), String::create(err_msg));
    throw instance->globalObject()->error();
}

}
