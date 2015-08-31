#ifndef Node_h
#define Node_h

#include "runtime/ESValue.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"

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
    BreakStatement,
    ContinueStatement,
    ReturnStatement,
    IfStatement,
    ForStatement,
    ForInStatement,
    WhileStatement,
    DoWhileStatement,
    SwitchStatement,
    SwitchCase,
    Declaration,
    VariableDeclaration,
    FunctionDeclaration,
    Pattern,
    Expression,
    ThisExpression,
    ExpressionStatement,
    ArrayExpression,
    UnaryExpressionBitwiseNot,
    UnaryExpressionLogicalNot,
    UnaryExpressionMinus,
    UnaryExpressionPlus,
    UnaryExpressionTypeOf,
    AssignmentExpression,
    AssignmentExpressionSimple,
    BinaryExpression,
    BinaryExpressionBitwiseAnd,
    BinaryExpressionBitwiseOr,
    BinaryExpressionBitwiseXor,
    BinaryExpressionDivison,
    BinaryExpressionEqual,
    BinaryExpressionGreaterThan,
    BinaryExpressionGreaterThanOrEqual,
    BinaryExpressionLeftShift,
    BinaryExpressionLessThan,
    BinaryExpressionLessThanOrEqual,
    BinaryExpressionLogicalAnd,
    BinaryExpressionLogicalOr,
    BinaryExpressionMinus,
    BinaryExpressionMod,
    BinaryExpressionMultiply,
    BinaryExpressionNotEqual,
    BinaryExpressionNotStrictEqual,
    BinaryExpressionPlus,
    BinaryExpressionSignedRightShift,
    BinaryExpressionStrictEqual,
    BinaryExpressionUnsignedRightShift,
    LogicalExpression,
    UpdateExpressionDecrementPostfix,
    UpdateExpressionDecrementPrefix,
    UpdateExpressionIncrementPostfix,
    UpdateExpressionIncrementPrefix,
    ObjectExpression,
    SequenceExpression,
    NewExpression,
    MemberExpression,
    MemberExpressionNonComputedCase,
    ConditionalExpression,
    CallExpression,
    VariableDeclarator,
    Identifier,
    IdentifierFastCase,
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
    virtual ESValue execute(ESVMInstance* instance)
    {
        RELEASE_ASSERT_NOT_REACHED();
        return ESValue();
    }

    virtual ESSlotAccessor executeForWrite(ESVMInstance* instance)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual ~Node()
    {

    }

    ALWAYS_INLINE const NodeType& type() { return m_nodeType; }
protected:
    NodeType m_nodeType;
    SourceLocation m_sourceLocation;
};

class ESSlotWriterForAST {
public:
    ALWAYS_INLINE static void prepareExecuteForWriteASTNode(ExecutionContext* ec)
    {
        ec->resetLastESObjectMetInMemberExpressionNode();
    }

    ALWAYS_INLINE static ESValue readValue(ESSlotAccessor slot, ExecutionContext* ec)
    {
        return slot.value(ec->lastESObjectMetInMemberExpressionNode());
    }

    ALWAYS_INLINE static void setValue(ESSlotAccessor slot, ExecutionContext* ec, ESValue v)
    {
        slot.setValue(v, ec->lastESObjectMetInMemberExpressionNode());
    }
};

}

#endif
