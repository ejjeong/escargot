#ifndef ExpressionNode_h
#define ExpressionNode_h

#include "Node.h"
#include "PatternNode.h"

namespace escargot {

typedef std::vector<Node *, gc_allocator<Node *>> ArgumentVector;

//Any expression node. Since the left-hand side of an assignment may be any expression in general, an expression can also be a pattern.
//interface Expression <: Node, Pattern { }
class ExpressionNode : public Node {
public:
    ExpressionNode(NodeType type)
        : Node(type)
    {
    }
protected:
};

typedef std::vector<Node *, gc_allocator<Node *>> ExpressionNodeVector;

}

#endif

