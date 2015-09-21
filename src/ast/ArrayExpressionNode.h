#ifndef ArrayExpressionNode_h
#define ArrayExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class ArrayExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    ArrayExpressionNode(ExpressionNodeVector&& elements)
        : ExpressionNode(NodeType::ArrayExpression)
    {
        m_elements = elements;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        unsigned len = m_elements.size();
        codeBlock->pushCode(CreateArray(len), this);
        for(unsigned i = 0; i < len ; i++) {
            codeBlock->pushCode(Push(ESValue(i)), this);
            m_elements[i]->generateExpressionByteCode(codeBlock, context);
            codeBlock->pushCode(SetObject(), this);
        }
    }
protected:
    ExpressionNodeVector m_elements;
};

}

#endif
