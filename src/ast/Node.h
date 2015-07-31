#ifndef Node_h
#define Node_h

#include "runtime/ESValue.h"

namespace escargot {

class ESVMInstance;
class ExecutionContext;
class ESSlot;
class ESScriptParser;

enum NodeType {
    Program,
    Function,
    FunctionExpression,
    Property,
    Statement,
    EmptyStatement,
    BlockStatement,
    ReturnStatement,
    IfStatement,
    ForStatement,
    WhileStatement,
    Declaration,
    VariableDeclaration,
    FunctionDeclaration,
    Pattern,
    Expression,
    ThisExpression,
    ExpressionStatement,
    ArrayExpression,
    AssignmentExpression,
    BinaryExpression,
    UpdateExpression,
    ObjectExpression,
    SequenceExpression,
    NewExpression,
    MemberExpression,
    CallExpression,
    VariableDeclarator,
    Identifier,
    Literal,
    NativeFunction,
    TryStatement,
    CatchClause,
    ThrowStatement,
};

class SourceLocation {
public:
    //TODO
};

class Node : public gc {
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
        return esUndefined;
    }

    virtual ~Node()
    {

    }

    ALWAYS_INLINE const NodeType& type() { return m_nodeType; }
protected:
    NodeType m_nodeType;
    SourceLocation m_sourceLocation;
};

}

#endif
