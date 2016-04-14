#ifndef FunctionExpressionNode_h
#define FunctionExpressionNode_h

#include "FunctionNode.h"

namespace escargot {

class FunctionExpressionNode : public FunctionNode {
public:
    friend class ScriptParser;
    FunctionExpressionNode(const InternalAtomicString& id, InternalAtomicStringVector&& params, Node* body, bool isGenerator, bool isExpression, bool isStrict)
        : FunctionNode(NodeType::FunctionExpression, id, std::move(params), body, isGenerator, isExpression, isStrict)
    {
        m_isGenerator = false;
        m_isExpression = true;
    }

    virtual NodeType type() { return NodeType::FunctionExpression; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        // size_t myResult = 0;
        // m_body->computeRoughCodeBlockSizeInWordSize(myResult);

        // CodeBlock* cb = CodeBlock::create(myResult);
        CodeBlock* cb = CodeBlock::create(CodeBlock::ExecutableType::FunctionCode, 0);
        if (context.m_shouldGenereateByteCodeInstantly) {
            initializeCodeBlock(cb, true);
            ByteCodeGenerateContext newContext(cb, false);
            m_body->generateStatementByteCode(cb, newContext);
            cb->pushCode(ReturnFunction(), newContext, this);
            codeBlock->pushCode(CreateFunction(m_id, m_nonAtomicId, cb, false, -1, false), context, this);
        } else {
            cb->m_ast = this;
            codeBlock->pushCode(CreateFunction(m_id, m_nonAtomicId, cb, false, -1, false), context, this);
        }
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 6;
    }


protected:
    ExpressionNodeVector m_defaults; // defaults: [ Expression ];
    // rest: Identifier | null;
};

}

#endif
