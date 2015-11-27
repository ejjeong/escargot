#ifndef ExpressionStatementNode_h
#define ExpressionStatementNode_h

#include "ExpressionNode.h"

namespace escargot {

// An expression statement, i.e., a statement consisting of a single expression.
class ExpressionStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    ExpressionStatementNode(Node* expression)
        : StatementNode(NodeType::ExpressionStatement)
    {
        m_expression = expression;
    }

    virtual NodeType type() { return NodeType::ExpressionStatement; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_expression->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(PopExpressionStatement(), context, this);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 1;
        m_expression->computeRoughCodeBlockSizeInWordSize(result);
    }

    Node* expression() { return m_expression; }

protected:
    Node* m_expression; // expression: Expression;
};

}

#endif
