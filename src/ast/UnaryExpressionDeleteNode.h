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

#ifndef UnaryExpressionDeleteNode_h
#define UnaryExpressionDeleteNode_h

#include "ExpressionNode.h"

namespace escargot {

class UnaryExpressionDeleteNode : public ExpressionNode {
public:
    friend class ScriptParser;
    UnaryExpressionDeleteNode(Node* argument)
        : ExpressionNode(NodeType::UnaryExpressionDelete)
    {
        m_argument = (ExpressionNode*) argument;
    }

    virtual NodeType type() { return NodeType::UnaryExpressionDelete; }


    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        if (m_argument->isMemberExpression()) {
            MemberExpressionNode* mem = (MemberExpressionNode*) m_argument;
            mem->generateResolveAddressByteCode(codeBlock, context);
            if (mem->isPreComputedCase()) {
                codeBlock->pushCode(Push(mem->propertyName().string()), context, this);
            }
            codeBlock->pushCode(UnaryDelete(true), context, this);
        } else if (m_argument->isIdentifier()) {
            if (((IdentifierNode *)m_argument)->canUseFastAccess())
                codeBlock->pushCode(Push(ESValue(ESValue::ESFalse)), context, this);
            else
                codeBlock->pushCode(UnaryDelete(false, ((IdentifierNode *)m_argument)->name().string()), context, this);
        } else if (m_argument->isLiteral()) {
            codeBlock->pushCode(Push(ESValue(ESValue::ESTrue)), context, this);
        } else {
            m_argument->generateExpressionByteCode(codeBlock, context);
            codeBlock->pushCode(Pop(), context, this);
            codeBlock->pushCode(Push(ESValue(ESValue::ESTrue)), context, this);
        }
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 1;
        m_argument->computeRoughCodeBlockSizeInWordSize(result);
    }

protected:
    ExpressionNode* m_argument;
};

}

#endif
