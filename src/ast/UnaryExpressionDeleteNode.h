#ifndef UnaryExpressionDeleteNode_h
#define UnaryExpressionDeleteNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionDeleteNode : public ExpressionNode {
public:
    friend class ScriptParser;
    UnaryExpressionDeleteNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionDelete)
    {
        m_argument = argument;
    }

    virtual NodeType type() { return NodeType::UnaryExpressionDelete; }


    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if (m_argument->isMemberExpression()) {
            MemberExpressionNode* mem = (MemberExpressionNode*) m_argument;
            mem->generateExpressionByteCode(codeBlock, context);
            if (mem->isPreComputedCase()) {
                ESValue v = codeBlock->peekCode<GetObjectPreComputedCase>(codeBlock->lastCodePosition<GetObjectPreComputedCase>())->m_propertyValue;
                codeBlock->popLastCode<GetObjectPreComputedCase>();
                codeBlock->pushCode(Push(v), context, this);
            } else
                codeBlock->popLastCode<GetObject>();
            codeBlock->pushCode(UnaryDelete(true), context, this);
        } else if (m_argument->isIdentifier()) {
            codeBlock->pushCode(Push(((IdentifierNode *)m_argument)->name().string()), context, this);
            // TODO
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 1;
        m_argument->computeRoughCodeBlockSizeInWordSize(result);
    }

protected:
    Node* m_argument;
};

}

#endif
