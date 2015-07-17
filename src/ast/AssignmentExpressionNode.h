#ifndef AssignmentExpressionNode_h
#define AssignmentExpressionNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"

namespace escargot {

//An assignment operator expression.
class AssignmentExpressionNode : public ExpressionNode {
public:
    /*
    enum AssignmentOperator {
        "=" | "+=" | "-=" | "*=" | "/=" | "%="
            | "<<=" | ">>=" | ">>>="
            | "|=" | "^=" | "&="
    }*/
    enum AssignmentOperator {
        Equal, //"="
    };

    AssignmentExpressionNode(Node* left, Node* right, AssignmentOperator oper)
            : ExpressionNode(NodeType::AssignmentExpression)
    {
        m_left = left;
        m_right = right;
        m_operator = oper;
    }

    virtual ESValue* execute(ESVMInstance* instance);
protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
    AssignmentOperator m_operator; //operator: AssignmentOperator
};

}

#endif
