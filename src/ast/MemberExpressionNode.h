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
        if(!m_computed) {
            ASSERT(m_property->type() == NodeType::Identifier);
            return true;
        }
        else {
            return false;
        }
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_object->generateExpressionByteCode(codeBlock, context);

        if(ESVMInstance::currentInstance()->currentInstance()->globalObject()->didSomePrototypeObjectDefineIndexedProperty()) {
            if(isPreComputedCase()) {
                ASSERT(m_property->type() == NodeType::Identifier);
                codeBlock->pushCode(GetObjectPreComputedCaseSlowMode(((IdentifierNode *)m_property)->nonAtomicName()), this);
                updateNodeIndex(context);
                WRITE_LAST_INDEX(m_nodeIndex, m_object->nodeIndex(), m_nodeIndex - 1);
            } else {
                m_property->generateExpressionByteCode(codeBlock, context);
                updateNodeIndex(context);
                codeBlock->pushCode(GetObjectSlowMode(), this);
                WRITE_LAST_INDEX(m_nodeIndex, m_object->nodeIndex(), m_property->nodeIndex());
            }
            return ;
        }
        if(isPreComputedCase()) {
            ASSERT(m_property->type() == NodeType::Identifier);
            updateNodeIndex(context);
            codeBlock->pushCode(GetObjectPreComputedCase(((IdentifierNode *)m_property)->nonAtomicName()), this);
            WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
        } else {
            m_property->generateExpressionByteCode(codeBlock, context);
            updateNodeIndex(context);
            codeBlock->pushCode(GetObject(), this);
            WRITE_LAST_INDEX(m_nodeIndex, m_object->nodeIndex(), m_property->nodeIndex());
        }
    }


    virtual void generatePutByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if(ESVMInstance::currentInstance()->currentInstance()->globalObject()->didSomePrototypeObjectDefineIndexedProperty()) {
            if(isPreComputedCase()) {
                ASSERT(m_property->type() == NodeType::Identifier);
                codeBlock->pushCode(SetObjectPreComputedCaseSlowMode(((IdentifierNode *)m_property)->nonAtomicName()), this);
            } else {
                updateNodeIndex(context);
                codeBlock->pushCode(SetObjectSlowMode(), this);
                WRITE_LAST_INDEX(m_nodeIndex, m_object->nodeIndex(), m_property->nodeIndex());
            }
            return ;
        }
        if(isPreComputedCase()) {
            ASSERT(m_property->type() == NodeType::Identifier);
            codeBlock->pushCode(SetObjectPreComputedCase(((IdentifierNode *)m_property)->nonAtomicName()), this);
        } else {
            updateNodeIndex(context);
            codeBlock->pushCode(SetObject(), this);
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
        if(ESVMInstance::currentInstance()->currentInstance()->globalObject()->didSomePrototypeObjectDefineIndexedProperty()) {
            if(isPreComputedCase()) {
                ASSERT(m_property->type() == NodeType::Identifier);
                codeBlock->pushCode(GetObjectWithPeekingPreComputedCaseSlowMode(((IdentifierNode *)m_property)->nonAtomicName()), this);
            } else {
                codeBlock->pushCode(GetObjectWithPeekingSlowMode(), this);
            }
            return ;
        }
        if(isPreComputedCase()) {
            ASSERT(m_property->type() == NodeType::Identifier);
            codeBlock->pushCode(GetObjectWithPeekingPreComputedCase(((IdentifierNode *)m_property)->nonAtomicName()), this);
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
