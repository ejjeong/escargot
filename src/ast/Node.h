#ifndef Node_h
#define Node_h

#include "runtime/ESValue.h"

namespace escargot {

class ESVMInstance;

enum NodeType {
    Program,
    Function,
    Statement,
    EmptyStatement,
    BlockStatement,
    Declaration,
    VariableDeclaration,
    FunctionDeclaration,
    Pattern,
    Expression,
    ExpressionStatement,
    AssignmentExpression,
    CallExpression,
    VariableDeclarator,
    Identifier,
    Literal,
    NativeFunction
};

class SourceLocation {
public:
    //TODO
};

class Node : public gc_cleanup {
protected:
    Node(NodeType type, SourceLocation loc = SourceLocation())
    {
        m_nodeType = type;
        m_sourceLocation = loc;
    }
public:
    virtual ESValue* execute(ESVMInstance* instance)
    {
        RELEASE_ASSERT_NOT_REACHED();
        return undefined;
    }

    virtual ~Node()
    {

    }

    const NodeType& type() { return m_nodeType; }
protected:
    NodeType m_nodeType;
    SourceLocation m_sourceLocation;
};

}

#endif
