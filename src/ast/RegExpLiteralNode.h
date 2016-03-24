#ifndef RegExpLiteralNode_h
#define RegExpLiteralNode_h

#include "Node.h"

namespace escargot {

// interface RegExpLiteral <: Node, Expression {
class RegExpLiteralNode : public Node {
public:
    RegExpLiteralNode(ESString* body, escargot::ESRegExpObject::Option flag)
        : Node(NodeType::RegExpLiteral)
    {
        m_body = body;
        m_flag = flag;
    }

    virtual NodeType type() { return NodeType::RegExpLiteral; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(InitRegExpObject(m_body, m_flag), context, this);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 3;
    }

    virtual bool isLiteral()
    {
        return true;
    }

    ESString* body() { return m_body; }
    escargot::ESRegExpObject::Option flag() { return m_flag; }

protected:
    ESString* m_body;
    escargot::ESRegExpObject::Option m_flag;
};

}

#endif
