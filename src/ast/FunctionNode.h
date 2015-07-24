#ifndef FunctionNode_h
#define FunctionNode_h

#include "Node.h"
#include "PatternNode.h"
#include "ExpressionNode.h"


namespace escargot {

class FunctionNode : public Node {
public:
    FunctionNode(NodeType type ,const ESString& id, ESStringVector&& params, Node* body,bool isGenerator, bool isExpression)
            : Node(type)
    {
        m_id = id;
        m_params = params;
        m_body = body;
        m_isGenerator = isGenerator;
        m_isExpression = isExpression;
    }

    ALWAYS_INLINE const ESStringVector& params() { return m_params; }
    ALWAYS_INLINE Node* body() { return m_body; }
    ALWAYS_INLINE const ESString& id() { return m_id; }
protected:
    ESString m_id; //id: Identifier;
    ESStringVector m_params; //params: [ Pattern ];
    //defaults: [ Expression ];
    //rest: Identifier | null;
    Node* m_body; //body: BlockStatement | Expression;
    bool m_isGenerator; //generator: boolean;
    bool m_isExpression; //expression: boolean;
};

}

#endif
