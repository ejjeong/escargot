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
        CodeBlock* cb = CodeBlock::create(0);
        if (context.m_shouldGenereateByteCodeInstantly) {
            cb->m_stackAllocatedIdentifiersCount = m_stackAllocatedIdentifiersCount;
            cb->m_heapAllocatedIdentifiers = std::move(m_heapAllocatedIdentifiers);
            cb->m_paramsInformation = std::move(m_paramsInformation);
            cb->m_needsHeapAllocatedExecutionContext = m_needsHeapAllocatedExecutionContext;
            cb->m_needsToPrepareGenerateArgumentsObject = m_needsToPrepareGenerateArgumentsObject;
            cb->m_needsComplexParameterCopy = m_needsComplexParameterCopy;
            // cb->m_params = std::move(m_params);
            // FIXME copy params if needs future
            cb->m_isStrict = m_isStrict;
            cb->m_isFunctionExpression = true;
            cb->m_argumentCount = m_params.size();
            cb->m_hasCode = true;
            cb->m_needsActivation = m_needsActivation;
            cb->m_functionExpressionNameIndex = m_functionIdIndex;
            cb->m_isFunctionExpressionNameHeapAllocated = m_functionIdIndexNeedsHeapAllocation;

            ByteCodeGenerateContext newContext(cb);
            m_body->generateStatementByteCode(cb, newContext);
            cb->pushCode(ReturnFunction(), newContext, this);
#ifndef NDEBUG
            cb->m_nonAtomicId = m_nonAtomicId;
            if (ESVMInstance::currentInstance()->m_reportUnsupportedOpcode) {
                char* code = cb->m_code.data();
                ByteCode* currentCode = (ByteCode *)(&code[0]);
                if (currentCode->m_orgOpcode != ExecuteNativeFunctionOpcode) {
                    dumpUnsupported(cb);
                }
            }
#endif
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
