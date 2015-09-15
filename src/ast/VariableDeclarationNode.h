#ifndef VariableDeclarationNode_h
#define VariableDeclarationNode_h

#include "DeclarationNode.h"
#include "VariableDeclaratorNode.h"

namespace escargot {

class VariableDeclarationNode : public DeclarationNode {
public:
    friend class ESScriptParser;
    VariableDeclarationNode(VariableDeclaratorVector&& decl)
            : DeclarationNode(VariableDeclaration)
    {
        m_declarations = decl;
    }

    void executeStatement(ESVMInstance* instance)
    {
        for(unsigned i = 0; i < m_declarations.size() ; i ++) {
            m_declarations[i]->executeExpression(instance);
        }
    }

    ESValue executeExpression(ESVMInstance* instance)
    {
        for(unsigned i = 0; i < m_declarations.size() ; i ++) {
            m_declarations[i]->executeExpression(instance);
        }
        return ESValue();
    }

    virtual void generateByteCode(CodeBlock* codeBlock)
    {
        for(unsigned i = 0; i < m_declarations.size() ; i ++) {
            m_declarations[i]->generateByteCode(codeBlock);
            if(m_declarations[i]->type() != NodeType::VariableDeclarator) {
                codeBlock->pushCode(Pop(), this);
            }
        }
    }

    VariableDeclaratorVector& declarations() { return m_declarations; }
protected:
    VariableDeclaratorVector m_declarations; //declarations: [ VariableDeclarator ];
    //kind: "var" | "let" | "const";
};

}

#endif
