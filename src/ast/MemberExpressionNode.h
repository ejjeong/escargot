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

    virtual void generateExpressionByteCodeWithoutGetObject(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_object->generateExpressionByteCode(codeBlock, context);
        if(m_computed) {
            m_property->generateExpressionByteCode(codeBlock, context);
        } else {
            if(m_property->type() == NodeType::Literal)
                codeBlock->pushCode(Push(((LiteralNode *)m_property)->value()), this);
            else {
                ASSERT(m_property->type() == NodeType::Identifier);
                codeBlock->pushCode(Push(((IdentifierNode *)m_property)->nonAtomicName()), this);
            }
        }
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_object->generateExpressionByteCode(codeBlock, context);
        if(m_computed) {
            m_property->generateExpressionByteCode(codeBlock, context);
        } else {
            if(m_property->type() == NodeType::Literal) {
                updateNodeIndex(context);
                codeBlock->pushCode(Push(((LiteralNode *)m_property)->value()), this);
                WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
            } else {
                ASSERT(m_property->type() == NodeType::Identifier);
                updateNodeIndex(context);
                codeBlock->pushCode(Push(((IdentifierNode *)m_property)->nonAtomicName()), this);
                WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
            }
        }
        updateNodeIndex(context);
        codeBlock->pushCode(GetObject(), this);
        WRITE_LAST_INDEX(m_nodeIndex, m_object->nodeIndex(), m_computed ? m_property->nodeIndex() : m_nodeIndex - 1);
    }


    virtual void generatePutByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(PutInObject(), this);
    }

    virtual void generateResolveAddressByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_object->generateExpressionByteCode(codeBlock, context);
        if(m_computed) {
            m_property->generateExpressionByteCode(codeBlock, context);
        } else {
            if(m_property->type() == NodeType::Literal) {
                updateNodeIndex(context);
                codeBlock->pushCode(Push(((LiteralNode *)m_property)->value()), this);
            } else {
                ASSERT(m_property->type() == NodeType::Identifier);
                updateNodeIndex(context);
                codeBlock->pushCode(Push(((IdentifierNode *)m_property)->nonAtomicName()), this);
            }
        }
    }

    virtual void generateReferenceResolvedAddressByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(GetObjectWithPeeking(), this);
    }
protected:
    Node* m_object; //object: Expression;
    Node* m_property; //property: Identifier | Expression;

    bool m_computed;
};

}

#endif
