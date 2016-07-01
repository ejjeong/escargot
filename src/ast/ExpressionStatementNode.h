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

#ifndef ExpressionStatementNode_h
#define ExpressionStatementNode_h

#include "ExpressionNode.h"

namespace escargot {

// An expression statement, i.e., a statement consisting of a single expression.
class ExpressionStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    ExpressionStatementNode(Node* expression)
        : StatementNode(NodeType::ExpressionStatement)
    {
        m_expression = expression;
    }

    virtual NodeType type() { return NodeType::ExpressionStatement; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_expression->generateExpressionByteCode(codeBlock, context);
        codeBlock->pushCode(PopExpressionStatement(), context, this);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 1;
        m_expression->computeRoughCodeBlockSizeInWordSize(result);
    }

    Node* expression() { return m_expression; }

protected:
    Node* m_expression; // expression: Expression;
};

}

#endif
