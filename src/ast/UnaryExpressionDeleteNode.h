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
        m_argument = (ExpressionNode*) argument;
    }

    virtual NodeType type() { return NodeType::UnaryExpressionDelete; }


    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if (m_argument->isMemberExpression()) {
            MemberExpressionNode* mem = (MemberExpressionNode*) m_argument;
            mem->generateResolveAddressByteCode(codeBlock, context);
            if (mem->isPreComputedCase()) {
                codeBlock->pushCode(Push(mem->propertyName().string()), context, this);
            }
            codeBlock->pushCode(UnaryDelete(true), context, this);
        } else if (m_argument->isIdentifier()) {
            codeBlock->pushCode(UnaryDelete(false, ((IdentifierNode *)m_argument)->name().string()), context, this);
        } else if (m_argument->isLiteral()) {
            codeBlock->pushCode(Push(ESValue(ESValue::ESTrue)), context, this);
        } else {
            m_argument->generateExpressionByteCode(codeBlock, context);
            codeBlock->pushCode(Pop(), context, this);
            codeBlock->pushCode(Push(ESValue(ESValue::ESTrue)), context, this);
        }
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 1;
        m_argument->computeRoughCodeBlockSizeInWordSize(result);
    }

protected:
    ExpressionNode* m_argument;
};

}

#endif
