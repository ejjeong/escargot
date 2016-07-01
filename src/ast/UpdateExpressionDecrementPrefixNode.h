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

#ifndef UpdateExpressionDecrementPrefixNode_h
#define UpdateExpressionDecrementPrefixNode_h

#include "ExpressionNode.h"

namespace escargot {

class UpdateExpressionDecrementPrefixNode : public ExpressionNode {
public:
    friend class ScriptParser;

    UpdateExpressionDecrementPrefixNode(Node *argument)
        : ExpressionNode(NodeType::UpdateExpressionDecrementPrefix)
    {
        m_argument = (ExpressionNode*)argument;
    }

    virtual NodeType type() { return NodeType::UpdateExpressionDecrementPrefix; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_argument->generateResolveAddressByteCode(codeBlock, context);
        m_argument->generateReferenceResolvedAddressByteCode(codeBlock, context);
        codeBlock->pushCode(ToNumber(), context, this);
        codeBlock->pushCode(Decrement(), context, this);
        m_argument->generatePutByteCode(codeBlock, context);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 2;
        m_argument->computeRoughCodeBlockSizeInWordSize(result);
    }
protected:
    ExpressionNode* m_argument;
};

}

#endif
