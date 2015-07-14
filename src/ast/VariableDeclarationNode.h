#ifndef VariableDeclarationNode_h
#define VariableDeclarationNode_h

#include "DeclarationNode.h"
#include "VariableDeclaratorNode.h"

namespace escargot {

class VariableDeclarationNode : public DeclarationNode {
public:
    VariableDeclarationNode(VariableDeclaratorVector&& decl)
            : DeclarationNode(VariableDeclaration)
    {
        m_declarations = decl;
    }
    virtual void execute(ESVMInstance* ) { }
protected:
    VariableDeclaratorVector m_declarations; //declarations: [ VariableDeclarator ];
    //kind: "var" | "let" | "const";
};

}

#endif
