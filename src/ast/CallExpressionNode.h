#ifndef CallExpressionNode_h
#define CallExpressionNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"

namespace escargot {

class CallExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    CallExpressionNode(Node* callee, ArgumentVector&& arguments)
            : ExpressionNode(NodeType::CallExpression)
    {
        m_callee = callee;
        m_arguments = arguments;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        /*
        instance->currentExecutionContext()->resetLastESObjectMetInMemberExpressionNode();
        ESValue fn = m_callee->executeExpression(instance);
        ESObject* receiver = instance->currentExecutionContext()->lastESObjectMetInMemberExpressionNode();
        if(receiver == NULL)
            receiver = instance->globalObject();

        ESValue* arguments = (ESValue*)alloca(sizeof(ESValue) * m_arguments.size());
        for(unsigned i = 0; i < m_arguments.size() ; i ++) {
            arguments[i] = m_arguments[i]->executeExpression(instance);
        }

        return ESFunctionObject::call(instance, fn, receiver, arguments, m_arguments.size(), instance);
        */
        return ESValue();
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(PrepareFunctionCall(), this);
        m_callee->generateExpressionByteCode(codeBlock, context);

        for(unsigned i = 0; i < m_arguments.size() ; i ++) {
            m_arguments[i]->generateExpressionByteCode(codeBlock, context);
        }

        codeBlock->pushCode(Push(ESValue(m_arguments.size())), this);
        codeBlock->pushCode(CallFunction(), this);
    }

protected:
    Node* m_callee;//callee: Expression;
    ArgumentVector m_arguments; //arguments: [ Expression ];
};

}

#endif
