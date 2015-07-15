#ifndef AssignmentExpressionNode_h
#define AssignmentExpressionNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"

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

    virtual ESValue* execute(ESVMInstance* instance)
    {

        if(m_operator == Equal) {
            //http://www.ecma-international.org/ecma-262/5.1/#sec-11.13.1
            //TODO
            ESValue* rval = m_right->execute(instance);
            ESValue* lref = m_left->execute(instance);
            JSObjectSlot* slot = lref->toHeapObject()->toJSObjectSlot();
            slot->setValue(rval);
        }

        return undefined;
    }
protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
    AssignmentOperator m_operator; //operator: AssignmentOperator
};

}

#endif
