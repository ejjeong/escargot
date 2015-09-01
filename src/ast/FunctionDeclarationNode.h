#ifndef FunctionDeclarationNode_h
#define FunctionDeclarationNode_h

#include "FunctionNode.h"

namespace escargot {

class FunctionDeclarationNode : public FunctionNode {
public:
    friend class ESScriptParser;
    FunctionDeclarationNode(const InternalAtomicString& id, InternalAtomicStringVector&& params, Node* body, bool isGenerator, bool isExpression, bool isBuiltInFunction = true)
            : FunctionNode(NodeType::FunctionDeclaration, id, std::move(params), body, isGenerator, isExpression, isBuiltInFunction)
    {
    }

    FunctionDeclarationNode(ESString* id, InternalAtomicStringVector&& params, Node* body, bool isGenerator, bool isExpression, bool isBuiltInFunction = true)
            : FunctionNode(NodeType::FunctionDeclaration, id->data(), std::move(params), body, isGenerator, isExpression, isBuiltInFunction)
    {
    }

    ESValue execute(ESVMInstance* instance)
    {
        ESFunctionObject* function = ESFunctionObject::create(instance->currentExecutionContext()->environment(), this);
        //FIXME these lines duplicate with FunctionExpressionNode::execute
        function->set__proto__(instance->globalObject()->functionPrototype());
        ESObject* prototype = ESObject::create();
        prototype->setConstructor(function);
        prototype->set__proto__(instance->globalObject()->object());
        function->setProtoType(prototype);
        function->set(strings->name, m_nonAtomicId);
        /////////////////////////////////////////////
        instance->currentExecutionContext()->environment()->record()->createMutableBindingForAST(m_id, nonAtomicId(), false);
        instance->currentExecutionContext()->environment()->record()->setMutableBinding(m_id, nonAtomicId(), function, false);
        return ESValue();
    }

protected:
};

}

#endif
