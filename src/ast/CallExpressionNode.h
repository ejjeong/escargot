#ifndef CallExpressionNode_h
#define CallExpressionNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"

namespace escargot {

class CallExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
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
                codeBlock->pushCode(Push(ESValue(m_arguments.size())), this);
                codeBlock->pushCode(CallEvalFunction(), this);
                return ;
            }
        }
        codeBlock->pushCode(PrepareFunctionCall(), this);
        m_callee->generateExpressionByteCode(codeBlock, context);
        updateNodeIndex(context);
        codeBlock->pushCode(PushFunctionCallReceiver(), this);
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

        updateNodeIndex(context);
        codeBlock->pushCode(Push(ESValue(m_arguments.size())), this);
        WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
        updateNodeIndex(context);
        codeBlock->pushCode(CallFunction(), this);
        WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
#ifdef ENABLE_ESJIT
        codeBlock->writeFunctionCallInfo(m_callee->nodeIndex(), receiverIndex, m_arguments.size(), argumentIndexes);
#endif
    }

protected:
    Node* m_callee;//callee: Expression;
    ArgumentVector m_arguments; //arguments: [ Expression ];
};

}

#endif
