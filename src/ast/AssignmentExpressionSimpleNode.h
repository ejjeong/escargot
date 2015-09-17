#ifndef AssignmentExpressionSimpleNode_h
#define AssignmentExpressionSimpleNode_h

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

    ESValue executeExpression(ESVMInstance* instance)
    {
        //http://www.ecma-international.org/ecma-262/5.1/#sec-11.13.1
        ESSlotAccessor slot = m_left->executeForWrite(instance);
        ESValue rvalue = m_right->executeExpression(instance);
        slot.setValue(rvalue);
        return rvalue;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock)
    {
        m_left->generateByteCodeWriteCase(codeBlock);
        m_right->generateExpressionByteCode(codeBlock);
        codeBlock->pushCode(Put(), this);
    }

protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
};

}

#endif
