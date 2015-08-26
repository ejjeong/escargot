#ifndef MemberExpressionNode_h
#define MemberExpressionNode_h

#include "ExpressionNode.h"
#include "PropertyNode.h"
#include "IdentifierNode.h"

namespace escargot {

class MemberExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    MemberExpressionNode(Node* object, Node* property, bool computed)
            : ExpressionNode(NodeType::MemberExpression)
    {
        m_object = object;
        m_property = property;
        m_computed = computed;
    }

    ESSlotAccessor executeForWrite(ESVMInstance* instance)
    {
        ESValue value = m_object->execute(instance);
        ExecutionContext* ec = instance->currentExecutionContext();
        ESSlotAccessor slot;

        if(UNLIKELY(value.isPrimitive())) {
            value = value.toObject();
        }

        ASSERT(value.isESPointer() && value.asESPointer()->isESObject());

        ESObject* obj  = value.asESPointer()->asESObject();
        ec->setLastESObjectMetInMemberExpressionNode(obj);

        if(!m_computed && m_property->type() == NodeType::Identifier) {
            if(obj->isESArrayObject()) {
                slot = obj->asESArrayObject()->definePropertyOrThrow(((IdentifierNode *)m_property)->nonAtomicName());
            } else {
                slot = obj->definePropertyOrThrow(((IdentifierNode *)m_property)->nonAtomicName(), true, true, true);
            }
        } else {
            ESValue computedPropertyValue = m_property->execute(instance);
            if(obj->isESArrayObject()) {
                slot = obj->asESArrayObject()->definePropertyOrThrow(computedPropertyValue);
            } else {
                ESString* computedPropertyName = computedPropertyValue.toString();
                slot = obj->definePropertyOrThrow(computedPropertyName, true, true, true);
            }

        }
        return slot;
    }
    ESValue execute(ESVMInstance* instance)
    {
        ESValue value = m_object->execute(instance);

        if(UNLIKELY(!m_computed)) {
            NodeType propertyNodeType = m_property->type();
            if(UNLIKELY(value.isESString() && propertyNodeType == NodeType::Identifier)) {
                instance->globalObject()->stringObjectProxy()->setString(value.asESString());
                ESSlot* slot = instance->globalObject()->stringObjectProxy()->findUntilPrototype(((IdentifierNode *)m_property)->nonAtomicName());
                if(slot->isDataProperty()) {
                    ESValue ret = slot->value(instance->globalObject()->stringObjectProxy());
                    if(ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->functionAST()->isBuiltInFunction()) {
                        instance->currentExecutionContext()->setLastESObjectMetInMemberExpressionNode(instance->globalObject()->stringObjectProxy());
                        return ret;
                    }
                } else {
                    if(slot->accessorData() == instance->stringObjectLengthAccessorData()) {
                        return slot->value(instance->globalObject()->stringObjectProxy());
                    }
                }
            } else if(UNLIKELY(value.isNumber() && propertyNodeType == NodeType::Identifier)) {
                instance->globalObject()->numberObjectProxy()->setNumberData(value.asNumber());
                ESSlot* slot = instance->globalObject()->numberObjectProxy()->findUntilPrototype(((IdentifierNode *)m_property)->nonAtomicName());
                if(slot->isDataProperty()) {
                    ESValue ret = slot->value(instance->globalObject()->numberObjectProxy());
                    if(ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->functionAST()->isBuiltInFunction()) {
                        instance->currentExecutionContext()->setLastESObjectMetInMemberExpressionNode(instance->globalObject()->numberObjectProxy());
                        return ret;
                    }
                }
            }

            if (value.isPrimitive()) {
                value = value.toObject();
            }
        }

        if(value.isESPointer() && value.asESPointer()->isESObject()) {
            ESObject* obj = value.asESPointer()->asESObject();
            ESSlotAccessor slot;
            ESString* computedPropertyName;
            ExecutionContext* ec = instance->currentExecutionContext();

            if(!m_computed && m_property->type() == NodeType::Identifier) {
                computedPropertyName = ((IdentifierNode *)m_property)->nonAtomicName();
                slot = ESSlotAccessor(obj->find(computedPropertyName));
            } else {
                ESValue computedPropertyValue = m_property->execute(instance);
                if(obj->isESArrayObject()) {
                    if(computedPropertyValue.isInt32())
                        slot = obj->asESArrayObject()->find(computedPropertyValue);
                    if(!slot.hasData()) {
                        computedPropertyName = computedPropertyValue.toString();
                    }
                } else {
                    computedPropertyName = computedPropertyValue.toString();
                    slot = ESSlotAccessor(obj->find(computedPropertyName));
                }

            }

            if(LIKELY(slot.hasData())) {
                ec->setLastESObjectMetInMemberExpressionNode(obj);
                return slot.value(obj);
            } else {
                ec->setLastESObjectMetInMemberExpressionNode(obj);
                //FIXME this code duplicated with ESObject::get
                ESValue prototype = obj->__proto__();
                while(prototype.isESPointer() && prototype.asESPointer()->isESObject()) {
                    ::escargot::ESObject* new_obj = prototype.asESPointer()->asESObject();
                    ESSlot* s = new_obj->find(computedPropertyName);
                    if(s)
                        return s->value(obj);
                    prototype = new_obj->__proto__();
                }

                return ESValue();
            }
        } else if (value.isESString()) {
            int prop_val = m_property->execute(instance).toInt32();
            if(LIKELY(0 <= prop_val && prop_val < value.asESString()->length())) {
                char16_t c = value.asESString()->string().data()[prop_val];
                if(LIKELY(c < ESCARGOT_ASCII_TABLE_MAX)) {
                    return strings->asciiTable[c];
                } else {
                    return ESString::create(c);
                }
            } else {
                return ESValue();
            }
            return value.asESString()->substring(prop_val, prop_val+1);
        } else {
            throw TypeError(ESString::create(u"MemberExpression: object doesn't have object type"));
        }
        return ESValue();
    }
protected:
    Node* m_object; //object: Expression;
    Node* m_property; //property: Identifier | Expression;
    bool m_computed;
};

}

#endif
