#ifndef FunctionExpressionNode_h
#define FunctionExpressionNode_h

#include "FunctionNode.h"

namespace escargot {

class FunctionExpressionNode : public FunctionNode {
public:
    friend class ESScriptParser;
    FunctionExpressionNode(const ESAtomicString& id, ESAtomicStringVector&& params, Node* body,bool isGenerator, bool isExpression)
            : FunctionNode(NodeType::FunctionExpression, id, std::move(params), body, isGenerator, isExpression)
    {
        m_isGenerator = false;
        m_isExpression = false;
    }

    virtual ESValue* execute(ESVMInstance* instance);
protected:
    ExpressionNodeVector m_defaults; //defaults: [ Expression ];
    //rest: Identifier | null;
};

}

#endif
