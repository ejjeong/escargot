#ifndef CallEvalFunctionExpressionNode_h
#define CallEvalFunctionExpressionNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"

namespace escargot {

class CallEvalFunctionExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    CallEvalFunctionExpressionNode(ArgumentVector&& arguments)
            : ExpressionNode(NodeType::CallEvalFunctionExpression)
    {
        m_arguments = arguments;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        /*
        ESValue callee = instance->currentExecutionContext()->resolveBinding(strings->atomicEval, strings->eval).value();
        if(callee.isESPointer() && (void *)callee.asESPointer() == (void *)instance->globalObject()->eval()) {
            ESObject* receiver = instance->globalObject();
            ESValue* arguments = (ESValue*)alloca(sizeof(ESValue) * m_arguments.size());
            for(unsigned i = 0; i < m_arguments.size() ; i ++) {
                arguments[i] = m_arguments[i]->executeExpression(instance);
            }
            ESValue argument = arguments[0];
            if(!argument.isESString()) {
                return argument;
            }
            ESValue ret = instance->runOnEvalContext([instance, &argument](){
                ESValue ret = instance->evaluate(const_cast<u16string &>(argument.asESString()->string()));
                return ret;
            }, true);
            return ret;
        } else {
            ESObject* receiver = instance->globalObject();
            ESValue* arguments = (ESValue*)alloca(sizeof(ESValue) * m_arguments.size());
            for(unsigned i = 0; i < m_arguments.size() ; i ++) {
                arguments[i] = m_arguments[i]->executeExpression(instance);
            }
            return ESFunctionObject::call(instance, callee, receiver, arguments, m_arguments.size(), false);
        }*/
        return ESValue();
    }

protected:
    ArgumentVector m_arguments; //arguments: [ Expression ];
};

}

#endif
