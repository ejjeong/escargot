#ifndef AssignmentExpressionLeftShiftNode_h
#define AssignmentExpressionLeftShiftNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"

namespace escargot {

//An assignment operator expression.
class AssignmentExpressionLeftShiftNode : public ExpressionNode {
public:
    friend class ScriptParser;

    AssignmentExpressionLeftShiftNode(Node* left, Node* right)
            : ExpressionNode(NodeType::AssignmentExpressionLeftShift)
    {
        m_left = left;
        m_right = right;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateResolveAddressByteCode(codeBlock, context);
        m_left->generateReferenceResolvedAddressByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);

        updateNodeIndex(context);
        codeBlock->pushCode(LeftShift(), context, this);
        WRITE_LAST_INDEX(m_nodeIndex, m_left->nodeIndex(), m_right->nodeIndex());

        updateNodeIndex(context);
        m_left->generatePutByteCode(codeBlock, context);
        WRITE_LAST_INDEX(m_nodeIndex, m_nodeIndex - 1, -1);
    }

protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
};

}

#endif
