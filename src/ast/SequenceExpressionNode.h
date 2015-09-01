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
        m_expressionsSize = m_expressions.size();
        m_rootedExpressions = m_expressions.data();
    }

    ESValue execute(ESVMInstance* instance)
    {
        for (unsigned i = 0; i < m_expressionsSize; i++) {
            m_rootedExpressions[i]->execute(instance);
        }
        return ESValue();
    }
protected:
    ExpressionNodeVector m_expressions; //expression: Expression;
    Node** m_rootedExpressions;
    size_t m_expressionsSize;
};

}

#endif
