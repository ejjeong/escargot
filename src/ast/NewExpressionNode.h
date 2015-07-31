#ifndef NewExpressionNode_h
#define NewExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class NewExpressionNode : public ExpressionNode {
public:
    friend class ESScriptParser;
    NewExpressionNode(Node* callee, ArgumentVector&& arguments)
            : ExpressionNode(NodeType::NewExpression)
    {
        m_callee = callee;
        m_arguments = arguments;
    }

    virtual ESValue execute(ESVMInstance* instance);
protected:
    Node* m_callee;
    ArgumentVector m_arguments;
};

}

#endif
