#ifndef UnaryExpressionDeleteNode_h
#define UnaryExpressionDeleteNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionDeleteNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    UnaryExpressionDeleteNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionDelete)
    {
        m_argument = argument;
    }

    ALWAYS_INLINE ESValue doDelete(ESValue willBeObj, ESValue key)
    {
        ESObject* obj = willBeObj.toObject();
        obj->deletePropety(key);
        //TODO return proper value
        return ESValue(true);
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        if(m_argument->type() == NodeType::MemberExpression) {
            MemberExpressionNode* mem = (MemberExpressionNode*)m_argument;
            return doDelete(mem->m_object->executeExpression(instance), mem->m_property->executeExpression(instance));
        } else if(m_argument->type() == NodeType::MemberExpressionLeftIdentifierFastCase) {
            MemberExpressionLeftIdentifierFastCaseNode* mem = (MemberExpressionLeftIdentifierFastCaseNode*)m_argument;
            ESValue value = instance->currentExecutionContext()->cachedDeclarativeEnvironmentRecordESValue()[mem->m_index];
            return doDelete(value, mem->m_property->executeExpression(instance));
        } else if(m_argument->type() == NodeType::MemberExpressionNonComputedCase) {
            MemberExpressionNonComputedCaseNode* mem = (MemberExpressionNonComputedCaseNode*)m_argument;
            return doDelete(mem->m_object->executeExpression(instance), mem->m_propertyValue);
        } else if(m_argument->type() == NodeType::MemberExpressionNonComputedCaseLeftIdentifierFastCase) {
            MemberExpressionNonComputedCaseLeftIdentifierFastCaseNode* mem = (MemberExpressionNonComputedCaseLeftIdentifierFastCaseNode*)m_argument;
            ESValue value = instance->currentExecutionContext()->cachedDeclarativeEnvironmentRecordESValue()[mem->m_index];
            return doDelete(value, mem->m_propertyValue);
        } else if(m_argument->type() == NodeType::Identifier) {
            IdentifierNode* id = (IdentifierNode*)m_argument;
            ESSlotAccessor acc = instance->currentExecutionContext()->resolveBinding(id->name(), id->nonAtomicName());
            if(acc.m_data == instance->globalObject()->find(id->nonAtomicName()).m_data) {
                instance->globalObject()->deletePropety(id->nonAtomicName());
                return ESValue(!instance->globalObject()->find(id->nonAtomicName()).hasData());
            }
        } else {
            m_argument->executeExpression(instance);
        }
        return ESValue(false);
    }
protected:
    Node* m_argument;
};

}

#endif
