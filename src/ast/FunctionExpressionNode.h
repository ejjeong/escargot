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
        /*
        ESFunctionObject* function = ESFunctionObject::create(instance->currentExecutionContext()->environment(), NULL);
        //FIXME these lines duplicate with FunctionDeclarationNode::execute
        function->set__proto__(instance->globalObject()->functionPrototype());
        ESObject* prototype = ESObject::create();
        prototype->setConstructor(function);
        prototype->set__proto__(instance->globalObject()->object()->protoType());
        function->setProtoType(prototype);
        function->set(strings->name, m_nonAtomicId);
        /////////////////////////////////////////////////////////////////////

        return function;
        */
        return ESValue();
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock)
    {
        CodeBlock* cb = new CodeBlock();
        cb->m_innerIdentifiers = std::move(m_innerIdentifiers);
        cb->m_needsActivation = m_needsActivation;
        cb->m_needsArgumentsObject = m_needsArgumentsObject;
        cb->m_nonAtomicParams = std::move(m_nonAtomicParams);
        cb->m_params = std::move(m_params);
        m_body->generateStatementByteCode(cb);
        cb->pushCode(ReturnFunction(), this);
        codeBlock->pushCode(CreateFunction(InternalAtomicString(), NULL, cb), this);
    }


protected:
    ExpressionNodeVector m_defaults; //defaults: [ Expression ];
    //rest: Identifier | null;
};

}

#endif
