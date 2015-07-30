#ifndef FunctionNode_h
#define FunctionNode_h

#include "Node.h"
#include "PatternNode.h"
#include "ExpressionNode.h"


namespace escargot {

class FunctionNode : public Node {
public:
    FunctionNode(NodeType type ,const ESAtomicString& id, ESAtomicStringVector&& params, Node* body,bool isGenerator, bool isExpression)
            : Node(type)
    {
        m_id = id;
        m_params = params;
        m_body = body;
        m_isGenerator = isGenerator;
        m_isExpression = isExpression;
        m_needsActivation = true;
        m_outerFunctionNode = NULL;
    }

    ALWAYS_INLINE const ESAtomicStringVector& params() { return m_params; }
    ALWAYS_INLINE Node* body() { return m_body; }
    ALWAYS_INLINE const ESAtomicString& id() { return m_id; }

    ALWAYS_INLINE bool needsActivation() { return m_needsActivation; } //child & parent AST has eval, with, catch
    ALWAYS_INLINE void setNeedsActivation(bool b) { m_needsActivation = b; }

    void setInnerIdentifiers(ESAtomicStringVector&& vec)
    {
        m_innerIdentifiers = vec;
    }

    ESAtomicStringVector& innerIdentifiers() { return m_innerIdentifiers; }

    void setOuterFunctionNode(FunctionNode* o) { m_outerFunctionNode = o; }
    FunctionNode* outerFunctionNode() { return m_outerFunctionNode; }

protected:
    ESAtomicString m_id; //id: Identifier;
    ESAtomicStringVector m_params; //params: [ Pattern ];
    ESAtomicStringVector m_innerIdentifiers;
    //defaults: [ Expression ];
    //rest: Identifier | null;
    Node* m_body; //body: BlockStatement | Expression;
    bool m_isGenerator; //generator: boolean;
    bool m_isExpression; //expression: boolean;

    bool m_needsActivation;
    FunctionNode* m_outerFunctionNode;
};

}

#endif
