#ifndef BreakStatmentNode_h
#define BreakStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class BreakStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    BreakStatementNode()
            : StatementNode(NodeType::BreakStatement)
    {
    }

        ESValue execute(ESVMInstance* instance)
    {
        instance->currentExecutionContext()->doBreak();
        RELEASE_ASSERT_NOT_REACHED();
        return ESValue();
    }
};

}

#endif
