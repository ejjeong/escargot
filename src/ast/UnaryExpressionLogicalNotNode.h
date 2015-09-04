#ifndef UnaryExpressionLogicalNotNode_h
#define UnaryExpressionLogicalNotNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionLogicalNotNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    UnaryExpressionLogicalNotNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionLogicalNot)
    {
        m_argument = argument;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        //www.ecma-international.org/ecma-262/6.0/index.html#sec-unary-minus-operator
        return ESValue(!m_argument->executeExpression(instance).toBoolean());
    }

protected:
    Node* m_argument;
};

}

#endif
