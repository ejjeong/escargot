#ifndef StatmentNode_h
#define StatmentNode_h

#include "Node.h"

namespace escargot {

class StatementNode : public Node {
public:
    StatementNode(NodeType type)
        : Node(type)
    {
    }
protected:
};

typedef std::vector<Node *, gc_allocator<Node *>> StatementNodeVector;

}

#endif

