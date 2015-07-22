#include "Escargot.h"
#include "ArrayExpressionNode.h"

#include "vm/ESVMInstance.h"

namespace escargot {

ESValue* ArrayExpressionNode::execute(ESVMInstance* instance)
{
    JSArray* arr = JSArray::create(0, instance->globalObject()->arrayPrototype());
    for(unsigned i = 0; i < m_elements.size() ; i++) {
        ESValue* result = m_elements[i]->execute(instance)->ensureValue();
        arr->set(ESString((int) i), result);
    }
    int len = m_elements.size();
    arr->setLength(len);
    return arr;
}

}
