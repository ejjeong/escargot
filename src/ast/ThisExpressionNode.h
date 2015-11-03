#ifndef ThisExpressionNode_h
#define ThisExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class ThisExpressionNode : public ExpressionNode {
public:
    ThisExpressionNode()
        : ExpressionNode(NodeType::ThisExpression) { }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(This(), context, this);
    }

protected:
};

}

#endif
