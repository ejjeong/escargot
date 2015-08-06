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
    if (!m_computed && value.isPrimitive()) {
        value = value.toObject();
    }

    if(value.isESPointer() && value.asESPointer()->isESObject()) {
        ESObject* obj = value.asESPointer()->asESObject();
        ESSlot* slot;
        InternalString computedPropertyName;
        ESValue computedPropertyValue;
        if(!m_computed && m_property->type() == NodeType::Identifier) {
            InternalString name = ((IdentifierNode *)m_property)->nonAtomicName();
            slot = obj->find(name);
            computedPropertyName = name;
            computedPropertyValue = ESValue(((IdentifierNode *)m_property)->esName());
        } else {
            computedPropertyValue = m_property->execute(instance);
            if(obj->isESArrayObject()) {
                if(computedPropertyValue.isInt32())
                    slot = obj->asESArrayObject()->findOnlyIndex(computedPropertyValue.asInt32());
                if(!slot) {
                    computedPropertyName = computedPropertyValue.toInternalString();
                    slot = obj->find(computedPropertyName);
                }
            } else {
                computedPropertyName = computedPropertyValue.toInternalString();
                slot = obj->find(computedPropertyName);
            }
        }

        ExecutionContext* ec = instance->currentExecutionContext();
        if(slot) {
            ec->setLastESObjectMetInMemberExpressionNode(obj, slot);
            return slot->value(obj);
        } else {
            //computedPropertyName.show();
            ec->setLastESObjectMetInMemberExpressionNode(obj, computedPropertyValue);
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
    } else if (value.isESString()) {
        int prop_val = m_property->execute(instance).asInt32();
        return value.asESString()->substring(prop_val, prop_val+1);
    } else {
        throw TypeError(L"MemberExpression: object doesn't have object type");
    }
    return ESValue();
}

}
