#ifndef ExpressionStatementNode_h
#define ExpressionStatementNode_h

#include "ExpressionNode.h"

namespace escargot {

//An expression statement, i.e., a statement consisting of a single expression.
class ExpressionStatementNode : public StatementNode {
public:
    friend class ESScriptParser;
    ExpressionStatementNode(Node* expression)
            : StatementNode(NodeType::ExpressionStatement)
    {
        m_expression = expression;
    }

    void executeStatement(ESVMInstance* instance)
    {
        instance->m_lastExpressionStatementValue = m_expression->executeExpression(instance);
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenereateContext& context)
    {
        m_expression->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(PopExpressionStatement(), this);
    }

    Node* expression() { return m_expression; }

protected:
    Node* m_expression; //expression: Expression;
};

}

#endif
