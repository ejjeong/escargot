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
        m_cachedHiddenClass = nullptr;
        m_cachedPropertyValue = nullptr;
    }

    ESSlotAccessor executeForWrite(ESVMInstance* instance)
    {
        ESValue value = m_object->execute(instance);
        ExecutionContext* ec = instance->currentExecutionContext();

        if(UNLIKELY(value.isPrimitive())) {
            value = value.toObject();
        }

        ASSERT(value.isESPointer() && value.asESPointer()->isESObject());

        ESObject* obj  = value.asESPointer()->asESObject();
        ec->setLastESObjectMetInMemberExpressionNode(obj);

        if(!m_computed && m_property->type() == NodeType::Identifier) {
            return obj->definePropertyOrThrow(((IdentifierNode *)m_property)->nonAtomicName(), true, true, true);
        } else {
            return obj->definePropertyOrThrow(m_property->execute(instance));
        }
    }
    ESValue execute(ESVMInstance* instance)
    {
        ESValue value = m_object->execute(instance);

        if(UNLIKELY(!m_computed)) {
            NodeType propertyNodeType = m_property->type();

            if(UNLIKELY(value.isESString() && propertyNodeType == NodeType::Identifier)) {
                instance->globalObject()->stringObjectProxy()->setString(value.asESString());
                ESSlotAccessor slot = instance->globalObject()->stringObjectProxy()->find(((IdentifierNode *)m_property)->nonAtomicName(), true);
                if(slot.isDataProperty()) {
                    ESValue ret = slot.readDataProperty();
                    if(ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->functionAST()->isBuiltInFunction()) {
                        instance->currentExecutionContext()->setLastESObjectMetInMemberExpressionNode(instance->globalObject()->stringObjectProxy());
                        return ret;
                    }
                } else {
                    if(slot.accessorData() == instance->stringObjectLengthAccessorData()) {
                        return slot.value(instance->globalObject()->stringObjectProxy());
                    }
                }
            } else if(UNLIKELY(value.isNumber() && propertyNodeType == NodeType::Identifier)) {
                instance->globalObject()->numberObjectProxy()->setNumberData(value.asNumber());
                ESSlotAccessor slot = instance->globalObject()->numberObjectProxy()->find(((IdentifierNode *)m_property)->nonAtomicName(), true);
                if(slot.isDataProperty()) {
                    ESValue ret = slot.value(instance->globalObject()->numberObjectProxy());
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
            ExecutionContext* ec = instance->currentExecutionContext();
            ec->setLastESObjectMetInMemberExpressionNode(obj);

            if(!m_computed && m_property->type() == NodeType::Identifier) {
                ESString* val = ((IdentifierNode *)m_property)->nonAtomicName();
                /*
                return obj->get(ESValue(val), true);
                */
                if(obj->isHiddenClassMode() && !obj->isESArrayObject()) {
                    if(m_cachedHiddenClass == obj->hiddenClass()) {
                        return obj->readHiddenClass(m_cachedIndex).value(obj);
                    } else {
                        size_t idx = obj->hiddenClass()->findProperty(val);
                        if(idx != SIZE_MAX) {
                            m_cachedHiddenClass = obj->hiddenClass();
                            m_cachedIndex = idx;
                            return obj->readHiddenClass(idx).value(obj);
                        } else {
                            m_cachedHiddenClass = nullptr;
                            ESSlotAccessor ac = obj->findOnlyPrototype(val);
                            if(ac.hasData())
                                return obj->findOnlyPrototype(val).value(obj);
                            return ESValue();
                        }
                    }
                } else {
                    return obj->get(val, true);
                }
            } else {
                //return obj->get(m_property->execute(instance), true);
                if(obj->isHiddenClassMode() && !obj->isESArrayObject()) {
                    ESString* val = m_property->execute(instance).toString();
                    if(m_cachedHiddenClass == obj->hiddenClass() && (val == m_cachedPropertyValue || *val == *m_cachedPropertyValue)) {
                        return obj->readHiddenClass(m_cachedIndex).value(obj);
                    } else {
                        size_t idx = obj->hiddenClass()->findProperty(val);
                        if(idx != SIZE_MAX) {
                            m_cachedHiddenClass = obj->hiddenClass();
                            m_cachedPropertyValue = val;
                            m_cachedIndex = idx;
                            return obj->readHiddenClass(idx).value(obj);
                        } else {
                            m_cachedHiddenClass = nullptr;
                            ESSlotAccessor ac = obj->findOnlyPrototype(val);
                            if(ac.hasData())
                                return obj->findOnlyPrototype(val).value(obj);
                            return ESValue();
                        }
                    }
                } else {
                    return obj->get(m_property->execute(instance), true);
                }
            }

            RELEASE_ASSERT_NOT_REACHED();
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
    ESHiddenClass* m_cachedHiddenClass;
    ESString* m_cachedPropertyValue;
    size_t m_cachedIndex;

    Node* m_object; //object: Expression;
    Node* m_property; //property: Identifier | Expression;
    bool m_computed;
};

}

#endif
