#ifndef CallExpressionNode_h
#define CallExpressionNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"
#include "MemberExpressionNode.h"

namespace escargot {

class CallExpressionNode : public ExpressionNode {
public:
    friend class ScriptParser;
    CallExpressionNode(Node* callee, ArgumentVector&& arguments)
        : ExpressionNode(NodeType::CallExpression)
    {
        m_callee = callee;
        m_arguments = arguments;
    }

    virtual NodeType type() { return NodeType::CallExpression; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if (m_callee->isIdentifier()) {
            if (((IdentifierNode *)m_callee)->name() == strings->eval) {
                codeBlock->pushCode(GetById(strings->eval, false), context, this);
                for (unsigned i = 0; i < m_arguments.size(); i ++) {
                    m_arguments[i]->generateExpressionByteCode(codeBlock, context);
                }
                codeBlock->pushCode(CallEvalFunction(m_arguments.size()), context, this);
                return;
            }
        }
        bool prevInCallingExpressionScope = context.m_inCallingExpressionScope;
        if (m_callee->isMemberExpression()) {
            context.m_inCallingExpressionScope = true;
            context.m_isHeadOfMemberExpression = true;
        }
        m_callee->generateExpressionByteCode(codeBlock, context);
        context.m_inCallingExpressionScope = false;

        for (unsigned i = 0; i < m_arguments.size(); i ++) {
            m_arguments[i]->generateExpressionByteCode(codeBlock, context);
        }
        context.m_inCallingExpressionScope = prevInCallingExpressionScope;

        if (!m_callee->isMemberExpression()) {
            codeBlock->pushCode(CallFunction(m_arguments.size()), context, this);
        } else {
            codeBlock->pushCode(CallFunctionWithReceiver(m_arguments.size()), context, this);
        }

    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 2;
        m_callee->computeRoughCodeBlockSizeInWordSize(result);
        for (unsigned i = 0; i < m_arguments.size() ; ++i) {
            m_arguments[i]->computeRoughCodeBlockSizeInWordSize(result);
        }
    }

protected:
    Node* m_callee; // callee: Expression;
    ArgumentVector m_arguments; // arguments: [ Expression ];
};

}

#endif
