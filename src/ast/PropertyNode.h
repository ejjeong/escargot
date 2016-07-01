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

#ifndef PropertyNode_h
#define PropertyNode_h

#include "Node.h"
#include "StatementNode.h"

namespace escargot {

class PropertyNode : public Node {
public:
    friend class ScriptParser;
    enum Kind {
        Init, Get, Set
    };

    PropertyNode(Node* key, Node* value, Kind kind)
        : Node(NodeType::Property)
    {
        m_key = key;
        m_value = value;
        m_kind = kind;
    }

    virtual NodeType type() { return NodeType::Property; }

    Node* key()
    {
        return m_key;
    }

    ESString* keyString()
    {
        if (m_key->isIdentifier()) {
            return ((IdentifierNode*)m_key)->name().string();
        } else {
            ASSERT(m_key->isLiteral());
            return ((LiteralNode*)m_key)->value().toString();
        }
    }

    Node* value()
    {
        return m_value;
    }

    Kind kind()
    {
        return m_kind;
    }

    bool isValidPropertyNode()
    {
        return m_key->isIdentifier() || m_key->isLiteral();
    }

protected:
    Node* m_key; // key: Literal | Identifier;
    Node* m_value; // value: Expression;
    Kind m_kind; // kind: "init" | "get" | "set";
};

}

#endif
