#ifndef PatternNodeNode_h
#define PatternNodeNode_h

#include "Node.h"

namespace escargot {

class PatternNode : public Node {
public:
    PatternNode(NodeType type)
        : Node(type)
    {
    }
protected:
};

typedef std::vector<Node *, gc_allocator<Node *>> PatternNodeVector;

}

#endif


