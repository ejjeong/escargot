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

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        CodeBlock* cb = CodeBlock::create();
        cb->m_innerIdentifiers = std::move(m_innerIdentifiers);
        cb->m_needsActivation = m_needsActivation;
        cb->m_needsArgumentsObject = m_needsArgumentsObject;
        cb->m_nonAtomicParams = std::move(m_nonAtomicParams);
        cb->m_params = std::move(m_params);
        ByteCodeGenerateContext newContext;
        m_body->generateStatementByteCode(cb, newContext);
        cb->pushCode(ReturnFunction(), this);

#ifndef NDEBUG
    if(ESVMInstance::currentInstance()->m_dumpByteCode) {
        char* code = cb->m_code.data();
        ByteCode* currentCode = (ByteCode *)(&code[0]);
        if(currentCode->m_orgOpcode != ExecuteNativeFunctionOpcode) {
            dumpBytecode(cb);
        }
    }
#endif
        codeBlock->pushCode(CreateFunction(m_id, m_nonAtomicId, cb), this);
    }

protected:
};

}

#endif
