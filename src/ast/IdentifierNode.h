#ifndef IdentifierNode_h
#define IdentifierNode_h

#include "Node.h"
#include "ExpressionNode.h"
#include "PatternNode.h"

namespace escargot {

//interface Identifier <: Node, Expression, Pattern {
class IdentifierNode : public Node {
public:
    friend class ESScriptParser;
    IdentifierNode(const InternalAtomicString& name)
            : Node(NodeType::Identifier)
    {
        m_name = name;
        m_nonAtomicName = name.data();
        m_esName = ESString::create(m_nonAtomicName);
        m_identifierCacheInvalidationCheckCount = SIZE_MAX;
        m_cachedSlot = NULL;
        m_canUseFastAccess = false;
        m_fastAccessIndex = SIZE_MAX;
    }

    ESValue execute(ESVMInstance* instance)
    {
        return executeForWrite(instance)->readDataProperty();
    }

    ALWAYS_INLINE ESSlot* executeForWrite(ESVMInstance* instance)
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

    const InternalAtomicString& name()
    {
        return m_name;
    }

    const InternalString& nonAtomicName()
    {
        return m_nonAtomicName;
    }

    ESString* esName()
    {
        return m_esName;
    }

    void setFastAccessIndex(size_t idx)
    {
        m_canUseFastAccess = true;
        m_fastAccessIndex = idx;
    }

protected:
    InternalAtomicString m_name;
    InternalString m_nonAtomicName;
    ESString* m_esName;

    size_t m_identifierCacheInvalidationCheckCount;
    ESSlot* m_cachedSlot;

    bool m_canUseFastAccess;
    size_t m_fastAccessIndex;
};

}

#endif
