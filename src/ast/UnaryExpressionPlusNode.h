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

#ifndef UnaryExpressionPlusNode_h
#define UnaryExpressionPlusNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionPlusNode : public ExpressionNode {
public:
    friend class ScriptParser;
    UnaryExpressionPlusNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionPlus)
    {
        m_argument = argument;
    }

    virtual NodeType type() { return NodeType::UnaryExpressionPlus; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(UnaryPlus(), context, this);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 1;
        m_argument->computeRoughCodeBlockSizeInWordSize(result);
    }
protected:
    Node* m_argument;
};

}

#endif
