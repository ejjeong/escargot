#ifndef FunctionExpressionNode_h
#define FunctionExpressionNode_h

#include "FunctionNode.h"

namespace escargot {

class FunctionExpressionNode : public FunctionNode {
public:
    FunctionExpressionNode(const ESString& id, ESStringVector&& params, Node* body,bool isGenerator, bool isExpression)
            : FunctionNode(NodeType::FunctionExpression, id, std::move(params), body, isGenerator, isExpression)
    {
        m_body = NULL;
        m_isGenerator = false;
        m_isExpression = false;
    }

    virtual ESValue* execute(ESVMInstance* instance);
protected:
    ESString m_id; //id: Identifier | null;
    PatternNodeVector m_params; //params: [ Pattern ];
    ExpressionNodeVector m_defaults; //defaults: [ Expression ];
    //rest: Identifier | null;
    Node* m_body;//body: BlockStatement | Expression;
    bool m_isGenerator;//generator: boolean;
    bool m_isExpression;//expression: boolean;
};

}

#endif
