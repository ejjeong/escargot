#ifndef AssignmentExpressionBitwiseOrNode_h
#define AssignmentExpressionBitwiseOrNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"

namespace escargot {

//An assignment operator expression.
class AssignmentExpressionBitwiseOrNode : public ExpressionNode {
public:
    friend class ESScriptParser;

    AssignmentExpressionBitwiseOrNode(Node* left, Node* right)
            : ExpressionNode(NodeType::AssignmentExpressionBitwiseOr)
    {
        m_left = left;
        m_right = right;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESSlotAccessor slot;
        slot = m_left->executeForWrite(instance);
        int32_t lnum = slot.value().toInt32();
        int32_t rnum = m_right->executeExpression(instance).toInt32();
        // http://www.ecma-international.org/ecma-262/5.1/#sec-11.10
        ESValue rvalue(lnum | rnum);
        slot.setValue(rvalue);
        return rvalue;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock)
    {
        m_left->generateByteCodeWriteCase(codeBlock);
        codeBlock->pushCode(ReferenceTopValueWithPeeking(), this);
        m_right->generateExpressionByteCode(codeBlock);
        codeBlock->pushCode(BitwiseOr(), this);
        codeBlock->pushCode(Put(), this);
    }

protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
};

}

#endif
