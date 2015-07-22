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

    virtual ESValue* execute(ESVMInstance* instance)
    {
        for(unsigned i = 0; i < m_declarations.size() ; i ++) {
            m_declarations[i]->execute(instance);
        }
        return esUndefined;
    }
protected:
    VariableDeclaratorVector m_declarations; //declarations: [ VariableDeclarator ];
    //kind: "var" | "let" | "const";
};

}

#endif
