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

#ifndef ESPRIMA_H
#define ESPRIMA_H

#include "Escargot.h"
#include "wtfbridge.h"
#include "ast/StatementNode.h"

namespace escargot {
class Node;
}
namespace esprima {

enum Token {
    BooleanLiteralToken = 1,
    EOFToken = 2,
    IdentifierToken = 3,
    KeywordToken = 4,
    NullLiteralToken = 5,
    NumericLiteralToken = 6,
    PunctuatorToken = 7,
    StringLiteralToken = 8,
    RegularExpressionToken = 9,
    TemplateToken = 10
};

extern const char16_t* TokenName[];
extern const char16_t* PuncuatorsTokens[];

/*
enum Syntax {
    AssignmentExpression,
    AssignmentPattern,
    ArrayExpression,
    ArrayPattern,
    ArrowFunctionExpression,
    BlockStatement,
    BinaryExpression,
    BreakStatement,
    CallExpression,
    CatchClause,
    ClassBody,
    ClassDeclaration,
    ClassExpression,
    ConditionalExpression,
    ContinueStatement,
    DoWhileStatement,
    DebuggerStatement,
    EmptyStatement,
    ExportAllDeclaration,
    ExportDefaultDeclaration,
    ExportNamedDeclaration,
    ExportSpecifier,
    ExpressionStatement,
    ForStatement,
    ForOfStatement,
    ForInStatement,
    FunctionDeclaration,
    FunctionExpression,
    Identifier,
    IfStatement,
    ImportDeclaration,
    ImportDefaultSpecifier,
    ImportNamespaceSpecifier,
    ImportSpecifier,
    Literal,
    LabeledStatement,
    LogicalExpression,
    MemberExpression,
    MetaProperty,
    MethodDefinition,
    NewExpression,
    ObjectExpression,
    ObjectPattern,
    Program,
    Property,
    RestElement,
    ReturnStatement,
    SequenceExpression,
    SpreadElement,
    Super,
    SwitchCase,
    SwitchStatement,
    TaggedTemplateExpression,
    TemplateElement,
    TemplateLiteral,
    ThisExpression,
    ThrowStatement,
    TryStatement,
    UnaryExpression,
    UpdateExpression,
    VariableDeclaration,
    VariableDeclarator,
    WhileStatement,
    WithStatement,
    YieldExpression
};*/

enum PlaceHolders {
    ArrowParameterPlaceHolder
};

enum PunctuatorsKind {
    LeftParenthesis,
    RightParenthesis,
    LeftBrace,
    RightBrace,
    Period,
    PeriodPeriodPeriod,
    Comma,
    Colon,
    SemiColon,
    LeftSquareBracket,
    RightSquareBracket,
    GuessMark,
    Wave,
    UnsignedRightShift,
    RightShift,
    LeftShift,
    Plus,
    Minus,
    Multiply,
    Divide,
    Mod,
    ExclamationMark,
    StrictEqual,
    NotStrictEqual,
    Equal,
    NotEqual,
    LogicalAnd,
    LogicalOr,
    PlusPlus,
    MinusMinus,
    BitwiseAnd,
    BitwiseOr,
    BitwiseXor,
    LeftInequality,
    RightInequality,
    InPunctuator,
    InstanceOfPunctuator,

    Substitution,
    UnsignedRightShiftEqual,
    RightShiftEqual,
    LeftShiftEqual,
    PlusEqual,
    MinusEqual,
    MultiplyEqual,
    DivideEqual,
    ModEqual,
    // ExclamationMarkEqual,
    BitwiseAndEqual,
    BitwiseOrEqual,
    BitwiseXorEqual,
    LeftInequalityEqual,
    RightInequalityEqual,
    SubstitutionEnd,

