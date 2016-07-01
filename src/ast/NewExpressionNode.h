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

#ifndef NewExpressionNode_h
#define NewExpressionNode_h

#include "ExpressionNode.h"

namespace escargot {

class NewExpressionNode : public ExpressionNode {
public:
    friend class ScriptParser;
    NewExpressionNode(Node* callee, ArgumentVector&& arguments)
        : ExpressionNode(NodeType::NewExpression)
    {
        m_callee = callee;
        m_arguments = arguments;
    }

    virtual NodeType type() { return NodeType::NewExpression; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        m_callee->generateExpressionByteCode(codeBlock, context);

        for (unsigned i = 0; i < m_arguments.size() ; i ++) {
            m_arguments[i]->generateExpressionByteCode(codeBlock, context);
        }

        codeBlock->pushCode(NewFunctionCall(m_arguments.size()), context, this);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 3;
        m_callee->computeRoughCodeBlockSizeInWordSize(result);

        for (unsigned i = 0; i < m_arguments.size() ; i ++) {
            m_arguments[i]->computeRoughCodeBlockSizeInWordSize(result);
        }
    }

protected:
    Node* m_callee;
    ArgumentVector m_arguments;
};

}

#endif
