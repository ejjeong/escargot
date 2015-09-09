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
        m_nonAtomicName = ESString::create(name.data());
        m_identifierCacheInvalidationCheckCount = std::numeric_limits<unsigned>::max();
        m_canUseFastAccess = false;
        m_fastAccessIndex = SIZE_MAX;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ASSERT(!(m_canUseFastAccess));
//        ASSERT(instance->currentExecutionContext()->needsActivation());
        if (LIKELY(m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
            return m_cachedSlot.readDataProperty();
        } else {
            ExecutionContext* ec = instance->currentExecutionContext();
            ESSlotAccessor slot = ec->resolveBinding(name(), nonAtomicName());
            //printf("%s\n", nonAtomicName()->utf8Data());
            if(LIKELY(slot.hasData())) {
                m_cachedSlot = ESSlotAccessor(slot);
                m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
                return m_cachedSlot.readDataProperty();
            }

            ReferenceError* receiver = ReferenceError::create();

            std::vector<ESValue> arguments;
            u16string err_msg;
            err_msg.append(nonAtomicName()->data());
            err_msg.append(u" is not defined");
            //arguments.push_back(String::create(err_msg));

            //TODO call constructor
            //ESFunctionObject::call(fn, receiver, &arguments[0], arguments.size(), instance);
            receiver->set(strings->message, ESString::create(std::move(err_msg)));

            throw ESValue(receiver);
        }
        RELEASE_ASSERT_NOT_REACHED();
    }

    ESSlotAccessor executeForWrite(ESVMInstance* instance)
    {
        ASSERT(!(m_canUseFastAccess && !instance->currentExecutionContext()->needsActivation()));
        if (LIKELY(m_identifierCacheInvalidationCheckCount == instance->identifierCacheInvalidationCheckCount())) {
            return m_cachedSlot;
        } else {
            ExecutionContext* ec = instance->currentExecutionContext();
            ESSlotAccessor slot = ec->resolveBinding(name(), nonAtomicName());

            if(LIKELY(slot.hasData())) {
                m_cachedSlot = slot;
                m_identifierCacheInvalidationCheckCount = instance->identifierCacheInvalidationCheckCount();
                return slot;
            } else {
                //CHECKTHIS true, true, false is right?
                instance->invalidateIdentifierCacheCheckCount();
                return instance->globalObject()->definePropertyOrThrow(m_nonAtomicName, true, true, true);
            }
        }
    }

    const InternalAtomicString& name()
    {
        return m_name;
    }

    ESString* nonAtomicName()
    {
        return m_nonAtomicName;
    }

    void setFastAccessIndex(size_t upIndex, size_t index)
    {
        m_canUseFastAccess = true;
        m_fastAccessIndex = index;
        m_fastAccessUpIndex = upIndex;
    }

    bool canUseFastAccess()
    {
        return m_canUseFastAccess;
    }

    size_t fastAccessIndex()
    {
        return m_fastAccessIndex;
    }

    size_t fastAccessUpIndex()
    {
        return m_fastAccessUpIndex;
    }

protected:
    InternalAtomicString m_name;
    ESString* m_nonAtomicName;

    unsigned m_identifierCacheInvalidationCheckCount;
    ESSlotAccessor m_cachedSlot;

    bool m_canUseFastAccess;
    size_t m_fastAccessIndex;
    size_t m_fastAccessUpIndex;
};

}

#endif
