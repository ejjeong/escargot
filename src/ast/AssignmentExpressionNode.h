#ifndef AssignmentExpressionNode_h
#define AssignmentExpressionNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"

namespace escargot {

//An assignment operator expression.
class AssignmentExpressionNode : public ExpressionNode {
public:
    AssignmentExpressionNode()
            : ExpressionNode(NodeType::AssignmentExpression)
    {
        m_left = NULL;
        m_right = NULL;
    }
    virtual void execute(ESVMInstance* ) { }
protected:
    PatternNode* m_left; //left: Pattern;
    ExpressionNode* m_right; //right: Expression;
};

}

#endif
