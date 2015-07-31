#include "Escargot.h"
#include "MemberExpressionNode.h"

#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"


namespace escargot {

ESValue* MemberExpressionNode::execute(ESVMInstance* instance)
{
    ESValue* value = m_object->execute(instance)->ensureValue();
    //TODO string,number-> stringObject, numberObject;
    if(value->isHeapObject() && value->toHeapObject()->isESString()) {
        ESStringObject* stringObject = ESStringObject::create(value->toHeapObject()->toESString()->string());
        stringObject->set__proto__(instance->globalObject()->stringPrototype());
        stringObject->setConstructor(instance->globalObject()->string());
        value = stringObject;
    }

    if(value->isHeapObject() && value->toHeapObject()->isESObject()) {
        ESObject* obj = value->toHeapObject()->toESObject();
        InternalAtomicString propertyName;
        ESValue* propertyVal = NULL;
        if(!m_computed && m_property->type() == NodeType::Identifier) {
            propertyName = ((IdentifierNode*)m_property)->name();
        } else {
            ESValue* tmpVal = m_property->execute(instance)->ensureValue();
            if(m_computed && obj->toHeapObject()->isESArrayObject() && tmpVal->isSmi())
                propertyVal = tmpVal;
            propertyName = InternalAtomicString(tmpVal->toInternalString().data());
        }

        instance->currentExecutionContext()->setLastESObjectMetInMemberExpressionNode(obj->toHeapObject()->toESObject(),
                propertyName, propertyVal);

        ESSlot* slot;
        if (obj->isESArrayObject() && propertyVal != NULL)
            slot = obj->toESArrayObject()->find(propertyVal);
        else
            slot = obj->find(propertyName);

        if(slot) {
            return slot;
        } else {
            ESValue* prototype = obj->__proto__();
            while(prototype && prototype->isHeapObject() && prototype->toHeapObject()->isESObject()) {
                ::escargot::ESObject* obj = prototype->toHeapObject()->toESObject();
                ESSlot* s = obj->find(propertyName);
                if(s)
                    return s;
                prototype = obj->__proto__();
            }
        }
        return esUndefined;
    } else {
        throw TypeError();
    }
    return esUndefined;
}
}
