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

    void executeStatement(ESVMInstance* instance)
    {
        /*
        ESFunctionObject* function = ESFunctionObject::create(instance->currentExecutionContext()->environment(), NULL);
        //FIXME these lines duplicate with FunctionExpressionNode::execute
        function->set__proto__(instance->globalObject()->functionPrototype());
        ESObject* prototype = ESObject::create();
        prototype->setConstructor(function);
        prototype->set__proto__(instance->globalObject()->object()->protoType());
        function->setProtoType(prototype);
        function->set(strings->name, m_nonAtomicId);
        /////////////////////////////////////////////
        if(instance->currentExecutionContext()->needsActivation()) {
            instance->currentExecutionContext()->environment()->record()->createMutableBindingForAST(m_id, nonAtomicId(), false);
        }
        instance->currentExecutionContext()->environment()->record()->setMutableBinding(m_id, nonAtomicId(), function, false);
        */
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        CodeBlock* cb = new CodeBlock();
        cb->m_innerIdentifiers = std::move(m_innerIdentifiers);
        cb->m_needsActivation = m_needsActivation;
        cb->m_needsArgumentsObject = m_needsArgumentsObject;
        cb->m_nonAtomicParams = std::move(m_nonAtomicParams);
        cb->m_params = std::move(m_params);
        ByteCodeGenerateContext newContext;
        m_body->generateStatementByteCode(cb, newContext);
        cb->pushCode(ReturnFunction(), this);
        codeBlock->pushCode(CreateFunction(m_id, m_nonAtomicId, cb), this);
    }

protected:
};

}

#endif
