#ifndef UnaryExpressionTypeOfNode_h
#define UnaryExpressionTypeOfNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionTypeOfNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    UnaryExpressionTypeOfNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionTypeOf)
    {
        m_argument = argument;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if(m_argument->type() == Identifier && !((IdentifierNode *)m_argument)->canUseFastAccess()) {
            codeBlock->pushCode(GetByIdWithoutException(
                    ((IdentifierNode *)m_argument)->name(),
                    ((IdentifierNode *)m_argument)->nonAtomicName()
                    ), this);
        } else
            m_argument->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(UnaryTypeOf(), this);
    }

protected:
    Node* m_argument;
};

}

#endif
