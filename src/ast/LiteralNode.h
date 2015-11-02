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

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(Push(m_value), context, this);
    }

    ESValue value() { return m_value; }
protected:
    ESValue m_value;
};

}

#endif

