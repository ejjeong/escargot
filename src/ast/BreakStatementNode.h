#ifndef BreakStatmentNode_h
#define BreakStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class BreakStatmentNode : public StatementNode {
public:
    friend class ESScriptParser;
    BreakStatmentNode()
            : StatementNode(NodeType::ReturnStatement)
    {
    }

    virtual ESValue execute(ESVMInstance* instance);
};

}

#endif
