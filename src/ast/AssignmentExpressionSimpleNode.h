#ifndef AssignmentExpressionSimpleNode_h
#define AssignmentExpressionSimpleNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"
// #include "MemberExpressionNode.h"

namespace escargot {

// An assignment operator expression.
class AssignmentExpressionSimpleNode : public ExpressionNode {
public:
    friend class ScriptParser;

    AssignmentExpressionSimpleNode(Node* left, Node* right)
        : ExpressionNode(NodeType::AssignmentExpressionSimple)
    {
        m_left = left;
        m_right = right;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateResolveAddressByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        m_left->generatePutByteCode(codeBlock, context);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        m_left->computeRoughCodeBlockSizeInWordSize(result);
        m_right->computeRoughCodeBlockSizeInWordSize(result);
    }

    virtual bool isAssignmentExpressionSimple()
    {
        return true;
    }

protected:
    Node* m_left; // left: Pattern;
    Node* m_right; // right: Expression;
};

}

#endif
