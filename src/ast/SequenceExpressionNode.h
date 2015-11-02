#ifndef SequenceExpressionNode_h
#define SequenceExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

//An sequence expression, i.e., a statement consisting of vector of expressions.
class SequenceExpressionNode : public ExpressionNode {
public:
    friend class ScriptParser;
    SequenceExpressionNode(ExpressionNodeVector&& expressions)
        : ExpressionNode(NodeType::SequenceExpression)
    {
        m_expressions = expressions;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        for (unsigned i = 0; i < m_expressions.size(); i++) {
            m_expressions[i]->generateExpressionByteCode(codeBlock, context);
            if (i < m_expressions.size() - 1)
                codeBlock->pushCode(Pop(), context, this);
        }
    }

protected:
    ExpressionNodeVector m_expressions; //expression: Expression;
};

}

#endif

