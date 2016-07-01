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

#ifndef UpdateExpressionDecrementPostfixNode_h
#define UpdateExpressionDecrementPostfixNode_h

#include "ExpressionNode.h"

namespace escargot {

class UpdateExpressionDecrementPostfixNode : public ExpressionNode {
public:
    friend class ScriptParser;

    UpdateExpressionDecrementPostfixNode(Node *argument)
        : ExpressionNode(NodeType::UpdateExpressionDecrementPostfix)
    {
        m_argument = (ExpressionNode*)argument;
        m_isSimpleCase = false;
    }

    virtual NodeType type() { return NodeType::UpdateExpressionDecrementPostfix; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if (m_isSimpleCase) {
            m_argument->generateResolveAddressByteCode(codeBlock, context);
            m_argument->generateReferenceResolvedAddressByteCode(codeBlock, context);
            codeBlock->pushCode(ToNumber(), context, this);
            codeBlock->pushCode(Decrement(), context, this);
            m_argument->generatePutByteCode(codeBlock, context);
            return;
        }
        m_argument->generateResolveAddressByteCode(codeBlock, context);
        m_argument->generateReferenceResolvedAddressByteCode(codeBlock, context);
        codeBlock->pushCode(ToNumber(), context, this);
        codeBlock->pushCode(DuplicateTopOfStackValue(), context, this);
        size_t pushPos = codeBlock->currentCodeSize();
        codeBlock->pushCode(PushIntoTempStack(), context, this);
        codeBlock->pushCode(Decrement(), context, this);
        m_argument->generatePutByteCode(codeBlock, context);
        codeBlock->pushCode(Pop(), context, this);
        codeBlock->pushCode(PopFromTempStack(pushPos), context, this);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 7;
        m_argument->computeRoughCodeBlockSizeInWordSize(result);
    }

protected:
    ExpressionNode* m_argument;
    bool m_isSimpleCase;
};

}

#endif
