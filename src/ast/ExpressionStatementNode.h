#ifndef ExpressionStatementNode_h
#define ExpressionStatementNode_h

#include "ExpressionNode.h"

namespace escargot {

//An expression statement, i.e., a statement consisting of a single expression.
class ExpressionStatementNode : public StatementNode {
public:
    ExpressionStatementNode(Node* expression)
            : StatementNode(NodeType::ExpressionStatement)
    {
        m_expression = expression;
    }

    ESValue* execute(ESVMInstance* instance)
    {
        m_expression->execute(instance);
        return esUndefined;
    }
protected:
    Node* m_expression; //expression: Expression;
};

}

#endif
