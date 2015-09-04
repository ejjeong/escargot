#ifndef IdentifierFastCaseNode_h
#define IdentifierFastCaseNode_h

#include "Node.h"
#include "ExpressionNode.h"
#include "PatternNode.h"

namespace escargot {

//interface Identifier <: Node, Expression, Pattern {
class IdentifierFastCaseNode : public Node {
public:
    friend class ESScriptParser;
#ifdef NDEBUG
    IdentifierFastCaseNode(size_t fastAccessIndex)
            : Node(NodeType::IdentifierFastCase)
    {
        m_fastAccessIndex = fastAccessIndex;
    }
#else
    IdentifierFastCaseNode(size_t fastAccessIndex, InternalAtomicString name)
            : Node(NodeType::IdentifierFastCase)
    {
        m_fastAccessIndex = fastAccessIndex;
        m_name = name;
    }
#endif

    ESValue executeExpression(ESVMInstance* instance)
    {
        return instance->currentExecutionContext()->cachedDeclarativeEnvironmentRecordESValue()[m_fastAccessIndex];
    }

    ESSlotAccessor executeForWrite(ESVMInstance* instance)
    {
        return ESSlotAccessor(&instance->currentExecutionContext()->cachedDeclarativeEnvironmentRecordESValue()[m_fastAccessIndex]);
    }


protected:
    size_t m_fastAccessIndex;
#ifndef NDEBUG
    InternalAtomicString m_name;
#endif
};

}

#endif
