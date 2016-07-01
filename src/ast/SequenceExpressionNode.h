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

#ifndef SequenceExpressionNode_h
#define SequenceExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

// An sequence expression, i.e., a statement consisting of vector of expressions.
class SequenceExpressionNode : public ExpressionNode {
public:
    friend class ScriptParser;
    SequenceExpressionNode(ExpressionNodeVector&& expressions)
        : ExpressionNode(NodeType::SequenceExpression)
    {
        m_expressions = expressions;
    }

    virtual NodeType type() { return NodeType::SequenceExpression; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        for (unsigned i = 0; i < m_expressions.size(); i++) {
            m_expressions[i]->generateExpressionByteCode(codeBlock, context);
            if (i < m_expressions.size() - 1)
                codeBlock->pushCode(Pop(), context, this);
        }
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += m_expressions.size();
        for (unsigned i = 0; i < m_expressions.size(); i++) {
            m_expressions[i]->computeRoughCodeBlockSizeInWordSize(result);
        }
    }

    const ExpressionNodeVector& expressions() { return m_expressions; }

protected:
    ExpressionNodeVector m_expressions; // expression: Expression;
};

}

#endif
