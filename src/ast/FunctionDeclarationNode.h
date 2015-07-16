#ifndef FunctionDeclarationNode_h
#define FunctionDeclarationNode_h

#include "DeclarationNode.h"

namespace escargot {

class FunctionDeclarationNode : public DeclarationNode {
public:
    FunctionDeclarationNode(const ESString& id, ESStringVector&& params, Node* body,bool isGenerator, bool isExpression)
            : DeclarationNode(FunctionDeclaration)
    {
        m_id = id;
        m_params = params;
        m_body = body;
        m_isGenerator = isGenerator;
        m_isExpression = isExpression;
    }

    virtual ESValue* execute(ESVMInstance* instance);
    ALWAYS_INLINE const ESStringVector& params() { return m_params; }
    ALWAYS_INLINE Node* body() { return m_body; }
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
