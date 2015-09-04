#ifndef AssignmentExpressionBitwiseAndNode_h
#define AssignmentExpressionBitwiseAndNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"

namespace escargot {

//An assignment operator expression.
class AssignmentExpressionBitwiseAndNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    AssignmentExpressionBitwiseAndNode(Node* left, Node* right)
            : ExpressionNode(NodeType::AssignmentExpressionBitwiseAnd)
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
        int32_t lnum = slot.value(ec->lastESObjectMetInMemberExpressionNode()).toInt32();
        int32_t rnum = m_right->executeExpression(instance).toInt32();
        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.10
        ESValue rvalue(lnum & rnum);
        ESSlotWriterForAST::setValue(slot, ec, rvalue);
        return rvalue;
    }

protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
};

}

#endif
