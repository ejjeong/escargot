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

        for(unsigned i = 0; i < m_arguments.size() ; i ++) {
            m_arguments[i]->generateExpressionByteCode(codeBlock, context);
        }

        codeBlock->pushCode(NewFunctionCall(m_arguments.size()), context, this);
    }

protected:
    Node* m_callee;
    ArgumentVector m_arguments;
};

}

#endif
