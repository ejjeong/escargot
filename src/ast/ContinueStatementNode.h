#ifndef ContinueStatmentNode_h
#define ContinueStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class ContinueStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    ContinueStatementNode()
            : StatementNode(NodeType::ReturnStatement)
    {
    }

    virtual ESValue execute(ESVMInstance* instance);
};

}

#endif
