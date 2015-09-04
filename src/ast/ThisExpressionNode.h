#ifndef ThisExpressionNode_h
#define ThisExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class ThisExpressionNode : public ExpressionNode {
public:
    ThisExpressionNode()
            : ExpressionNode(NodeType::ThisExpression) { }

    ESValue executeExpression(ESVMInstance* instance)
    {
        return instance->currentExecutionContext()->resolveThisBinding();
    }

protected:
};

}

#endif
