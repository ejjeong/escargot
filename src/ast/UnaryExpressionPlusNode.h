#ifndef UnaryExpressionPlusNode_h
#define UnaryExpressionPlusNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionPlusNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    UnaryExpressionPlusNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionPlus)
    {
        m_argument = argument;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        //http://www.ecma-international.org/ecma-262/6.0/index.html#sec-unary-plus-operator
        return ESValue(m_argument->executeExpression(instance).toNumber());
    }

protected:
    Node* m_argument;
};

}

#endif
