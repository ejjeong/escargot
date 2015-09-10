#ifndef MemberExpressionLeftIdentifierFastCaseNode_h
#define MemberExpressionLeftIdentifierFastCaseNode_h

#include "ExpressionNode.h"
#include "PropertyNode.h"
#include "IdentifierNode.h"

namespace escargot {

class MemberExpressionLeftIdentifierFastCaseNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    friend class UnaryExpressionDeleteNode;
    MemberExpressionLeftIdentifierFastCaseNode(size_t idx, Node* property, bool computed)
            : ExpressionNode(NodeType::MemberExpressionLeftIdentifierFastCase)
    {
        ASSERT(computed);
        m_index = idx;
        m_property = property;
        m_cachedHiddenClass = nullptr;
        m_cachedPropertyValue = nullptr;

    }

    ESSlotAccessor executeForWrite(ESVMInstance* instance)
    {
        ESValue value = instance->currentExecutionContext()->cachedDeclarativeEnvironmentRecordESValue()[m_index];
        ExecutionContext* ec = instance->currentExecutionContext();

        if(UNLIKELY(!value.isObject())) {
            value = value.toObject();
        }

        ASSERT(value.isESPointer() && value.asESPointer()->isESObject());

        ESObject* obj  = value.asESPointer()->asESObject();
        ec->setLastESObjectMetInMemberExpressionNode(obj);

        ESValue key = m_property->executeExpression(instance);
        ESSlotAccessor slot = obj->findOwnProperty(key);
        if(slot.hasData()) {
            return slot;
        } else {
            return obj->definePropertyOrThrow(key, true, true, true);
        }
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESValue value = instance->currentExecutionContext()->cachedDeclarativeEnvironmentRecordESValue()[m_index];
        ESValue propertyValue = m_property->executeExpression(instance);

        if(UNLIKELY(value.isESString())) {
            if(propertyValue.isInt32()) {
               int prop_val = propertyValue.toInt32();
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
                instance->globalObject()->stringObjectProxy()->setString(value.asESString());
                ESSlotAccessor slot = instance->globalObject()->stringObjectProxy()->find(propertyValue, true);
                if(slot.hasData()) {
                    if(slot.isDataProperty()) {
                        ESValue ret = slot.readDataProperty();
                        if(ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->functionAST()->isBuiltInFunction()) {
                            instance->currentExecutionContext()->setLastESObjectMetInMemberExpressionNode(instance->globalObject()->stringObjectProxy());
                            return ret;
                        }
                    } else {
                        if(slot.accessorData() == instance->stringObjectLengthAccessorData()) {
                            return slot.value();
                        }
                    }
                }
            }
        } else if(UNLIKELY(value.isNumber())) {
            instance->globalObject()->numberObjectProxy()->setNumberData(value.asNumber());
            ESSlotAccessor slot = instance->globalObject()->numberObjectProxy()->find(propertyValue, true);
            if(slot.hasData() && slot.isDataProperty()) {
                ESValue ret = slot.value();
                if(ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->functionAST()->isBuiltInFunction()) {
                    instance->currentExecutionContext()->setLastESObjectMetInMemberExpressionNode(instance->globalObject()->numberObjectProxy());
                    return ret;
                }
            }
        }

        if (UNLIKELY(!value.isObject())) {
            value = value.toObject();
        }

        ESObject* obj = value.asESPointer()->asESObject();
        ExecutionContext* ec = instance->currentExecutionContext();
        ec->setLastESObjectMetInMemberExpressionNode(obj);

        if(obj->isHiddenClassMode() && !obj->isESArrayObject()) {
            ESString* val = propertyValue.toString();
            if(m_cachedHiddenClass == obj->hiddenClass() && (val == m_cachedPropertyValue || *val == *m_cachedPropertyValue)) {
                return obj->readHiddenClass(m_cachedIndex).value();
            } else {
                size_t idx = obj->hiddenClass()->findProperty(val);
                if(idx != SIZE_MAX) {
                    m_cachedHiddenClass = obj->hiddenClass();
                    m_cachedPropertyValue = val;
                    m_cachedIndex = idx;
                    return obj->readHiddenClass(idx).value();
                } else {
                    m_cachedHiddenClass = nullptr;
                    ESSlotAccessor ac = obj->findOnlyPrototype(val);
                    if(ac.hasData())
                        return obj->findOnlyPrototype(val).value();
                    return ESValue();
                }
            }
        } else {
            return obj->get(propertyValue, true);
        }
        RELEASE_ASSERT_NOT_REACHED();
    }
protected:
    size_t m_index;
    ESHiddenClass* m_cachedHiddenClass;
    ESString* m_cachedPropertyValue;
    size_t m_cachedIndex;

    Node* m_property; //property: Identifier | Expression;
};

}

#endif
