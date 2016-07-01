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

#ifndef ContinueStatmentNode_h
#define ContinueStatmentNode_h

#include "StatementNode.h"

namespace escargot {

class ContinueStatementNode : public StatementNode {
public:
    friend class ScriptParser;
    ContinueStatementNode()
        : StatementNode(NodeType::ContinueStatement)
    {
    }

    virtual NodeType type() { return NodeType::ContinueStatement; }

    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(Jump(SIZE_MAX), context, this);
        context.pushContinuePositions(codeBlock->lastCodePosition<Jump>());
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 2;
    }
};

}

#endif
