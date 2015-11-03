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

    ESValue executeExpression(ESVMInstance* instance)
    {
        return ESValue();
    }

protected:
};

}

#endif
