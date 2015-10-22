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
        updateNodeIndex(context);
        codeBlock->pushCode(CreateArray(len), context, this);
        WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
        int arrayIndex = m_nodeIndex;
        for(unsigned i = 0; i < len ; i++) {
            updateNodeIndex(context);
            codeBlock->pushCode(Push(ESValue(i)), context, this);
            WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
            int keyIndex = m_nodeIndex;
            int sourceIndex;
            if(m_elements[i]) {
                m_elements[i]->generateExpressionByteCode(codeBlock, context);
                sourceIndex = m_elements[i]->nodeIndex();
            } else {
                updateNodeIndex(context);
                codeBlock->pushCode(Push(ESValue(ESValue::ESEmptyValue)), context, this);
                WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
                sourceIndex = m_nodeIndex;
              }
            updateNodeIndex(context);
            codeBlock->pushCode(InitObject(arrayIndex), context, this);
            WRITE_LAST_INDEX(m_nodeIndex, keyIndex, sourceIndex);
        }
        m_nodeIndex = arrayIndex;
    }
protected:
    ExpressionNodeVector m_elements;
};

}

#endif