    Arrow,
    PunctuatorsKindEnd,
};

extern const char16_t* KeywordTokens[];

enum KeywordKind {
    NotKeyword,
    If,
    In,
    Do,
    Var,
    For,
    New,
    Try,
    This,
    Else,
    Case,
    Void,
    With,
    Enum,
    While,
    Break,
    Catch,
    Throw,
    Const,
    Class,
    Super,
    Return,
    Typeof,
    Delete,
    Switch,
    Export,
    Import,
    Default,
    Finally,
    Extends,
    Function,
    Continue,
    Debugger,
    InstanceofKeyword,
    StrictModeReservedWord,
    Implements,
    Interface,
    Package,
    Private,
    Protected,
    Public,
    Static,
    Yield,
    Let,
    KeywordKindEnd
};

// typedef std::basic_string<char16_t, std::char_traits<char16_t>, escargot::ESSimpleAllocatorStd<char16_t> > ParserString;
typedef std::basic_string<char16_t, std::char_traits<char16_t>, std::allocator<char16_t> > ParserStringStd;

class ParserString {
public:
    ParserString()
    {
        m_buffer = NULL;
        m_length = 0;
    }

    ParserString(char16_t ch)
    {
        m_buffer = NULL;
        m_length = 0;
        m_stdString = ch;
    }

    ParserString(const ParserString& ps)
    {
        m_buffer = ps.m_buffer;
        m_length = ps.m_length;
        m_stdString = ps.m_stdString;
        m_isASCIIString = ps.m_isASCIIString;
    }

    void operator =(const ParserString& ps)
    {
        m_buffer = ps.m_buffer;
        m_length = ps.m_length;
        m_stdString = ps.m_stdString;
        m_isASCIIString = ps.m_isASCIIString;
    }

    ParserString(const ParserStringStd& ps)
    {
        m_buffer = 0;
        m_length = 0;
        m_stdString = ps;
    }

    static const size_t npos = static_cast<size_t>(-1);

    void convertIntoStdString()
    {
        if (m_buffer) {
            if (m_isASCIIString) {
                m_stdString.clear();
                m_stdString.shrink_to_fit();
                m_stdString.reserve(m_length);
                for (size_t i = 0; i < m_length; i ++)
                    m_stdString += (char16_t)bufferAsASCIIBuffer()[i];
            } else {
                m_stdString.assign(bufferAsUTF16Buffer(), &bufferAsUTF16Buffer()[m_length]);
            }
            m_buffer = 0;
            m_length = 0;
        } else {

        }
    }

    size_t find(char16_t c)
    {
        if (m_buffer) {
            for (size_t i = 0; i < m_length ; i ++) {
                if (operator[](i) == c) {
                    return i;
                }
            }
            return npos;
        } else {
            return m_stdString.find(c);
        }
    }

    bool operator ==(const char16_t* src) const
    {
        if (m_buffer) {
            for (unsigned i = 0; i < m_length ; i++) {
                char16_t s = src[i];
                if (s != operator[](i)) {
                    return false;
                }
            }
            return src[m_length] == 0;
        } else
            return m_stdString == src;
    }

    bool operator ==(const ParserString& src) const
    {
        if (m_buffer) {
            if (m_length != src.length()) {
                return false;
            }
            for (unsigned i = 0; i < m_length ; i++) {
                char16_t s = src[i];
                if (s != operator[](i)) {
                    return false;
                }
            }
            return true;
        } else {
            // FIXME
            const_cast<ParserString &>(src).convertIntoStdString();
            return m_stdString == src.m_stdString;
        }
    }

    escargot::ESString* toESString()
    {
        if (m_buffer) {
            if (m_isASCIIString) {
                return escargot::ESString::create(std::move(escargot::ASCIIString(bufferAsASCIIBuffer(), m_length)));
            } else {
                return escargot::ESString::createASCIIStringIfNeeded(bufferAsUTF16Buffer(), m_length);
            }
        } else {
            return escargot::ESString::createASCIIStringIfNeeded(std::move(escargot::UTF16String(m_stdString.begin(), m_stdString.end())));
        }
    }

    escargot::InternalAtomicString toInternalAtomicString()
    {
        if (m_buffer) {
            if (m_isASCIIString) {
                return escargot::InternalAtomicString(bufferAsASCIIBuffer(), m_length);
            } else {
                return escargot::InternalAtomicString(bufferAsUTF16Buffer(), m_length);
            }
        } else {
            return escargot::InternalAtomicString(m_stdString.data(), m_stdString.length());
        }
    }

