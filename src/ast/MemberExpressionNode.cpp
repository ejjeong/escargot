#include "Escargot.h"
#include "MemberExpressionNode.h"

#include "IdentifierNode.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"


namespace escargot {

ESValue MemberExpressionNode::execute(ESVMInstance* instance)
{
    ESValue value = m_object->execute(instance);
    //TODO string,number-> stringObject, numberObject;
    if(value.isESPointer() && value.asESPointer()->isESString()) {
        ESStringObject* stringObject = ESStringObject::create(value.asESPointer()->asESString()->string());
        stringObject->set__proto__(instance->globalObject()->stringPrototype());
        stringObject->setConstructor(instance->globalObject()->string());
        value = stringObject;
    } else if (value.isPrimitive()) {
        value = value.toObject();
    }

    if(value.isESPointer() && value.asESPointer()->isESObject()) {
        ESObject* obj = value.asESPointer()->asESObject();
        ESSlot* slot;
        InternalAtomicString computedPropertyName;
        ESValue tmpVal;
        bool isPropertyNameComputed = false;
        if(!m_computed && m_property->type() == NodeType::Identifier) {
            InternalAtomicString name = ((IdentifierNode*)m_property)->name();
            instance->currentExecutionContext()->setLastESObjectMetInMemberExpressionNode(obj, name);
            slot = obj->find(name);
            computedPropertyName = name;
            isPropertyNameComputed = true;
        } else {
            tmpVal = m_property->execute(instance);
            instance->currentExecutionContext()->setLastESObjectMetInMemberExpressionNode(obj, tmpVal);
            if(obj->isESArrayObject()) {
                slot = obj->asESArrayObject()->find(tmpVal);
            } else {
                isPropertyNameComputed = true;
                computedPropertyName = InternalAtomicString(&tmpVal);
                slot = obj->find(computedPropertyName);
            }
        }

        if(slot) {
            return slot->value(obj);
        } else {
            if(!isPropertyNameComputed) {
                computedPropertyName = InternalAtomicString(&tmpVal);
            }
            //FIXME this code duplicated with ESObject::get
            ESValue prototype = obj->__proto__();
            while(prototype.isESPointer() && prototype.asESPointer()->isESObject()) {
                ::escargot::ESObject* obj = prototype.asESPointer()->asESObject();
                ESSlot* s = obj->find(computedPropertyName);
                if(s)
                    return s->value();
                prototype = obj->__proto__();
            }
        }
    } else {
        throw TypeError();
    }
    return ESValue();
}

}
