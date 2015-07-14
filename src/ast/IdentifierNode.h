#ifndef IdentifierNode_h
#define IdentifierNode_h

#include "Node.h"
#include "ExpressionNode.h"
#include "PatternNode.h"

namespace escargot {

//interface Identifier <: Node, Expression, Pattern {
class IdentifierNode : public Node {
public:
    IdentifierNode(const ESString& name)
            : Node(NodeType::Identifier)
    {
        m_name = name;
    }
    virtual void execute(ESVMInstance* ) { }
protected:
    ESString m_name;
};

}

#endif
