#ifndef ObjectExpressionNode_h
#define ObjectExpressionNode_h

#include "ExpressionNode.h"
#include "PropertyNode.h"

namespace escargot {

typedef std::vector<PropertyNode *, gc_allocator<PropertyNode *>> PropertiesNodeVector;

class ObjectExpressionNode : public ExpressionNode {
public:
    ObjectExpressionNode(PropertiesNodeVector&& properties)
            : ExpressionNode(NodeType::ObjectExpression)
    {
        m_properties = properties;
    }

    virtual ESValue* execute(ESVMInstance* instance)
    {
        JSObject* obj = JSObject::create();
        for(unsigned i = 0; i < m_properties.size() ; i ++) {
            PropertyNode* p = m_properties[i];
            ESValue* key = p->key()->execute(instance)->ensureValue();
            ESValue* value = p->value()->execute(instance)->ensureValue();

            obj->set(key->toESString(), value);
        }
        return obj;
    }
protected:
    PropertiesNodeVector m_properties;
};

}

#endif
