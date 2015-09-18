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

        slot = m_left->executeForWrite(instance);
        ESValue rvalue(slot.value().toNumber() * m_right->executeExpression(instance).toNumber());
        slot.setValue(rvalue);
        return rvalue;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenereateContext& context)
    {
        m_left->generateByteCodeWriteCase(codeBlock, context);
        codeBlock->pushCode(ReferenceTopValueWithPeeking(), this);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(Multiply(), this);
        codeBlock->pushCode(Put(), this);
    }

protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
};

}

#endif