    void operator +=(char16_t src)
    {
        convertIntoStdString();
        m_stdString += src;
    }

    void assign(const char16_t* start, const char16_t* end)
    {
        m_buffer = 0;
        m_length = 0;
        m_stdString.assign(start, end);
    }

    ParserString substr(size_t pos, size_t n)
    {
        convertIntoStdString();
        return ParserString(m_stdString.substr(pos, n));
    }

    size_t length() const
    {
        if (m_buffer)
            return m_length;
        return m_stdString.length();
    }

    char16_t operator[](const size_t& idx) const
    {
        if (m_buffer) {
            if (m_isASCIIString)
                return bufferAsASCIIBuffer()[idx];
            else
                return bufferAsUTF16Buffer()[idx];
        }
        return m_stdString[idx];
    }

    const char* bufferAsASCIIBuffer() const
    {
        ASSERT(m_isASCIIString);
        return (const char*)m_buffer;
    }

    const char16_t* bufferAsUTF16Buffer() const
    {
        ASSERT(!m_isASCIIString);
        return (const char16_t*)m_buffer;
    }

    ParserStringStd m_stdString;
    bool m_isASCIIString;
    const void* m_buffer;
    size_t m_length;
};

struct ParseStatus : public JSC::Yarr::RefCounted<ParseStatus> {
    Token m_type;
    ParserString m_value;
    bool m_octal;
    size_t m_lineNumber;
    size_t m_lineStart;
    size_t m_start;
    size_t m_end;
    int m_prec;

    // ParserString m_value_cooked;
    // ParserString m_value_raw;
    bool m_head;
    bool m_tail;

    double m_valueNumber;

    ParserString m_regexBody;
    ParserString m_regexFlag;

    PunctuatorsKind m_punctuatorsKind;
    KeywordKind m_keywordKind;

    ~ParseStatus()
    {
    }

    ParseStatus()
        : m_octal(false)
        , m_prec(-1)
        , m_head(false)
        , m_tail(false)
        , m_valueNumber(0)
    {
    }

    ParseStatus(Token t, size_t a, size_t b, size_t c, size_t d)
        : m_type(t)
        , m_octal(false)
        , m_lineNumber(a)
        , m_lineStart(b)
        , m_start(c)
        , m_end(d)
        , m_prec(-1)
        , m_head(false)
        , m_tail(false)
        , m_valueNumber(0)
    {
    }

    ParseStatus(Token t, ParserString&& data, size_t a, size_t b, size_t c, size_t d)
    {
        m_valueNumber = 0;

        m_type = t;
        m_value = std::move(data);
        m_lineNumber = a;
        m_lineStart = b;
        m_start = c;
        m_end = d;
        m_head = false;
        m_tail = false;
        m_prec = -1;
        m_octal = false;
    }

    ParseStatus(Token t, ParserString&& data, bool octal, size_t a, size_t b, size_t c, size_t d)
    {
        m_valueNumber = 0;

        m_type = t;
        m_value = std::move(data);
        m_octal = octal;
        m_lineNumber = a;
        m_lineStart = b;
        m_start = c;
        m_end = d;
        m_head = false;
        m_tail = false;
        m_prec = -1;
    }

