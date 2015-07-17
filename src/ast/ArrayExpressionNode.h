#ifndef ArrayExpressionNode_h
#define ArrayExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class ArrayExpressionNode : public ExpressionNode {
public:
    ArrayExpressionNode(ExpressionNodeVector&& elements)
        : ExpressionNode(NodeType::ArrayExpression)
    {
        m_elements = elements;
    }

    virtual ESValue* execute(ESVMInstance* instance)
    {
        JSArray* arr = JSArray::create();
        for(unsigned i = 0; i < m_elements.size() ; i++) {
            ESValue* result = m_elements[i]->execute(instance)->ensureValue();
            arr->set(ESString((int) i), result);
        }
        //TODO
        return arr;
    }
protected:
    ExpressionNodeVector m_elements;
};

}

#endif
