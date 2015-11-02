#ifndef LogicalExpressionNode_h
#define LogicalExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class LogicalExpressionNode : public ExpressionNode {
public:
    friend class ScriptParser;
    enum LogicalExpressionOperator {
        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.11
        // Binary Logical Operators
        LogicalAnd, // "&&"
        LogicalOr, // "||"
    };

    LogicalExpressionNode(Node *left, Node* right, ESString* oper)
        : ExpressionNode(NodeType::LogicalExpression)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;

        // Binary Logical Operator
        if (*oper == u"&&")
            m_operator = LogicalAnd;
        else if (*oper == u"||")
            m_operator = LogicalOr;

        // TODO
        else
            RELEASE_ASSERT_NOT_REACHED();
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
    LogicalExpressionOperator m_operator;
};

}

#endif



