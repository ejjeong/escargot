#ifndef FunctionNode_h
#define FunctionNode_h

#include "Node.h"
#include "PatternNode.h"
#include "ExpressionNode.h"


namespace escargot {

class FunctionNode : public Node {
public:
    FunctionNode()
            : Node(NodeType::Function)
    {
        m_body = NULL;
        m_isGenerator = false;
        m_isExpression = false;
    }
    virtual void execute(ESVMInstance* ) { }
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
