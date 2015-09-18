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
        m_cachedHiddenClass = nullptr;
        m_cachedPropertyValue = nullptr;
        m_computed = computed;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
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
        codeBlock->pushCode(GetObject(), this);
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
            if(m_property->type() == NodeType::Literal)
                codeBlock->pushCode(Push(((LiteralNode *)m_property)->value()), this);
            else {
                ASSERT(m_property->type() == NodeType::Identifier);
                codeBlock->pushCode(Push(((IdentifierNode *)m_property)->nonAtomicName()), this);
            }
        }
    }

    virtual void generateReferenceResolvedAddressByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(GetObjectWithPeeking(), this);
    }
protected:
    ESHiddenClass* m_cachedHiddenClass;
    ESString* m_cachedPropertyValue;
    size_t m_cachedIndex;

    Node* m_object; //object: Expression;
    Node* m_property; //property: Identifier | Expression;

    bool m_computed;
};

}

#endif
