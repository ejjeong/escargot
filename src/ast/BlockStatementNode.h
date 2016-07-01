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

#ifndef BlockStatementNode_h
#define BlockStatementNode_h

#include "StatementNode.h"

namespace escargot {

// A block statement, i.e., a sequence of statements surrounded by braces.
class BlockStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    BlockStatementNode(StatementNodeVector&& body)
        : StatementNode(NodeType::BlockStatement)
    {
        m_body = body;
    }

    virtual NodeType type() { return NodeType::BlockStatement; }


    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        for (unsigned i = 0; i < m_body.size() ; ++i) {
            m_body[i]->generateStatementByteCode(codeBlock, context);
        }
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        for (unsigned i = 0; i < m_body.size() ; ++i) {
            m_body[i]->computeRoughCodeBlockSizeInWordSize(result);
        }
    }

    size_t size() { return m_body.size(); }

protected:
    StatementNodeVector m_body; // body: [ Statement ];
};


}

#endif
