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

#ifndef NDEBUG
class SourceLocation {
public:
    size_t m_lineNumber;
    size_t m_lineStart;
    // TODO
};
#endif

class Node : public gc {
    friend class ScriptParser;
protected:
    Node(NodeType type)
    {
#ifndef NDEBUG
        m_nodeType = type;
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

    virtual void generatePutByteCode(CodeBlock* codeBlock, ByteCodeGenerateContext& context)
    {
        RELEASE_ASSERT_NOT_REACHED();
    }

    virtual void computeRoughCodeBlockSizeInWordSize(size_t& result)
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

#ifndef NDEBUG
    ALWAYS_INLINE void setSourceLocation(size_t lineNum, size_t lineStart)
    {
        m_sourceLocation.m_lineNumber = lineNum;
        m_sourceLocation.m_lineStart = lineStart;
    }
    const SourceLocation& sourceLocation() { return m_sourceLocation; }
#else
    ALWAYS_INLINE void setSourceLocation(size_t lineNum, size_t lineStart)
    {
    }
#endif
    virtual NodeType type() { RELEASE_ASSERT_NOT_REACHED(); }

    /*
    inline void* operator new(size_t size)
    {
        printf("%d\n",(int)size);
        return GC_malloc(size);
    }
    */

    virtual bool isIdentifier()
    {
        return false;
    }

    virtual bool isLiteral()
    {
        return false;
    }

    virtual bool isMemberExpression()
    {
        return false;
    }

    virtual bool isVariableDeclarator()
    {
        return false;
    }

    virtual bool isAssignmentExpressionSimple()
    {
        return false;
    }
protected:
#ifndef NDEBUG
    NodeType m_nodeType;
    SourceLocation m_sourceLocation;
#endif
};

}

#endif
