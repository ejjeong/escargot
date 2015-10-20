#ifndef NewExpressionNode_h
#define NewExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class NewExpressionNode : public ExpressionNode {
public:
    friend class ScriptParser;
    NewExpressionNode(Node* callee, ArgumentVector&& arguments)
            : ExpressionNode(NodeType::NewExpression)
    {
        m_callee = callee;
        m_arguments = arguments;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_callee->generateExpressionByteCode(codeBlock, context);
        updateNodeIndex(context);

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
        codeBlock->pushCode(NewFunctionCall(m_arguments.size()), context, this);
        WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
#ifdef ENABLE_ESJIT
        codeBlock->writeFunctionCallInfo(m_callee->nodeIndex(), -1, m_arguments.size(), argumentIndexes);
#endif
    }

protected:
    Node* m_callee;
    ArgumentVector m_arguments;
};

}

#endif
