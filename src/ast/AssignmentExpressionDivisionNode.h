#ifndef AssignmentExpressionDivisionNode_h
#define AssignmentExpressionDivisionNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"

namespace escargot {

//An assignment operator expression.
class AssignmentExpressionDivisionNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    AssignmentExpressionDivisionNode(Node* left, Node* right)
            : ExpressionNode(NodeType::AssignmentExpressionDivision)
    {
        m_left = left;
        m_right = right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESSlotAccessor slot;
        slot = m_left->executeForWrite(instance);
        ESValue rvalue(slot.value().toNumber() / m_right->executeExpression(instance).toNumber());
        slot.setValue(rvalue);
        return rvalue;
    }

protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
};

}

#endif
