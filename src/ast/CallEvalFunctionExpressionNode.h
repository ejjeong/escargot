#ifndef CallEvalFunctionExpressionNode_h
#define CallEvalFunctionExpressionNode_h

#include "ExpressionNode.h"
#include "PatternNode.h"

namespace escargot {

class CallEvalFunctionExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    CallEvalFunctionExpressionNode(ArgumentVector&& arguments)
            : ExpressionNode(NodeType::CallEvalFunctionExpression)
    {
        m_arguments = arguments;
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        for(unsigned i = 0; i < m_arguments.size() ; i ++) {
            m_arguments[i]->generateExpressionByteCode(codeBlock, context);
        }
        codeBlock->pushCode(Push(ESValue(m_arguments.size())), this);
        codeBlock->pushCode(CallEvalFunction(), this);
    }
protected:
    ArgumentVector m_arguments; //arguments: [ Expression ];
};

}

#endif
