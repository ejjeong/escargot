#ifndef AssignmentExpressionMultiplyNode_h
#define AssignmentExpressionMultiplyNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"

namespace escargot {

//An assignment operator expression.
class AssignmentExpressionMultiplyNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    AssignmentExpressionMultiplyNode(Node* left, Node* right)
            : ExpressionNode(NodeType::AssignmentExpressionMultiply)
    {
        m_left = left;
        m_right = right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESSlotAccessor slot;
        ExecutionContext* ec = instance->currentExecutionContext();
        ESSlotWriterForAST::prepareExecuteForWriteASTNode(ec);

        slot = m_left->executeForWrite(instance);
        ESValue rvalue(slot.value(ec->lastESObjectMetInMemberExpressionNode()).toNumber() * m_right->executeExpression(instance).toNumber());
        ESSlotWriterForAST::setValue(slot, ec, rvalue);
        return rvalue;
    }

protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
};

}

#endif
