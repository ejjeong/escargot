#ifndef ExpressionNode_h
#define ExpressionNode_h

#include "Node.h"
#include "PatternNode.h"

namespace escargot {

//Any expression node. Since the left-hand side of an assignment may be any expression in general, an expression can also be a pattern.
//interface Expression <: Node, Pattern { }
class ExpressionNode : public Node {
public:
    ExpressionNode(NodeType type)
            : Node(type)
    {
    }
    virtual void execute(ESVMInstance* ) { }
protected:
};

typedef std::vector<Node *, gc_allocator<ExpressionNode *>> ExpressionNodeVector;

}

#endif