    void* operator new(size_t, void* p) { return p; }
    void* operator new[](size_t, void* p) { return p; }
    void* operator new(size_t size);
    void operator delete(void* p);
    void* operator new[](size_t size)
    {
        RELEASE_ASSERT_NOT_REACHED();
        return malloc(size);
    }
    void operator delete[](void* p)
    {
        RELEASE_ASSERT_NOT_REACHED();
        return free(p);
    }
};

struct Curly {
    char m_curly[4];
    Curly() { }
    Curly(const char curly[4])
    {
        m_curly[0] = curly[0];
        m_curly[1] = curly[1];
        m_curly[2] = curly[2];
        m_curly[3] = curly[3];
    }
};

struct ParseContext {
    ParseContext(escargot::ESString* src, bool strict)
        : m_sourceString(src)
        , m_index(0)
        , m_lineNumber(src->length() > 0? 1 : 0)
        , m_lineStart(0)
        , m_startIndex(m_index)
        , m_startLineNumber(m_lineNumber)
        , m_startLineStart(m_lineStart)
        , m_length(src->length())
        , m_allowIn(true)
        , m_inFunctionBody(false)
        , m_inIteration(false)
        , m_inSwitch(false)
        , m_inCatch(false)
        , m_lastCommentStart(-1)
        , m_strict(strict)
        , m_scanning(false)
        , m_isBindingElement(false)
        , m_isAssignmentTarget(false)
        , m_isFunctionIdentifier(false)
        , m_firstCoverInitializedNameError(NULL)
        , m_lookahead(nullptr)
        , m_parenthesizedCount(0)
        , m_stackCounter(0)
        , m_currentBody(nullptr)
    {
    }

    escargot::ESString* m_sourceString;
    size_t m_index;
    size_t m_lineNumber;
    size_t m_lineStart;
    size_t m_startIndex;
    size_t m_startLineNumber;
    size_t m_startLineStart;
    size_t m_lastIndex;
    size_t m_lastLineNumber;
    size_t m_lastLineStart;
    size_t m_length;
    bool m_allowIn;
    bool m_allowYield;
    std::vector<std::pair<escargot::ESString *, bool> > m_labelSet;
    bool m_inFunctionBody;
    bool m_inIteration;
    bool m_inSwitch;
    bool m_inCatch;
    int m_lastCommentStart;
    std::vector<Curly> m_curlyStack;
    bool m_strict;
    bool m_scanning;
    bool m_hasLineTerminator;
    bool m_isBindingElement;
    bool m_isAssignmentTarget;
    bool m_isFunctionIdentifier;
    JSC::Yarr::RefPtr<ParseStatus> m_firstCoverInitializedNameError;
    JSC::Yarr::RefPtr<ParseStatus> m_lookahead;
    int m_parenthesizedCount;
    int m_stackCounter;
    escargot::StatementNodeVector* m_currentBody;
};

struct ParserStackChecker {
    ParserStackChecker(ParseContext* ctx);
    ~ParserStackChecker()
    {
        m_ctx->m_stackCounter--;
    }
    void* operator new(std::size_t) = delete;
    ParseContext* m_ctx;
};

// ECMA-262 11.2 White Space

ALWAYS_INLINE bool isWhiteSpace(char16_t ch)
{
    return (ch == 0x20) || (ch == 0x09) || (ch == 0x0B) || (ch == 0x0C) || (ch == 0xA0)
        || (ch >= 0x1680 && (ch == 0x1680 || ch == 0x180E  || ch == 0x2000 || ch == 0x2001
        || ch == 0x2002 || ch == 0x2003 || ch == 0x2004 || ch == 0x2005 || ch == 0x2006
        || ch == 0x2007 || ch == 0x2008 || ch == 0x2009 || ch == 0x200A || ch == 0x202F
        || ch == 0x205F || ch == 0x3000 || ch == 0xFEFF));
}

// ECMA-262 11.3 Line Terminators

ALWAYS_INLINE bool isLineTerminator(char16_t ch)
{
    return (ch == 0x0A) || (ch == 0x0D) || (ch == 0x2028) || (ch == 0x2029);
}

escargot::ProgramNode* parse(escargot::ESString* source, bool strict);
escargot::ProgramNode* parseSingleFunction(escargot::ESString* argSource, escargot::ESString* bodySource);
}

typedef escargot::ESErrorObject::Code ErrorCode;

struct EsprimaError {
    EsprimaError(size_t lineNumber, escargot::ESString* message, escargot::ESErrorObject::Code code = escargot::ESErrorObject::Code::SyntaxError)
        : m_lineNumber(lineNumber), m_message(message), m_code(code) { }

    size_t m_lineNumber;
    escargot::ESString* m_message;
    escargot::ESErrorObject::Code m_code;
};

#endif
