#ifndef IdentifierFastCaseWithActivationNode_h
#define IdentifierFastCaseWithActivationNode_h

#include "Node.h"
#include "ExpressionNode.h"
#include "PatternNode.h"

namespace escargot {

//interface Identifier <: Node, Expression, Pattern {
class IdentifierFastCaseWithActivationNode : public Node {
public:
    friend class ESScriptParser;
#ifdef NDEBUG
    IdentifierFastCaseWithActivationNode(size_t fastAccessIndex, size_t fastAccessUpIndex)
            : Node(NodeType::IdentifierFastCaseWithActivation)
    {
        m_fastAccessIndex = fastAccessIndex;
        m_fastAccessUpIndex = fastAccessUpIndex;
    }
#else
    IdentifierFastCaseWithActivationNode(size_t fastAccessIndex, size_t fastAccessUpIndex, InternalAtomicString name)
            : Node(NodeType::IdentifierFastCaseWithActivation)
    {
        m_fastAccessIndex = fastAccessIndex;
        m_fastAccessUpIndex = fastAccessUpIndex;
        m_name = name;
    }
#endif

    ESValue executeExpression(ESVMInstance* instance)
    {
        LexicalEnvironment* env = instance->currentExecutionContext()->environment();
        for(unsigned i = 0; i < m_fastAccessUpIndex; i ++) {
            env = env->outerEnvironment();
        }
        ASSERT(env->record()->isDeclarativeEnvironmentRecord());
        return *env->record()->toDeclarativeEnvironmentRecord()->bindingValueForActivationMode(m_fastAccessIndex);
    }

    ESSlotAccessor executeForWrite(ESVMInstance* instance)
    {
        LexicalEnvironment* env = instance->currentExecutionContext()->environment();
        for(unsigned i = 0; i < m_fastAccessUpIndex; i ++) {
            env = env->outerEnvironment();
        }
        ASSERT(env->record()->isDeclarativeEnvironmentRecord());
        return ESSlotAccessor(env->record()->toDeclarativeEnvironmentRecord()->bindingValueForActivationMode(m_fastAccessIndex));
    }


protected:
    size_t m_fastAccessIndex;
    size_t m_fastAccessUpIndex;
#ifndef NDEBUG
    InternalAtomicString m_name;
#endif
};

}

#endif
