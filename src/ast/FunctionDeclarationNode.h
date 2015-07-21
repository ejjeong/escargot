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
};

}

#endif
