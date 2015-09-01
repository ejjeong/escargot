#ifndef ArrayExpressionNode_h
#define ArrayExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class ArrayExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    ArrayExpressionNode(ExpressionNodeVector&& elements)
        : ExpressionNode(NodeType::ArrayExpression)
    {
        m_elements = elements;
    }

    //$ 12.2.5.3
    ESValue execute(ESVMInstance* instance)
    {
        unsigned len = m_elements.size();
        ESArrayObject* arr = ESArrayObject::create(len, instance->globalObject()->arrayPrototype());
        for(unsigned i = 0; i < len ; i++) {
            ESValue result = m_elements[i]->execute(instance);
            arr->set(i, result);
        }
        return arr;
    }
protected:
    ExpressionNodeVector m_elements;
};

}

#endif
