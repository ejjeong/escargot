#ifndef ObjectExpressionNode_h
#define ObjectExpressionNode_h

#include "ExpressionNode.h"
#include "PropertyNode.h"
#include "IdentifierNode.h"

namespace escargot {

typedef std::vector<PropertyNode *, gc_allocator<PropertyNode *>> PropertiesNodeVector;

class ObjectExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    ObjectExpressionNode(PropertiesNodeVector&& properties)
            : ExpressionNode(NodeType::ObjectExpression)
    {
        m_properties = properties;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESObject* obj = ESObject::create();
        obj->setConstructor(instance->globalObject()->object());
        obj->set__proto__(instance->globalObject()->objectPrototype());

        for(unsigned i = 0; i < m_properties.size() ; i ++) {
            PropertyNode* p = m_properties[i];
            ESValue key;
            if(p->key()->type() == NodeType::Identifier) {
                key = ((IdentifierNode* )p->key())->nonAtomicName();
            } else {
                key = p->key()->executeExpression(instance);
            }
            obj->set(key, p->value()->executeExpression(instance));
        }
        return obj;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(CreateObject(m_properties.size()), this);
        for(unsigned i = 0; i < m_properties.size() ; i ++) {
            PropertyNode* p = m_properties[i];
            if(p->key()->type() == NodeType::Identifier) {
                codeBlock->pushCode(Push(((IdentifierNode* )p->key())->nonAtomicName()), this);
            } else {
                ASSERT(p->key()->type() == NodeType::Literal);
                codeBlock->pushCode(Push(((LiteralNode* )p->key())->value()), this);
            }
            p->value()->generateExpressionByteCode(codeBlock, context);
            codeBlock->pushCode(SetObject(), this);
        }
    }
protected:
    PropertiesNodeVector m_properties;
};

}

#endif
