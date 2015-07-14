#ifndef LiteralNode_h
#define LiteralNode_h

#include "Node.h"

namespace escargot {

//interface Literal <: Node, Expression {
class LiteralNode : public Node {
public:
    LiteralNode(const ESValue& value)
            : Node(NodeType::Literal)
    {
        m_value = value;
    }
    virtual void execute(ESVMInstance* ) { }
protected:
    ESValue m_value;
};

}

#endif
