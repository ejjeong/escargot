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

/*
    ESValue executeExpression(ESVMInstance* instance)
    {
        if(m_argument->type() == NodeType::MemberExpression) {
            MemberExpressionNode* mem = (MemberExpressionNode*)m_argument;
            return doDelete(mem->m_object->executeExpression(instance), mem->m_property->executeExpression(instance));
        } else if(m_argument->type() == NodeType::Identifier) {
            IdentifierNode* id = (IdentifierNode*)m_argument;
            ESSlotAccessor acc = instance->currentExecutionContext()->resolveBinding(id->name(), id->nonAtomicName());
            if(acc.m_targetObject == instance->globalObject()) {
                //TODO consider strict-mode
                instance->globalObject()->deletePropety(id->nonAtomicName());
                return ESValue(!instance->globalObject()->find(id->nonAtomicName()).isEmpty());
            }
        } else {
            m_argument->executeExpression(instance);
        }
        return ESValue(false);
    }
    */

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if (m_argument->type() == NodeType::MemberExpression) {
            MemberExpressionNode* mem = (MemberExpressionNode*) m_argument;
            mem->generateExpressionByteCodeWithoutGetObject(codeBlock, context);
            codeBlock->pushCode(UnaryDelete(), this);
        } else if (m_argument->type() == NodeType::Identifier) {
            // TODO This work with the flag configurable
            codeBlock->pushCode(Push(((IdentifierNode *)m_argument)->nonAtomicName()), this);
        } else {
            RELEASE_ASSERT_NOT_REACHED();
         }
    }

protected:
    Node* m_argument;
};

}

#endif
