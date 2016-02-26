#ifndef ObjectExpressionNode_h
#define ObjectExpressionNode_h

#include "ExpressionNode.h"
#include "IdentifierNode.h"
#include "PropertyNode.h"

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

    virtual NodeType type() { return NodeType::ObjectExpression; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(CreateObject(m_properties.size()), context, this);
        for (unsigned i = 0; i < m_properties.size() ; i ++) {
            PropertyNode* p = m_properties[i];
            if (p->key()->isIdentifier()) {
                p->key()->generateExpressionByteCode(codeBlock, context);
            } else {
                codeBlock->pushCode(Push(p->keyString()), context, this);
            }

            p->value()->generateExpressionByteCode(codeBlock, context);

            if (p->kind() == PropertyNode::Kind::Init) {
                codeBlock->pushCode(InitObject(), context, this);

            } else if (p->kind() == PropertyNode::Kind::Get) {
                codeBlock->pushCode(SetObjectPropertyGetter(), context, this);
            } else {
                ASSERT(p->kind() == PropertyNode::Kind::Set);
                codeBlock->pushCode(SetObjectPropertySetter(), context, this);
            }
        }
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 3 + m_properties.size() * 2;
        for (unsigned i = 0; i < m_properties.size() ; i ++) {
            PropertyNode* p = m_properties[i];
            p->value()->computeRoughCodeBlockSizeInWordSize(result);
        }
    }
protected:
    PropertiesNodeVector m_properties;
};

}

#endif
