#ifndef UnaryExpressionTypeOfNode_h
#define UnaryExpressionTypeOfNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionTypeOfNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    UnaryExpressionTypeOfNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionTypeOf)
    {
        m_argument = argument;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        //www.ecma-international.org/ecma-262/6.0/index.html#sec-unary-minus-operator
        ESValue v = m_argument->executeExpression(instance);
        if(v.isUndefined())
            return strings->undefined;
        else if(v.isNull())
            return strings->null;
        else if(v.isBoolean())
            return strings->boolean;
        else if(v.isNumber())
            return strings->number;
        else if(v.isESString())
            return strings->string;
        else if(v.isESPointer()) {
            ESPointer* p = v.asESPointer();
            if(p->isESFunctionObject()) {
                return strings->function;
            } else {
                return strings->object;
            }
        }
        else
            RELEASE_ASSERT_NOT_REACHED();
    }

protected:
    Node* m_argument;
};

}

#endif
