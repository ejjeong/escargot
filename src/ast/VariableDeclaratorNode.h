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

#ifndef VariableDeclaratorNode_h
#define VariableDeclaratorNode_h

#include "Node.h"
#include "PatternNode.h"
#include "ExpressionNode.h"
#include "IdentifierNode.h"

namespace escargot {

class VariableDeclaratorNode : public Node {
public:
    friend class ScriptParser;
    VariableDeclaratorNode(Node* id, ExpressionNode* init = NULL, bool isForFunctionDeclaration = false)
        : Node(NodeType::VariableDeclarator)
    {
        m_id = id;
        m_init = init;
        m_flags.m_isGlobalScope = false;
        m_flags.m_isForFunctionDeclaration = isForFunctionDeclaration;
    }

    virtual NodeType type() { return NodeType::VariableDeclarator; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        ASSERT(m_id->isIdentifier());
        ASSERT(m_init == NULL);
        IdentifierNode* id = (IdentifierNode*)m_id;
        if (!id->canUseFastAccess()) {
            if (UNLIKELY(id->name() == strings->arguments && !m_flags.m_isGlobalScope && !m_flags.m_isForFunctionDeclaration)) {
                // do not create dynamic binding
            } else {
                codeBlock->pushCode(CreateBinding(((IdentifierNode *)m_id)->name()), context, this);
            }
        }
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 2;
    }

    Node* id() { return m_id; }
    ExpressionNode* init() { return m_init; }
    void clearInit()
    {
        m_init = NULL;
    }

    virtual bool isVariableDeclarator()
    {
        return true;
    }

    void setIsGlobalScope(bool isGlobalScope)
    {
        m_flags.m_isGlobalScope = isGlobalScope;
    }

    bool isGlobalScope()
    {
        return m_flags.m_isGlobalScope;
    }

    bool isForFunctionDeclaration()
    {
        return m_flags.m_isForFunctionDeclaration;
    }


protected:
    Node* m_id; // id: Pattern;
    ExpressionNode* m_init; // init: Expression | null;
    struct {
        bool m_isGlobalScope:1;
        bool m_isForFunctionDeclaration:1;
    } m_flags;
};


typedef std::vector<Node *, gc_allocator<Node *>> VariableDeclaratorVector;

}

#endif
