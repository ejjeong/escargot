#ifndef AssignmentExpressionUnsignedRightShiftNode_h
#define AssignmentExpressionUnsignedRightShiftNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"

namespace escargot {

// An assignment operator expression.
class AssignmentExpressionUnsignedShiftNode : public ExpressionNode {
public:
    friend class ScriptParser;

    AssignmentExpressionUnsignedShiftNode(Node* left, Node* right)
        : ExpressionNode(NodeType::AssignmentExpressionUnsignedRightShift)
    {
        m_left = left;
        m_right = right;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateResolveAddressByteCode(codeBlock, context);
        m_left->generateReferenceResolvedAddressByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(UnsignedRightShift(), context, this);
        m_left->generatePutByteCode(codeBlock, context);
    }

protected:
    Node* m_left; // left: Pattern;
    Node* m_right; // right: Expression;
};

}

#endif


