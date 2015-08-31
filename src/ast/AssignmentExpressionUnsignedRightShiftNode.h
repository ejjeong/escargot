#ifndef AssignmentExpressionUnsignedRightShiftNode_h
#define AssignmentExpressionUnsignedRightShiftNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"

namespace escargot {

//An assignment operator expression.
class AssignmentExpressionUnsignedShiftNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    AssignmentExpressionUnsignedShiftNode(Node* left, Node* right)
            : ExpressionNode(NodeType::AssignmentExpressionUnsignedRightShift)
    {
        m_left = left;
        m_right = right;
    }

    ESValue execute(ESVMInstance* instance)
    {
        ESSlotAccessor slot;
        ExecutionContext* ec = instance->currentExecutionContext();
        ESSlotWriterForAST::prepareExecuteForWriteASTNode(ec);

        slot = m_left->executeForWrite(instance);
        int32_t lnum = slot.value(ec->lastESObjectMetInMemberExpressionNode()).toInt32();
        int32_t rnum = m_right->execute(instance).toInt32();
        unsigned int shiftCount = ((unsigned int)rnum) & 0x1F;
        lnum = ((unsigned int)lnum) >> shiftCount;
        ESValue rvalue(lnum);
        ESSlotWriterForAST::setValue(slot, ec, rvalue);
        return rvalue;
    }

protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
};

}

#endif
