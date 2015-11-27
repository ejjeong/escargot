#ifndef BinaryExpressionDivisionNode_h
#define BinaryExpressionDivisionNode_h

#include "ExpressionNode.h"

namespace escargot {

class BinaryExpressionDivisionNode : public ExpressionNode {
public:
    friend class ScriptParser;

    BinaryExpressionDivisionNode(Node *left, Node* right)
        : ExpressionNode(NodeType::BinaryExpressionDivison)
    {
        m_left = (ExpressionNode*)left;
        m_right = (ExpressionNode*)right;
    }

    virtual NodeType type() { return NodeType::BinaryExpressionDivison; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateExpressionByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(Division(), context, this);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 1;
        m_left->computeRoughCodeBlockSizeInWordSize(result);
        m_right->computeRoughCodeBlockSizeInWordSize(result);
    }

protected:
    ExpressionNode* m_left;
    ExpressionNode* m_right;
};

}

#endif
