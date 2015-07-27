#ifndef IdentifierNode_h
#define IdentifierNode_h

#include "Node.h"
#include "ExpressionNode.h"
#include "PatternNode.h"

namespace escargot {

//interface Identifier <: Node, Expression, Pattern {
class IdentifierNode : public Node {
public:
    IdentifierNode(const ESAtomicString& name)
            : Node(NodeType::Identifier)
    {
        m_name = name;
    }

    ESValue* execute(ESVMInstance* instance);

    const ESAtomicString& name()
    {
        return m_name;
    }

protected:
    ESAtomicString m_name;
};

}

#endif
