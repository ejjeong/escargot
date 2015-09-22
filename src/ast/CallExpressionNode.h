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
        codeBlock->pushCode(PushFunctionCallReceiver(), this);

        for(unsigned i = 0; i < m_arguments.size() ; i ++) {
            m_arguments[i]->generateExpressionByteCode(codeBlock, context);
        }

        codeBlock->pushCode(Push(ESValue(m_arguments.size())), this);
        codeBlock->pushCode(CallFunction(), this);
    }

protected:
    Node* m_callee;//callee: Expression;
    ArgumentVector m_arguments; //arguments: [ Expression ];
};

}

#endif
