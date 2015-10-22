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
        WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
#ifdef ENABLE_ESJIT
        int arrayIndex = m_nodeIndex;
#endif
        for(unsigned i = 0; i < len ; i++) {
            codeBlock->pushCode(Push(ESValue(i)), context, this);
            WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
#ifdef ENABLE_ESJIT
            int keyIndex = m_nodeIndex;
            int sourceIndex;
#endif
            if(m_elements[i]) {
                m_elements[i]->generateExpressionByteCode(codeBlock, context);
#ifdef ENABLE_ESJIT
                sourceIndex = m_elements[i]->nodeIndex();
#endif
            } else {
                updateNodeIndex(context);
                codeBlock->pushCode(Push(ESValue(ESValue::ESEmptyValue)), context, this);
                WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
#ifdef ENABLE_ESJIT
                sourceIndex = m_nodeIndex;
#endif
              }
            updateNodeIndex(context);
#ifdef ENABLE_ESJIT
            codeBlock->pushCode(InitObject(arrayIndex), context, this);
#else
            codeBlock->pushCode(InitObject(), context, this);
#endif
            WRITE_LAST_INDEX(m_nodeIndex, keyIndex, sourceIndex);
        }
#ifdef ENABLE_ESJIT
        m_nodeIndex = arrayIndex;
#endif
            if(m_elements[i])
                m_elements[i]->generateExpressionByteCode(codeBlock, context);
            else
                codeBlock->pushCode(Push(ESValue(ESValue::ESEmptyValue)), context, this);
            codeBlock->pushCode(InitObject(), context, this);
        }
    }
protected:
    ExpressionNodeVector m_elements;
};

}

#endif
