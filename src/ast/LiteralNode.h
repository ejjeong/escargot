#ifndef LiteralNode_h
#define LiteralNode_h

#include "Node.h"

namespace escargot {

//interface Literal <: Node, Expression {
class LiteralNode : public Node {
public:
    LiteralNode(ESValue value)
            : Node(NodeType::Literal)
    {
        m_value = value;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        return m_value;
    }

    virtual void generateByteCode(CodeBlock* codeBlock)
    {
        Push push(m_value);
        codeBlock->pushCode(push, this);
    }

    ESValue value() { return m_value; }
protected:
    ESValue m_value;
};

}

#endif
