#ifndef UnaryExpressionDeleteNode_h
#define UnaryExpressionDeleteNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionDeleteNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    UnaryExpressionDeleteNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionDelete)
    {
        m_argument = argument;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if (m_argument->type() == NodeType::MemberExpression) {
            MemberExpressionNode* mem = (MemberExpressionNode*) m_argument;
            mem->generateExpressionByteCode(codeBlock, context);
            if(mem->isPreComputedCase()) {
                ESValue v = codeBlock->peekCode<GetObjectPreComputedCase>(codeBlock->lastCodePosition<GetObjectPreComputedCase>())->m_propertyValue;
                codeBlock->popLastCode<GetObjectPreComputedCase>();
                codeBlock->pushCode(Push(v), this);
            } else
                codeBlock->popLastCode<GetObject>();
            codeBlock->pushCode(UnaryDelete(), this);
        } else if (m_argument->type() == NodeType::Identifier) {
            // TODO This work with the flag configurable
            codeBlock->pushCode(Push(((IdentifierNode *)m_argument)->nonAtomicName()), this);
        } else {
            RELEASE_ASSERT_NOT_REACHED();
         }
    }

protected:
    Node* m_argument;
};

}

#endif
