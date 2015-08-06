#ifndef FunctionDeclarationNode_h
#define FunctionDeclarationNode_h

#include "FunctionNode.h"

namespace escargot {

class FunctionDeclarationNode : public FunctionNode {
public:
    friend class ESScriptParser;
    FunctionDeclarationNode(const InternalAtomicString& id, InternalAtomicStringVector&& params, Node* body, bool isGenerator, bool isExpression)
            : FunctionNode(NodeType::FunctionDeclaration, id, std::move(params), body, isGenerator, isExpression)
    {
    }

    FunctionDeclarationNode(const InternalString& id, InternalAtomicStringVector&& params, Node* body, bool isGenerator, bool isExpression)
            : FunctionNode(NodeType::FunctionDeclaration, id.data(), std::move(params), body, isGenerator, isExpression)
    {
    }

    virtual ESValue execute(ESVMInstance* instance);
protected:
};

}

#endif
