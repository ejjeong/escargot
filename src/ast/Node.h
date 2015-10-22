#ifndef Node_h
#define Node_h

#include "runtime/ESValue.h"
#include "vm/ESVMInstance.h"
#include "runtime/ExecutionContext.h"
#include "runtime/Environment.h"
#include "bytecode/ByteCode.h"

namespace escargot {

class ESVMInstance;
class ExecutionContext;
class ESSlot;
class ScriptParser;

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
    BreakLabelStatement,
    ContinueStatement,
    ContinueLabelStatement,
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
    UnaryExpressionDelete,
    UnaryExpressionLogicalNot,
    UnaryExpressionMinus,
    UnaryExpressionPlus,
    UnaryExpressionTypeOf,
    UnaryExpressionVoid,
    AssignmentExpression,
    AssignmentExpressionBitwiseAnd,
    AssignmentExpressionBitwiseOr,
    AssignmentExpressionBitwiseXor,
    AssignmentExpressionDivision,
    AssignmentExpressionLeftShift,
    AssignmentExpressionMinus,
    AssignmentExpressionMod,
    AssignmentExpressionMultiply,
    AssignmentExpressionPlus,
    AssignmentExpressionSignedRightShift,
    AssignmentExpressionUnsignedRightShift,
    AssignmentExpressionSimple,
    BinaryExpression,
    BinaryExpressionBitwiseAnd,
    BinaryExpressionBitwiseOr,
    BinaryExpressionBitwiseXor,
    BinaryExpressionDivison,
    BinaryExpressionEqual,
    BinaryExpressionGreaterThan,
    BinaryExpressionGreaterThanOrEqual,
    BinaryExpressionIn,
    BinaryExpressionInstanceOf,
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
    ConditionalExpression,
    CallExpression,
    VariableDeclarator,
    Identifier,
    LabeledStatement,
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
    friend class ScriptParser;
protected:
    Node(NodeType type)
    {
        m_nodeType = type;
#ifdef ENABLE_ESJIT
        m_nodeIndex = -1;
#endif
    }
public:
    virtual void generateStatementByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void generateExpressionByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void generateResolveAddressByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void generateReferenceResolvedAddressByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void generatePutByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context, int sourceIndex = -1)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    bool isMemberExpresion()
    {
        return m_nodeType == NodeType::MemberExpression;
    }

    virtual ~Node()
    {

    }

    ALWAYS_INLINE void setSourceLocation(size_t lineNum, size_t lineStart)
    {
        m_sourceLocation.m_lineNumber = lineNum;
        m_sourceLocation.m_lineStart = lineStart;
    }

    const SourceLocation& sourceLocation() { return m_sourceLocation; }

    ALWAYS_INLINE const NodeType& type() { return m_nodeType; }

    void updateNodeIndex(ByteCodeGenerateContext& context)
    {
#ifdef ENABLE_ESJIT
        m_nodeIndex = context.getCurrentNodeIndex();
        context.setCurrentNodeIndex(m_nodeIndex+1);
#endif
    }

#ifdef ENABLE_ESJIT
    int nodeIndex() { return m_nodeIndex; }
#endif
protected:
    NodeType m_nodeType;
    SourceLocation m_sourceLocation;
#ifdef ENABLE_ESJIT
    int m_nodeIndex;
#endif
};

}

#endif
