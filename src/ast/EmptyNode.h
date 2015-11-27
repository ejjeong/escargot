#ifndef EmptyNode_h
#define EmptyNode_h

#include "Node.h"

namespace escargot {

class EmptyNode : public Node {
public:
    EmptyNode()
        : Node(NodeType::Empty)
    {
    }

    virtual NodeType type() { return NodeType::Empty; }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
    }

protected:
};

}

#endif
