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

#ifndef CatchClauseNode_h
#define CatchClauseNode_h

#include "Node.h"
#include "ExpressionNode.h"
#include "IdentifierNode.h"
#include "BlockStatementNode.h"

namespace escargot {

// interface CatchClause <: Node {
class CatchClauseNode : public Node {
public:
    friend class ScriptParser;
    CatchClauseNode(Node *param, Node *guard, Node *body)
        : Node(NodeType::CatchClause)
    {
        m_param = (IdentifierNode*) param;
        m_guard = (ExpressionNode*) guard;
        m_body = (BlockStatementNode*) body;
    }

    virtual NodeType type() { return NodeType::CatchClause; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if (!m_body->size()) {
            codeBlock->pushCode(Push(ESValue()), context, this);
            codeBlock->pushCode(PopExpressionStatement(), context, this);
        } else {
            m_body->generateStatementByteCode(codeBlock, context);
        }
    }

    IdentifierNode* param()
    {
        return m_param;
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        m_body->computeRoughCodeBlockSizeInWordSize(result);
    }

protected:
    IdentifierNode* m_param;
    ExpressionNode* m_guard;
    BlockStatementNode* m_body;
};

typedef std::vector<Node *, gc_allocator<CatchClauseNode *>> CatchClauseNodeVector;

}

#endif
