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

#ifndef ContinueLabelStatmentNode_h
#define ContinueLabelStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class ContinueLabelStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    ContinueLabelStatementNode(size_t upIndex, ESString* label)
        : StatementNode(NodeType::ContinueLabelStatement)
    {
        m_upIndex = upIndex;
        m_label = label;
    }

    virtual NodeType type() { return NodeType::ContinueLabelStatement; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(Jump(SIZE_MAX), context, this);
        context.pushLabeledContinuePositions(codeBlock->lastCodePosition<Jump>(), m_label);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 2;
    }

protected:
    size_t m_upIndex;
    ESString* m_label; // for debug
};

}

#endif
