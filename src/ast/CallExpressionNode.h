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

    virtual ESValue execute(ESVMInstance* instance)
    {
        instance->currentExecutionContext()->resetLastESObjectMetInMemberExpressionNode();
        ESValue fn = m_callee->execute(instance);
        ESObject* receiver = instance->currentExecutionContext()->lastESObjectMetInMemberExpressionNode();
        if(receiver == NULL)
            receiver = instance->globalObject();

        ESValue* arguments = (ESValue*)alloca(sizeof(ESValue) * m_arguments.size());
        for(unsigned i = 0; i < m_arguments.size() ; i ++) {
            arguments[i] = m_arguments[i]->execute(instance);
        }

        return ESFunctionObject::call(fn, receiver, arguments, m_arguments.size(), instance);
    }

protected:
    Node* m_callee;//callee: Expression;
    ArgumentVector m_arguments; //arguments: [ Expression ];
};

}

#endif
