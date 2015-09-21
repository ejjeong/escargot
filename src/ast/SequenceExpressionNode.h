#ifndef SequenceExpressionNode_h
#define SequenceExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

//An sequence expression, i.e., a statement consisting of vector of expressions.
class SequenceExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    SequenceExpressionNode(ExpressionNodeVector&& expressions)
            : ExpressionNode(NodeType::SequenceExpression)
    {
        m_expressions = expressions;
    }

protected:
    ExpressionNodeVector m_expressions; //expression: Expression;
};

}

#endif
