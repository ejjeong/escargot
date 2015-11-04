#ifndef ArrayExpressionNode_h
#define ArrayExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class ArrayExpressionNode : public ExpressionNode {
public:
    friend class ScriptParser;
    ArrayExpressionNode(ExpressionNodeVector&& elements)
        : ExpressionNode(NodeType::ArrayExpression)
    {
        m_elements = elements;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        unsigned len = m_elements.size();
        codeBlock->pushCode(CreateArray(len), context, this);
        for (unsigned i = 0; i < len; i++) {
            codeBlock->pushCode(Push(ESValue(i)), context, this);
            if (m_elements[i]) {
                m_elements[i]->generateExpressionByteCode(codeBlock, context);
            } else {
                codeBlock->pushCode(Push(ESValue(ESValue::ESEmptyValue)), context, this);
            }
            codeBlock->pushCode(InitObject(), context, this);
        }
    }
protected:
    ExpressionNodeVector m_elements;
};

}

#endif
