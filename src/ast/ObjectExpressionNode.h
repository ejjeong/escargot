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

    ESValue execute(ESVMInstance* instance)
    {
        ESObject* obj = ESObject::create();
        obj->setConstructor(instance->globalObject()->object());
        obj->set__proto__(instance->globalObject()->objectPrototype());

        for(unsigned i = 0; i < m_properties.size() ; i ++) {
            PropertyNode* p = m_properties[i];
            InternalString key;
            if(p->key()->type() == NodeType::Identifier) {
                key = ((IdentifierNode* )p->key())->nonAtomicName();
            } else {
                ESValue ret = p->key()->execute(instance);
                key = ret.toInternalString();
            }
            ESValue value = p->value()->execute(instance);
            obj->set(key, value);
        }
        return obj;
    }
protected:
    PropertiesNodeVector m_properties;
};

}

#endif
