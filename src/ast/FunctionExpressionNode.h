#ifndef FunctionExpressionNode_h
#define FunctionExpressionNode_h

#include "FunctionNode.h"

namespace escargot {

class FunctionExpressionNode : public FunctionNode {
public:
    friend class ESScriptParser;
    FunctionExpressionNode(const InternalAtomicString& id, InternalAtomicStringVector&& params, Node* body,bool isGenerator, bool isExpression)
            : FunctionNode(NodeType::FunctionExpression, id, std::move(params), body, isGenerator, isExpression)
    {
        m_isGenerator = false;
        m_isExpression = false;
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        ESFunctionObject* function = ESFunctionObject::create(instance->currentExecutionContext()->environment(), this);
        //FIXME these lines duplicate with FunctionDeclarationNode::execute
        function->set__proto__(instance->globalObject()->functionPrototype());
        ESObject* prototype = ESObject::create();
        prototype->setConstructor(function);
        prototype->set__proto__(instance->globalObject()->object()->protoType());
        function->setProtoType(prototype);
        function->set(strings->name, m_nonAtomicId);
        /////////////////////////////////////////////////////////////////////

        return function;
    }


protected:
    ExpressionNodeVector m_defaults; //defaults: [ Expression ];
    //rest: Identifier | null;
};

}

#endif
