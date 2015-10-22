#ifndef ObjectExpressionNode_h
#define ObjectExpressionNode_h

#include "ExpressionNode.h"
#include "PropertyNode.h"
#include "IdentifierNode.h"

namespace escargot {

typedef std::vector<PropertyNode *, gc_allocator<PropertyNode *>> PropertiesNodeVector;

class ObjectExpressionNode : public ExpressionNode {
public:
    friend class ScriptParser;
    ObjectExpressionNode(PropertiesNodeVector&& properties)
            : ExpressionNode(NodeType::ObjectExpression)
    {
        m_properties = properties;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(CreateObject(m_properties.size()), context, this);
        for(unsigned i = 0; i < m_properties.size() ; i ++) {
            PropertyNode* p = m_properties[i];
            if(p->key()->type() == NodeType::Identifier) {
                codeBlock->pushCode(Push(((IdentifierNode* )p->key())->name().string()), context, this);
            } else {
                ASSERT(p->key()->type() == NodeType::Literal);
                codeBlock->pushCode(Push(((LiteralNode* )p->key())->value()), context, this);
            }

            p->value()->generateExpressionByteCode(codeBlock, context);

            if(p->kind() == PropertyNode::Kind::Init) {
#ifndef ENABLE_ESJIT
                codeBlock->pushCode(InitObject(), context, this);
#else
                codeBlock->pushCode(InitObject(-1), context, this);
#endif
            } else if(p->kind() == PropertyNode::Kind::Get) {
                codeBlock->pushCode(SetObjectPropertyGetter(), context, this);
            } else {
                ASSERT(p->kind() == PropertyNode::Kind::Set);
                codeBlock->pushCode(SetObjectPropertySetter(), context, this);
            }
        }
    }
protected:
    PropertiesNodeVector m_properties;
};

}

#endif
