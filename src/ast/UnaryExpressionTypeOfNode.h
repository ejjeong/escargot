#ifndef UnaryExpressionTypeOfNode_h
#define UnaryExpressionTypeOfNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionTypeOfNode : public ExpressionNode {
public:
    friend class ScriptParser;
    UnaryExpressionTypeOfNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionTypeOf)
    {
        m_argument = argument;
    }

    virtual NodeType type() { return NodeType::UnaryExpressionTypeOf; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if (m_argument->isIdentifier() && !((IdentifierNode *)m_argument)->canUseFastAccess()) {
            codeBlock->pushCode(GetByIdWithoutException(
                ((IdentifierNode *)m_argument)->name(),
                ((IdentifierNode *)m_argument)->onlySearchGlobal()
                ), context, this);
        } else
            m_argument->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(UnaryTypeOf(), context, this);
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
