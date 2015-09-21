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

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
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
        codeBlock->pushCode(CreateFunction(InternalAtomicString(), NULL, cb), this);
    }


protected:
    ExpressionNodeVector m_defaults; //defaults: [ Expression ];
    //rest: Identifier | null;
};

}

#endif
