#ifndef FunctionDeclarationNode_h
#define FunctionDeclarationNode_h

#include "FunctionNode.h"

namespace escargot {

class FunctionDeclarationNode : public FunctionNode {
public:
    FunctionDeclarationNode(const ESString& id, ESStringVector&& params, Node* body,bool isGenerator, bool isExpression)
            : FunctionNode(NodeType::FunctionDeclaration, id, std::move(params), body, isGenerator, isExpression)
    {
    }

    virtual ESValue* execute(ESVMInstance* instance);
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
