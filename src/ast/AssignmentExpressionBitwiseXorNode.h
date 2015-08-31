#ifndef AssignmentExpressionBitwiseXorNode_h
#define AssignmentExpressionBitwiseXorNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"

namespace escargot {

//An assignment operator expression.
class AssignmentExpressionBitwiseXorNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    AssignmentExpressionBitwiseXorNode(Node* left, Node* right)
            : ExpressionNode(NodeType::AssignmentExpressionBitwiseXor)
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
        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.10
        ESValue rvalue(lnum ^ rnum);
        ESSlotWriterForAST::setValue(slot, ec, rvalue);
        return rvalue;
    }

protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
};

}

#endif
