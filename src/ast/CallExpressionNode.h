#ifndef CallExpressionNode_h
#define CallExpressionNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"

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

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if(m_callee->type() == NodeType::Identifier) {
            if(((IdentifierNode *)m_callee)->name() == InternalAtomicString(u"eval")) {
                for(unsigned i = 0; i < m_arguments.size() ; i ++) {
                    m_arguments[i]->generateExpressionByteCode(codeBlock, context);
                }
                codeBlock->pushCode(CallEvalFunction(m_arguments.size()), context, this);
                return ;
            }
        }
        bool prevInCallingExpressionScope = context.m_inCallingExpressionScope;
        context.m_inCallingExpressionScope = true;
        context.m_isHeadOfMemberExpression = true;
        m_callee->generateExpressionByteCode(codeBlock, context);
        context.m_inCallingExpressionScope = prevInCallingExpressionScope;
        updateNodeIndex(context);
#ifdef ENABLE_ESJIT
        int receiverIndex = m_nodeIndex;
#endif
        WRITE_LAST_INDEX(receiverIndex, -1, -1);

#ifdef ENABLE_ESJIT
        int* argumentIndexes = (int*)alloca(sizeof(int) * m_arguments.size());
#endif
        for(unsigned i = 0; i < m_arguments.size() ; i ++) {
            m_arguments[i]->generateExpressionByteCode(codeBlock, context);
#ifdef ENABLE_ESJIT
            argumentIndexes[i] = m_arguments[i]->nodeIndex();
#endif
        }

        if(!m_callee->isMemberExpresion()) {
            updateNodeIndex(context);
            codeBlock->pushCode(CallFunction(m_arguments.size()), context, this);
            WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
#ifdef ENABLE_ESJIT
            codeBlock->writeFunctionCallInfo(m_callee->nodeIndex(), receiverIndex, m_arguments.size(), argumentIndexes);
#endif
        } else {
            updateNodeIndex(context);
            codeBlock->pushCode(CallFunctionWithReceiver(m_arguments.size()), context, this);
            WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
#ifdef ENABLE_ESJIT
            codeBlock->writeFunctionCallInfo(m_callee->nodeIndex(), receiverIndex, m_arguments.size(), argumentIndexes);
#endif
        }


    }

protected:
    Node* m_callee;//callee: Expression;
    ArgumentVector m_arguments; //arguments: [ Expression ];
};

}

#endif
