#ifndef ArrayExpressionNode_h
#define ArrayExpressionNode_h

#include "ExpressionNode.h"
#include "vm/ESVMInstance.h"

namespace escargot {

class ArrayExpressionNode : public ExpressionNode {
public:
    ArrayExpressionNode(ExpressionNodeVector&& elements)
        : ExpressionNode(NodeType::ArrayExpression)
    {
        m_elements = elements;
    }

    //$ 12.2.5.3
    virtual ESValue* execute(ESVMInstance* instance);
protected:
    ExpressionNodeVector m_elements;
};

}

#endif
