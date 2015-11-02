#ifndef VariableDeclarationNode_h
#define VariableDeclarationNode_h

#include "DeclarationNode.h"
#include "VariableDeclaratorNode.h"

namespace escargot {

class VariableDeclarationNode : public DeclarationNode {
public:
    friend class ScriptParser;
    VariableDeclarationNode(VariableDeclaratorVector&& decl)
        : DeclarationNode(VariableDeclaration)
    {
        m_declarations = decl;
    }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        for (unsigned i = 0; i < m_declarations.size() ; i ++) {
            if (m_declarations[i]->type() == NodeType::VariableDeclarator) {
                m_declarations[i]->generateStatementByteCode(codeBlock, context);
            } else if (m_declarations[i]->type() == NodeType::AssignmentExpressionSimple) {
                m_declarations[i]->generateExpressionByteCode(codeBlock, context);
                codeBlock->pushCode(Pop(), context, this);
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        for (unsigned i = 0; i < m_declarations.size() ; i ++) {
            if (m_declarations[i]->type() == NodeType::VariableDeclarator) {
                m_declarations[i]->generateStatementByteCode(codeBlock, context);
            } else if (m_declarations[i]->type() == NodeType::AssignmentExpressionSimple) {
                m_declarations[i]->generateExpressionByteCode(codeBlock, context);
                if (i < m_declarations.size() - 1)
                    codeBlock->pushCode(Pop(), context, this);
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }

    VariableDeclaratorVector& declarations() { return m_declarations; }
protected:
    VariableDeclaratorVector m_declarations; // declarations: [ VariableDeclarator ];
    // kind: "var" | "let" | "const";
};

}

#endif



