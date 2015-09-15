#ifndef VariableDeclaratorNode_h
#define VariableDeclaratorNode_h

#include "Node.h"
#include "PatternNode.h"
#include "ExpressionNode.h"
#include "IdentifierNode.h"

namespace escargot {

class VariableDeclaratorNode : public Node {
public:
    friend class ESScriptParser;
    VariableDeclaratorNode(Node* id,ExpressionNode* init=NULL)
            : Node(NodeType::VariableDeclarator)
    {
        m_id = id;
        m_init = init;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ASSERT(m_id->type() == NodeType::Identifier);
        ASSERT(!((IdentifierNode *)m_id)->canUseFastAccess());
        instance->currentExecutionContext()->environment()->record()->createMutableBindingForAST(((IdentifierNode *)m_id)->name(),
                ((IdentifierNode *)m_id)->nonAtomicName(), false);
        return ESValue();
    }

    void executeStatement(ESVMInstance* instance)
    {
        ASSERT(m_id->type() == NodeType::Identifier);
        ASSERT(!((IdentifierNode *)m_id)->canUseFastAccess());
        instance->currentExecutionContext()->environment()->record()->createMutableBindingForAST(((IdentifierNode *)m_id)->name(),
                ((IdentifierNode *)m_id)->nonAtomicName(), false);
    }

    virtual void generateByteCode(CodeBlock* codeBlock)
    {
        ASSERT(m_id->type() == NodeType::Identifier);
        ASSERT(!((IdentifierNode *)m_id)->canUseFastAccess());
        ASSERT(m_init == NULL);
        codeBlock->pushCode(CreateBinding(((IdentifierNode *)m_id)->name(), ((IdentifierNode *)m_id)->nonAtomicName()), this);
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
