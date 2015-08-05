#include "Escargot.h"
#include "NewExpressionNode.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue NewExpressionNode::execute(ESVMInstance* instance)
{
    ESValue fn = m_callee->execute(instance);
    if(!fn.isESPointer() || !fn.asESPointer()->isESFunctionObject())
        throw TypeError();
    ESFunctionObject* function = fn.asESPointer()->asESFunctionObject();
    ESObject* receiver;
    if (function == instance->globalObject()->date()) {
        receiver = ESDateObject::create();
    } else if (function == instance->globalObject()->array()) {
        receiver = ESArrayObject::create();
    } else if (function == instance->globalObject()->string()) {
        receiver = ESStringObject::create();
    } else {
        receiver = ESObject::create();
    }
    receiver->setConstructor(fn);
    receiver->set__proto__(function->protoType());

    ESValue* arguments = (ESValue*)alloca(sizeof(ESValue) * m_arguments.size());
    for(unsigned i = 0; i < m_arguments.size() ; i ++) {
        arguments[i] = m_arguments[i]->execute(instance);
    }

    ESFunctionObject::call(fn, receiver, arguments, m_arguments.size(), instance, true);
    return receiver;
}

}
