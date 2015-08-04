#include "Escargot.h"
#include "NewExpressionNode.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue NewExpressionNode::execute(ESVMInstance* instance)
{
    ESValue fn = m_callee->execute(instance).ensureValue();
    if(!fn.isESPointer() || !fn.asESPointer()->isESFunctionObject())
        throw TypeError();
    ESFunctionObject* function = fn.asESPointer()->asESFunctionObject();
    ESObject* receiver;
    if (function == instance->globalObject()->date()) {
        receiver = ESDateObject::create();
        receiver->asESDateObject()->setTimeValue();
    } else if (function == instance->globalObject()->array()) {
        receiver = ESArrayObject::create();
    } else if (function == instance->globalObject()->string()) {
        receiver = ESStringObject::create();
    } else {
        receiver = ESObject::create();
    }
    receiver->setConstructor(fn);
    receiver->set__proto__(function->protoType());

    std::vector<ESValue, gc_allocator<ESValue>> arguments;
    for(unsigned i = 0; i < m_arguments.size() ; i ++) {
        arguments.push_back(m_arguments[i]->execute(instance).ensureValue());
    }

    ESFunctionObject::call(fn, receiver, &arguments[0], arguments.size(), instance);
    return receiver;
}

}
