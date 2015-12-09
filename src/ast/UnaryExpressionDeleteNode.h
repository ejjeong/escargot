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
            if (mem->isPreComputedCase()) {
                codeBlock->pushCode(Push(mem->propertyName().string()), context, this);
            }
            mem->generateResolveAddressByteCode(codeBlock, context);
            codeBlock->pushCode(UnaryDelete(true), context, this);
        } else if (m_argument->isIdentifier()) {
            codeBlock->pushCode(UnaryDelete(false, ((IdentifierNode *)m_argument)->name().string()), context, this);
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
