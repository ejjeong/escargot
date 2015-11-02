#ifndef FunctionExpressionNode_h
#define FunctionExpressionNode_h

#include "FunctionNode.h"

namespace escargot {

class FunctionExpressionNode : public FunctionNode {
public:
    friend class ScriptParser;
    FunctionExpressionNode(const InternalAtomicString& id, InternalAtomicStringVector&& params, Node* body,bool isGenerator, bool isExpression, bool isStrict)
        : FunctionNode(NodeType::FunctionExpression, id, std::move(params), body, isGenerator, isExpression, isStrict)
    {
        m_isGenerator = false;
        m_isExpression = true;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        CodeBlock* cb = CodeBlock::create();
        cb->m_innerIdentifiers = std::move(m_innerIdentifiers);
        cb->m_needsActivation = m_needsActivation;
        cb->m_params = std::move(m_params);
        cb->m_isStrict = m_isStrict;
        cb->m_isFunctionExpression = true;

        ByteCodeGenerateContext newContext;
        m_body->generateStatementByteCode(cb, newContext);
#ifdef ENABLE_ESJIT
        cb->m_tempRegisterSize = newContext.m_currentSSARegisterCount;
#endif
        cb->pushCode(ReturnFunction(), newContext, this);
#ifndef NDEBUG
        if(ESVMInstance::currentInstance()->m_dumpByteCode) {
            char* code = cb->m_code.data();
            ByteCode* currentCode = (ByteCode *)(&code[0]);
            if(currentCode->m_orgOpcode != ExecuteNativeFunctionOpcode) {
                if (m_nonAtomicId)
                cb->m_nonAtomicId = m_nonAtomicId;
                dumpBytecode(cb);
            }
        }
        if (ESVMInstance::currentInstance()->m_reportUnsupportedOpcode) {
            char* code = cb->m_code.data();
            ByteCode* currentCode = (ByteCode *)(&code[0]);
            if(currentCode->m_orgOpcode != ExecuteNativeFunctionOpcode) {
                dumpUnsupported(cb);
            }
        }
#endif
        codeBlock->pushCode(CreateFunction(m_id, m_nonAtomicId, cb, false), context, this);
#ifdef ENABLE_ESJIT
        newContext.cleanupSSARegisterCount();
#endif
    }


protected:
    ExpressionNodeVector m_defaults; // defaults: [ Expression ];
    // rest: Identifier | null;
};

}

#endif


