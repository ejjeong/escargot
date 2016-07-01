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

#ifndef DoWhileStatementNode_h
#define DoWhileStatementNode_h

#include "StatementNode.h"
#include "ExpressionNode.h"

namespace escargot {

class DoWhileStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    DoWhileStatementNode(Node *test, Node *body)
        : StatementNode(NodeType::DoWhileStatement)
    {
        m_test = (ExpressionNode*) test;
        m_body = (StatementNode*) body;
    }

    virtual NodeType type() { return NodeType::DoWhileStatement; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        ByteCodeGenerateContext newContext(context);
#ifdef ENABLE_ESJIT
        codeBlock->pushCode(LoopStart(), newContext, this);
#endif
        size_t doStart = codeBlock->currentCodeSize();
        m_body->generateStatementByteCode(codeBlock, newContext);

        size_t testPos = codeBlock->currentCodeSize();
        m_test->generateExpressionByteCode(codeBlock, newContext);
        codeBlock->pushCode(JumpIfTopOfStackValueIsTrue(doStart), newContext, this);

        size_t doEnd = codeBlock->currentCodeSize();

        newContext.consumeContinuePositions(codeBlock, testPos);
        newContext.consumeBreakPositions(codeBlock, doEnd);
        newContext.m_positionToContinue = testPos;
        newContext.propagateInformationTo(context);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 3;
        m_test->computeRoughCodeBlockSizeInWordSize(result);
        m_body->computeRoughCodeBlockSizeInWordSize(result);
    }

protected:
    ExpressionNode *m_test;
    StatementNode *m_body;
};

}

#endif
