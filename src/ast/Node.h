#ifndef Node_h
#define Node_h

#include "runtime/ESValue.h"

namespace escargot {

class ESVMInstance;
class ExecutionContext;
class JSSlot;

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
    NewExpression,
    MemberExpression,
    CallExpression,
    VariableDeclarator,
    Identifier,
    Literal,
    NativeFunction,
		TryStatement,
		CatchClause,
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
        m_needsActivation = false;
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

    const NodeType& type() { return m_nodeType; }
    bool needsActivation() { return m_needsActivation; } //parent AST has eval, with, catch
protected:
    NodeType m_nodeType;
    SourceLocation m_sourceLocation;
    bool m_needsActivation;
};

}

#endif
