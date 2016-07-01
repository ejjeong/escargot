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

#ifndef RegExpLiteralNode_h
#define RegExpLiteralNode_h

#include "Node.h"

namespace escargot {

// interface RegExpLiteral <: Node, Expression {
class RegExpLiteralNode : public Node {
public:
    RegExpLiteralNode(ESString* body, escargot::ESRegExpObject::Option flag)
        : Node(NodeType::RegExpLiteral)
    {
        m_body = body;
        m_flag = flag;
    }

    virtual NodeType type() { return NodeType::RegExpLiteral; }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        codeBlock->pushCode(InitRegExpObject(m_body, m_flag), context, this);
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
    {
        result += 3;
    }

    virtual bool isLiteral()
    {
        return true;
    }

    ESString* body() { return m_body; }
    escargot::ESRegExpObject::Option flag() { return m_flag; }

protected:
    ESString* m_body;
    escargot::ESRegExpObject::Option m_flag;
};

}

#endif
