#ifndef NewExpressionNode_h
#define NewExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class NewExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    NewExpressionNode(Node* callee, ArgumentVector&& arguments)
            : ExpressionNode(NodeType::NewExpression)
    {
        m_callee = callee;
        m_arguments = arguments;
    }

    ESValue execute(ESVMInstance* instance)
    {
        ESValue fn = m_callee->execute(instance);
        if(!fn.isESPointer() || !fn.asESPointer()->isESFunctionObject())
            throw TypeError(L"NewExpression: constructor is not an function object");
        ESFunctionObject* function = fn.asESPointer()->asESFunctionObject();
        ESObject* receiver;
        if (function == instance->globalObject()->date()) {
            receiver = ESDateObject::create();
        } else if (function == instance->globalObject()->array()) {
            receiver = ESArrayObject::create();
        } else if (function == instance->globalObject()->string()) {
            receiver = ESStringObject::create();
        } else if (function == instance->globalObject()->regexp()) {
            receiver = ESRegExpObject::create(NULL);
        } else {
            receiver = ESObject::create();
        }
        receiver->setConstructor(fn);
        receiver->set__proto__(function->protoType());

        ESValue* arguments = (ESValue*)alloca(sizeof(ESValue) * m_arguments.size());
        for(unsigned i = 0; i < m_arguments.size() ; i ++) {
            arguments[i] = m_arguments[i]->execute(instance);
        }

        ESFunctionObject::call(fn, receiver, arguments, m_arguments.size(), instance, true);
        return receiver;
    }

protected:
    Node* m_callee;
    ArgumentVector m_arguments;
};

}

#endif
