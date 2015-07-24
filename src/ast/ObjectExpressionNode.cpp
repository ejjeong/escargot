#include "Escargot.h"
#include "ObjectExpressionNode.h"

#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue* ObjectExpressionNode::execute(ESVMInstance* instance)
{
    JSObject* obj = JSObject::create();
    obj->setConstructor(instance->globalObject()->object());
    obj->set__proto__(instance->globalObject()->objectPrototype());

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

}

