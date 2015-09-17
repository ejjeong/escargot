#ifndef MemberExpressionNode_h
#define MemberExpressionNode_h

#include "ExpressionNode.h"
#include "PropertyNode.h"
#include "IdentifierNode.h"

namespace escargot {

class MemberExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    friend class UnaryExpressionDeleteNode;
    MemberExpressionNode(Node* object, Node* property, bool computed)
            : ExpressionNode(NodeType::MemberExpression)
    {
        m_object = object;
        m_property = property;
        m_cachedHiddenClass = nullptr;
        m_cachedPropertyValue = nullptr;
        m_computed = computed;
    }

    ESSlotAccessor executeForWrite(ESVMInstance* instance)
    {
        ESValue value = m_object->executeExpression(instance);
        ExecutionContext* ec = instance->currentExecutionContext();

        if(UNLIKELY(!value.isObject())) {
            value = value.toObject();
        }

        ASSERT(value.isESPointer() && value.asESPointer()->isESObject());

        ESObject* obj  = value.asESPointer()->asESObject();
        ec->setLastESObjectMetInMemberExpressionNode(obj);

        ESValue key = m_property->executeExpression(instance);
        return ESSlotAccessor(obj, key);
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        /*
        ESValue value = m_object->executeExpression(instance);
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
                ESValue ret = instance->globalObject()->stringObjectProxy()->find(propertyValue, true);
                if(!ret.isEmpty()) {
                    if(ret.isESPointer() && ret.asESPointer()->isESFunctionObject() && ret.asESPointer()->asESFunctionObject()->functionAST()->isBuiltInFunction()) {
                        instance->currentExecutionContext()->setLastESObjectMetInMemberExpressionNode(instance->globalObject()->stringObjectProxy());
                        return ret;
                    }
                }
            }
        } else if(UNLIKELY(value.isNumber())) {
            instance->globalObject()->numberObjectProxy()->setNumberData(value.asNumber());
            ESValue ret = instance->globalObject()->numberObjectProxy()->find(propertyValue, true);
            if(!ret.isEmpty()) {
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
                return obj->readHiddenClass(m_cachedIndex);
            } else {
                size_t idx = obj->hiddenClass()->findProperty(val);
                if(idx != SIZE_MAX) {
                    m_cachedHiddenClass = obj->hiddenClass();
                    m_cachedPropertyValue = val;
                    m_cachedIndex = idx;
                    return obj->readHiddenClass(idx);
                } else {
                    m_cachedHiddenClass = nullptr;
                    ESValue v = obj->findOnlyPrototype(val);
                    if(v.isEmpty())
                        return ESValue();
                    return v;
                }
            }
        } else {
            return obj->get(propertyValue, true);
        }
        */
        RELEASE_ASSERT_NOT_REACHED();
    }


    virtual void generateExpressionByteCode(CodeBlock* codeBlock)
    {
        m_object->generateExpressionByteCode(codeBlock);
        if(m_computed) {
            m_property->generateExpressionByteCode(codeBlock);
        } else {
            if(m_property->type() == NodeType::Literal)
                codeBlock->pushCode(Push(((LiteralNode *)m_property)->value()), this);
            else {
                ASSERT(m_property->type() == NodeType::Identifier);
                codeBlock->pushCode(Push(((IdentifierNode *)m_property)->nonAtomicName()), this);
            }
        }
        codeBlock->pushCode(GetObject(), this);
    }

    virtual void generateByteCodeWriteCase(CodeBlock* codeBlock)
    {
        m_object->generateExpressionByteCode(codeBlock);
        if(m_computed) {
            m_property->generateExpressionByteCode(codeBlock);
        } else {
            if(m_property->type() == NodeType::Literal)
                codeBlock->pushCode(Push(((LiteralNode *)m_property)->value()), this);
            else {
                ASSERT(m_property->type() == NodeType::Identifier);
                codeBlock->pushCode(Push(((IdentifierNode *)m_property)->nonAtomicName()), this);
            }
        }
        codeBlock->pushCode(ResolveAddressInObject(), this);
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
