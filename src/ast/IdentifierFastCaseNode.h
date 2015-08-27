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
    IdentifierFastCaseNode(size_t fastAccessIndex)
            : Node(NodeType::IdentifierFastCase)
    {
        m_fastAccessIndex = fastAccessIndex;
    }

    ESValue execute(ESVMInstance* instance)
    {
        ExecutionContext* ec = instance->currentExecutionContext();
        return *ec->environment()->record()->toDeclarativeEnvironmentRecord()->getBindingValueForNonActivationMode(m_fastAccessIndex);
    }

    ESSlotAccessor executeForWrite(ESVMInstance* instance)
    {
        ExecutionContext* ec = instance->currentExecutionContext();
        return ESSlotAccessor(ec->environment()->record()->toDeclarativeEnvironmentRecord()->getBindingValueForNonActivationMode(m_fastAccessIndex));
    }


protected:
    size_t m_fastAccessIndex;
};

}

#endif
