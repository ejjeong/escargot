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

#ifndef LabeledStatementNode_h
#define LabeledStatementNode_h

#include "StatementNode.h"

namespace escargot {

class LabeledStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    LabeledStatementNode(StatementNode* statementNode, ESString* label)
        : StatementNode(NodeType::LabeledStatement)
    {
        m_statementNode = statementNode;
        m_label = label;
    }

    virtual NodeType type() { return NodeType::LabeledStatement; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        size_t start = codeBlock->currentCodeSize();
        context.m_positionToContinue = start;
        size_t basePointerBeforeLabel = context.m_offsetToBasePointer;
        m_statementNode->generateStatementByteCode(codeBlock, context);
        size_t end = codeBlock->currentCodeSize();
        ASSERT(context.m_offsetToBasePointer >= basePointerBeforeLabel);
        codeBlock->pushCode(LoadStackPointer(context.m_offsetToBasePointer - basePointerBeforeLabel), context, this);
        context.consumeLabeledBreakPositions(codeBlock, end, m_label);
        context.consumeLabeledContinuePositions(codeBlock, context.m_positionToContinue, m_label);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 6;
        m_statementNode->computeRoughCodeBlockSizeInWordSize(result);
    }

protected:
    StatementNode* m_statementNode;
    ESString* m_label;
};

}

#endif
