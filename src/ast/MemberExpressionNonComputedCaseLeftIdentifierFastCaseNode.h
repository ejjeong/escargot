#ifndef MemberExpressionNodeNonComputedCaseLeftIdentifierFastCase_h
#define MemberExpressionNodeNonComputedCaseLeftIdentifierFastCase_h

#include "ExpressionNode.h"
#include "PropertyNode.h"
#include "IdentifierNode.h"

namespace escargot {

class MemberExpressionNonComputedCaseLeftIdentifierFastCaseNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    friend class UnaryExpressionDeleteNode;
    MemberExpressionNonComputedCaseLeftIdentifierFastCaseNode(size_t idx, ESValue value, bool computed)
            : ExpressionNode(NodeType::MemberExpressionNonComputedCaseLeftIdentifierFastCase)
    {
        ASSERT(!computed);
        m_index = idx;
        m_propertyValue = value;
        m_cachedHiddenClass = nullptr;
    }

    ESSlotAccessor executeForWrite(ESVMInstance* instance)
    {
        ESValue value = instance->currentExecutionContext()->cachedDeclarativeEnvironmentRecordESValue()[m_index];
        ExecutionContext* ec = instance->currentExecutionContext();

        if(UNLIKELY(value.isPrimitive())) {
            value = value.toObject();
        }

        ASSERT(value.isESPointer() && value.asESPointer()->isESObject());

        ESObject* obj  = value.asESPointer()->asESObject();
        ec->setLastESObjectMetInMemberExpressionNode(obj);

        ESSlotAccessor slot = obj->findOwnProperty(m_propertyValue);
        if(slot.hasData()) {
            return slot;
        } else {
            return obj->definePropertyOrThrow(m_propertyValue, true, true, true);
        }
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESValue value = instance->currentExecutionContext()->cachedDeclarativeEnvironmentRecordESValue()[m_index];

        if(UNLIKELY(value.isESString())) {
            if(m_propertyValue.isInt32()) {
               int prop_val = m_propertyValue.toInt32();
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
                ESSlotAccessor slot = instance->globalObject()->stringObjectProxy()->find(m_propertyValue, true);
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
            }
        } else if(UNLIKELY(value.isNumber())) {
            instance->globalObject()->numberObjectProxy()->setNumberData(value.asNumber());
            ESSlotAccessor slot = instance->globalObject()->numberObjectProxy()->find(m_propertyValue, true);
            if(slot.isDataProperty()) {
                ESValue ret = slot.value(instance->globalObject()->numberObjectProxy());
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

        if(obj->isHiddenClassMode()) {
            if(m_cachedHiddenClass == obj->hiddenClass()) {
                return obj->readHiddenClass(m_cachedIndex).value(obj);
            } else {
                size_t idx = obj->hiddenClass()->findProperty(m_propertyValue.asESString());
                if(idx != SIZE_MAX) {
                    m_cachedHiddenClass = obj->hiddenClass();
                    m_cachedIndex = idx;
                    return obj->readHiddenClass(idx).value(obj);
                } else {
                    ESSlotAccessor ac = obj->findOnlyPrototype(m_propertyValue.asESString());
                    if(ac.hasData())
                        return ac.value(obj);
                    return ESValue();
                }
            }
        } else {
            return obj->get(m_propertyValue, true);
        }
        RELEASE_ASSERT_NOT_REACHED();
    }
protected:
    ESHiddenClass* m_cachedHiddenClass;
    ESValue m_propertyValue;
    size_t m_cachedIndex;

    size_t m_index;
};

}

#endif
