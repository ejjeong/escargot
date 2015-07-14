#ifndef Node_h
#define Node_h

namespace escargot {

class ESVMInstance;

enum NodeType {
    Program,
    Function,
    Statement,
    EmptyStatement,
    BlockStatement,
    Pattern,
    Expression,
    ExpressionStatement,
    AssignmentExpression,
    VariableDeclarator,
    Identifier,
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
    virtual void execute(ESVMInstance* ) = 0;
    const NodeType& type() { return m_nodeType; }
protected:
    NodeType m_nodeType;
    SourceLocation m_sourceLocation;
};

}

#endif
