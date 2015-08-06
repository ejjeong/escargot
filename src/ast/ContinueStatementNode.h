#ifndef ContinueStatmentNode_h
#define ContinueStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class ContinueStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    ContinueStatementNode()
            : StatementNode(NodeType::ContinueStatement)
    {
    }

    ESValue execute(ESVMInstance* instance)
    {
        instance->currentExecutionContext()->doContinue();
        RELEASE_ASSERT_NOT_REACHED();
        return ESValue();
    }
};

}

#endif
