#ifndef SequenceExpressionNode_h
#define SequenceExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

//An sequence expression, i.e., a statement consisting of vector of expressions.
class SequenceExpressionNode : public ExpressionNode {
public:
    SequenceExpressionNode(ExpressionNodeVector&& expressions)
            : ExpressionNode(NodeType::Expression)
    {
        m_expressions = expressions;
    }

    ESValue* execute(ESVMInstance* instance)
    {
    		 for (unsigned i = 0; i < m_expressions.size(); i++) {
    			 m_expressions[i]->execute(instance);
    		 }
        return undefined;
    }
protected:
    ExpressionNodeVector m_expressions; //expression: Expression;
};

}

#endif
