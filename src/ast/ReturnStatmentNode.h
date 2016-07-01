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

#ifndef ReturnStatmentNode_h
#define ReturnStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class ReturnStatmentNode : public StatementNode {
public:
    friend class ScriptParser;
    ReturnStatmentNode(Node* argument)
        : StatementNode(NodeType::ReturnStatement)
    {
        m_argument = argument;
    }

    virtual NodeType type() { return NodeType::ReturnStatement; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if (m_argument) {
            m_argument->generateExpressionByteCode(codeBlock, context);
            codeBlock->pushCode(ReturnFunctionWithValue(), context, this);
        } else {
            codeBlock->pushCode(ReturnFunction(), context, this);
        }
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 2;
    }
protected:
    Node* m_argument;
};

}

#endif
