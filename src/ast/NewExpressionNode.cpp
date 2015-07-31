#include "Escargot.h"
#include "NewExpressionNode.h"

#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

namespace escargot {

ESValue* NewExpressionNode::execute(ESVMInstance* instance)
{
    ESValue* fn = m_callee->execute(instance)->ensureValue();
    if(!fn->isHeapObject() || !fn->toHeapObject()->isJSFunction())
        throw TypeError();
    JSFunction* function = fn->toHeapObject()->toJSFunction();
    JSObject* receiver;
    if (function == instance->globalObject()->date()) {
        receiver = ESDateObject::create();
        receiver->toESDateObject()->setTimeValue();
    } else {
        receiver = JSObject::create();
    }
    receiver->setConstructor(fn);
    receiver->set__proto__(function->protoType());

    std::vector<ESValue*, gc_allocator<ESValue*>> arguments;
    for(unsigned i = 0; i < m_arguments.size() ; i ++) {
        ESValue* result = m_arguments[i]->execute(instance)->ensureValue();
        arguments.push_back(result);
    }

    JSFunction::call(fn, receiver, &arguments[0], arguments.size(), instance);

    return receiver;
}

}
