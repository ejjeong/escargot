#ifndef VariableDeclaratorNode_h
#define VariableDeclaratorNode_h

#include "Node.h"
#include "PatternNode.h"
#include "ExpressionNode.h"
#include "IdentifierNode.h"

namespace escargot {

class VariableDeclaratorNode : public Node {
public:
    friend class ScriptParser;
    VariableDeclaratorNode(Node* id, ExpressionNode* init = NULL)
        : Node(NodeType::VariableDeclarator)
    {
        m_id = id;
        m_init = init;
    }

    virtual NodeType type() { return NodeType::VariableDeclarator; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        ASSERT(m_id->isIdentifier());
        ASSERT(m_init == NULL);
        if (!((IdentifierNode *)m_id)->canUseFastAccess())
            codeBlock->pushCode(CreateBinding(((IdentifierNode *)m_id)->name()), context, this);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 2;
    }

    Node* id() { return m_id; }
    ExpressionNode* init() { return m_init; }
    void clearInit()
    {
        m_init = NULL;
    }

    virtual bool isVariableDeclarator()
    {
        return true;
    }

protected:
    Node* m_id; // id: Pattern;
    ExpressionNode* m_init; // init: Expression | null;
};


typedef std::vector<Node *, gc_allocator<Node *>> VariableDeclaratorVector;

}

#endif
