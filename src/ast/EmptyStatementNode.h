#ifndef EmptyStatmentNode_h
#define EmptyStatmentNode_h

#include "StatementNode.h"

namespace escargot {

//An empty statement, i.e., a solitary semicolon.
class EmptyStatementNode : public StatementNode {
public:
    EmptyStatementNode()
            : StatementNode(NodeType::EmptyStatement)
    {
    }

    virtual ESValue execute(ESVMInstance* instance)
    {
        return ESValue();
    }
protected:
};

}

#endif
