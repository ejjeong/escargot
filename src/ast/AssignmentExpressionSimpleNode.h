#ifndef AssignmentExpressionSimpleNode_h
#define AssignmentExpressionSimpleNode_h

#include "BinaryExpressionNode.h"
#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"

namespace escargot {

//An assignment operator expression.
class AssignmentExpressionSimpleNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    AssignmentExpressionSimpleNode(Node* left, Node* right)
            : ExpressionNode(NodeType::AssignmentExpressionSimple)
    {
        m_left = left;
        m_right = right;
    }

    ESValue execute(ESVMInstance* instance)
    {
        ExecutionContext* ec = instance->currentExecutionContext();
        ESSlotWriterForAST::prepareExecuteForWriteASTNode(ec);

        //http://www.ecma-international.org/ecma-262/5.1/#sec-11.13.1
        ESValue rvalue = m_right->execute(instance);
        ESSlotAccessor slot = m_left->executeForWrite(instance);
        ESSlotWriterForAST::setValue(slot, ec, rvalue);
        return rvalue;
    }

protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
};

}

#endif
