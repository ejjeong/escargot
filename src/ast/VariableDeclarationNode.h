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

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenereateContext& context)
    {
        for(unsigned i = 0; i < m_declarations.size() ; i ++) {
            if(m_declarations[i]->type() == NodeType::VariableDeclarator) {
                m_declarations[i]->generateStatementByteCode(codeBlock, context);
            } else if(m_declarations[i]->type() == NodeType::AssignmentExpressionSimple) {
                m_declarations[i]->generateExpressionByteCode(codeBlock, context);
                codeBlock->pushCode(Pop(), this);
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenereateContext& context)
    {
        for(unsigned i = 0; i < m_declarations.size() ; i ++) {
            if(m_declarations[i]->type() == NodeType::VariableDeclarator) {
                RELEASE_ASSERT_NOT_REACHED();
            } else if(m_declarations[i]->type() == NodeType::AssignmentExpressionSimple) {
                m_declarations[i]->generateExpressionByteCode(codeBlock, context);
            } else {
                RELEASE_ASSERT_NOT_REACHED();
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
