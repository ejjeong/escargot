#ifndef AssignmentExpressionSimpleNode_h
#define AssignmentExpressionSimpleNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "IdentifierNode.h"
//#include "MemberExpressionNode.h"

namespace escargot {

//An assignment operator expression.
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
#ifndef ENABLE_ESJIT
        m_left->generatePutByteCode(codeBlock, context);
#else
        m_left->generatePutByteCode(codeBlock, context, m_right->nodeIndex());
#endif
        if (m_left->type() == escargot::NodeType::Identifier) {
            m_left->updateNodeIndex(context);
#ifdef ENABLE_ESJIT
            if (m_right->nodeIndex() == -1) { /* For, var a = b = .. = something */
                if (((AssignmentExpressionSimpleNode*)m_right)->m_left != nullptr &&
                    ((AssignmentExpressionSimpleNode*)m_right)->m_left->type() == escargot::NodeType::Identifier) {
                    WRITE_LAST_INDEX(m_left->nodeIndex(), ((AssignmentExpressionSimpleNode*)m_right)->m_left->nodeIndex(), -1);
                }
            } else {
                WRITE_LAST_INDEX(m_left->nodeIndex(), m_right->nodeIndex(), -1);
            }
#endif
         }
    }

protected:
    Node* m_left; //left: Pattern;
    Node* m_right; //right: Expression;
};

}

#endif
