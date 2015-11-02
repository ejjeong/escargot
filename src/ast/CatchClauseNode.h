#ifndef CatchClauseNode_h
#define CatchClauseNode_h

#include "Node.h"
#include "ExpressionNode.h"
#include "IdentifierNode.h"
#include "BlockStatementNode.h"

namespace escargot {

//interface CatchClause <: Node {
class CatchClauseNode : public Node {
    public:
    friend class ScriptParser;
    CatchClauseNode(Node *param, Node *guard, Node *body)
        : Node(NodeType::CatchClause)
    {
        m_param = (IdentifierNode*) param;
        m_guard = (ExpressionNode*) guard;
        m_body = (BlockStatementNode*) body;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_body->generateStatementByteCode(codeBlock, context);
    }

    IdentifierNode* param() {
        return m_param;
    }

    protected:
    IdentifierNode* m_param;
    ExpressionNode* m_guard;
    BlockStatementNode* m_body;
};

typedef std::vector<Node *, gc_allocator<CatchClauseNode *>> CatchClauseNodeVector;

}

#endif

