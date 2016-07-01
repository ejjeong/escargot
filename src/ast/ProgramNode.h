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

#ifndef ProgramNode_h
#define ProgramNode_h

#include "Node.h"
#include "StatementNode.h"

namespace escargot {

class ProgramNode : public Node {
public:
    friend class ScriptParser;
    ProgramNode(StatementNodeVector&& body, bool isStrict)
        : Node(NodeType::Program)
    {
        m_body = body;
        m_isStrict = isStrict;
        m_roughCodeblockSizeInWordSize = 0;
    }

    virtual NodeType type() { return NodeType::Program; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        for (unsigned i = 0; i < m_body.size() ; i ++) {
            m_body[i]->generateStatementByteCode(codeBlock, context);
#ifndef NDEBUG
            codeBlock->pushCode(CheckStackPointer(this->m_sourceLocation.m_lineNumber), context, this);
#endif
        }
        codeBlock->pushCode(End(), context, this);
        codeBlock->m_isStrict = m_isStrict;
    }

    StatementNodeVector body()
    {
        return m_body;
    }

    size_t roughCodeblockSizeInWordSize()
    {
        return m_roughCodeblockSizeInWordSize;
    }

    void setRoughCodeblockSizeInWordSize(size_t siz)
    {
        m_roughCodeblockSizeInWordSize = siz;
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        for (unsigned i = 0; i < m_body.size() ; i ++) {
            m_body[i]->computeRoughCodeBlockSizeInWordSize(m_roughCodeblockSizeInWordSize);
        }
    }

    bool isStrict()
    {
        return m_isStrict;
    }

protected:
    StatementNodeVector m_body; // body: [ Statement ];
    bool m_isStrict;
    size_t m_roughCodeblockSizeInWordSize;
};

}

#endif
