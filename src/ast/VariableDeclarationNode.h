/*
 * Copyright (c) 2016 Samsung Electronics Co., Ltd
 *
 *    Licensed under the Apache License, Version 2.0 (the "License");
 *    you may not use this file except in compliance with the License.
 *    You may obtain a copy of the License at
 *
 *        http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing, software
 *    distributed under the License is distributed on an "AS IS" BASIS,
 *    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *    See the License for the specific language governing permissions and
 *    limitations under the License.
 */

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

    virtual NodeType type() { return NodeType::VariableDeclaration; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        for (unsigned i = 0; i < m_declarations.size() ; i ++) {
            if (m_declarations[i]->isVariableDeclarator()) {
                m_declarations[i]->generateStatementByteCode(codeBlock, context);
            } else if (m_declarations[i]->isAssignmentExpressionSimple()) {
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
            if (m_declarations[i]->isVariableDeclarator()) {
                m_declarations[i]->generateStatementByteCode(codeBlock, context);
            } else if (m_declarations[i]->isAssignmentExpressionSimple()) {
                m_declarations[i]->generateExpressionByteCode(codeBlock, context);
                if (i < m_declarations.size() - 1)
                    codeBlock->pushCode(Pop(), context, this);
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
        }
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        for (unsigned i = 0; i < m_declarations.size() ; i ++) {
            m_declarations[i]->computeRoughCodeBlockSizeInWordSize(result);
        }
    }

    VariableDeclaratorVector& declarations() { return m_declarations; }
protected:
    VariableDeclaratorVector m_declarations; // declarations: [ VariableDeclarator ];
    // kind: "var" | "let" | "const";
};

}

#endif
