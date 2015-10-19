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
        updateNodeIndex(context);
        WRITE_LAST_INDEX(m_nodeIndex, -1, -1);
    }

protected:
};

}

#endif
