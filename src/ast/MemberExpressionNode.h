#ifndef MemberExpressionNode_h
#define MemberExpressionNode_h

#include "ExpressionNode.h"
#include "PropertyNode.h"

namespace escargot {

class MemberExpressionNode : public ExpressionNode {
public:
    MemberExpressionNode(Node* object, Node* property)
            : ExpressionNode(NodeType::MemberExpression)
    {
        m_object = object;
        m_property = property;
    }

    virtual ESValue* execute(ESVMInstance* instance);
protected:
    Node* m_object; //object: Expression;
    Node* m_property; //property: Identifier | Expression;
};

}

#endif
