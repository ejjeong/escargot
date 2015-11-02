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
    VariableDeclaratorNode(Node* id,ExpressionNode* init=NULL)
        : Node(NodeType::VariableDeclarator)
    {
        m_id = id;
        m_init = init;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        ASSERT(m_id->type() == NodeType::Identifier);
        ASSERT(m_init == NULL);
        if(!((IdentifierNode *)m_id)->canUseFastAccess())
            codeBlock->pushCode(CreateBinding(((IdentifierNode *)m_id)->name()), context, this);
    }

    Node* id() { return m_id; }
    ExpressionNode* init() { return m_init; }
    void clearInit()
    {
        m_init = NULL;
    }

protected:
    Node* m_id; //id: Pattern;
    ExpressionNode* m_init; //init: Expression | null;
};


typedef std::vector<Node *, gc_allocator<Node *>> VariableDeclaratorVector;

}

#endif

