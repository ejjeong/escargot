#ifndef FunctionDeclarationNode_h
#define FunctionDeclarationNode_h

#include "FunctionNode.h"

namespace escargot {

class FunctionDeclarationNode : public FunctionNode {
public:
    friend class ScriptParser;
    FunctionDeclarationNode(const InternalAtomicString& id, InternalAtomicStringVector&& params, Node* body, bool isGenerator, bool isExpression, bool isStrict)
        : FunctionNode(NodeType::FunctionDeclaration, id, std::move(params), body, isGenerator, isExpression, isStrict)
    {
        m_isExpression = false;
    }

    virtual NodeType type() { return NodeType::FunctionDeclaration; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        // size_t myResult = 0;
        // m_body->computeRoughCodeBlockSizeInWordSize(myResult);
        // CodeBlock* cb = CodeBlock::create(myResult);
        CodeBlock* cb = generateByteCode(nullptr, this, CodeBlock::ExecutableType::FunctionCode, false, context.m_shouldGenerateByteCodeInstantly);
        codeBlock->pushCode(CreateFunction(m_id, m_nonAtomicId, cb, true, m_functionIdIndex, m_functionIdIndexNeedsHeapAllocation), context, this);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 6;
    }

protected:
};

}

#endif
