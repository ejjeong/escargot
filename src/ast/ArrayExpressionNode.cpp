#include "Escargot.h"
#include "ArrayExpressionNode.h"

#include "vm/ESVMInstance.h"

namespace escargot {

ESValue ArrayExpressionNode::execute(ESVMInstance* instance)
{
    int len = m_elements.size();
    ESArrayObject* arr = ESArrayObject::create(len, instance->globalObject()->arrayPrototype());
    for(unsigned i = 0; i < m_elements.size() ; i++) {
        ESValue result = m_elements[i]->execute(instance).ensureValue();
        arr->set(i, result);
    }
    return arr;
}

}
