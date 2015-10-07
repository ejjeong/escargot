#ifndef MemberExpressionNode_h
#define MemberExpressionNode_h

#include "ExpressionNode.h"
#include "PropertyNode.h"
#include "IdentifierNode.h"

namespace escargot {

class MemberExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    friend class UnaryExpressionDeleteNode;
    MemberExpressionNode(Node* object, Node* property, bool computed)
            : ExpressionNode(NodeType::MemberExpression)
    {
        m_object = object;
        m_property = property;
        m_computed = computed;
    }

    bool isPreComputedCase()
    {
        return m_property->type() == NodeType::Literal;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_object->generateExpressionByteCode(codeBlock, context);

        if(isPreComputedCase()) {
            updateNodeIndex(context);
            codeBlock->pushCode(GetObjectPreComputedCase(((LiteralNode *)m_property)->value()), this);
            WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
            updateNodeIndex(context);
            WRITE_LAST_INDEX(m_nodeIndex, m_object->nodeIndex(), m_nodeIndex - 1);
        } else {
            m_property->generateExpressionByteCode(codeBlock, context);
            updateNodeIndex(context);
            codeBlock->pushCode(GetObject(), this);
            WRITE_LAST_INDEX(m_nodeIndex, m_object->nodeIndex(), m_property->nodeIndex());
        }
    }


    virtual void generatePutByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if(isPreComputedCase()) {
            ASSERT(m_property->type() == NodeType::Literal);
            codeBlock->pushCode(PutInObjectPreComputedCase(((LiteralNode *)m_property)->value()), this);
        } else {
            updateNodeIndex(context);
            codeBlock->pushCode(PutInObject(), this);
            WRITE_LAST_INDEX(m_nodeIndex, m_object->nodeIndex(), m_property->nodeIndex());
        }
    }

    virtual void generateResolveAddressByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_object->generateExpressionByteCode(codeBlock, context);
        if(isPreComputedCase()) {
        } else {
            m_property->generateExpressionByteCode(codeBlock, context);
        }
    }

    virtual void generateReferenceResolvedAddressByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if(isPreComputedCase()) {
            ASSERT(m_property->type() == NodeType::Literal);
            codeBlock->pushCode(GetObjectWithPeekingPreComputedCase(((LiteralNode *)m_property)->value()), this);
        } else {
            codeBlock->pushCode(GetObjectWithPeeking(), this);
        }
    }
#ifdef ENABLE_ESJIT
    int objectIndex() { return m_object->nodeIndex(); }
    int propertyIndex() { return m_property->nodeIndex(); }
#endif
protected:
    Node* m_object; //object: Expression;
    Node* m_property; //property: Identifier | Expression;

    bool m_computed;
};

}

#endif
