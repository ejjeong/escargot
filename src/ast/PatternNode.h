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
    virtual void execute(ESVMInstance* ) { }
protected:
};

typedef std::vector<Node *, gc_allocator<PatternNode *>> PatternNodeVector;

}

#endif
