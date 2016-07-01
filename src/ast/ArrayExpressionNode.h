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

#ifndef ArrayExpressionNode_h
#define ArrayExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class ArrayExpressionNode : public ExpressionNode {
public:
    friend class ScriptParser;
    ArrayExpressionNode(ExpressionNodeVector&& elements)
        : ExpressionNode(NodeType::ArrayExpression)
    {
        m_elements = elements;
    }

    virtual NodeType type() { return NodeType::ArrayExpression; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        unsigned len = m_elements.size();
        codeBlock->pushCode(CreateArray(len), context, this);
        for (unsigned i = 0; i < len; i++) {
            codeBlock->pushCode(Push(ESValue(i)), context, this);
            if (m_elements[i]) {
                m_elements[i]->generateExpressionByteCode(codeBlock, context);
            } else {
                codeBlock->pushCode(Push(ESValue(ESValue::ESEmptyValue)), context, this);
            }
            codeBlock->pushCode(InitObject(), context, this);
        }
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += m_elements.size() * 2;
        unsigned len = m_elements.size();
        for (unsigned i = 0; i < len; i++) {
            if (m_elements[i]) {
                m_elements[i]->computeRoughCodeBlockSizeInWordSize(result);
            }
        }
    }
protected:
    ExpressionNodeVector m_elements;
};

}

#endif
