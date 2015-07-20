#ifndef ObjectExpressionNode_h
#define ObjectExpressionNode_h

#include "ExpressionNode.h"
#include "PropertyNode.h"
#include "IdentifierNode.h"

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
            ESString key;
            if(p->key()->type() == NodeType::Identifier) {
                key = ((IdentifierNode* )p->key())->name();
            } else {
                key = p->key()->execute(instance)->ensureValue()->toESString();
            }
            ESValue* value = p->value()->execute(instance)->ensureValue();
            obj->set(key, value);
        }
        return obj;
    }
protected:
    PropertiesNodeVector m_properties;
};

}

#endif
