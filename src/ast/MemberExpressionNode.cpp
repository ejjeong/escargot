#include "Escargot.h"
#include "MemberExpressionNode.h"

#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"


namespace escargot {

ESValue MemberExpressionNode::execute(ESVMInstance* instance)
{
    ESValue value = m_object->execute(instance).ensureValue();
    //TODO string,number-> stringObject, numberObject;
    if(value.isESPointer() && value.asESPointer()->isESString()) {
        ESStringObject* stringObject = ESStringObject::create(value.asESPointer()->asESString()->string());
        stringObject->set__proto__(instance->globalObject()->stringPrototype());
        stringObject->setConstructor(instance->globalObject()->string());
        value = stringObject;
    }

    if(value.isESPointer() && value.asESPointer()->isESObject()) {
        ESObject* obj = value.asESPointer()->asESObject();
        InternalAtomicString propertyName;
        ESValue propertyVal;
        if(!m_computed && m_property->type() == NodeType::Identifier) {
            propertyName = ((IdentifierNode*)m_property)->name();
        } else {
            ESValue tmpVal = m_property->execute(instance).ensureValue();
            if(m_computed && obj->isESArrayObject())
                propertyVal = tmpVal;
            else
                propertyName = InternalAtomicString(tmpVal.toInternalString().data());
        }

        instance->currentExecutionContext()->setLastESObjectMetInMemberExpressionNode(obj,
                propertyName, propertyVal);

        ESSlot* slot;
        if (obj->isESArrayObject() && propertyVal.isInt32())
            slot = obj->asESArrayObject()->find(propertyVal);
        else
            slot = obj->find(propertyName);

        if(slot) {
            return slot;
        } else {
            ESValue prototype = obj->__proto__();
            while(prototype.isESPointer() && prototype.asESPointer()->isESObject()) {
                ::escargot::ESObject* obj = prototype.asESPointer()->asESObject();
                ESSlot* s = obj->find(propertyName);
                if(s)
                    return s;
                prototype = obj->__proto__();
            }
        }
    } else {
        throw TypeError();
    }
    return ESValue();
}

}
