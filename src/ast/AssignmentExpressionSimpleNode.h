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

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_left->generateResolveAddressByteCode(codeBlock, context);
        m_right->generateExpressionByteCode(codeBlock, context);
        m_left->updateNodeIndex(context);
        m_left->generatePutByteCode(codeBlock, context);
        WRITE_LAST_INDEX(m_left->nodeIndex(), m_right->nodeIndex(), -1);
    }

protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
};

}

#endif
