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
        m_nonAtomicName = name;
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

    void setFastAccessIndex(size_t idx)
    {
        m_canUseFastAccess = true;
        m_fastAccessIndex = idx;
    }

protected:
    InternalAtomicString m_name;
    InternalString m_nonAtomicName;

    size_t m_identifierCacheInvalidationCheckCount;
    ESSlot* m_cachedSlot;

    bool m_canUseFastAccess;
    size_t m_fastAccessIndex;
};

}

#endif
