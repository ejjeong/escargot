#ifndef ReturnStatmentNode_h
#define ReturnStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class ReturnStatmentNode : public StatementNode {
public:
    friend class ESScriptParser;
    ReturnStatmentNode(Node* argument)
            : StatementNode(NodeType::ReturnStatement)
    {
        m_argument = argument;
    }

    ESValue execute(ESVMInstance* instance)
    {
        instance->currentExecutionContext()->doReturn(m_argument ? m_argument->execute(instance) : ESValue());
        RELEASE_ASSERT_NOT_REACHED();
        return ESValue();
    }

protected:
    Node* m_argument;
};

}

#endif
