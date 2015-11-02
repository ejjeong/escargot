#ifndef MemberExpressionNode_h
#define MemberExpressionNode_h

#include "ExpressionNode.h"
#include "PropertyNode.h"
#include "IdentifierNode.h"

namespace escargot {

class MemberExpressionNode : public ExpressionNode {
public:
    friend class ScriptParser;
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
        bool prevHead = context.m_isHeadOfMemberExpression;
        context.m_isHeadOfMemberExpression = false;
        m_object->generateExpressionByteCode(codeBlock, context);

        if(ESVMInstance::currentInstance()->globalObject()->didSomePrototypeObjectDefineIndexedProperty()) {
            if(isPreComputedCase()) {
                ASSERT(m_property->type() == NodeType::Identifier);
                if(context.m_inCallingExpressionScope && prevHead)
                    codeBlock->pushCode(GetObjectPreComputedCaseAndPushObjectSlowMode(((IdentifierNode *)m_property)->name().string()), context, this);
                else
                    codeBlock->pushCode(GetObjectPreComputedCaseSlowMode(((IdentifierNode *)m_property)->name().string()), context, this);
            } else {
                m_property->generateExpressionByteCode(codeBlock, context);
                if(context.m_inCallingExpressionScope && prevHead)
                    codeBlock->pushCode(GetObjectAndPushObjectSlowMode(), context, this);
                else
                    codeBlock->pushCode(GetObjectSlowMode(), context, this);
            }
            return ;
        }
        if(isPreComputedCase()) {
            ASSERT(m_property->type() == NodeType::Identifier);
            if(context.m_inCallingExpressionScope && prevHead)
                codeBlock->pushCode(GetObjectPreComputedCaseAndPushObject(((IdentifierNode *)m_property)->name().string()), context, this);
            else
                codeBlock->pushCode(GetObjectPreComputedCase(((IdentifierNode *)m_property)->name().string()), context, this);
        } else {
            m_property->generateExpressionByteCode(codeBlock, context);
            if(context.m_inCallingExpressionScope && prevHead)
                codeBlock->pushCode(GetObjectAndPushObject(), context, this);
            else
                codeBlock->pushCode(GetObject(), context, this);
        }
    }


    virtual void generatePutByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context, int sourceIndex = -1)
    {
        if(ESVMInstance::currentInstance()->globalObject()->didSomePrototypeObjectDefineIndexedProperty()) {
            if(isPreComputedCase()) {
                ASSERT(m_property->type() == NodeType::Identifier);
                codeBlock->pushCode(SetObjectPreComputedCaseSlowMode(((IdentifierNode *)m_property)->name().string()), context, this);
            } else {
                codeBlock->pushCode(SetObjectSlowMode(), context, this);
            }
            return ;
        }
        if(isPreComputedCase()) {
            ASSERT(m_property->type() == NodeType::Identifier);
            codeBlock->pushCode(SetObjectPreComputedCase(((IdentifierNode *)m_property)->name().string()), context, this);
        } else {
            codeBlock->pushCode(SetObject(), context, this);
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
        if(ESVMInstance::currentInstance()->globalObject()->didSomePrototypeObjectDefineIndexedProperty()) {
            if(isPreComputedCase()) {
                ASSERT(m_property->type() == NodeType::Identifier);
                codeBlock->pushCode(GetObjectWithPeekingPreComputedCaseSlowMode(((IdentifierNode *)m_property)->name().string()), context, this);
            } else {
                codeBlock->pushCode(GetObjectWithPeekingSlowMode(), context, this);
            }
            return ;
        }
        if(isPreComputedCase()) {
            ASSERT(m_property->type() == NodeType::Identifier);
            codeBlock->pushCode(GetObjectWithPeekingPreComputedCase(((IdentifierNode *)m_property)->name().string()), context, this);
        } else {
            codeBlock->pushCode(GetObjectWithPeeking(), context, this);
        }
    }
protected:
    Node* m_object; //object: Expression;
    Node* m_property; //property: Identifier | Expression;

    bool m_computed;
};

}

#endif

