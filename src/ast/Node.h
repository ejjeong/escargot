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
    Empty,
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
    AssignmentExpressionBitwiseAnd,
    AssignmentExpressionBitwiseOr,
    AssignmentExpressionBitwiseXor,
    AssignmentExpressionDivision,
    AssignmentExpressionLeftShift,
    AssignmentExpressionMinus,
    AssignmentExpressionMultiply,
    AssignmentExpressionPlus,
    AssignmentExpressionSignedRightShift,
    AssignmentExpressionUnsignedRightShift,
    AssignmentExpressionSimple,
    AssignmentExpressionSimpleLeftIdentifierFastCase,
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
    MemberExpressionLeftIdentifierFastCase,
    MemberExpressionNonComputedCase,
    MemberExpressionNonComputedCaseLeftIdentifierFastCase,
    ConditionalExpression,
    CallExpression,
    VariableDeclarator,
    Identifier,
    IdentifierFastCase,
    IdentifierFastCaseWithActivation,
    Literal,
    NativeFunction,
    TryStatement,
    CatchClause,
    ThrowStatement,
};

class SourceLocation {
public:
    size_t m_lineNumber;
    size_t m_lineStart;
    //TODO
};

class Node : public gc {
    friend class ESScriptParser;
protected:
    Node(NodeType type)
    {
        m_nodeType = type;
    }
public:
    virtual void executeStatement(ESVMInstance* instance)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual ESValue executeExpression(ESVMInstance* instance)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual ESSlotAccessor executeForWrite(ESVMInstance* instance)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual ~Node()
    {

    }

    ALWAYS_INLINE void setSourceLocation(size_t lineNum, size_t lineStart)
    {
        m_sourceLocation.m_lineNumber = lineNum;
        m_sourceLocation.m_lineStart = lineStart;
    }

    ALWAYS_INLINE const NodeType& type() { return m_nodeType; }
protected:
    NodeType m_nodeType;
    SourceLocation m_sourceLocation;
};

class ControlFlowNode {
public:
    ControlFlowNode()
    {
        m_isSlowCase = false;
    }
    void markAsSlowCase()
    {
        m_isSlowCase = true;
    }
protected:
    bool m_isSlowCase;
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
