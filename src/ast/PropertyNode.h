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

    Node* key()
    {
        return m_key;
    }

    Node* value()
    {
        return m_value;
    }

    Kind kind()
    {
        return m_kind;
    }

protected:
    Node* m_key; // key: Literal | Identifier;
    Node* m_value; // value: Expression;
    Kind m_kind; // kind: "init" | "get" | "set";
};

}

#endif


