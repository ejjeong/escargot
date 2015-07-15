#ifndef BlockStatementNode_h
#define BlockStatementNode_h

#include "StatementNode.h"

namespace escargot {

//A block statement, i.e., a sequence of statements surrounded by braces.
class BlockStatementNode : public StatementNode {
public:
    BlockStatementNode()
            : StatementNode(NodeType::BlockStatement)
    {
    }
protected:
    StatementNodeVector m_body;// body: [ Statement ];
};

}

#endif
