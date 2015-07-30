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
    IdentifierNode(const ESAtomicString& name)
            : Node(NodeType::Identifier)
    {
        m_name = name;
        m_cachedExecutionContext = NULL;
        m_identifierCacheInvalidationCheckCount = 0;
        m_cachedSlot = NULL;
        m_canUseFastAccess = false;
        m_fastAccessIndex = SIZE_MAX;
    }

    ESValue* execute(ESVMInstance* instance);

    const ESAtomicString& name()
    {
        return m_name;
    }

protected:
    ESAtomicString m_name;

    ExecutionContext* m_cachedExecutionContext;
    size_t m_identifierCacheInvalidationCheckCount;
    JSSlot* m_cachedSlot;

    bool m_canUseFastAccess;
    size_t m_fastAccessIndex;
};

}

#endif
