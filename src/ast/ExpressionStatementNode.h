#ifndef ExpressionStatementNode_h
#define ExpressionStatementNode_h

#include "ExpressionNode.h"

namespace escargot {

//An expression statement, i.e., a statement consisting of a single expression.
class ExpressionStatementNode : public StatementNode {
public:
    ExpressionStatementNode()
            : StatementNode(NodeType::ExpressionStatement)
    {
        m_expression = NULL;
    }
    virtual void execute(ESVMInstance* ) { }
protected:
    ExpressionNode* m_expression; //expression: Expression;
};

}

#endif
