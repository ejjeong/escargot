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

#ifndef UnaryExpressionTypeOfNode_h
#define UnaryExpressionTypeOfNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionTypeOfNode : public ExpressionNode {
public:
    friend class ScriptParser;
    UnaryExpressionTypeOfNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionTypeOf)
    {
        m_argument = argument;
    }

    virtual NodeType type() { return NodeType::UnaryExpressionTypeOf; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if (m_argument->isIdentifier() && !((IdentifierNode *)m_argument)->canUseFastAccess()) {
            if (((IdentifierNode *)m_argument)->name() == strings->arguments && !context.m_isGlobalScope && !context.m_hasArgumentsBinding)
                codeBlock->pushCode(GetArgumentsObject(), context, this);
            else
                codeBlock->pushCode(GetByIdWithoutException(
                    ((IdentifierNode *)m_argument)->name(),
                    ((IdentifierNode *)m_argument)->onlySearchGlobal()
                    ), context, this);
        } else
            m_argument->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(UnaryTypeOf(), context, this);
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
