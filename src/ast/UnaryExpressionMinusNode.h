#ifndef UnaryExpressionMinusNode_h
#define UnaryExpressionMinusNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionMinusNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    UnaryExpressionMinusNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionMinus)
    {
        m_argument = argument;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-unary-minus-operator
        return ESValue(-m_argument->executeExpression(instance).toNumber());
    }
protected:
    Node* m_argument;
};

}

#endif
