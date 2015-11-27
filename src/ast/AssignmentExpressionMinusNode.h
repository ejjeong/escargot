#ifndef AssignmentExpressionMinusNode_h
#define AssignmentExpressionMinusNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"

namespace escargot {

// An assignment operator expression.
class AssignmentExpressionMinusNode : public ExpressionNode {
public:
    friend class ScriptParser;

    AssignmentExpressionMinusNode(Node* left, Node* right)
        : ExpressionNode(NodeType::AssignmentExpressionMinus)
    {
        m_left = left;
        m_right = right;
    }

    virtual NodeType type() { return NodeType::AssignmentExpressionMinus; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateResolveAddressByteCode(codeBlock, context);
        m_left->generateReferenceResolvedAddressByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(Minus(), context, this);
        m_left->generatePutByteCode(codeBlock, context);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 1;
        m_left->computeRoughCodeBlockSizeInWordSize(result);
        m_right->computeRoughCodeBlockSizeInWordSize(result);
    }

protected:
    Node* m_left; // left: Pattern;
    Node* m_right; // right: Expression;
};

}

#endif
