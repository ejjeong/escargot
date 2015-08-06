#include "Escargot.h"
#include "ObjectExpressionNode.h"

#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue ObjectExpressionNode::execute(ESVMInstance* instance)
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

}

