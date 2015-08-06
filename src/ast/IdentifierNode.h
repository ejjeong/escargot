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

    ESValue execute(ESVMInstance* instance);
    ESSlot* executeForWrite(ESVMInstance* instance);

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
