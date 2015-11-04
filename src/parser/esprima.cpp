#include "Escargot.h"
#include "esprima.h"

#include "ast/AST.h"
#include "wtfbridge.h"

using namespace JSC::Yarr;

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

const char16_t* TokenName[] = {
    u"",
    u"Boolean",
    u"<end>",
    u"Identifier",
    u"Keyword",
    u"Null",
    u"Numeric",
    u"Punctuator",
    u"String",
    u"RegularExpression",
    u"Template",
};

const char16_t* FnExprTokens[] = {
    u"(", u"{", u"[", u"in", u"typeof", u"instanceof", u"new",
    u"return", u"case", u"delete", u"throw", u"void",
    // assignment operators
    u"=", u"+=", u"-=", u"*=", u"/=", u"%=", u"<<=", u">>=", u">>>=",
    u"&=", u"|=", u"^=", u",",
    // binary/unary operators
    u"+", u"-", u"*", u"/", u"%", u"++", u"--", u"<<", u">>", u">>>", u"&",
    u"|", u"^", u"!", u"~", u"&&", u"||", u"?", u":", u"===", u"==", u">=",
    u"<=", u"<", u">", u"!=", u"!=="
};

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
};

enum PlaceHolders {
    ArrowParameterPlaceHolder
};

// TODO handle error

ALWAYS_INLINE bool isDecimalDigit(char16_t ch)
{
    return (ch >= '0' && ch <= '9'); // 0..9
}


ALWAYS_INLINE bool isHexDigit(char16_t ch)
{
    return isDecimalDigit(ch) || (ch >= 'a' && ch <= 'f') || (ch >= 'A' && ch <= 'F');
}

ALWAYS_INLINE bool isOctalDigit(char16_t ch)
{
    return (ch >= '0' && ch < '9'); // 0..9
}



// ECMA-262 11.6 Identifier Names and Identifiers
ALWAYS_INLINE char16_t fromCodePoint(char16_t cp)
{
    if (cp < 0x10000) {
        return cp;
    } else {
        RELEASE_ASSERT_NOT_REACHED();
        // String.fromCharCode(0xD800 + ((cp - 0x10000) >> 10)) +
        // String.fromCharCode(0xDC00 + ((cp - 0x10000) & 1023));
    }
}

ALWAYS_INLINE bool isIdentifierStart(char16_t ch)
{
    // TODO
    return (ch == 36) || (ch == 95) || // $ (dollar) and _ (underscore)
        (ch >= 65 && ch <= 90) || // A..Z
        (ch >= 97 && ch <= 122) || // a..z
        (ch == 92); // \ (backslash)
}

ALWAYS_INLINE bool isIdentifierPart(char16_t ch)
{
    // TODO
    return (ch == 36) || (ch == 95) || // $ (dollar) and _ (underscore)
        (ch >= 65 && ch <= 90) || // A..Z
        (ch >= 97 && ch <= 122) || // a..z
        (ch >= 48 && ch <= 57) || // 0..9
        (ch == 92); // \ (backslash)
}

// ECMA-262 11.6.2.2 Future Reserved Words

ALWAYS_INLINE bool isFutureReservedWord(const std::u16string& id)
{
    if (id == u"enum") {
        return true;
    } else if (id == u"export") {
        return true;
    } else if (id == u"import") {
        return true;
    } else if (id == u"super") {
        return true;
    } else {
        return false;
    }
}

ALWAYS_INLINE bool  isStrictModeReservedWord(const std::u16string& id)
{
    if (id == u"implements") {
        return true;
    } else if (id == u"interface") {
        return true;
    } else if (id == u"package") {
        return true;
    } else if (id == u"private") {
        return true;
    } else if (id == u"protected") {
        return true;
    } else if (id == u"public") {
        return true;
    } else if (id == u"static") {
        return true;
    } else if (id == u"yield") {
        return true;
    } else if (id == u"let") {
        return true;
    }
    return false;
}

ALWAYS_INLINE bool isRestrictedWord(const std::u16string& id)
{
    return id == u"eval" || id == u"arguments";
}


// ECMA-262 11.6.2.1 Keywords

ALWAYS_INLINE bool isKeyword(const std::u16string& id)
{
    // 'const' is specialized as Keyword in V8.
    // 'yield' and 'let' are for compatibility with SpiderMonkey and ES.next.
    // Some others are from future reserved words.

    switch (id.length()) {
    case 2:
        return (id == u"if") || (id == u"in") || (id == u"do");
    case 3:
        return (id == u"var") || (id == u"for") || (id == u"new") ||
            (id == u"try") || (id == u"let");
    case 4:
        return (id == u"this") || (id == u"else") || (id == u"case") ||
            (id == u"void") || (id == u"with") || (id == u"enum");
    case 5:
        return (id == u"while") || (id == u"break") || (id == u"catch") ||
            (id == u"throw") || (id == u"const") || (id == u"yield") ||
            (id == u"class") || (id == u"super");
    case 6:
        return (id == u"return") || (id == u"typeof") || (id == u"delete") ||
            (id == u"switch") || (id == u"export") || (id == u"import");
    case 7:
        return (id == u"default") || (id == u"finally") || (id == u"extends");
    case 8:
        return (id == u"function") || (id == u"continue") || (id == u"debugger");
    case 10:
        return (id == u"instanceof");
    default:
        return false;
    }
}

struct ParseStatus;

ParseStatus* psMalloc();
void psFree(void* p);

struct ParseStatus : public RefCounted<ParseStatus> {
    Token m_type;
    std::u16string m_value;
    bool m_octal;
    size_t m_lineNumber;
    size_t m_lineStart;
    size_t m_start;
    size_t m_end;
    int m_prec;

    // std::u16string m_value_cooked;
    // std::u16string m_value_raw;
    bool m_head;
    bool m_tail;

    double m_valueNumber;

    std::u16string m_regexBody;
    std::u16string m_regexFlag;

    ~ParseStatus()
    {
    }

    ParseStatus()
    {
        m_valueNumber = 0;
        m_head = false;
        m_tail = false;
        m_octal = false;
        m_prec = -1;
    }

    ParseStatus(Token t, size_t a, size_t b, size_t c, size_t d)
    {
        m_valueNumber = 0;

        m_type = t;
        m_lineNumber = a;
        m_lineStart = b;
        m_start = c;
        m_end = d;
        m_head = false;
        m_tail = false;
        m_prec = -1;
        m_octal = false;
    }

    ParseStatus(Token t, std::u16string&& data, size_t a, size_t b, size_t c, size_t d)
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

    ParseStatus(Token t, std::u16string&& data, bool octal, size_t a, size_t b, size_t c, size_t d)
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
    void* operator new(size_t size)
    {
        return psMalloc();
    }
    void operator delete(void* p)
    {
        return psFree(p);
    }
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

bool isPSMallocInited = false;
#define PS_POOL_SIZE 32
ParseStatus* psPool[PS_POOL_SIZE];
size_t psPoolUsage = 0;

ParseStatus* psMalloc()
{
    if (psPoolUsage == 0) {
        return new (malloc(sizeof (ParseStatus)))ParseStatus;
    }
    ParseStatus* ps = psPool[psPoolUsage - 1];
    psPoolUsage--;
    return ps;
}

void psFree(void* p)
{
    if (psPoolUsage < PS_POOL_SIZE) {
        psPool[psPoolUsage++] = (ParseStatus *)p;
    } else
        free(p);
}

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
    ParseContext(const escargot::u16string& src)
        : m_source(src)
    {
    }
    const escargot::u16string& m_source;
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
    std::vector<escargot::ESString *, gc_allocator<escargot::ESString *>> m_labelSet;
    bool m_inFunctionBody;
    bool m_inIteration;
    bool m_inSwitch;
    int m_lastCommentStart;
    std::vector<Curly> m_curlyStack;
    bool m_strict;
    bool m_scanning;
    bool m_hasLineTerminator;
    bool m_isBindingElement;
    bool m_isAssignmentTarget;
    RefPtr<ParseStatus> m_firstCoverInitializedNameError;
    RefPtr<ParseStatus> m_lookahead;
    int m_parenthesizedCount;
    escargot::StatementNodeVector* m_currentBody;
};


void throwUnexpectedToken(/*token, message*/)
{
    // throw unexpectedTokenError(token, message);
    throw u"unexpectedTokenError";
}

void tolerateUnexpectedToken(/*token, message*/)
{
    /*
    var error = unexpectedTokenError(token, message);
    if (extra.errors) {
        recordError(error);
    } else {
        throw error;
    }
     */
    throw u"unexpectedTokenError";
}

void tolerateError(const char16_t* error)
{
    throw error;
}

struct OctalToDecimalResult {
    int code;
    bool octal;
};

OctalToDecimalResult octalToDecimal(ParseContext* ctx, char16_t ch)
{
    // \0 is not octal escape sequence
    bool octal = (ch != '0');
    int code = ch - '0';

    if (ctx->m_index < ctx->m_length && isOctalDigit(ctx->m_source[ctx->m_index])) {
        octal = true;
        code = code * 8 + ctx->m_source[ctx->m_index++] - '0';

        // 3 digits are only allowed when string starts
        // with 0, 1, 2, 3
        if (ch >= '0' && ch <= '3' &&
            ctx->m_index < ctx->m_length &&
            isOctalDigit(ctx->m_source[ctx->m_index])) {
            code = code * 8 + ctx->m_source[ctx->m_index++] - '0';
        }
    }

    OctalToDecimalResult r;
    r.code = code;
    r.octal = octal;

    return r;
}

char16_t scanHexEscape(ParseContext* ctx, char16_t prefix)
{
    int i, len, ch, code = 0;

    len = (prefix == 'u') ? 4 : 2;
    for (i = 0; i < len; ++i) {
        if (ctx->m_index < ctx->m_length && isHexDigit(ctx->m_source[ctx->m_index])) {
            ch = ctx->m_source[ctx->m_index++];
            int c;
            if (ch >= '0' && ch <= '9') {
                c = ch - '0';
            } else if (ch >= 'a' && ch <= 'f') {
                c = ch - 'a' + 10;
            } else if (ch >= 'A' && ch <= 'F') {
                c = ch - 'A' + 10;
            }
            code = code * 16 + c;
        } else {
            throwUnexpectedToken();
        }
    }
    return code;
}


char16_t scanUnicodeCodePointEscape(ParseContext* ctx)
{
    char16_t ch, code;

    ch = ctx->m_source[ctx->m_index];
    code = 0;

    // At least, one hex digit is required.
    if (ch == '}') {
        throwUnexpectedToken();
    }

    while (ctx->m_index < ctx->m_length) {
        ch = ctx->m_source[ctx->m_index++];
        if (!isHexDigit(ch)) {
            break;
        }

        int c;
        if (ch >= '0' && ch <= '9') {
            c = ch - '0';
        } else if (ch >= 'a' && ch <= 'f') {
            c = ch - 'a';
        } else if (ch >= 'A' && ch <= 'F') {
            c = ch - 'A';
        }

        code = code * 16 + c;
    }

    if (code > 0x10FFFF || ch != '}') {
        throwUnexpectedToken();
    }

    return fromCodePoint(code);
}

char16_t codePointAt(ParseContext* ctx, size_t i)
{
    char16_t cp, first, second;

    cp = ctx->m_source[i];
    if (cp >= 0xD800 && cp <= 0xDBFF) {
        second = ctx->m_source[i + 1];
        if (second >= 0xDC00 && second <= 0xDFFF) {
            first = cp;
            cp = (first - 0xD800) * 0x400 + second - 0xDC00 + 0x10000;
        }
    }

    return cp;
}

std::u16string getComplexIdentifier(ParseContext* ctx)
{
    char16_t cp;
    char16_t ch;
    std::u16string id;

    cp = codePointAt(ctx, ctx->m_index);
    id = {fromCodePoint(cp)};
    ctx->m_index += id.length();

    // '\u' (U+005C, U+0075) denotes an escaped character.
    if (cp == 0x5C) {
        if (ctx->m_source[ctx->m_index] != 0x75) {
            throwUnexpectedToken();
        }
        ++ctx->m_index;
        if (ctx->m_source[ctx->m_index] == '{') {
            ++ctx->m_index;
            ch = scanUnicodeCodePointEscape(ctx);
        } else {
            ch = scanHexEscape(ctx, 'u');
            cp = ch;
            if (!ch || ch == '\\' || !isIdentifierStart(cp)) {
                throwUnexpectedToken();
            }
        }
        id = ch;
    }

    while (ctx->m_index < ctx->m_length) {
        cp = codePointAt(ctx, ctx->m_index);
        if (!isIdentifierPart(cp)) {
            break;
        }
        ch = fromCodePoint(cp);
        id += ch;
        // TODO currently, fromCodePoint returns char16_t
        // index += ch.length;
        ctx->m_index += 1;

        // '\u' (U+005C, U+0075) denotes an escaped character.
        if (cp == 0x5C) {
            // CHECKTHIS id.length() - 1 is right?
            id = id.substr(0, id.length() - 1);
            if (ctx->m_source[ctx->m_index] != 0x75) {
                throwUnexpectedToken();
            }
            ++ctx->m_index;
            if (ctx->m_source[ctx->m_index] == '{') {
                ++ctx->m_index;
                ch = scanUnicodeCodePointEscape(ctx);
            } else {
                ch = scanHexEscape(ctx, 'u');
                // cp = ch.charCodeAt(0);
                cp = ch;
                if (!ch || ch == '\\' || !isIdentifierPart(cp)) {
                    throwUnexpectedToken();
                }
            }
            id += ch;
        }
    }

    return id;
}

std::u16string getIdentifier(ParseContext* ctx)
{
    size_t start;
    char16_t ch;

    start = ctx->m_index++;
    while (ctx->m_index < ctx->m_length) {
        ch = ctx->m_source[ctx->m_index];
        if (ch == 0x5C) {
            // Blackslash (U+005C) marks Unicode escape sequence.
            ctx->m_index = start;
            return getComplexIdentifier(ctx);
        } else if (ch >= 0xD800 && ch < 0xDFFF) {
            // Need to handle surrogate pairs.
            ctx->m_index = start;
            return getComplexIdentifier(ctx);
        }
        if (isIdentifierPart(ch)) {
            ++ctx->m_index;
        } else {
            break;
        }
    }

    std::u16string ret;
    ret.reserve(ctx->m_index-start);

    for (size_t i = 0; i < ctx->m_index-start; i ++)
        ret.push_back(ctx->m_source[start + i]);
    // return ctx->m_source.substr(start, ctx->m_index-start);
    return ret;
}

void skipSingleLineComment(ParseContext* ctx, int offset)
{
    size_t start;
    char16_t ch, comment;

    start = ctx->m_index - offset;
    /*
    loc = {
        start: {
            line: lineNumber,
            column: index - lineStart - offset
        }
    };*/

    while (ctx->m_index < ctx->m_length) {
        ch = ctx->m_source[ctx->m_index];
        ++ctx->m_index;
        if (isLineTerminator(ch)) {
            ctx->m_hasLineTerminator = true;
            /*
            if (extra.comments) {
                comment = source.slice(start + offset, index - 1);
                loc.end = {
                    line: lineNumber,
                    column: index - lineStart - 1
                };
            addComment('Line', comment, start, index - 1, loc);
            }*/
            if (ch == 13 && ctx->m_source[ctx->m_index] == 10) {
                ++ctx->m_index;
            }
            ++ctx->m_lineNumber;
            ctx->m_lineStart = ctx->m_index;
            return;
        }
    }
    /*
    if (extra.comments) {
        comment = source.slice(start + offset, index);
        loc.end = {
            line: lineNumber,
            column: index - lineStart
        };
        addComment('Line', comment, start, index, loc);
    }
    */
}

void skipMultiLineComment(ParseContext* ctx)
{
    size_t start;
    // , loc,
    char16_t ch;
    // , comment;
    /*
    if (extra.comments) {
        start = index - 2;
        loc = {
            start: {
                line: lineNumber,
                column: index - lineStart - 2
            }
        };
    }
     */
    while (ctx->m_index < ctx->m_length) {
        ch = ctx->m_source[ctx->m_index];
        if (isLineTerminator(ch)) {
            if (ch == 0x0D && ctx->m_source[ctx->m_index + 1] == 0x0A) {
                ++ctx->m_index;
            }
            ctx->m_hasLineTerminator = true;
            ++ctx->m_lineNumber;
            ++ctx->m_index;
            ctx->m_lineStart = ctx->m_index;
        } else if (ch == 0x2A) {
            // Block comment ends with '*/'.
            if (ctx->m_source[ctx->m_index + 1] == 0x2F) {
                ++ctx->m_index;
                ++ctx->m_index;
                /*
                if (extra.comments) {
                    comment = source.slice(start + 2, index - 2);
                    loc.end = {
                        line: lineNumber,
                        column: index - lineStart
                    };
                    addComment('Block', comment, start, index, loc);
                }
                 */
                return;
            }
            ++ctx->m_index;
        } else {
            ++ctx->m_index;
        }
    }

    // Ran off the end of the file - the whole thing is a comment
    /*
    if (extra.comments) {
        loc.end = {
            line: lineNumber,
            column: index - lineStart
        };
        comment = source.slice(start + 2, index);
        addComment('Block', comment, start, index, loc);
    }
    */
    tolerateUnexpectedToken();
}

void skipComment(ParseContext* ctx)
{
    char16_t ch;
    bool start;
    ctx->m_hasLineTerminator = false;

    start = (ctx->m_index == 0);
    while (ctx->m_index < ctx->m_length) {
        ch = ctx->m_source[ctx->m_index];

        if (isWhiteSpace(ch)) {
            ++ctx->m_index;
        } else if (isLineTerminator(ch)) {
            ctx->m_hasLineTerminator = true;
            ++ctx->m_index;
            if (ch == 0x0D && ctx->m_source[ctx->m_index] == 0x0A) {
                ++ctx->m_index;
            }
            ++ctx->m_lineNumber;
            ctx->m_lineStart = ctx->m_index;
            start = true;
        } else if (ch == 0x2F) { // U+002F is '/'
            ch = ctx->m_source[ctx->m_index + 1];
            if (ch == 0x2F) {
                ++ctx->m_index;
                ++ctx->m_index;
                skipSingleLineComment(ctx, 2);
                start = true;
            } else if (ch == 0x2A) { // U+002A is '*'
                ++ctx->m_index;
                ++ctx->m_index;
                skipMultiLineComment(ctx);
            } else {
                break;
            }
        } else if (start && ch == 0x2D) { // U+002D is '-'
            // U+003E is '>'
            if ((ctx->m_source[ctx->m_index + 1] == 0x2D) && (ctx->m_source[ctx->m_index + 2] == 0x3E)) {
                // '-->' is a single-line comment
                ctx->m_index += 3;
                skipSingleLineComment(ctx, 3);
            } else {
                break;
            }
        } else if (ch == 0x3C) { // U+003C is '<'
            if (ctx->m_source[ctx->m_index + 1] == '!' && ctx->m_source[ctx->m_index + 2] == '-' && ctx->m_source[ctx->m_index + 3] == '-') {
                ++ctx->m_index; // `<`
                ++ctx->m_index; // `!`
                ++ctx->m_index; // `-`
                ++ctx->m_index; // `-`
                skipSingleLineComment(ctx, 4);
            } else {
                break;
            }
        } else {
            break;
        }
    }
}

PassRefPtr<ParseStatus> scanIdentifier(ParseContext* ctx)
{
    size_t start;
    std::u16string id;
    Token type;

    start = ctx->m_index;

    // Backslash (U+005C) starts an escaped character.
    id = (ctx->m_source[ctx->m_index] == 0x5C) ? getComplexIdentifier(ctx) : getIdentifier(ctx);

    // There is no keyword or literal with only one character.
    // Thus, it must be an identifier.
    if (id.length() == 1) {
        type = Token::IdentifierToken;
    } else if (isKeyword(id)) {
        type = Token::KeywordToken;
    } else if (id == u"null") {
        type = Token::NullLiteralToken;
    } else if (id == u"true" || id == u"false") {
        type = Token::BooleanLiteralToken;
    } else {
        type = Token::IdentifierToken;
    }

    ParseStatus* ps = new ParseStatus(type, std::move(id), ctx->m_lineNumber, ctx->m_lineStart, start, ctx->m_index);
    return adoptRef(ps);
}

// ECMA-262 11.7 Punctuators

PassRefPtr<ParseStatus> scanPunctuator(ParseContext* ctx)
{
    ParseStatus* token = new ParseStatus(Token::PunctuatorToken, ctx->m_lineNumber, ctx->m_lineStart, ctx->m_index, ctx->m_index);
    /*
    token = {
        type: Token.Punctuator,
        value: '',
        lineNumber: lineNumber,
        lineStart: lineStart,
        start: index,
        end: index
    };
     */
    // Check for most common single-character punctuators.
    char16_t str = ctx->m_source[ctx->m_index];
    std::u16string resultStr;
    resultStr.reserve(4);
    resultStr += str;
    switch (str) {
    case '(':
        /*
        if (extra.tokenize) {
            extra.openParenToken = extra.tokens.length;
        }
         */
        ++ctx->m_index;
        break;

    case '{':
        /*
        if (extra.tokenize) {
            extra.openCurlyToken = extra.tokens.length;
        }
        */
        ctx->m_curlyStack.push_back(Curly("{\0\0"));
        ++ctx->m_index;
        break;

    case '.':
        ++ctx->m_index;
        if (ctx->m_source[ctx->m_index] == '.' && ctx->m_source[ctx->m_index + 1] == '.') {
            // Spread operator: ...
            ctx->m_index += 2;
            resultStr = u"...";
        }
        break;

    case '}':
        ++ctx->m_index;
        ctx->m_curlyStack.pop_back();
        break;
    case ')':
    case ';':
    case ',':
    case '[':
    case ']':
    case ':':
    case '?':
    case '~':
        ++ctx->m_index;
        break;

    default:
        // 4-character punctuator.
        resultStr.resize(4);
        resultStr[0] = ctx->m_source[ctx->m_index + 0];
        resultStr[1] = ctx->m_source[ctx->m_index + 1];
        resultStr[2] = ctx->m_source[ctx->m_index + 2];
        resultStr[3] = ctx->m_source[ctx->m_index + 3];
        if (resultStr == u">>>=") {
            ctx->m_index += 4;
        } else {
            // 3-character punctuators.

            if (resultStr.compare(0, 3, u"===") == 0 || resultStr.compare(0, 3, u"!==") == 0 || resultStr.compare(0, 3, u">>>") == 0 ||
                resultStr.compare(0, 3, u"<<=") == 0 || resultStr.compare(0, 3, u">>=") == 0) {
                ctx->m_index += 3;
                resultStr.pop_back();
            } else {
                // 2-character punctuators.
                if (resultStr.compare(0, 2, u"&&") == 0 || resultStr.compare(0, 2, u"||") == 0 || resultStr.compare(0, 2, u"==") == 0 || resultStr.compare(0, 2, u"!=") == 0 ||
                    resultStr.compare(0, 2, u"+=") == 0 || resultStr.compare(0, 2, u"-=") == 0 || resultStr.compare(0, 2, u"*=") == 0 || resultStr.compare(0, 2, u"/=") == 0 ||
                    resultStr.compare(0, 2, u"++") == 0 || resultStr.compare(0, 2, u"--") == 0 || resultStr.compare(0, 2, u"<<") == 0 || resultStr.compare(0, 2, u">>") == 0 ||
                    resultStr.compare(0, 2, u"&=") == 0 || resultStr.compare(0, 2, u"|=") == 0 || resultStr.compare(0, 2, u"^=") == 0 || resultStr.compare(0, 2, u"%=") == 0 ||
                    resultStr.compare(0, 2, u"<=") == 0 || resultStr.compare(0, 2, u">=") == 0 || resultStr.compare(0, 2, u"=>") == 0) {
                    ctx->m_index += 2;
                    resultStr.pop_back();
                    resultStr.pop_back();
                } else {
                    // 1-character punctuators.
                    char16_t str = ctx->m_source[ctx->m_index];
                    // if ('<>=!+-*%&|^/'.indexOf(str) >= 0) {
                    if (str == u'<' || str == u'>' || str == u'=' || str == u'!' || str == u'+' || str == u'-'
                        || str == u'*' || str == u'%' || str == u'&' || str == u'|' || str == u'^' || str == u'/') {
                        ++ctx->m_index;
                        resultStr.pop_back();
                        resultStr.pop_back();
                        resultStr.pop_back();
                    }
                }
            }
        }
    }

    if (ctx->m_index == token->m_start) {
        throwUnexpectedToken();
    }

    token->m_end = ctx->m_index;
    token->m_value = std::move(resultStr);
    return adoptRef(token);
}

PassRefPtr<ParseStatus> scanStringLiteral(ParseContext* ctx)
{
    std::u16string str;
    char16_t quote;
    size_t start;
    char16_t ch, unescaped;
    OctalToDecimalResult octToDec;
    bool octal = false;

    const size_t smallBufferMax = 128;
    char16_t smallBuffer[smallBufferMax + 1];
    size_t smallBufferUsage = 0;
    bool strInited = false;

    quote = ctx->m_source[ctx->m_index];
    ASSERT((quote == '\'' || quote == '"'));

    start = ctx->m_index;
    ++ctx->m_index;

    while (ctx->m_index < ctx->m_length) {
        if (smallBufferUsage >= smallBufferMax && !strInited) {
            str.assign(&smallBuffer[0], &smallBuffer[smallBufferUsage]);
            strInited = true;
        }

        ch = ctx->m_source[ctx->m_index++];

        if (ch == quote) {
            quote = '\0';
            break;
        } else if (ch == '\\') {
            ch = ctx->m_source[ctx->m_index++];
            if (!ch || !isLineTerminator(ch)) {
                switch (ch) {
                case 'u':
                case 'x':
                    if (ctx->m_source[ctx->m_index] == '{') {
                        ++ctx->m_index;
                        if (smallBufferUsage < smallBufferMax) {
                            ASSERT(!strInited);
                            smallBuffer[smallBufferUsage++] = scanUnicodeCodePointEscape(ctx);
                        } else
                            str += scanUnicodeCodePointEscape(ctx);
                    } else {
                        unescaped = scanHexEscape(ctx, ch);

                        if (smallBufferUsage < smallBufferMax) {
                            ASSERT(!strInited);
                            smallBuffer[smallBufferUsage++] = unescaped;
                        } else
                            str += unescaped;
                    }
                    break;
                case 'n':
                    if (smallBufferUsage < smallBufferMax) {
                        ASSERT(!strInited);
                        smallBuffer[smallBufferUsage++] = '\n';
                    } else
                        str += '\n';
                    break;
                case 'r':
                    if (smallBufferUsage < smallBufferMax) {
                        ASSERT(!strInited);
                        smallBuffer[smallBufferUsage++] = '\r';
                    } else
                        str += '\r';
                    break;
                case 't':
                    if (smallBufferUsage < smallBufferMax) {
                        ASSERT(!strInited);
                        smallBuffer[smallBufferUsage++] = '\t';
                    } else
                        str += '\t';
                    break;
                case 'b':
                    if (smallBufferUsage < smallBufferMax) {
                        ASSERT(!strInited);
                        smallBuffer[smallBufferUsage++] = '\b';
                    } else
                        str += '\b';
                    break;
                case 'f':
                    if (smallBufferUsage < smallBufferMax) {
                        ASSERT(!strInited);
                        smallBuffer[smallBufferUsage++] = '\f';
                    } else
                        str += '\f';
                    break;
                case 'v':
                    if (smallBufferUsage < smallBufferMax) {
                        ASSERT(!strInited);
                        smallBuffer[smallBufferUsage++] = '\x0B';
                    } else
                        str += '\x0B';
                    break;
                case '8':
                case '9':
                    // str += ch;
                    tolerateUnexpectedToken();
                    break;

                default:
                    if (isOctalDigit(ch)) {
                        bool r;
                        size_t c = 0;
                        size_t l = 1;
                        octToDec = octalToDecimal(ctx, ch);
                        octal = octToDec.octal || octal;
                        if (smallBufferUsage < smallBufferMax) {
                            ASSERT(!strInited);
                            smallBuffer[smallBufferUsage++] = octToDec.code;
                        } else
                            str += octToDec.code;
                    } else {
                        if (smallBufferUsage < smallBufferMax) {
                            ASSERT(!strInited);
                            smallBuffer[smallBufferUsage++] = ch;
                        } else
                            str += ch;
                    }
                    break;
                }
            } else {
                ++ctx->m_lineNumber;
                if (ch == '\r' && ctx->m_source[ctx->m_index] == '\n') {
                    ++ctx->m_index;
                }
                ctx->m_lineStart = ctx->m_index;
            }
        } else if (isLineTerminator(ch)) {
            break;
        } else {
            if (smallBufferUsage < smallBufferMax) {
                ASSERT(!strInited);
                smallBuffer[smallBufferUsage++] = ch;
            } else
                str += ch;
        }
    }

    if (!strInited) {
        str.assign(&smallBuffer[0], &smallBuffer[smallBufferUsage]);
    }
    if (quote != '\0') {
        throwUnexpectedToken();
    }

    ParseStatus* ps =  new ParseStatus(
        Token::StringLiteralToken,
        std::move(str),
        octal,
        ctx->m_startLineNumber,
        ctx->m_startLineStart,
        start,
        ctx->m_index
    );
    return adoptRef(ps);
}

// ECMA-262 11.8.6 Template Literal Lexical Components
PassRefPtr<ParseStatus> scanTemplate(ParseContext* ctx)
{
    // var cooked = '', ch, start, rawOffset, terminated, head, tail, restore, unescaped;
    char16_t ch, unescaped;
    bool terminated = false;
    bool tail = false;
    size_t start = ctx->m_index;
    bool head = (ctx->m_source[ctx->m_index] == '`');
    size_t rawOffset = 2;
    size_t restore;
    std::u16string cooked;

    ++ctx->m_index;

    while (ctx->m_index < ctx->m_length) {
        ch = ctx->m_source[ctx->m_index++];
        if (ch == '`') {
            rawOffset = 1;
            tail = true;
            terminated = true;
            break;
        } else if (ch == '$') {
            if (ctx->m_source[ctx->m_index] == '{') {
                ctx->m_curlyStack.push_back(Curly("${\0"));
                ++ctx->m_index;
                terminated = true;
                break;
            }
            cooked += ch;
        } else if (ch == '\\') {
            ch = ctx->m_source[ctx->m_index++];
            if (!isLineTerminator(ch)) {
                switch (ch) {
                case 'n':
                    cooked += '\n';
                    break;
                case 'r':
                    cooked += '\r';
                    break;
                case 't':
                    cooked += '\t';
                    break;
                case 'u':
                case 'x':
                    if (ctx->m_source[ctx->m_index] == '{') {
                        ++ctx->m_index;
                        cooked += scanUnicodeCodePointEscape(ctx);
                    } else {
                        restore = ctx->m_index;
                        unescaped = scanHexEscape(ctx, ch);
                        if (unescaped) {
                            cooked += unescaped;
                        } else {
                            ctx->m_index = restore;
                            cooked += ch;
                        }
                    }
                    break;
                case 'b':
                    cooked += '\b';
                    break;
                case 'f':
                    cooked += '\f';
                    break;
                case 'v':
                    cooked += '\v';
                    break;

                default:
                    if (ch == '0') {
                        if (isDecimalDigit(ctx->m_source[ctx->m_index])) {
                            // Illegal: \01 \02 and so on
                            throw u"TemplateOctalLiteral";
                            // throwError(Messages.TemplateOctalLiteral);
                        }
                        cooked.push_back('\0');
                    } else if (isOctalDigit(ch)) {
                        // Illegal: \1 \2
                        // throwError(Messages.TemplateOctalLiteral);
                        throw u"TemplateOctalLiteral";
                    } else {
                        cooked += ch;
                    }
                    break;
                }
            } else {
                ++ctx->m_lineNumber;
                if (ch == '\r' && ctx->m_source[ctx->m_index] == '\n') {
                    ++ctx->m_index;
                }
                ctx->m_lineStart = ctx->m_index;
            }
        } else if (isLineTerminator(ch)) {
            ++ctx->m_lineNumber;
            if (ch == '\r' && ctx->m_source[ctx->m_index] == '\n') {
                ++ctx->m_index;
            }
            ctx->m_lineStart = ctx->m_index;
            cooked += '\n';
        } else {
            cooked += ch;
        }
    }

    if (!terminated) {
        throwUnexpectedToken();
    }

    if (!head) {
        ctx->m_curlyStack.pop_back();
    }

    ParseStatus* status;
    status = new ParseStatus();
    status->m_type = Token::TemplateToken;
    status->m_value = std::move(cooked);
    // status->m_value_cooked = cooked;
    // status->m_value_raw = ctx->m_source.substr(start + 1, ctx->m_index - rawOffset - start + 1);
    status->m_head = head;
    status->m_tail = tail;
    status->m_lineNumber = ctx->m_lineNumber;
    status->m_lineStart = ctx->m_lineStart;
    status->m_start = start;
    status->m_end = ctx->m_index;
    return adoptRef(status);
    /*
    return {
        type: Token.Template,
        value: {
            cooked: cooked,
            raw: source.slice(start + 1, index - rawOffset)
        },
        head: head,
        tail: tail,
        lineNumber: lineNumber,
        lineStart: lineStart,
        start: start,
        end: index
    };
    */
}

PassRefPtr<ParseStatus> scanHexLiteral(ParseContext* ctx, size_t start)
{
    std::wstring number;

    while (ctx->m_index < ctx->m_length) {
        if (!isHexDigit(ctx->m_source[ctx->m_index])) {
            break;
        }
        number += ctx->m_source[ctx->m_index++];
    }

    if (number.length() == 0) {
        throwUnexpectedToken();
    }

    if (isIdentifierStart(ctx->m_source[ctx->m_index])) {
        throwUnexpectedToken();
    }

    long int ll = wcstol(number.data(), NULL, 16);
    ParseStatus* ps = new ParseStatus;
    ps->m_type = Token::NumericLiteralToken;
    // ps->m_value = number.data();
    ps->m_valueNumber = ll;
    ps->m_lineNumber = ctx->m_lineNumber;
    ps->m_lineStart = ctx->m_lineStart;
    ps->m_start = start;
    ps->m_end = ctx->m_index;
    /*
    return {
        type: Token.NumericLiteral,
        value: parseInt('0x' + number, 16),
        lineNumber: lineNumber,
        lineStart: lineStart,
        start: start,
        end: index
    };
     */
    return adoptRef(ps);
}

PassRefPtr<ParseStatus> scanBinaryLiteral(ParseContext* ctx, size_t start)
{
    char16_t ch;
    std::wstring number;

    while (ctx->m_index < ctx->m_length) {
        ch = ctx->m_source[ctx->m_index];
        if (ch != '0' && ch != '1') {
            break;
        }
        number += ctx->m_source[ctx->m_index++];
    }

    if (number.length() == 0) {
        // only 0b or 0B
        throwUnexpectedToken();
    }

    if (ctx->m_index < ctx->m_length) {
        ch = ctx->m_source[ctx->m_index];
        /* istanbul ignore else */
        if (isIdentifierStart(ch) || isDecimalDigit(ch)) {
            throwUnexpectedToken();
        }
    }

    long int ll = wcstol(number.data(), NULL, 2);

    ParseStatus* ps = new ParseStatus;
    ps->m_type = Token::NumericLiteralToken;
    // ps->m_value = number;
    ps->m_valueNumber = ll;
    ps->m_lineNumber = ctx->m_lineNumber;
    ps->m_lineStart = ctx->m_lineStart;
    ps->m_start = start;
    ps->m_end = ctx->m_index;
    return adoptRef(ps);
    /*
    return {
        type: Token.NumericLiteral,
        value: parseInt(number, 2),
        lineNumber: lineNumber,
        lineStart: lineStart,
        start: start,
        end: index
    };
    */
}

PassRefPtr<ParseStatus> scanOctalLiteral(ParseContext* ctx, char16_t prefix, size_t start)
{
    std::wstring number;
    bool octal;

    if (isOctalDigit(prefix)) {
        octal = true;
        number = '0' + ctx->m_source[ctx->m_index++];
    } else {
        octal = false;
        ++ctx->m_index;
        // number = '';
    }

    while (ctx->m_index < ctx->m_length) {
        if (!isOctalDigit(ctx->m_source[ctx->m_index])) {
            break;
        }
        number += ctx->m_source[ctx->m_index++];
    }

    if (!octal && number.length() == 0) {
        // only 0o or 0O
        throwUnexpectedToken();
    }

    if (isIdentifierStart(ctx->m_source[ctx->m_index]) || isDecimalDigit(ctx->m_source[ctx->m_index])) {
        throwUnexpectedToken();
    }

    long int ll = wcstol(number.data(), NULL, 8);

    ParseStatus* ps = new ParseStatus;
    ps->m_type = Token::NumericLiteralToken;
    // ps->m_value = number;
    ps->m_valueNumber = ll;
    ps->m_lineNumber = ctx->m_lineNumber;
    ps->m_lineStart = ctx->m_lineStart;
    ps->m_start = start;
    ps->m_end = ctx->m_index;
    return adoptRef(ps);
    /*
    return {
        type: Token.NumericLiteral,
        value: parseInt(number, 8),
        octal: octal,
        lineNumber: lineNumber,
        lineStart: lineStart,
        start: start,
        end: index
    };
     */
}

bool isImplicitOctalLiteral(ParseContext* ctx)
{
    size_t i;
    char16_t ch;

    // Implicit octal, unless there is a non-octal digit.
    // (Annex B.1.1 on Numeric Literals)
    for (size_t i = ctx->m_index + 1; i < ctx->m_length; ++i) {
        ch = ctx->m_source[i];
        if (ch == '8' || ch == '9') {
            return false;
        }
        if (!isOctalDigit(ch)) {
            return true;
        }
    }

    return true;
}

PassRefPtr<ParseStatus> scanNumericLiteral(ParseContext* ctx)
{
    std::wstring number;
    number.reserve(32);
    size_t start;
    char16_t ch;

    ch = ctx->m_source[ctx->m_index];
    ASSERT(isDecimalDigit(ch) || (ch == '.'));

    start = ctx->m_index;

    if (ch != '.') {
        number = ctx->m_source[ctx->m_index++];
        ch = ctx->m_source[ctx->m_index];

        // Hex number starts with '0x'.
        // Octal number starts with '0'.
        // Octal number in ES6 starts with '0o'.
        // Binary number in ES6 starts with '0b'.
        if (number[0] == '0') {
            if (ch == 'x' || ch == 'X') {
                ++ctx->m_index;
                return scanHexLiteral(ctx, start);
            }
            if (ch == 'b' || ch == 'B') {
                ++ctx->m_index;
                return scanBinaryLiteral(ctx, start);
            }
            if (ch == 'o' || ch == 'O') {
                return scanOctalLiteral(ctx, ch, start);
            }

            if (isOctalDigit(ch)) {
                if (isImplicitOctalLiteral(ctx)) {
                    return scanOctalLiteral(ctx, ch, start);
                }
            }
        }

        while (isDecimalDigit(ctx->m_source[ctx->m_index])) {
            number += ctx->m_source[ctx->m_index++];
        }
        ch = ctx->m_source[ctx->m_index];
    }

    if (ch == '.') {
        number += ctx->m_source[ctx->m_index++];
        while (isDecimalDigit(ctx->m_source[ctx->m_index])) {
            number += ctx->m_source[ctx->m_index++];
        }
        ch = ctx->m_source[ctx->m_index];
    }

    if (ch == 'e' || ch == 'E') {
        number += ctx->m_source[ctx->m_index++];

        ch = ctx->m_source[ctx->m_index];
        if (ch == '+' || ch == '-') {
            number += ctx->m_source[ctx->m_index++];
        }
        if (isDecimalDigit(ctx->m_source[ctx->m_index])) {
            while (isDecimalDigit(ctx->m_source[ctx->m_index])) {
                number += ctx->m_source[ctx->m_index++];
            }
        } else {
            throwUnexpectedToken();
        }
    }

    if (isIdentifierStart(ctx->m_source[ctx->m_index])) {
        throwUnexpectedToken();
    }

    double ll = wcstod(number.data(), NULL);

    ParseStatus* ps = new ParseStatus;
    ps->m_type = Token::NumericLiteralToken;
    // ps->m_value = number;
    ps->m_valueNumber = ll;
    ps->m_lineNumber = ctx->m_lineNumber;
    ps->m_lineStart = ctx->m_lineStart;
    ps->m_start = start;
    ps->m_end = ctx->m_index;
    return adoptRef(ps);
    /*
    return {
        type: Token.NumericLiteral,
        value: parseFloat(number),
        lineNumber: lineNumber,
        lineStart: lineStart,
        start: start,
        end: index
    };
     */
}


ALWAYS_INLINE PassRefPtr<ParseStatus> advance(ParseContext* ctx)
{
    char16_t cp;

    if (ctx->m_index >= ctx->m_length) {
        /*
        return {
            type: Token.EOF,
            lineNumber: lineNumber,
            lineStart: lineStart,
            start: index,
            end: index
        };
         */
        ParseStatus* ps = new ParseStatus(Token::EOFToken, ctx->m_lineNumber, ctx->m_lineStart, ctx->m_index, ctx->m_index);
        return adoptRef(ps);
    }

    cp = ctx->m_source[ctx->m_index];

    if (isIdentifierStart(cp)) {
        RefPtr<ParseStatus> token;
        token = scanIdentifier(ctx);
        if (ctx->m_strict && isStrictModeReservedWord(token->m_value)) {
            token->m_type = Token::KeywordToken;
        }
        return token;
    }

    // Very common: ( and ) and ;
    if (cp == 0x28 || cp == 0x29 || cp == 0x3B) {
        return scanPunctuator(ctx);
    }

    // String literal starts with single quote (U+0027) or double quote (U+0022).
    if (cp == 0x27 || cp == 0x22) {
        return scanStringLiteral(ctx);
    }

    // Dot (.) U+002E can also start a floating-point number, hence the need
    // to check the next character.
    if (cp == 0x2E) {
        if (isDecimalDigit(ctx->m_source[ctx->m_index + 1])) {
            return scanNumericLiteral(ctx);
        }
        return scanPunctuator(ctx);
    }

    if (isDecimalDigit(cp)) {
        return scanNumericLiteral(ctx);
    }
    /*
    // Slash (/) U+002F can also start a regex.
    if (extra.tokenize && cp === 0x2F) {
        return advanceSlash();
    }
     */
    // Template literals start with ` (U+0060) for template head
    // or } (U+007D) for template middle or template tail.
    if (cp == 0x60 || (cp == 0x7D && strcmp(ctx->m_curlyStack[ctx->m_curlyStack.size() - 1].m_curly, "${") == 0)) {
        return scanTemplate(ctx);
    }

    // Possible identifier start in a surrogate pair.
    if (cp >= 0xD800 && cp < 0xDFFF) {
        cp = codePointAt(ctx, ctx->m_index);
        if (isIdentifierStart(cp)) {
            return scanIdentifier(ctx);
        }
    }

    return scanPunctuator(ctx);
        }

void peek(ParseContext* ctx)
{
    ctx->m_scanning = true;

    skipComment(ctx);

    ctx->m_lastIndex = ctx->m_index;
    ctx->m_lastLineNumber = ctx->m_lineNumber;
    ctx->m_lastLineStart = ctx->m_lineStart;

    ctx->m_startIndex = ctx->m_index;
    ctx->m_startLineNumber = ctx->m_lineNumber;
    ctx->m_startLineStart = ctx->m_lineStart;

    // lookahead = (typeof extra.tokens !== 'undefined') ? collectToken() : advance();
    ctx->m_lookahead = advance(ctx);
    ctx->m_scanning = false;
}

ALWAYS_INLINE PassRefPtr<ParseStatus> lex(ParseContext* ctx)
{
    RefPtr<ParseStatus> token;
    ctx->m_scanning = true;

    ctx->m_lastIndex = ctx->m_index;
    ctx->m_lastLineNumber = ctx->m_lineNumber;
    ctx->m_lastLineStart = ctx->m_lineStart;

    skipComment(ctx);

    token = ctx->m_lookahead;

    ctx->m_startIndex = ctx->m_index;
    ctx->m_startLineNumber = ctx->m_lineNumber;
    ctx->m_startLineStart = ctx->m_lineStart;

    // lookahead = (typeof extra.tokens !== 'undefined') ? collectToken() : advance();
    ctx->m_lookahead = advance(ctx);
    ctx->m_scanning = false;
    return token;
}

// Expect the next token to match the specified punctuator.
// If not, an exception will be thrown.
/*
void expect(ParseContext* ctx, value) {
    ParseStatus* token = lex();
    if (token->m_type != Token::PunctuatorToken || token->m_value != value) {
        throwUnexpectedToken(token);
    }
}
 */
ALWAYS_INLINE void expect(ParseContext* ctx, const std::u16string& value)
{
    RefPtr<ParseStatus> token = lex(ctx);
    // CHECKTHIS. compare value!
    if (token->m_type != Token::PunctuatorToken || token->m_value != value) {
        throwUnexpectedToken();
    }
}

ALWAYS_INLINE void expect(ParseContext* ctx, const char16_t* value)
{
    RefPtr<ParseStatus> token = lex(ctx);
    // CHECKTHIS. compare value!
    if (token->m_type != Token::PunctuatorToken || token->m_value != value) {
        throwUnexpectedToken();
    }
}

ALWAYS_INLINE void expect(ParseContext* ctx, const char16_t& value)
{
    RefPtr<ParseStatus> token = lex(ctx);
    // CHECKTHIS. compare value!
    if (token->m_type != Token::PunctuatorToken || (token->m_value[0] != value || token->m_value.length() != 1)) {
        throwUnexpectedToken();
    }
}


/**
 * @name expectCommaSeparator
 * @description Quietly expect a comma when in tolerant mode, otherwise delegates
 * to <code>expect(value)</code>
 * @since 2.0
 */
void expectCommaSeparator(ParseContext* ctx)
{
    /*
    ParseStatus* token;
    if (extra.errors) {
        token = lookahead;
        if (token.type === Token.Punctuator && token.value === ',') {
            lex();
        } else if (token.type === Token.Punctuator && token.value === ';') {
            lex();
            tolerateUnexpectedToken(token);
        } else {
            tolerateUnexpectedToken(token, Messages.UnexpectedToken);
        }
    } else {
        expect(',');
    }
     */
    expect(ctx, ',');
}

// Expect the next token to match the specified keyword.
// If not, an exception will be thrown.

void expectKeyword(ParseContext* ctx, const std::u16string& keyword)
{
    RefPtr<ParseStatus> token = lex(ctx);
    if (token->m_type != Token::KeywordToken || token->m_value != keyword) {
        throwUnexpectedToken();
    }
}

void expectKeyword(ParseContext* ctx, const char16_t* keyword)
{
    RefPtr<ParseStatus> token = lex(ctx);
    if (token->m_type != Token::KeywordToken || token->m_value != keyword) {
        throwUnexpectedToken();
    }
}

// Return true if the next token matches the specified punctuator.

ALWAYS_INLINE bool match(ParseContext* ctx, const std::u16string& value)
{
    return ctx->m_lookahead->m_type == Token::PunctuatorToken && ctx->m_lookahead->m_value == value;
}

ALWAYS_INLINE bool match(ParseContext* ctx, const char16_t& value)
{
    return LIKELY(ctx->m_lookahead->m_type == Token::PunctuatorToken && ctx->m_lookahead->m_value.length() == 1 && ctx->m_lookahead->m_value[0] == value);
}

// Return true if the next token matches the specified keyword

bool matchKeyword(ParseContext* ctx, const std::u16string& keyword)
{
    return ctx->m_lookahead->m_type == Token::KeywordToken && ctx->m_lookahead->m_value == keyword;
}

bool matchKeyword(ParseContext* ctx, const char16_t* keyword)
{
    return ctx->m_lookahead->m_type == Token::KeywordToken && ctx->m_lookahead->m_value == keyword;
}

// Return true if the next token matches the specified contextual keyword
// (where an identifier is sometimes a keyword depending on the context)

bool matchContextualKeyword(ParseContext* ctx, const std::u16string& keyword)
{
    return ctx->m_lookahead->m_type == Token::IdentifierToken && ctx->m_lookahead->m_value == keyword;
}

bool matchContextualKeyword(ParseContext* ctx, const char16_t* keyword)
{
    return ctx->m_lookahead->m_type == Token::IdentifierToken && ctx->m_lookahead->m_value == keyword;
}

// Return true if the next token is an assignment operator

bool matchAssign(ParseContext* ctx)
{
    if (ctx->m_lookahead->m_type != Token::PunctuatorToken) {
        return false;
    }
    const std::u16string& op= ctx->m_lookahead->m_value;
    return op == u"=" ||
        op == u"*=" ||
        op == u"/=" ||
        op == u"%=" ||
        op == u"+=" ||
        op == u"-=" ||
        op == u"<<=" ||
        op == u">>=" ||
        op == u">>>=" ||
        op == u"&=" ||
        op == u"^=" ||
        op == u"|=";
}

void consumeSemicolon(ParseContext* ctx)
{
    // Catch the very common case first: immediately a semicolon (U+003B).
    if (ctx->m_source[ctx->m_startIndex] == 0x3B || match(ctx, ';')) {
        lex(ctx);
        return;
    }

    if (ctx->m_hasLineTerminator) {
        return;
    }

    // FIXME(ikarienator): this is seemingly an issue in the previous location info convention.
    ctx->m_lastIndex = ctx->m_startIndex;
    ctx->m_lastLineNumber = ctx->m_startLineNumber;
    ctx->m_lastLineStart = ctx->m_startLineStart;

    if (ctx->m_lookahead->m_type != Token::EOFToken && !match(ctx, '}')) {
        throwUnexpectedToken();
    }
}

escargot::Node* isolateCoverGrammar(ParseContext* ctx, std::function<escargot::Node* (ParseContext* ctx)> parser)
{
    bool oldIsBindingElement = ctx->m_isBindingElement,
        oldIsAssignmentTarget = ctx->m_isAssignmentTarget;
    RefPtr<ParseStatus> oldFirstCoverInitializedNameError = ctx->m_firstCoverInitializedNameError;
    escargot::Node* result;
    ctx->m_isBindingElement = true;
    ctx->m_isAssignmentTarget = true;
    ctx->m_firstCoverInitializedNameError = NULL;
    result = parser(ctx);
    if (ctx->m_firstCoverInitializedNameError) {
        throwUnexpectedToken();
    }
    ctx->m_isBindingElement = oldIsBindingElement;
    ctx->m_isAssignmentTarget = oldIsAssignmentTarget;
    ctx->m_firstCoverInitializedNameError = oldFirstCoverInitializedNameError;
    return result;
}

escargot::Node* inheritCoverGrammar(ParseContext* ctx, std::function<escargot::Node* (ParseContext* ctx)> parser)
{
    bool oldIsBindingElement = ctx->m_isBindingElement,
        oldIsAssignmentTarget = ctx->m_isAssignmentTarget;
    RefPtr<ParseStatus> oldFirstCoverInitializedNameError = ctx->m_firstCoverInitializedNameError;
    escargot::Node* result;
    ctx->m_isBindingElement = true;
    ctx->m_isAssignmentTarget = true;
    ctx->m_firstCoverInitializedNameError = NULL;
    result = parser(ctx);
    ctx->m_isBindingElement = ctx->m_isBindingElement && oldIsBindingElement;
    ctx->m_isAssignmentTarget = ctx->m_isAssignmentTarget && oldIsAssignmentTarget;
    // ctx->m_firstCoverInitializedNameError = oldFirstCoverInitializedNameError || ctx->m_firstCoverInitializedNameError;
    if (oldFirstCoverInitializedNameError)
        ctx->m_firstCoverInitializedNameError = oldFirstCoverInitializedNameError;
    return result;
}


bool isIdentifierName(ParseStatus* token)
{
    return token->m_type == Token::IdentifierToken ||
        token->m_type == Token::KeywordToken ||
        token->m_type == Token::BooleanLiteralToken ||
        token->m_type == Token::NullLiteralToken;
}

escargot::Node* finishLiteralNode(ParseContext* ctx, RefPtr<ParseStatus> ps)
{
    escargot::LiteralNode* nd;
    if (ps->m_type == Token::StringLiteralToken) {
        escargot::u16string estr(ps->m_value.begin(), ps->m_value.end());
        nd = new escargot::LiteralNode(escargot::ESString::create(std::move(estr)));
    } else if (ps->m_type == Token::NumericLiteralToken) {
        nd = new escargot::LiteralNode(escargot::ESValue(ps->m_valueNumber));
    } else {
        RELEASE_ASSERT_NOT_REACHED();
    }
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}


void rearrangeNode(escargot::StatementNodeVector& body)
{
    /*
#ifndef NDEBUG
    puts("----------");
    for (size_t i = 0; i < body.size() ; i ++) {
        if (body[i]->type() == escargot::NodeType::FunctionDeclaration) {
            puts("FD");
        } else if (body[i]->type() == escargot::NodeType::VariableDeclarator) {
            puts("VariableDeclarator");
        } else {
            puts("O");
        }
    }
    puts("----------");
#endif
     */

    size_t firstNormalStatementPlace = SIZE_MAX; // not var, function decl.
    for (size_t i = 0; i < body.size() ; i ++) {
        if (body[i]->type() != escargot::NodeType::FunctionDeclaration &&
            body[i]->type() != escargot::NodeType::VariableDeclarator) {
            firstNormalStatementPlace = i;
            break;
        }
    }
    if (firstNormalStatementPlace != SIZE_MAX) {
        for (size_t i = 0; i < firstNormalStatementPlace ; i ++) {
            if (body[i]->type() == escargot::NodeType::FunctionDeclaration) {
                for (size_t j = firstNormalStatementPlace - 1 ; j > i ; j --) {
                    if (body[j]->type() != escargot::NodeType::FunctionDeclaration) {
                        std::iter_swap(body.begin() + i, body.begin() + j);
                        break;
                    }
                }
            }
        }
    }

/*
#ifndef NDEBUG
    puts("----------");
    for (size_t i = 0; i < body.size() ; i ++) {
        if (body[i]->type() == escargot::NodeType::FunctionDeclaration) {
            puts("FD");
        } else if (body[i]->type() == escargot::NodeType::VariableDeclarator) {
            puts("VariableDeclaration");
        } else {
            puts("O");
        }
    }
    puts("----------");
#endif
 */

#ifndef NDEBUG
    bool findFD = false;
    for (size_t i = 0; i < body.size() ; i ++) {
        if (body[i]->type() == escargot::NodeType::FunctionDeclaration) {
            findFD = true;
        } else if (findFD && (body[i]->type() == escargot::NodeType::VariableDeclarator)) {
            ASSERT_NOT_REACHED();
        }
    }
#endif
}

void reinterpretExpressionAsPattern(ParseContext* ctx, escargot::Node* expr);
escargot::Node* parseAssignmentExpression(ParseContext* ctx);
escargot::Node* parseFunctionDeclaration(ParseContext* ctx/*node, identifierIsOptional*/);
escargot::Node* parseYieldExpression(ParseContext* ctx);
escargot::Node* parseStatement(ParseContext* ctx);
escargot::Node* parsePattern(ParseContext* ctx, std::vector<RefPtr<ParseStatus> >& params);
escargot::Node* parseLeftHandSideExpression(ParseContext* ctx);
escargot::Node* parseNonComputedProperty(ParseContext* ctx);
escargot::Node* parsePatternWithDefault(ParseContext* ctx, std::vector<RefPtr<ParseStatus> >& params);
escargot::Node* parseExpression(ParseContext* ctx)
{
    escargot::Node* expr;
    // ParseStatus* startToken = ctx->m_lookahead;
    escargot::ExpressionNodeVector expressions;

    expr = isolateCoverGrammar(ctx, parseAssignmentExpression);

    if (match(ctx, ',')) {
        expressions.clear();
        expressions.push_back(expr);

        while (ctx->m_startIndex < ctx->m_length) {
            if (!match(ctx, ',')) {
                break;
            }
            lex(ctx);
            expressions.push_back(isolateCoverGrammar(ctx, parseAssignmentExpression));
        }

        // expr = new WrappingNode(startToken).finishSequenceExpression(expressions);
        expr = new escargot::SequenceExpressionNode(std::move(expressions));
        expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    }

    return expr;
}

// ECMA-262 13.2 Block

escargot::Node* parseStatementListItem(ParseContext* ctx)
{
    if (ctx->m_lookahead->m_type == Token::KeywordToken) {
        if (ctx->m_lookahead->m_value == u"function")
            return parseFunctionDeclaration(ctx);
        /*
        switch (lookahead.value) {
        case 'export':
            if (sourceType !== 'module') {
                tolerateUnexpectedToken(lookahead, Messages.IllegalExportDeclaration);
            }
            return parseExportDeclaration();
        case 'import':
            if (sourceType !== 'module') {
                tolerateUnexpectedToken(lookahead, Messages.IllegalImportDeclaration);
            }
            return parseImportDeclaration();
        case 'const':
        case 'let':
            return parseLexicalDeclaration({inFor: false});
        case 'function':
            return parseFunctionDeclaration(new Node());
        case 'class':
            return parseClassDeclaration();
        }
         */
    }

    return parseStatement(ctx);
}

escargot::StatementNodeVector parseStatementList(ParseContext* ctx)
{
    escargot::StatementNodeVector list;
    // var list = [];
    while (ctx->m_startIndex < ctx->m_length) {
        if (match(ctx, '}')) {
            break;
        }
        escargot::Node* nd = parseStatementListItem(ctx);
        if (nd)
            list.push_back(nd);
    }

    return std::move(list);
}

escargot::Node* parseBlock(ParseContext* ctx)
{
    // var block, node = new Node();
    escargot::StatementNodeVector body;
    expect(ctx, '{');

    body = parseStatementList(ctx);

    expect(ctx, '}');
    escargot::Node* nd;
    nd = new escargot::BlockStatementNode(std::move(body));
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

// ECMA-262 13.3.2 Variable Statement

escargot::Node* parseVariableIdentifier(ParseContext* ctx)
{
    // var token, node = new Node();
    RefPtr<ParseStatus> token;

    token = lex(ctx);

    if (token->m_type == Token::KeywordToken && token->m_value == u"yield") {
        /*
        if (strict) {
            tolerateUnexpectedToken(token, Messages.StrictReservedWord);
        } if (!state.allowYield) {
            throwUnexpectedToken(token);
        }
         */
        RELEASE_ASSERT_NOT_REACHED();
    } else if (token->m_type != Token::IdentifierToken) {
        if (ctx->m_strict && token->m_type == Token::KeywordToken && isStrictModeReservedWord(token->m_value)) {
            tolerateUnexpectedToken();
            // tolerateUnexpectedToken(token, Messages.StrictReservedWord);
        } else {
            // throwUnexpectedToken(token);
            throwUnexpectedToken();
        }
    }
    /*
    else if (sourceType === 'module' && token.type === Token.Identifier && token.value === 'await') {
        tolerateUnexpectedToken(token);
    }
    */

    escargot::Node* nd = new escargot::IdentifierNode(escargot::InternalAtomicString(token->m_value.data()));
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

escargot::VariableDeclaratorNode* parseVariableDeclaration(ParseContext* ctx)
{
    // var init = null, id, node = new Node(), params = [];
    escargot::Node* init = nullptr;
    std::vector<RefPtr<ParseStatus> > params;
    escargot::Node* id = parsePattern(ctx, params);

    // ECMA-262 12.2.1
    // if (strict && isRestrictedWord(id.name)) {
    // TODO: not alawys idenifier node!
    ASSERT(id->type() == escargot::NodeType::Identifier);
    if (ctx->m_strict && isRestrictedWord(((escargot::IdentifierNode *)id)->nonAtomicName()->data())) {
        // tolerateError(Messages.StrictVarName);
        tolerateError(u"Messages.StrictVarName");
    }

    if (match(ctx,  '=')) {
        lex(ctx);
        init = isolateCoverGrammar(ctx, parseAssignmentExpression);
    } else if (id->type() != escargot::NodeType::Identifier) {
        expect(ctx, '=');
    }

    // return node.finishVariableDeclarator(id, init);
    escargot::VariableDeclaratorNode* nd = new escargot::VariableDeclaratorNode(id, (escargot::ExpressionNode *)init);
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

escargot::VariableDeclaratorVector parseVariableDeclarationList(ParseContext* ctx, bool excludeVariableDeclaratorNode = true)
{
    escargot::VariableDeclaratorVector list;
    // var list = [];

    do {
        escargot::VariableDeclaratorNode* node = parseVariableDeclaration(ctx);
        ctx->m_currentBody->insert(ctx->m_currentBody->begin(), node);
        if (!excludeVariableDeclaratorNode)
            list.push_back(node);
        if (node->init()) {
            ASSERT(node->id()->type() == escargot::NodeType::Identifier);
            escargot::Node* id = ((escargot::IdentifierNode *) node->id())->clone();
            escargot::Node* init = node->init();
            node->clearInit();
            escargot::Node* nd = new escargot::AssignmentExpressionSimpleNode(id, init);
            nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
            list.push_back(nd);
        }


        if (!match(ctx, ',')) {
            break;
        }
        lex(ctx);
    } while (ctx->m_startIndex < ctx->m_length);

    return list;
}

escargot::Node* parseVariableStatement(ParseContext* ctx /*node*/)
{
    escargot::VariableDeclaratorVector declarations;

    expectKeyword(ctx, u"var");

    declarations = parseVariableDeclarationList(ctx);

    consumeSemicolon(ctx);

    // return node.finishVariableDeclaration(declarations);
    escargot::Node* nd = new escargot::VariableDeclarationNode(std::move(declarations));
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

// ECMA-262 13.3.1 Let and Const Declarations

escargot::Node* parseLexicalBinding(ParseContext* ctx, escargot::u16string& kind /*options*/)
{
    // var init = null, id, node = new Node(), params = [];

    std::vector<RefPtr<ParseStatus> > params;
    escargot::Node* id = parsePattern(ctx, params);
    escargot::Node* init = nullptr;

    // ECMA-262 12.2.1
    if (ctx->m_strict && isRestrictedWord(((escargot::IdentifierNode *)id)->nonAtomicName()->data())) {
        // tolerateError(Messages.StrictVarName);
        tolerateError(u"Messages.StrictVarName");
    }

    if (kind == u"const") {
        if (!matchKeyword(ctx, u"in") && !matchContextualKeyword(ctx, u"of")) {
            expect(ctx, '=');
            init = isolateCoverGrammar(ctx, parseAssignmentExpression);
        }
    }
    // FIXME options.inFor is not always true!
    if (match(ctx, '=')) {
        expect(ctx, '=');
        init = isolateCoverGrammar(ctx, parseAssignmentExpression);
    }
    /*
    else if ((!options.inFor && id.type !== Syntax.Identifier) || match('=')) {
        expect('=');
        init = isolateCoverGrammar(parseAssignmentExpression);
    }
     */

    // return node.finishVariableDeclarator(id, init);
    ASSERT(!init);
    escargot::Node* nd = new escargot::VariableDeclaratorNode(id);
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}
/*
function parseBindingList(kind, options) {
    var list = [];

    do {
        list.push(parseLexicalBinding(kind, options));
        if (!match(',')) {
            break;
        }
        lex();
    } while (startIndex < length);

    return list;
}
 */

escargot::Node* parseLexicalDeclaration(/*options*/)
{
    RELEASE_ASSERT_NOT_REACHED();
    /*
    var kind, declarations, node = new Node();

    kind = lex().value;
    assert(kind === 'let' || kind === 'const', 'Lexical declaration must be either let or const');

    declarations = parseBindingList(kind, options);

    consumeSemicolon();

    return node.finishLexicalDeclaration(declarations, kind);
     */
}

escargot::Node* parseRestElement(/*params*/)
{
    RELEASE_ASSERT_NOT_REACHED();
    /*
    var param, node = new Node();

    lex();

    if (match('{')) {
        throwError(Messages.ObjectPatternAsRestParameter);
    }

    params.push(lookahead);

    param = parseVariableIdentifier();

    if (match('=')) {
        throwError(Messages.DefaultRestParameter);
    }

    if (!match(')')) {
        throwError(Messages.ParameterAfterRestParameter);
    }

    return node.finishRestElement(param);
     */
}

// ECMA-262 13.4 Empty Statement

escargot::Node* parseEmptyStatement(ParseContext* ctx/*node*/)
{
    expect(ctx, ';');
    // return node.finishEmptyStatement();
    escargot::Node* nd = new escargot::EmptyStatementNode();
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

// ECMA-262 12.4 Expression Statement

escargot::Node* parseExpressionStatement(ParseContext* ctx/*node*/)
{
    escargot::Node* expr = parseExpression(ctx);
    consumeSemicolon(ctx);
    // return node.finishExpressionStatement(expr);
    escargot::Node* nd = new escargot::ExpressionStatementNode(expr);
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

// ECMA-262 13.6 If statement

escargot::Node* parseIfStatement(ParseContext* ctx/*node*/)
{
    escargot::Node* test;
    escargot::Node* consequent;
    escargot::Node* alternate;

    expectKeyword(ctx, u"if");

    expect(ctx, '(');

    test = parseExpression(ctx);

    expect(ctx, ')');

    consequent = parseStatement(ctx);

    if (matchKeyword(ctx, u"else")) {
        lex(ctx);
        alternate = parseStatement(ctx);
    } else {
        alternate = nullptr;
    }

    // return node.finishIfStatement(test, consequent, alternate);
    escargot::Node* nd = new escargot::IfStatementNode(test, consequent, alternate);
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

// ECMA-262 13.7 Iteration Statements

escargot::Node* parseDoWhileStatement(ParseContext* ctx/*node*/)
{
    bool oldInIteration;
    escargot::Node* body;
    escargot::Node* test;

    expectKeyword(ctx, u"do");

    oldInIteration = ctx->m_inIteration;
    ctx->m_inIteration = true;

    body = parseStatement(ctx);

    ctx->m_inIteration = oldInIteration;

    expectKeyword(ctx, u"while");

    expect(ctx, '(');

    test = parseExpression(ctx);

    expect(ctx, ')');

    if (match(ctx, ';')) {
        lex(ctx);
    }

    // return node.finishDoWhileStatement(body, test);
    escargot::Node* nd = new escargot::DoWhileStatementNode(test, body);
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

escargot::Node* parseWhileStatement(ParseContext* ctx/*node*/)
{
    bool oldInIteration;
    escargot::Node* body;
    escargot::Node* test;

    expectKeyword(ctx, u"while");

    expect(ctx, '(');

    test = parseExpression(ctx);

    expect(ctx, ')');

    oldInIteration = ctx->m_inIteration;
    ctx->m_inIteration = true;

    body = parseStatement(ctx);

    ctx->m_inIteration = oldInIteration;

    // return node.finishWhileStatement(test, body);
    escargot::Node* nd = new escargot::WhileStatementNode(test, body);
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

escargot::Node* parseForStatement(ParseContext* ctx/*node*/)
{
    // var init, forIn, initSeq, initStartToken, test, update, left, right, kind, declarations,
    // body, oldInIteration, previousAllowIn = state.allowIn;
    escargot::Node* init;
    escargot::Node* test;
    escargot::Node* update;
    escargot::Node* left = NULL;
    escargot::Node* right;
    escargot::Node* body;
    bool previousAllowIn = ctx->m_allowIn;

    init = test = update = nullptr;
    bool forIn = true;

    expectKeyword(ctx, u"for");

    expect(ctx, '(');

    if (match(ctx, ';')) {
        lex(ctx);
    } else {
        if (matchKeyword(ctx, u"var")) {
            // init = new Node();
            lex(ctx);

            ctx->m_allowIn = false;
            init = new escargot::VariableDeclarationNode(parseVariableDeclarationList(ctx, false));
            init->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
            ctx->m_allowIn = previousAllowIn;

            if (((escargot::VariableDeclarationNode *)init)->declarations().size() == 1 && matchKeyword(ctx, u"in")) {
                lex(ctx);
                // left = init;
                left = ((escargot::VariableDeclaratorNode *)(((escargot::VariableDeclarationNode *)init)->declarations()[0]))->id();
                right = parseExpression(ctx);
                init = nullptr;
            } else if (((escargot::VariableDeclarationNode *)init)->declarations().size() == 1 && ((escargot::VariableDeclaratorNode *)((escargot::VariableDeclarationNode *)init)->declarations()[0])->init() == nullptr &&
                matchContextualKeyword(ctx, u"of")) {
                lex(ctx);
                left = init;
                right = parseAssignmentExpression(ctx);
                init = nullptr;
                forIn = false;
            } else {
                escargot::VariableDeclaratorVector& vec = ((escargot::VariableDeclarationNode *)init)->declarations();
                for (unsigned i = 0 ; i < vec.size() ; i ++) {
                    if (vec[i]->type() == escargot::NodeType::VariableDeclarator) {
                        vec.erase(vec.begin() + i);
                        i = 0;
                    }
                }
                expect(ctx, ';');
            }
        } else {
            // ParseStatus* initStartToken = ctx->m_lookahead;
            ctx->m_allowIn = false;
            init = inheritCoverGrammar(ctx, parseAssignmentExpression);
            ctx->m_allowIn = previousAllowIn;

            if (matchKeyword(ctx, u"in")) {
                if (!ctx->m_isAssignmentTarget) {
                    tolerateError(u"Messages.InvalidLHSInForIn");
                }

                lex(ctx);
                reinterpretExpressionAsPattern(ctx, init);
                left = init;
                right = parseExpression(ctx);
                init = nullptr;
            } else if (matchContextualKeyword(ctx, u"of")) {
                if (!ctx->m_isAssignmentTarget) {
                    tolerateError(u"Messages.InvalidLHSInForLoop");
                }

                lex(ctx);
                reinterpretExpressionAsPattern(ctx, init);
                left = init;
                right = parseAssignmentExpression(ctx);
                init = nullptr;
                forIn = false;
            } else {
                escargot::ExpressionNodeVector initSeq;
                if (match(ctx, ',')) {
                    initSeq.push_back(init);
                    // initSeq = [init];
                    while (match(ctx, ',')) {
                        lex(ctx);
                        // initSeq.push(isolateCoverGrammar(parseAssignmentExpression));
                        initSeq.push_back(isolateCoverGrammar(ctx, parseAssignmentExpression));
                    }
                    // init = new WrappingNode(initStartToken).finishSequenceExpression(initSeq);
                    init = new escargot::SequenceExpressionNode(std::move(initSeq));
                    init->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
                }
                expect(ctx, ';');
            }
        }
    }

    // if (typeof left === 'undefined') {
    if (left == NULL) {

        if (!match(ctx, ';')) {
            test = parseExpression(ctx);
        }
        expect(ctx, ';');

        if (!match(ctx, ')')) {
            update = parseExpression(ctx);
        }
    }

    expect(ctx, ')');

    bool oldInIteration = ctx->m_inIteration;
    ctx->m_inIteration = true;

    body = isolateCoverGrammar(ctx, parseStatement);

    ctx->m_inIteration = oldInIteration;

    if (left == NULL) {
        escargot::Node* nd = new escargot::ForStatementNode(init, test, update, body);
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    } else {
        if (forIn) {
            // FIXME what is each?
            escargot::Node* nd = new escargot::ForInStatementNode(left, right, body, false);
            nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
            return nd;
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
    }
    /*
    return (typeof left === 'undefined') ?
            node.finishForStatement(init, test, update, body) :
            forIn ? node.finishForInStatement(left, right, body) :
                    node.finishForOfStatement(left, right, body);
     */

}

// ECMA-262 13.8 The continue statement

escargot::Node* parseContinueStatement(ParseContext* ctx/*node*/)
{
    // var label = null, key;

    expectKeyword(ctx, u"continue");

    // Optimize the most common form: 'continue;'.
    if (ctx->m_source[ctx->m_startIndex] == 0x3B) {
        lex(ctx);

        if (!ctx->m_inIteration) {
            // throwError(Messages.IllegalContinue);
            throw u"Messages.IllegalContinue";
        }

        // return node.finishContinueStatement(null);
        escargot::Node* nd = new escargot::ContinueStatementNode();
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    }

    if (ctx->m_hasLineTerminator) {
        if (!ctx->m_inIteration) {
            // throwError(Messages.IllegalContinue);
            throw u"Messages.IllegalContinue";
        }

        // return node.finishContinueStatement(null);
        escargot::Node* nd = new escargot::ContinueStatementNode();
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    }

    escargot::Node* label = NULL;
    size_t upCount = 0;
    if (ctx->m_lookahead->m_type == Token::IdentifierToken) {
        label = parseVariableIdentifier(ctx);
        escargot::ESString* key = ((escargot::IdentifierNode *)label)->nonAtomicName();

        auto iter = ctx->m_labelSet.rbegin();
        bool find = false;
        while (iter != ctx->m_labelSet.rend()) {
            if ((*iter)->string() == key->string()) {
                find = true;
                break;
            }
            upCount++;
            iter++;
        }
        if (!find) {
            throw u"Error(Messages.UnknownLabel, label.name)";
        }
    }

    consumeSemicolon(ctx);

    if (label == NULL && !(ctx->m_inIteration || ctx->m_inSwitch)) {
        throw u"throwError(Messages.IllegalContinue);";
    }

    if (label) {
        escargot::Node* nd = new escargot::ContinueLabelStatementNode(upCount, ((escargot::IdentifierNode *)label)->nonAtomicName());
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    } else {
        escargot::Node* nd = new escargot::ContinueStatementNode();
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    }
}

// ECMA-262 13.9 The break statement

escargot::Node* parseBreakStatement(ParseContext* ctx/*node*/)
{
    // var label = null, key;

    expectKeyword(ctx, u"break");

    // Catch the very common case first: immediately a semicolon (U+003B).
    if (ctx->m_source[ctx->m_lastIndex] == 0x3B) {
        lex(ctx);

        if (!(ctx->m_inIteration || ctx->m_inSwitch)) {
            // throwError(Messages.IllegalBreak);
            throw u"Messages.IllegalBreak";
        }

        // return node.finishBreakStatement(null);
        escargot::Node* nd = new escargot::BreakStatementNode();
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    }

    if (ctx->m_hasLineTerminator) {
        if (!(ctx->m_inIteration || ctx->m_inSwitch)) {
            // throwError(Messages.IllegalBreak);
            throw u"Messages.IllegalBreak";
        }
        escargot::Node* nd = new escargot::BreakStatementNode();
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    }

    escargot::Node* label = NULL;
    size_t upCount = 0;
    if (ctx->m_lookahead->m_type == Token::IdentifierToken) {
        label = parseVariableIdentifier(ctx);
        escargot::ESString* key = ((escargot::IdentifierNode *)label)->nonAtomicName();

        auto iter = ctx->m_labelSet.rbegin();
        bool find = false;
        while (iter != ctx->m_labelSet.rend()) {
            if ((*iter)->string() == key->string()) {
                find = true;
                break;
            }
            upCount++;
            iter++;
        }
        if (!find) {
            throw u"Error(Messages.UnknownLabel, label.name)";
        }
    }

    consumeSemicolon(ctx);

    if (label == NULL && !(ctx->m_inIteration || ctx->m_inSwitch)) {
        throw u"throwError(Messages.IllegalBreak);";
    }
    if (label) {
        escargot::Node* nd = new escargot::BreakLabelStatementNode(upCount, ((escargot::IdentifierNode *)label)->nonAtomicName());
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    } else {
        escargot::Node* nd = new escargot::BreakStatementNode();
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    }
}

// ECMA-262 13.10 The return statement

escargot::Node* parseReturnStatement(ParseContext* ctx/*node*/)
{
    escargot::Node* argument = nullptr;
    // var argument = null;

    expectKeyword(ctx, u"return");

    if (!ctx->m_inFunctionBody) {
        // tolerateError(Messages.IllegalReturn);
        throw u"Messages.IllegalReturn";
    }

    // 'return' followed by a space and an identifier is very common.
    if (ctx->m_source[ctx->m_lastIndex] == 0x20) {
        if (isIdentifierStart(ctx->m_source[ctx->m_lastIndex])) {
            argument = parseExpression(ctx);
            consumeSemicolon(ctx);
            escargot::Node* nd = new escargot::ReturnStatmentNode(argument);
            nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
            return nd;
        }
    }

    if (ctx->m_hasLineTerminator) {
        // HACK
        // return node.finishReturnStatement(null);
        escargot::Node* nd = new escargot::ReturnStatmentNode(nullptr);
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    }

    if (!match(ctx, ';')) {
        if (!match(ctx, '}') && ctx->m_lookahead->m_type != Token::EOFToken) {
            argument = parseExpression(ctx);
        }
    }

    consumeSemicolon(ctx);

    escargot::Node* nd = new escargot::ReturnStatmentNode(argument);
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

// ECMA-262 13.11 The with statement

escargot::Node* parseWithStatement(ParseContext* ctx)
{
    /*
    var object, body;

    if (strict) {
        tolerateError(Messages.StrictModeWith);
    }

    expectKeyword('with');

    expect('(');

    object = parseExpression();

    expect(')');

    body = parseStatement();

    return node.finishWithStatement(object, body);
     */
    RELEASE_ASSERT_NOT_REACHED();
}

// ECMA-262 13.12 The switch statement

escargot::SwitchCaseNode* parseSwitchCase(ParseContext* ctx)
{
    // var test, consequent = [], statement, node = new Node();
    escargot::Node* test; // , consequent = [], statement, node = new Node();
    escargot::StatementNodeVector consequent;
    if (matchKeyword(ctx, u"default")) {
        lex(ctx);
        test = nullptr;
    } else {
        expectKeyword(ctx, u"case");
        test = parseExpression(ctx);
    }
    expect(ctx, ':');

    while (ctx->m_startIndex < ctx->m_length) {
        if (match(ctx, '}') || matchKeyword(ctx, u"default") || matchKeyword(ctx, u"case")) {
            break;
        }
        escargot::Node* statement = parseStatementListItem(ctx);
        if (statement)
            consequent.push_back(statement);
    }

    // return node.finishSwitchCase(test, consequent);
    escargot::SwitchCaseNode* nd = new escargot::SwitchCaseNode(test, std::move(consequent));
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

escargot::Node* parseSwitchStatement(ParseContext* ctx/*node*/)
{
    // var discriminant, cases, clause, oldInSwitch, defaultFound;

    expectKeyword(ctx, u"switch");

    expect(ctx, '(');

    escargot::Node* discriminant = parseExpression(ctx);

    expect(ctx, ')');

    expect(ctx, '{');

    escargot::StatementNodeVector casesA;
    escargot::StatementNodeVector casesB;
    // cases = [];

    if (match(ctx, '}')) {
        lex(ctx);
        // return node.finishSwitchStatement(discriminant, cases);
        escargot::Node* nd = new escargot::SwitchStatementNode(discriminant, std::move(casesA), nullptr, std::move(casesB), false);
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    }

    bool oldInSwitch = ctx->m_inSwitch;
    ctx->m_inSwitch = true;
    bool defaultFound = false;
    escargot::Node* def = nullptr;
    while (ctx->m_startIndex < ctx->m_length) {
        if (match(ctx, '}')) {
            break;
        }
        escargot::SwitchCaseNode* clause = parseSwitchCase(ctx);

        if (clause->isDefaultNode()) {
            if (defaultFound) {
                throw u"Messages.MultipleDefaultsInSwitch";
            }
            defaultFound = true;
            def = clause;
        } else if (defaultFound)
            casesA.push_back(clause);
        else
            casesB.push_back(clause);
    }

    ctx->m_inSwitch = oldInSwitch;

    expect(ctx, '}');

    // return node.finishSwitchStatement(discriminant, cases);
    escargot::Node* nd = new escargot::SwitchStatementNode(discriminant, std::move(casesA), def, std::move(casesB), false);
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

// ECMA-262 13.14 The throw statement

escargot::Node* parseThrowStatement(ParseContext* ctx/*node*/)
{
    escargot::Node* argument;

    expectKeyword(ctx, u"throw");

    if (ctx->m_hasLineTerminator) {
        throw u"Messages.NewlineAfterThrow";
        // throwError(Messages.NewlineAfterThrow);
    }

    argument = parseExpression(ctx);

    consumeSemicolon(ctx);

    // return node.finishThrowStatement(argument);
    escargot::Node* nd = new escargot::ThrowStatementNode(argument);
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

// ECMA-262 13.15 The try statement

escargot::Node* parseCatchClause(ParseContext* ctx/*node*/)
{
    // var param, params = [], paramMap = {}, key, i, body, node = new Node();


    expectKeyword(ctx, u"catch");

    expect(ctx, '(');
    if (match(ctx, ')')) {
        // throwUnexpectedToken(lookahead);
        throwUnexpectedToken();
    }

    std::vector<RefPtr<ParseStatus> > params;
    escargot::Node* param = parsePattern(ctx, params);
    for (unsigned i = 0; i < params.size(); i++) {
        // TODO
        /*
        key = '$' + params[i].value;
        if (Object.prototype.hasOwnProperty.call(paramMap, key)) {
            tolerateError(Messages.DuplicateBinding, params[i].value);
        }
        paramMap[key] = true;
         */
    }

    // ECMA-262 12.14.1
    // TODO
    /*
    if (ctx->m_strict && isRestrictedWord(param.name)) {
        tolerateError(Messages.StrictCatchVariable);
    }
    */

    expect(ctx, ')');
    escargot::Node* body = parseBlock(ctx);
    // return node.finishCatchClause(param, body);
    escargot::Node* nd = new escargot::CatchClauseNode(param, nullptr, body);
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

escargot::Node* parseTryStatement(ParseContext* ctx/*node*/)
{
    // var block, handler = null, finalizer = null;

    expectKeyword(ctx, u"try");

    escargot::Node* block = parseBlock(ctx);
    escargot::Node* handler = nullptr;
    escargot::Node* finalizer = nullptr;

    if (matchKeyword(ctx, u"catch")) {
        handler = parseCatchClause(ctx);
    }

    if (matchKeyword(ctx, u"finally")) {
        lex(ctx);
        finalizer = parseBlock(ctx);
    }

    if (!handler && !finalizer) {
        throw u"Messages.NoCatchOrFinally";
    }

    // return node.finishTryStatement(block, handler, finalizer);
    escargot::Node* nd = new escargot::TryStatementNode(block, handler, escargot::CatchClauseNodeVector(), finalizer);
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

// ECMA-262 13.16 The debugger statement
/*
function parseDebuggerStatement(node) {
    expectKeyword('debugger');

    consumeSemicolon();

    return node.finishDebuggerStatement();
}
*/
// 13 Statements

escargot::Node* parseStatement(ParseContext* ctx)
{
    Token type = ctx->m_lookahead->m_type;
    /*
    var type = lookahead.type,
                expr,
                labeledBody,
                key,
                node;
     */

    if (type == Token::EOFToken) {
        // throwUnexpectedToken(lookahead);
        throwUnexpectedToken();
    }

    if (type == Token::PunctuatorToken && ctx->m_lookahead->m_value == u"{") {
        return parseBlock(ctx);
    }
    ctx->m_isAssignmentTarget = ctx->m_isBindingElement = true;

    if (type == Token::PunctuatorToken) {
        if (ctx->m_lookahead->m_value == u";") {
            return parseEmptyStatement(ctx);
        } else if (ctx->m_lookahead->m_value == u"(") {
            return parseExpressionStatement(ctx);
        }
    } else if (type == Token::KeywordToken) {
        if (ctx->m_lookahead->m_value == u"break") {
            return parseBreakStatement(ctx);
        } else if (ctx->m_lookahead->m_value == u"break") {
            return parseBreakStatement(ctx);
        } else if (ctx->m_lookahead->m_value == u"continue") {
            return parseContinueStatement(ctx);
        } else if (ctx->m_lookahead->m_value == u"debugger") {
            RELEASE_ASSERT_NOT_REACHED();
        } else if (ctx->m_lookahead->m_value == u"do") {
            return parseDoWhileStatement(ctx);
        } else if (ctx->m_lookahead->m_value == u"for") {
            return parseForStatement(ctx);
        } else if (ctx->m_lookahead->m_value == u"function") {
            RELEASE_ASSERT_NOT_REACHED();
            return parseFunctionDeclaration(ctx);
        } else if (ctx->m_lookahead->m_value == u"if") {
            return parseIfStatement(ctx);
        } else if (ctx->m_lookahead->m_value == u"return") {
            return parseReturnStatement(ctx);
        } else if (ctx->m_lookahead->m_value == u"switch") {
            return parseSwitchStatement(ctx);
        } else if (ctx->m_lookahead->m_value == u"throw") {
            return parseThrowStatement(ctx);
        } else if (ctx->m_lookahead->m_value == u"try") {
            return parseTryStatement(ctx);
        } else if (ctx->m_lookahead->m_value == u"var") {
            return parseVariableStatement(ctx);
        } else if (ctx->m_lookahead->m_value == u"while") {
            return parseWhileStatement(ctx);
        } else if (ctx->m_lookahead->m_value == u"with") {
            return parseWithStatement(ctx);
        }

    }

    escargot::Node* expr = parseExpression(ctx);

    // ECMA-262 12.12 Labelled Statements
    if ((expr->type() == escargot::NodeType::Identifier) && match(ctx, ':')) {
        lex(ctx);

        escargot::ESString* key = ((escargot::IdentifierNode *)expr)->nonAtomicName();
        auto iter = ctx->m_labelSet.begin();
        while (iter != ctx->m_labelSet.end()) {
            if ((*iter)->string() == key->string()) {
                throw u"throwError(Messages.Redeclaration, 'Label', expr.name);";
            }
            iter++;
        }

        ctx->m_labelSet.push_back(key);
        escargot::Node* labeledBody = parseStatement(ctx);
        ctx->m_labelSet.pop_back();
        escargot::LabeledStatementNode* nd = new escargot::LabeledStatementNode((escargot::StatementNode *)labeledBody, key);
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    }

    consumeSemicolon(ctx);

    // return node.finishExpressionStatement(expr);
    escargot::Node* nd = new escargot::ExpressionStatementNode(expr);
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

// ECMA-262 14.1 Function Definition

escargot::Node* parseFunctionSourceElements(ParseContext* ctx)
{
    /*var statement, body = [], token, directive, firstRestricted,
        oldLabelSet, oldInIteration, oldInSwitch, oldInFunctionBody, oldParenthesisCount,
        node = new Node();
     */

    expect(ctx, {'{'});
    escargot::StatementNodeVector* prevBody = ctx->m_currentBody;
    escargot::StatementNodeVector body;
    ctx->m_currentBody = &body;
    RefPtr<ParseStatus> firstRestricted;

    while (ctx->m_startIndex < ctx->m_length) {
        if (ctx->m_lookahead->m_type != Token::StringLiteralToken) {
            break;
        }
        RefPtr<ParseStatus> token = ctx->m_lookahead;

        escargot::Node* statement = parseStatementListItem(ctx);
        if (statement)
            body.push_back(statement);
        ASSERT(statement->type() == escargot::NodeType::ExpressionStatement);
        if (((escargot::ExpressionStatementNode *) statement)->expression()->type() != escargot::NodeType::Literal) {
            // this is not directive
            break;
        }
        bool strict = true;
        if (token->m_end - 1 - (token->m_start + 1) == 10) {
            static const char16_t* s = u"use strict";
            for (size_t i = 0 ; i < 10 ; i ++) {
                if (s[i] != ctx->m_source[token->m_start + 1 + i]) {
                    strict = false;
                }
            }
        } else {
            strict = false;
        }
        if (strict) {
            ctx->m_strict = true;
            if (firstRestricted) {
                // tolerateUnexpectedToken(firstRestricted, Messages.StrictOctalLiteral);
                tolerateUnexpectedToken();
            }
        } else {
            if (!firstRestricted && token->m_octal) {
                firstRestricted = token;
            }
        }

        // directive = source.slice(token.start + 1, token.end - 1);
        /*
        escargot::u16string directive = ctx->m_source.substr(token->m_start + 1,
            token->m_end - 1 - (token->m_start + 1));
        // directive = source.slice(token.start + 1, token.end - 1);
        if (directive == u"use strict") {
            ctx->m_strict = true;
            if (firstRestricted) {
                // tolerateUnexpectedToken(firstRestricted, Messages.StrictOctalLiteral);
                tolerateUnexpectedToken();
            }
        } else {
            if (!firstRestricted && token->m_octal) {
                firstRestricted = token;
            }
        }
        */
    }

    std::vector<escargot::ESString *, gc_allocator<escargot::ESString *>> oldLabelSet = ctx->m_labelSet;
    bool oldInIteration = ctx->m_inIteration;
    bool oldInSwitch = ctx->m_inSwitch;
    bool oldInFunctionBody = ctx->m_inFunctionBody;
    int oldParenthesisCount = ctx->m_parenthesizedCount;

    ctx->m_labelSet.clear();
    ctx->m_inIteration = false;
    ctx->m_inSwitch = false;
    ctx->m_inFunctionBody = true;
    ctx->m_parenthesizedCount = 0;

    while (ctx->m_startIndex < ctx->m_length) {
        if (match(ctx, '}')) {
            break;
        }
        escargot::Node* nd = parseStatementListItem(ctx);
        if (nd)
            body.push_back(nd);
    }

    expect(ctx, '}');

    ctx->m_labelSet = oldLabelSet;
    ctx->m_inIteration = oldInIteration;
    ctx->m_inSwitch = oldInSwitch;
    ctx->m_inFunctionBody = oldInFunctionBody;
    ctx->m_parenthesizedCount = oldParenthesisCount;

    rearrangeNode(*ctx->m_currentBody);

    ctx->m_currentBody = prevBody;
    escargot::Node* nd;
    nd = new escargot::BlockStatementNode(std::move(body));
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}


/*
function validateParam(options, param, name) {
    var key = '$' + name;
    if (strict) {
        if (isRestrictedWord(name)) {
            options.stricted = param;
            options.message = Messages.StrictParamName;
        }
        if (Object.prototype.hasOwnProperty.call(options.paramSet, key)) {
            options.stricted = param;
            options.message = Messages.StrictParamDupe;
        }
    } else if (!options.firstRestricted) {
        if (isRestrictedWord(name)) {
            options.firstRestricted = param;
            options.message = Messages.StrictParamName;
        } else if (isStrictModeReservedWord(name)) {
            options.firstRestricted = param;
            options.message = Messages.StrictReservedWord;
        } else if (Object.prototype.hasOwnProperty.call(options.paramSet, key)) {
            options.stricted = param;
            options.message = Messages.StrictParamDupe;
        }
    }
    options.paramSet[key] = true;
}
*/


/*
function parseParam(options) {
    var token, param, params = [], i, def;

    token = lookahead;
    if (token.value === '...') {
        param = parseRestElement(params);
        validateParam(options, param.argument, param.argument.name);
        options.params.push(param);
        options.defaults.push(null);
        return false;
    }

    param = parsePatternWithDefault(params);
    for (i = 0; i < params.length; i++) {
        validateParam(options, params[i], params[i].value);
    }

    if (param.type === Syntax.AssignmentPattern) {
        def = param.right;
        param = param.left;
        ++options.defaultCount;
    }

    options.params.push(param);
    options.defaults.push(def);

    return !match(')');
}
 */

bool parseParam(ParseContext* ctx, escargot::InternalAtomicStringVector& vec/*, options*/)
{
    // var token, param, params = [], i, def;

    RefPtr<ParseStatus> token = ctx->m_lookahead;
    /*
    if (token.value === '...') {
        param = parseRestElement(params);
        validateParam(options, param.argument, param.argument.name);
        options.params.push(param);
        options.defaults.push(null);
        return false;
    }
    */

    std::vector<RefPtr<ParseStatus> > params;
    escargot::Node* param = parsePatternWithDefault(ctx, params);

    /*
    for (i = 0; i < params.length; i++) {
        validateParam(options, params[i], params[i].value);
    }

    if (param.type === Syntax.AssignmentPattern) {
        def = param.right;
        param = param.left;
        ++options.defaultCount;
    }
    */

    // options.params.push(param);
    // options.defaults.push(def);
    ASSERT(param->type() == escargot::NodeType::Identifier);
    vec.push_back(((escargot::IdentifierNode *)param)->name());
    return !match(ctx, ')');
}

escargot::InternalAtomicStringVector parseParams(ParseContext* ctx/*, ParseStatus* firstRestricted*/)
{
    /*
    var options;

    options = {
        params: [],
        defaultCount: 0,
        defaults: [],
        firstRestricted: firstRestricted
    };
    */

    escargot::InternalAtomicStringVector vec;
    expect(ctx, '(');

    if (!match(ctx, ')')) {
        while (ctx->m_startIndex < ctx->m_length) {
            if (!parseParam(ctx, vec)) {
                break;
            }
            expect(ctx, ',');
        }
        /*
        options.paramSet = {};
        while (startIndex < length) {
            if (!parseParam(options)) {
                break;
        }
        expect(',');
        }
        */
    }

    expect(ctx, ')');

    /*
    if (options.defaultCount === 0) {
        options.defaults = [];
    }

    return {
        params: options.params,
        defaults: options.defaults,
        stricted: options.stricted,
        firstRestricted: options.firstRestricted,
        message: options.message
    };
    */

    return vec;
}


escargot::Node* parseFunctionDeclaration(ParseContext* ctx/*node, identifierIsOptional*/)
{
    // var id = null, params = [], defaults = [], body, token, stricted, tmp, firstRestricted, message, previousStrict,
    // isGenerator, previousAllowYield;

    bool previousAllowYield = ctx->m_allowYield;
    RefPtr<ParseStatus> firstRestricted;
    expectKeyword(ctx, u"function");

    bool isGenerator = match(ctx, '*');
    if (isGenerator) {
        lex(ctx);
    }

    // if (!identifierIsOptional || !match('(')) {
    escargot::Node* id;
    if (!match(ctx, '(')) {
        RefPtr<ParseStatus> token = ctx->m_lookahead;
        id = parseVariableIdentifier(ctx);
        if (ctx->m_strict) {
            if (isRestrictedWord(token->m_value)) {
                // tolerateUnexpectedToken(token, Messages.StrictFunctionName);
                tolerateUnexpectedToken();
            }
        } else {
            if (isRestrictedWord(token->m_value)) {
                firstRestricted = token;
                // message = Messages.StrictFunctionName;
            } else if (isStrictModeReservedWord(token->m_value)) {
                firstRestricted = token;
                // message = Messages.StrictReservedWord;
            }
        }
    }

    ctx->m_allowYield = !isGenerator;
    escargot::InternalAtomicStringVector params = parseParams(ctx/*, firstRestricted*/);
    /*
    tmp = parseParams(firstRestricted);
    params = tmp.params;
    defaults = tmp.defaults;
    stricted = tmp.stricted;
    firstRestricted = tmp.firstRestricted;
    if (tmp.message) {
        message = tmp.message;
    }
    */


    bool previousStrict = ctx->m_strict;
    escargot::Node* body = parseFunctionSourceElements(ctx);
    if (ctx->m_strict && firstRestricted) {
        // throwUnexpectedToken(firstRestricted, message);
        throwUnexpectedToken();
    }
    /*
    if (strict && stricted) {
        tolerateUnexpectedToken(stricted, message);
    }
    */

    // return node.finishFunctionDeclaration(id, params, defaults, body, isGenerator);
    ASSERT(id->type() == escargot::NodeType::Identifier);

    escargot::Node* nd = new escargot::FunctionDeclarationNode(((escargot::IdentifierNode *)id)->nonAtomicName(), std::move(params), body, isGenerator, false, ctx->m_strict);
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    ctx->m_currentBody->insert(ctx->m_currentBody->begin(), nd);

    escargot::IdentifierNode* idNode = new escargot::IdentifierNode(((escargot::IdentifierNode *)id)->name());
    idNode->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    escargot::VariableDeclaratorNode* v = new escargot::VariableDeclaratorNode(
        idNode
    );

    ctx->m_currentBody->insert(ctx->m_currentBody->begin(), v);
    ctx->m_strict = previousStrict;
    ctx->m_allowYield = previousAllowYield;

    return NULL;
}

escargot::Node* parseFunctionExpression(ParseContext* ctx)
{
    /*
    var token, id = null, stricted, firstRestricted, message, tmp,
        params = [], defaults = [], body, previousStrict, node = new Node(),
        isGenerator, previousAllowYield;
     */

    bool previousAllowYield = ctx->m_allowYield;

    expectKeyword(ctx, u"function");

    bool isGenerator = match(ctx, '*');
    if (isGenerator) {
        lex(ctx);
    }

    ctx->m_allowYield = !isGenerator;
    escargot::Node* id = nullptr;
    RefPtr<ParseStatus> firstRestricted;
    if (!match(ctx, '(')) {
        RefPtr<ParseStatus> token = ctx->m_lookahead;
        id = (!ctx->m_strict && !isGenerator && matchKeyword(ctx, u"yield")) ? parseNonComputedProperty(ctx) : parseVariableIdentifier(ctx);
        ASSERT(id->type() == escargot::NodeType::Identifier);
        if (ctx->m_strict) {
            if (isRestrictedWord(token->m_value)) {
                // tolerateUnexpectedToken(token, Messages.StrictFunctionName);
                tolerateUnexpectedToken();
            }
        } else {
            if (isRestrictedWord(token->m_value)) {
                firstRestricted = token;
                // message = Messages.StrictFunctionName;
            } else if (isStrictModeReservedWord(token->m_value)) {
                firstRestricted = token;
                // message = Messages.StrictReservedWord;
            }
        }
    }

    escargot::InternalAtomicStringVector params = parseParams(ctx/*, firstRestricted*/);
    /*
    tmp = parseParams(firstRestricted);
    params = tmp.params;
    defaults = tmp.defaults;
    stricted = tmp.stricted;
    firstRestricted = tmp.firstRestricted;
    if (tmp.message) {
        message = tmp.message;
    }
    */

    bool previousStrict = ctx->m_strict;
    escargot::Node* body = parseFunctionSourceElements(ctx);
    /*
    if (strict && firstRestricted) {
        throwUnexpectedToken(firstRestricted, message);
    }
    if (strict && stricted) {
        tolerateUnexpectedToken(stricted, message);
    }
     */

    // return node.finishFunctionExpression(id, params, defaults, body, isGenerator);
    escargot::Node* nd;
    if (id)
        nd = new escargot::FunctionExpressionNode(((escargot::IdentifierNode *)id)->name(),
            std::move(params), body, isGenerator, true, ctx->m_strict);
    else
        nd = new escargot::FunctionExpressionNode(escargot::strings->emptyString,
            std::move(params), body, isGenerator, true, ctx->m_strict);
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);

    ctx->m_strict = previousStrict;
    ctx->m_allowYield = previousAllowYield;
    return nd;
}


/*
escargot::Node* parseVariableIdentifier(ParseContext* ctx)
{
    ParseStatus* token = lex(ctx);

    if (token->m_type == Token::KeywordToken && token->m_value == u"yield") {
        if (ctx->m_strict) {
            tolerateUnexpectedToken();
        }
        tolerateUnexpectedToken();
        if (!ctx->m_allowYield) {
            throwUnexpectedToken();
        }
    } else if (token->m_type != Token::IdentifierToken) {
        if (ctx->m_strict && token->m_type == Token::KeywordToken && isStrictModeReservedWord(token->m_value)) {
            tolerateUnexpectedToken();
        } else {
            throwUnexpectedToken();
        }
    } else if (sourceType === 'module' && token.type === Token.Identifier && token.value === 'await') {
        tolerateUnexpectedToken(token);
    }

    return new escargot::IdentifierNode(escargot::InternalAtomicString(token->m_value));
}
*/

escargot::Node* parsePattern(ParseContext* ctx, std::vector<RefPtr<ParseStatus> >& params);
escargot::Node* parseLeftHandSideExpression(ParseContext* ctx);
escargot::Node* parseNonComputedProperty(ParseContext* ctx);

escargot::Node* parsePatternWithDefault(ParseContext* ctx, std::vector<RefPtr<ParseStatus> >& params)
{
    // RefPtr<ParseStatus> startToken = ctx->m_lookahead;
    escargot::Node* pattern;
    escargot::Node* right;
    bool previousAllowYield;
    pattern = parsePattern(ctx, params);
    if (match(ctx, '=')) {
        lex(ctx);
        previousAllowYield = ctx->m_allowYield;
        ctx->m_allowYield = true;
        right = isolateCoverGrammar(ctx, parseAssignmentExpression);
        ctx->m_allowYield = previousAllowYield;
        // pattern = new WrappingNode(startToken).finishAssignmentPattern(pattern, right);
        pattern = new escargot::AssignmentExpressionSimpleNode(pattern, right);
        pattern->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    }
    return pattern;
}

escargot::Node*  parseArrayPattern(ParseContext* ctx, std::vector<RefPtr<ParseStatus> >& params)
{
    // var node = new Node(), elements = [], rest, restNode;
    expect(ctx, '[');
    escargot::ExpressionNodeVector elements;
    while (!match(ctx, ']')) {
        if (match(ctx, ',')) {
            lex(ctx);
            elements.push_back(NULL);
        } else {
            if (match(ctx, u"...")) {
                // TODO implement rest
                // https://developer.mozilla.org/ko/docs/Web/JavaScript/Reference/Functions/rest_parameters
                RELEASE_ASSERT_NOT_REACHED();
                break;
            } else {
                elements.push_back(parsePatternWithDefault(ctx, params));
            }
            if (!match(ctx, ']')) {
                expect(ctx, ',');
            }
        }

    }

    expect(ctx, ']');

    escargot::Node* nd = new escargot::ArrayExpressionNode(std::move(elements));
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

// ECMA-262 12.2.5 Array Initializer

escargot::Node* parseArrayInitializer(ParseContext* ctx)
{
    // var elements = [], node = new Node(), restSpread;
    escargot::ExpressionNodeVector elements;

    expect(ctx, '[');

    while (!match(ctx, ']')) {
        if (match(ctx, ',')) {
            lex(ctx);
            elements.push_back(NULL);
        } else if (match(ctx, u"...")) {
            RELEASE_ASSERT_NOT_REACHED();

            /*
            restSpread = new Node();
            lex();
            restSpread.finishSpreadElement(inheritCoverGrammar(parseAssignmentExpression));

            if (!match(']')) {
                isAssignmentTarget = isBindingElement = false;
                expect(',');
            }
            elements.push(restSpread);
             */

        } else {
            elements.push_back(inheritCoverGrammar(ctx, parseAssignmentExpression));

            if (!match(ctx, ']')) {
                expect(ctx, ',');
            }
        }
    }

    lex(ctx);

    // return node.finishArrayExpression(elements);
    escargot::Node* nd = new escargot::ArrayExpressionNode(std::move(elements));
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

escargot::Node* parseObjectPropertyKey(ParseContext* ctx)
{
    // var token, node = new Node(), expr;

    RefPtr<ParseStatus> token = lex(ctx);

    // Note: This function is called only from parseObjectProperty(), where
    // EOF and Punctuator tokens are already filtered out.
    escargot::Node* nd;
    switch (token->m_type) {
    case Token::StringLiteralToken:
        if (ctx->m_strict && token->m_octal) {
            // tolerateUnexpectedToken(token, Messages.StrictOctalLiteral);
            tolerateUnexpectedToken();
        }
        {
            escargot::InternalAtomicString str(token->m_value.data());
            nd = new escargot::LiteralNode(str.string());
            nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        }
        return nd;
    case Token::NumericLiteralToken:
        if (ctx->m_strict && token->m_octal) {
            // tolerateUnexpectedToken(token, Messages.StrictOctalLiteral);
            tolerateUnexpectedToken();
        }
        nd = new escargot::LiteralNode(escargot::ESValue(token->m_valueNumber));
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    case Token::IdentifierToken:
    case Token::BooleanLiteralToken:
    case Token::NullLiteralToken:
    case Token::KeywordToken:
        // return node.finishIdentifier(token.value);
        nd = new escargot::IdentifierNode(escargot::InternalAtomicString(token->m_value.data()));
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    case Token::PunctuatorToken:
        if (token->m_value == u"[") {
            escargot::Node* expr = isolateCoverGrammar(ctx, parseAssignmentExpression);
            expect(ctx, ']');
            return expr;
        }
        break;
    default:
        break;
    };
    throwUnexpectedToken();
    RELEASE_ASSERT_NOT_REACHED();
}

escargot::PropertyNode* parsePropertyPattern(ParseContext* ctx, std::vector<RefPtr<ParseStatus> >& params)
{
    // var node = new Node(),
    escargot::Node* key; // , keyToken,
    RefPtr<ParseStatus> keyToken;
    bool computed = match(ctx, '[');
    escargot::Node* init;
    // , init;
    if (ctx->m_lookahead->m_type == Token::IdentifierToken) {
        keyToken = ctx->m_lookahead;
        key = parseVariableIdentifier(ctx);
        if (match(ctx, '=')) {
            params.push_back(keyToken);
            lex(ctx);
            init = parseAssignmentExpression(ctx);
            // AssignmentExpressionNode(Node* left, Node* right, ESString* oper)
            escargot::Node* value = new escargot::AssignmentExpressionSimpleNode(key, init);
            value->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
            escargot::PropertyNode* nd = new escargot::PropertyNode(key, value , escargot::PropertyNode::Kind::Init);
            nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
            return nd;
            /*return node.finishProperty(
                'init', key, false,
                new WrappingNode(keyToken).finishAssignmentPattern(key, init), false, false);
             */
        } else if (!match(ctx, ':')) {
            params.push_back(keyToken);
            // return node.finishProperty('init', key, false, key, false, true);
            escargot::PropertyNode* nd = new escargot::PropertyNode(key, key , escargot::PropertyNode::Kind::Init);
            nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
            return nd;
        }
    } else {
        // key = parseObjectPropertyKey(ctx, params);
        key = parseObjectPropertyKey(ctx);
    }
    expect(ctx, ':');
    init = parsePatternWithDefault(ctx, params);
    // return node.finishProperty('init', key, computed, init, false, false);
    escargot::PropertyNode* nd = new escargot::PropertyNode(key, init , escargot::PropertyNode::Kind::Init);
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

escargot::Node* parseObjectPattern(ParseContext* ctx, std::vector<RefPtr<ParseStatus> >& params)
{
    // var node = new Node(), properties = [];
    escargot::PropertiesNodeVector properties;
    expect(ctx, '{');

    while (!match(ctx, '}')) {
        properties.push_back(parsePropertyPattern(ctx, params));
        if (!match(ctx, '}')) {
            expect(ctx, ',');
        }
    }

    lex(ctx);

    escargot::Node* nd = new escargot::ObjectExpressionNode(std::move(properties));
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

escargot::Node* parsePattern(ParseContext* ctx, std::vector<RefPtr<ParseStatus> >& params)
{
    if (match(ctx, '[')) {
        return parseArrayPattern(ctx, params);
    } else if (match(ctx, '{')) {
        return parseObjectPattern(ctx, params);
    }
    params.push_back(ctx->m_lookahead);
    return parseVariableIdentifier(ctx);
}

struct ScanRegExpBodyResult {
    std::u16string m_value;
    std::u16string m_literal;
};

ScanRegExpBodyResult* scanRegExpBody(ParseContext* ctx)
{
    char16_t ch;
    std::u16string str;
    // , str, classMarker, terminated, body;

    ch = ctx->m_source[ctx->m_index];
    // assert(ch === '/', 'Regular expression literal must start with a slash');
    ASSERT(ch == '/');
    str = ctx->m_source[ctx->m_index++];

    bool classMarker = false;
    bool terminated = false;
    while (ctx->m_index < ctx->m_length) {
        ch = ctx->m_source[ctx->m_index++];
        str += ch;
        if (ch == '\\') {
            ch = ctx->m_source[ctx->m_index++];
            // ECMA-262 7.8.5
            if (isLineTerminator(ch)) {
                // throwUnexpectedToken(null, Messages.UnterminatedRegExp);
                throwUnexpectedToken();
            }
            str += ch;
        } else if (isLineTerminator(ch)) {
            // throwUnexpectedToken(null, Messages.UnterminatedRegExp);
            throwUnexpectedToken();
        } else if (classMarker) {
            if (ch == ']') {
                classMarker = false;
            }
        } else {
            if (ch == '/') {
                terminated = true;
                break;
            } else if (ch == '[') {
                classMarker = true;
            }
        }
    }

    if (!terminated) {
        // throwUnexpectedToken(null, Messages.UnterminatedRegExp);
        throwUnexpectedToken();
    }

    // Exclude leading and trailing slash.
    std::u16string body = str.substr(1, str.length() - 2);
    ScanRegExpBodyResult* result = new ScanRegExpBodyResult();
    result->m_value = std::move(body);
    result->m_literal = std::move(str);
    return result;
    /*
    return {
        value: std::move(body),
                literal: str
    };
    */
}

struct ScanRegExpFlagsResult {
    std::u16string m_value;
    std::u16string m_literal;
};


ScanRegExpFlagsResult* scanRegExpFlags(ParseContext* ctx)
{
    // var ch, str, flags, restore;
    char16_t ch;

    std::u16string str;
    std::u16string flags;
    size_t restore;
    while (ctx->m_index < ctx->m_length) {
        ch = ctx->m_source[ctx->m_index];
        if (!isIdentifierPart(ch)) {
            break;
        }

        ++ctx->m_index;
        if (ch == '\\' && ctx->m_index < ctx->m_length) {
            ch = ctx->m_source[ctx->m_index];
            if (ch == 'u') {
                ++ctx->m_index;
                restore = ctx->m_index;
                ch = scanHexEscape(ctx, 'u');
                if (ch) {
                    flags += ch;
                    for (str += u"\\u"; restore < ctx->m_index; ++restore) {
                        str += ctx->m_source[restore];
                    }
                } else {
                    ctx->m_index = restore;
                    flags += 'u';
                    str += u"\\u";
                }
                tolerateUnexpectedToken();
            } else {
                str += '\\';
                tolerateUnexpectedToken();
            }
        } else {
            flags += ch;
            str += ch;
        }
    }

    ScanRegExpFlagsResult* result = new ScanRegExpFlagsResult();
    result->m_value = std::move(flags);
    result->m_literal = std::move(str);
    return result;
    /*
    return {
        value: flags,
        literal: str
    };
    */
}

PassRefPtr<ParseStatus> scanRegExp(ParseContext* ctx)
{
    ctx->m_scanning = true;
    // var start, body, flags, value;
    size_t start;

    ctx->m_lookahead = NULL;
    skipComment(ctx);
    start = ctx->m_index;

    ScanRegExpBodyResult* body = scanRegExpBody(ctx);
    ScanRegExpFlagsResult* flags = scanRegExpFlags(ctx);
    // value = testRegExp(body.value, flags.value);
    ctx->m_scanning = false;
    /*
     if (extra.tokenize) {
        return {
            type: Token.RegularExpression,
            value: value,
            regex: {
                pattern: body.value,
                flags: flags.value
            },
            lineNumber: lineNumber,
            lineStart: lineStart,
            start: start,
            end: index
        };
    }
    */

    ParseStatus* ps = new ParseStatus();
    ps->m_regexBody = std::move(body->m_value);
    ps->m_regexFlag = std::move(flags->m_value);
    delete body;
    delete flags;
    // ps->m_value = value;
    ps->m_start = start;
    ps->m_end = ctx->m_index;
    /*
    return {
        literal: body.literal + flags.literal,
        value: value,
        regex: {
            pattern: body.value,
            flags: flags.value
        },
        start: start,
        end: index
    };
    */
    return adoptRef(ps);
}

bool lookaheadPropertyName(ParseContext* ctx)
{
    switch (ctx->m_lookahead->m_type) {
    case Token::IdentifierToken:
    case Token::StringLiteralToken:
    case Token::BooleanLiteralToken:
    case Token::NullLiteralToken:
    case Token::NumericLiteralToken:
    case Token::KeywordToken:
        return true;
    case Token::PunctuatorToken:
        return ctx->m_lookahead->m_value == u"[";
    default:
        return false;
    }
}

escargot::Node* parsePropertyFunction(ParseContext* ctx, escargot::InternalAtomicStringVector& vec/*paramInfo, isGenerator*/)
{
    // var previousStrict, body;

    ctx->m_isAssignmentTarget = ctx->m_isBindingElement = false;

    bool previousStrict = ctx->m_strict;
    escargot::Node* body = isolateCoverGrammar(ctx, parseFunctionSourceElements);

    /*
    // TODO
    if (strict && paramInfo.firstRestricted) {
        tolerateUnexpectedToken(paramInfo.firstRestricted, paramInfo.message);
    }
    if (strict && paramInfo.stricted) {
        tolerateUnexpectedToken(paramInfo.stricted, paramInfo.message);
    }
    */

    // return node.finishFunctionExpression(null, paramInfo.params, paramInfo.defaults, body, isGenerator);
    escargot::Node* nd = new escargot::FunctionExpressionNode(escargot::strings->emptyString, std::move(vec), body, false, true, ctx->m_strict);
    ctx->m_strict = previousStrict;
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

// This function is to try to parse a MethodDefinition as defined in 14.3. But in the case of object literals,
// it might be called at a position where there is in fact a short hand identifier pattern or a data property.
// This can only be determined after we consumed up to the left parentheses.
// 
// In order to avoid back tracking, it returns `null` if the position is not a MethodDefinition and the caller
// is responsible to visit other options.
escargot::Node* tryParseMethodDefinition(ParseContext* ctx, ParseStatus* token, escargot::Node* key, bool computed)
{
    bool previousAllowYield = ctx->m_allowYield;
    // var value, options, methodNode, params,
    // previousAllowYield = state.allowYield;

    if (token->m_type == Token::IdentifierToken) {
        // check for `get` and `set`;

        if (token->m_value == u"get" && lookaheadPropertyName(ctx)) {
            computed = match(ctx, '[');
            key = parseObjectPropertyKey(ctx);
            // methodNode = new Node();
            expect(ctx, '(');
            expect(ctx, ')');

            ctx->m_allowYield = false;
            escargot::InternalAtomicStringVector vec;
            escargot::Node* value = parsePropertyFunction(ctx, vec
                /*
                {
                    params: [],
                    defaults: [],
                    stricted: null,
                    firstRestricted: null,
                    message: null
                }, false
                */
                );
            ctx->m_allowYield = previousAllowYield;

            // return node.finishProperty('get', key, computed, value, false, false);
            escargot::Node* nd = new escargot::PropertyNode(key, value, escargot::PropertyNode::Get);
            nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
            return nd;
        } else if (token->m_value == u"set" && lookaheadPropertyName(ctx)) {
            computed = match(ctx, '[');
            key = parseObjectPropertyKey(ctx);
            // methodNode = new Node();
            expect(ctx, '(');
            /*
            options = {
                params: [],
                defaultCount: 0,
                defaults: [],
                firstRestricted: null,
                paramSet: {}
            };
            */
            escargot::InternalAtomicStringVector vec;
            if (match(ctx, ')')) {
                // tolerateUnexpectedToken(lookahead);
                tolerateUnexpectedToken();
            } else {
                ctx->m_allowYield = false;
                parseParam(ctx, vec);
                // parseParam(options);
                ctx->m_allowYield = previousAllowYield;
                /*
                if (options.defaultCount === 0) {
                    options.defaults = [];
                }
                */
            }
            expect(ctx, ')');

            ctx->m_allowYield = false;
            escargot::Node* value = parsePropertyFunction(ctx, vec/*methodNode, options, false*/);
            ctx->m_allowYield = previousAllowYield;

            // return node.finishProperty('set', key, computed, value, false, false);
            escargot::Node* nd = new escargot::PropertyNode(key, value, escargot::PropertyNode::Set);
            nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
            return nd;
        }
    } else if (token->m_type == Token::PunctuatorToken && token->m_value == u"*" && lookaheadPropertyName(ctx)) {
        computed = match(ctx, '[');
        key = parseObjectPropertyKey(ctx);
        // methodNode = new Node();

        ctx->m_allowYield = false;
        escargot::InternalAtomicStringVector params = parseParams(ctx);
        ctx->m_allowYield = previousAllowYield;

        ctx->m_allowYield = false;
        escargot::Node* value = parsePropertyFunction(ctx, params);
        ctx->m_allowYield = previousAllowYield;

        escargot::Node* nd = new escargot::PropertyNode(key, value, escargot::PropertyNode::Init);
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
        // return node.finishProperty('init', key, computed, value, true, false);
    }

    if (key && match(ctx, '(')) {
        // escargot::Node* value = parsePropertyMethodFunction(ctx);
        // return node.finishProperty('init', key, computed, value, true, false);
        RELEASE_ASSERT_NOT_REACHED();
    }

    // Not a MethodDefinition.
    return NULL;
}

escargot::Node* parseObjectProperty(ParseContext* ctx, bool& hasProto)
{
    RefPtr<ParseStatus> token = ctx->m_lookahead;
    // , node = new Node(), computed, key, maybeMethod, proto, value;
    bool proto;
    escargot::Node* key;
    bool computed = match(ctx, '[');
    if (match(ctx, '*')) {
        lex(ctx);
    } else {
        key = parseObjectPropertyKey(ctx);
    }

    escargot::Node* maybeMethod = tryParseMethodDefinition(ctx, token.get(), key, computed);
    if (maybeMethod) {
        return maybeMethod;
    }

    if (!key) {
        throwUnexpectedToken();
        // throwUnexpectedToken(lookahead);
    }

    // Check for duplicated __proto__
    if (!computed) {
        // proto = (key.type === Syntax.Identifier && key.name === '__proto__') ||
        // (key.type === Syntax.Literal && key.value === '__proto__');
        proto = (key->type() == escargot::NodeType::Identifier && *((escargot::IdentifierNode *)key)->nonAtomicName() == u"__proto__") ||
            (key->type() == escargot::NodeType::Literal && ((escargot::LiteralNode *)key)->value().equalsTo(escargot::ESString::create(u"__proto__")));
        if (hasProto && proto) {
            // tolerateError(Messages.DuplicateProtoProperty);
            tolerateError(u"Messages.DuplicateProtoProperty");
        }
        hasProto |= proto;
    }

    if (match(ctx, ':')) {
        lex(ctx);
        escargot::Node* value = inheritCoverGrammar(ctx, parseAssignmentExpression);
        // return node.finishProperty('init', key, computed, value, false, false);
        escargot::Node* nd = new escargot::PropertyNode(key, value, escargot::PropertyNode::Kind::Init);
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    }

    if (token->m_type == Token::IdentifierToken) {
        if (match(ctx, '=')) {
            ctx->m_firstCoverInitializedNameError = ctx->m_lookahead;
            lex(ctx);
            escargot::Node* value = isolateCoverGrammar(ctx, parseAssignmentExpression);
            // return node.finishProperty('init', key, computed,
            // new WrappingNode(token).finishAssignmentPattern(key, value), false, true);
            escargot::Node* nd = new escargot::PropertyNode(key, new escargot::AssignmentExpressionSimpleNode(key, value), escargot::PropertyNode::Kind::Init);
            nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
            return nd;
        }
        // return node.finishProperty('init', key, computed, key, false, true);
        escargot::Node* nd = new escargot::PropertyNode(key, key, escargot::PropertyNode::Kind::Init);
        nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        return nd;
    }

    // throwUnexpectedToken(lookahead);
    throwUnexpectedToken();
    RELEASE_ASSERT_NOT_REACHED();
}

escargot::Node* parseObjectInitializer(ParseContext* ctx)
{
    // var properties = [], hasProto = {value: false}, node = new Node();
    escargot::PropertiesNodeVector properties;
    bool hasProto = false;
    expect(ctx, '{');

    while (!match(ctx, '}')) {
        properties.push_back((escargot::PropertyNode *)parseObjectProperty(ctx, hasProto));

        if (!match(ctx, '}')) {
            expectCommaSeparator(ctx);
        }
    }

    expect(ctx, '}');

    // return node.finishObjectExpression(properties);
    escargot::Node* nd = new escargot::ObjectExpressionNode(std::move(properties));
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

void reinterpretExpressionAsPattern(ParseContext* ctx, escargot::Node* expr)
{
    /*
    var i;
    switch (expr.type) {
    case Syntax.Identifier:
    case Syntax.MemberExpression:
    case Syntax.RestElement:
    case Syntax.AssignmentPattern:
        break;
    case Syntax.SpreadElement:
        expr.type = Syntax.RestElement;
        reinterpretExpressionAsPattern(expr.argument);
        break;
    case Syntax.ArrayExpression:
        expr.type = Syntax.ArrayPattern;
        for (i = 0; i < expr.elements.length; i++) {
            if (expr.elements[i] !== null) {
                reinterpretExpressionAsPattern(expr.elements[i]);
            }
        }
        break;
    case Syntax.ObjectExpression:
        expr.type = Syntax.ObjectPattern;
        for (i = 0; i < expr.properties.length; i++) {
            reinterpretExpressionAsPattern(expr.properties[i].value);
        }
        break;
    case Syntax.AssignmentExpression:
        expr.type = Syntax.AssignmentPattern;
        reinterpretExpressionAsPattern(expr.left);
        break;
    default:
        // Allow other node type for tolerant parsing.
        break;
    }
    */
}

// ECMA-262 12.2.9 Template Literals

void parseTemplateElement(ParseContext* ctx/*, option*/)
{
    RELEASE_ASSERT_NOT_REACHED();

    /*
    var node, token;

    if (lookahead.type !== Token.Template || (option.head && !lookahead.head)) {
        throwUnexpectedToken();
    }

    node = new Node();
    token = lex();

    return node.finishTemplateElement({ raw: token.value.raw, cooked: token.value.cooked }, token.tail);
    */
}

escargot::Node* parseTemplateLiteral(ParseContext* ctx)
{
    RELEASE_ASSERT_NOT_REACHED();
    /*
    var quasi, quasis, expressions, node = new Node();

    quasi = parseTemplateElement({ head: true });
    quasis = [ quasi ];
    expressions = [];

    while (!quasi.tail) {
        expressions.push(parseExpression());
        quasi = parseTemplateElement({ head: false });
        quasis.push(quasi);
    }

    return node.finishTemplateLiteral(quasis, expressions);
     */
}

// ECMA-262 12.2.10 The Grouping Operator

escargot::Node* parseGroupExpression(ParseContext* ctx)
{
    expect(ctx, '(');

    if (match(ctx, ')')) {
        // arrow function
        RELEASE_ASSERT_NOT_REACHED();
    }

    RefPtr<ParseStatus> startToken = ctx->m_lookahead;
    if (match(ctx, u"...")) {
        // rest element
        RELEASE_ASSERT_NOT_REACHED();
    }

    ctx->m_isBindingElement = true;
    escargot::Node* expr = inheritCoverGrammar(ctx, parseAssignmentExpression);

    if (match(ctx, ',')) {
        escargot::ExpressionNodeVector expressions;
        ctx->m_isAssignmentTarget = false;
        expressions.push_back(expr);

        while (ctx->m_startIndex < ctx->m_length) {
            if (!match(ctx, ',')) {
                break;
            }
            lex(ctx);

            if (match(ctx, u"...")) {
                // rest element
                RELEASE_ASSERT_NOT_REACHED();
                /*
                if (!isBindingElement) {
                    throwUnexpectedToken(lookahead);
                }
                expressions.push(parseRestElement(params));
                expect(')');
                if (!match('=>')) {
                    expect('=>');
                }
                isBindingElement = false;
                for (i = 0; i < expressions.length; i++) {
                    reinterpretExpressionAsPattern(expressions[i]);
                }
                return {
                    type: PlaceHolders.ArrowParameterPlaceHolder,
                    params: expressions
                };
                */
            }

            expressions.push_back(inheritCoverGrammar(ctx, parseAssignmentExpression));
        }

        expr = new escargot::SequenceExpressionNode(std::move(expressions));
        expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    }


    expect(ctx, ')');

    if (match(ctx, u"=>")) {
        // arrow function
        RELEASE_ASSERT_NOT_REACHED();
        /*
        if (expr.type === Syntax.Identifier && expr.name === 'yield') {
            return {
                type: PlaceHolders.ArrowParameterPlaceHolder,
                params: [expr]
            };
        }

        if (!isBindingElement) {
            throwUnexpectedToken(lookahead);
        }

        if (expr.type === Syntax.SequenceExpression) {
            for (i = 0; i < expr.expressions.length; i++) {
                reinterpretExpressionAsPattern(expr.expressions[i]);
            }
        } else {
            reinterpretExpressionAsPattern(expr);
        }

        expr = {
            type: PlaceHolders.ArrowParameterPlaceHolder,
            params: expr.type === Syntax.SequenceExpression ? expr.expressions : [expr]
        };
        */
    }
    ctx->m_isBindingElement = false;
    return expr;

    /*
    var expr, expressions, startToken, i, params = [];

    expect(ctx, {'('});

    if (match(ctx, {')'})) {
        lex(ctx);
        if (!match(ctx. {'=>'})) {
            expect(ctx, {'=>'});
        }
        return {
            type: PlaceHolders.ArrowParameterPlaceHolder,
            params: [],
            rawParams: []
        };
    }

    startToken = lookahead;
    if (match('...')) {
        expr = parseRestElement(params);
        expect(')');
        if (!match('=>')) {
            expect('=>');
        }
        return {
            type: PlaceHolders.ArrowParameterPlaceHolder,
            params: [expr]
        };
    }

    isBindingElement = true;
    expr = inheritCoverGrammar(parseAssignmentExpression);

    if (match(',')) {
        isAssignmentTarget = false;
        expressions = [expr];

        while (startIndex < length) {
            if (!match(',')) {
                break;
            }
            lex();

            if (match('...')) {
                if (!isBindingElement) {
                    throwUnexpectedToken(lookahead);
                }
                expressions.push(parseRestElement(params));
                expect(')');
                if (!match('=>')) {
                    expect('=>');
                }
                isBindingElement = false;
                for (i = 0; i < expressions.length; i++) {
                    reinterpretExpressionAsPattern(expressions[i]);
                }
                return {
                    type: PlaceHolders.ArrowParameterPlaceHolder,
                    params: expressions
                };
            }

            expressions.push(inheritCoverGrammar(parseAssignmentExpression));
        }

        expr = new WrappingNode(startToken).finishSequenceExpression(expressions);
    }


    expect(')');

    if (match('=>')) {
        if (expr.type === Syntax.Identifier && expr.name === 'yield') {
            return {
                type: PlaceHolders.ArrowParameterPlaceHolder,
                params: [expr]
            };
        }

        if (!isBindingElement) {
            throwUnexpectedToken(lookahead);
        }

        if (expr.type === Syntax.SequenceExpression) {
            for (i = 0; i < expr.expressions.length; i++) {
                reinterpretExpressionAsPattern(expr.expressions[i]);
            }
        } else {
            reinterpretExpressionAsPattern(expr);
        }

        expr = {
            type: PlaceHolders.ArrowParameterPlaceHolder,
            params: expr.type === Syntax.SequenceExpression ? expr.expressions : [expr]
        };
    }
    isBindingElement = false;
    return expr;
     */

}

// ECMA-262 12.2 Primary Expressions

escargot::Node* parsePrimaryExpression(ParseContext* ctx)
{
    // var type, token, expr, node;
    RefPtr<ParseStatus> token;
    escargot::Node* expr;
    escargot::Node* node;

    if (match(ctx, '(')) {
        ctx->m_isBindingElement = false;
        return inheritCoverGrammar(ctx, parseGroupExpression);
    }

    if (match(ctx, '[')) {
        return inheritCoverGrammar(ctx, parseArrayInitializer);
    }

    if (match(ctx, '{')) {
        return inheritCoverGrammar(ctx, parseObjectInitializer);
    }

    Token type = ctx->m_lookahead->m_type;
    // node = new Node();

    if (type == Token::IdentifierToken) {
        /*
         if (sourceType === 'module' && lookahead.value === 'await') {
        tolerateUnexpectedToken(lookahead);
        }*/
        expr = new escargot::IdentifierNode(escargot::InternalAtomicString(lex(ctx)->m_value.data()));
        expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        // expr = node.finishIdentifier(lex().value);
    } else if (type == Token::StringLiteralToken || type == Token::NumericLiteralToken) {
        ctx->m_isAssignmentTarget = ctx->m_isBindingElement = false;
        if (ctx->m_strict && ctx->m_lookahead->m_octal) {
            // tolerateUnexpectedToken(lookahead, Messages.StrictOctalLiteral);
            tolerateUnexpectedToken();
        }
        expr = finishLiteralNode(ctx, lex(ctx));
    } else if (type == Token::KeywordToken) {
        if (!ctx->m_strict && ctx->m_allowYield && matchKeyword(ctx, u"yield")) {
            return parseNonComputedProperty(ctx);
        }
        ctx->m_isAssignmentTarget = ctx->m_isBindingElement = false;
        if (matchKeyword(ctx, u"function")) {
            return parseFunctionExpression(ctx);
        }
        if (matchKeyword(ctx, u"this")) {
            lex(ctx);
            escargot::Node* nd = new escargot::ThisExpressionNode();
            nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
            return nd;
        }
        if (matchKeyword(ctx, u"class")) {
            RELEASE_ASSERT_NOT_REACHED();
            // return parseClassExpression();
        }
        // throwUnexpectedToken(lex());
        throwUnexpectedToken();
    } else if (type == Token::BooleanLiteralToken) {
        ctx->m_isAssignmentTarget = ctx->m_isBindingElement = false;
        token = lex(ctx);
        bool token_value = (token->m_value == u"true");
        expr = new escargot::LiteralNode(escargot::ESValue(token_value));
        expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    } else if (type == Token::NullLiteralToken) {
        ctx->m_isAssignmentTarget = ctx->m_isBindingElement = false;
        token = lex(ctx);
        expr = new escargot::LiteralNode(escargot::ESValue(escargot::ESValue::ESNull));
        expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    } else if (match(ctx, '/') || match(ctx, u"/=")) {
        ctx->m_isAssignmentTarget = ctx->m_isBindingElement = false;
        ctx->m_index = ctx->m_startIndex;
        /*
        if (typeof extra.tokens !== 'undefined') {
            token = collectRegex(ctx);
        } else {
            token = scanRegExp(ctx);
        }
        */
        token = scanRegExp(ctx);
        lex(ctx);
        // expr = node.finishLiteral(token);
        int f = 0;
        if (token->m_regexFlag.find('i') != std::u16string::npos) {
            f = f | escargot::ESRegExpObject::IgnoreCase;
        }
        if (token->m_regexFlag.find('g') != std::u16string::npos) {
            f = f | escargot::ESRegExpObject::Global;
        }
        if (token->m_regexFlag.find('m') != std::u16string::npos) {
            f = f | escargot::ESRegExpObject::MultiLine;
        }
        /*
        if (flag & JSREG_STICKY) {
            f = f | ESRegExpObject::Sticky;
        }
        */
        expr = new escargot::LiteralNode(escargot::ESRegExpObject::create(
            escargot::ESString::create(token->m_regexBody.data()), (escargot::ESRegExpObject::Option)f));
        expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        // parsedNode = new LiteralNode(ESRegExpObject::create(source, (escargot::ESRegExpObject::Option)f, escargot::ESVMInstance::currentInstance()->globalObject()->regexpPrototype()));
    } else if (type == Token::TemplateToken) {
        expr = parseTemplateLiteral(ctx);
    } else {
        // throwUnexpectedToken(lex());
        throwUnexpectedToken();
    }

    return expr;
}

// ECMA-262 12.3 Left-Hand-Side Expressions

escargot::ArgumentVector parseArguments(ParseContext* ctx)
{
    // var args = [], expr;
    escargot::Node* expr;

    escargot::ArgumentVector args;

    expect(ctx, '(');

    if (!match(ctx, ')')) {
        while (ctx->m_startIndex < ctx->m_length) {
            if (match(ctx, u"...")) {
                /*
                expr = new Node();
                lex();
                expr.finishSpreadElement(isolateCoverGrammar(parseAssignmentExpression));
                 */
                RELEASE_ASSERT_NOT_REACHED();
            } else {
                expr = isolateCoverGrammar(ctx, parseAssignmentExpression);
            }
            args.push_back(expr);
            if (match(ctx, ')')) {
                break;
            }
            expectCommaSeparator(ctx);
        }
    }

    expect(ctx, ')');

    return args;
}

escargot::Node* parseNonComputedProperty(ParseContext* ctx)
{
    // var token, node = new Node();

    RefPtr<ParseStatus> token = lex(ctx);

    if (!isIdentifierName(token.get())) {
        throwUnexpectedToken();
    }

    // return node.finishIdentifier(token.value);
    escargot::Node* nd = new escargot::IdentifierNode(escargot::InternalAtomicString(token->m_value.data()));
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

escargot::Node* parseNonComputedMember(ParseContext* ctx)
{
    expect(ctx, '.');

    return parseNonComputedProperty(ctx);
}

escargot::Node* parseComputedMember(ParseContext* ctx)
{
    escargot::Node* expr;

    expect(ctx, '[');

    expr = isolateCoverGrammar(ctx, parseExpression);

    expect(ctx, ']');

    return expr;
}

// ECMA-262 12.3.3 The new Operator

escargot::Node* parseNewExpression(ParseContext* ctx)
{
    // var callee, args, node = new Node();
    escargot::Node* callee;

    expectKeyword(ctx, u"new");

    if (match(ctx, '.')) {
        lex(ctx);
        if (ctx->m_lookahead->m_type == Token::IdentifierToken && ctx->m_lookahead->m_value == u"target") {
            if (ctx->m_inFunctionBody) {
                lex(ctx);
                RELEASE_ASSERT_NOT_REACHED();
                // return node.finishMetaProperty('new', 'target');
            }
        }
        throwUnexpectedToken();
    }

    callee = isolateCoverGrammar(ctx, parseLeftHandSideExpression);
    escargot::ArgumentVector args;
    if (match(ctx, '(')) {
        args = parseArguments(ctx);
    }

    ctx->m_isAssignmentTarget = ctx->m_isBindingElement = false;

    // return node.finishNewExpression(callee, args);
    escargot::Node* nd = new escargot::NewExpressionNode(callee, std::move(args));
    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

// ECMA-262 12.3.4 Function Calls

escargot::Node* parseLeftHandSideExpressionAllowCall(ParseContext* ctx)
{
    // var quasi, expr, args, property, startToken, ;
    escargot::Node* quasi;
    escargot::Node* expr;
    escargot::Node* property;
    escargot::ArgumentVector args;
    bool previousAllowIn = ctx->m_allowIn;

    // RefPtr<ParseStatus> startToken = ctx->m_lookahead;
    ctx->m_allowIn = true;

    if (matchKeyword(ctx, u"super") && ctx->m_inFunctionBody) {
        RELEASE_ASSERT_NOT_REACHED();
        /*
        expr = new Node();
        lex();
        expr = expr.finishSuper();
        if (!match('(') && !match('.') && !match('[')) {
            throwUnexpectedToken(lookahead);
        }*/
    } else {
        expr = inheritCoverGrammar(ctx, matchKeyword(ctx, u"new") ? parseNewExpression : parsePrimaryExpression);
    }

    for (;;) {
        if (match(ctx, '.')) {
            ctx->m_isBindingElement = false;
            ctx->m_isAssignmentTarget = true;
            property = parseNonComputedMember(ctx);
            // expr = new WrappingNode(startToken).finishMemberExpression('.', expr, property);
            ASSERT(property->type() == escargot::NodeType::Identifier);
            expr = new escargot::MemberExpressionNode(expr, property, false);
            expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        } else if (match(ctx, '(')) {
            ctx->m_isBindingElement = false;
            ctx->m_isAssignmentTarget = false;
            args = parseArguments(ctx);
            // expr = new WrappingNode(startToken).finishCallExpression(expr, args);
            expr = new escargot::CallExpressionNode(expr, std::move(args));
            expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        } else if (match(ctx, '[')) {
            ctx->m_isBindingElement = false;
            ctx->m_isAssignmentTarget = true;
            property = parseComputedMember(ctx);
            // expr = new WrappingNode(startToken).finishMemberExpression('[', expr, property);
            expr = new escargot::MemberExpressionNode(expr, property, true);
            expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        } else if (ctx->m_lookahead->m_type == Token::TemplateToken && ctx->m_lookahead->m_head) {
            quasi = parseTemplateLiteral(ctx);
            // expr = new WrappingNode(startToken).finishTaggedTemplateExpression(expr, quasi);
            RELEASE_ASSERT_NOT_REACHED();
        } else {
            break;
        }
    }
    ctx->m_allowIn = previousAllowIn;

    return expr;
}

// ECMA-262 12.3 Left-Hand-Side Expressions

escargot::Node* parseLeftHandSideExpression(ParseContext* ctx)
{
    // var quasi, expr, property, startToken;
    // assert(state.allowIn, 'callee of new expression always allow in keyword.');
    ASSERT(ctx->m_allowIn);
    escargot::Node* expr;
    escargot::Node* quasi;
    escargot::Node* property;
    // RefPtr<ParseStatus> startToken = ctx->m_lookahead;

    if (matchKeyword(ctx, u"super") && ctx->m_inFunctionBody) {
        RELEASE_ASSERT_NOT_REACHED();
        /*
        expr = new Node();
        lex();
        expr = expr.finishSuper();
        if (!match('[') && !match('.')) {
            throwUnexpectedToken(lookahead);
        }
        */
    } else {
        expr = inheritCoverGrammar(ctx, matchKeyword(ctx, u"new") ? parseNewExpression : parsePrimaryExpression);
    }

    for (;;) {
        if (match(ctx, '[')) {
            ctx->m_isBindingElement = false;
            ctx->m_isAssignmentTarget = true;
            property = parseComputedMember(ctx);
            // expr = new WrappingNode(startToken).finishMemberExpression('[', expr, property);
            // computed = accessor === '[';
            expr = new escargot::MemberExpressionNode(expr, property, true);
            expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        } else if (match(ctx, '.')) {
            ctx->m_isBindingElement = false;
            ctx->m_isAssignmentTarget = true;
            property = parseNonComputedMember(ctx);
            // expr = new WrappingNode(startToken).finishMemberExpression('.', expr, property);
            ASSERT(property->type() == escargot::NodeType::Identifier);
            expr = new escargot::MemberExpressionNode(expr, property, false);
            expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        } else if (ctx->m_lookahead->m_type == Token::TemplateToken && ctx->m_lookahead->m_head) {
            quasi = parseTemplateLiteral(ctx);
            RELEASE_ASSERT_NOT_REACHED();
            // expr = new WrappingNode(startToken).finishTaggedTemplateExpression(expr, quasi);
        } else {
            break;
        }
    }
    return expr;
}

// ECMA-262 12.4 Postfix Expressions

escargot::Node* parsePostfixExpression(ParseContext* ctx)
{
    // var expr, token, startToken = lookahead;
    escargot::Node* expr;

    expr = inheritCoverGrammar(ctx, parseLeftHandSideExpressionAllowCall);

    if (!ctx->m_hasLineTerminator && ctx->m_lookahead->m_type == Token::PunctuatorToken) {
        if (match(ctx, u"++") || match(ctx, u"--")) {
            // ECMA-262 11.3.1, 11.3.2
            if (ctx->m_strict && expr->type() == escargot::NodeType::Identifier && isRestrictedWord(((escargot::IdentifierNode *)expr)->nonAtomicName()->data())) {
                tolerateError(u"Messages.StrictLHSPostfix");
            }

            if (!ctx->m_isAssignmentTarget) {
                tolerateError(u"Messages.InvalidLHSInAssignment");
            }

            ctx->m_isAssignmentTarget = ctx->m_isBindingElement = false;

            RefPtr<ParseStatus> token = lex(ctx);
            // expr = new WrappingNode(startToken).finishPostfixExpression(token.value, expr);
            if (token->m_value == u"++")
                expr = new escargot::UpdateExpressionIncrementPostfixNode(expr);
            else
                expr = new escargot::UpdateExpressionDecrementPostfixNode(expr);
            expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        }
    }

    return expr;
}


// ECMA-262 12.5 Unary Operators

escargot::Node* parseUnaryExpression(ParseContext* ctx)
{
    // var token, expr, startToken;

    RefPtr<ParseStatus> token;
    // RefPtr<ParseStatus> startToken;
    escargot::Node* expr;
    if (ctx->m_lookahead->m_type != Token::PunctuatorToken && ctx->m_lookahead->m_type != Token::KeywordToken) {
        expr = parsePostfixExpression(ctx);
    } else if (match(ctx, u"++") || match(ctx, u"--")) {
        // startToken = ctx->m_lookahead;
        token = lex(ctx);
        expr = inheritCoverGrammar(ctx, parseUnaryExpression);
        // ECMA-262 11.4.4, 11.4.5
        if (ctx->m_strict && expr->type() == escargot::NodeType::Identifier && isRestrictedWord(((escargot::IdentifierNode *)expr)->nonAtomicName()->data())) {
            tolerateError(u"Messages.StrictLHSPrefix");
        }

        if (!ctx->m_isAssignmentTarget) {
            tolerateError(u"Messages.InvalidLHSInAssignment");
        }
        // expr = new WrappingNode(startToken).finishUnaryExpression(token.value, expr);
        // expr = new escargot::UnaryExpressionNode(expr, escargot::ESString::create(token->m_value.data()));
        if (token->m_value == u"++")
            expr = new escargot::UpdateExpressionIncrementPrefixNode(expr);
        else
            expr = new escargot::UpdateExpressionDecrementPrefixNode(expr);
        expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        ctx->m_isAssignmentTarget = ctx->m_isBindingElement = false;
    } else if (match(ctx, '+') || match(ctx, '-') || match(ctx, '~') || match(ctx, '!')) {
        // startToken = ctx->m_lookahead;
        token = lex(ctx);
        expr = inheritCoverGrammar(ctx, parseUnaryExpression);
        // expr = new WrappingNode(startToken).finishUnaryExpression(token.value, expr);
        if (token->m_value == u"+") {
            expr = new escargot::UnaryExpressionPlusNode(expr);
        } else if (token->m_value == u"-") {
            expr = new escargot::UnaryExpressionMinusNode(expr);
        } else if (token->m_value == u"~") {
            expr = new escargot::UnaryExpressionBitwiseNotNode(expr);
        } else if (token->m_value == u"!") {
            expr = new escargot::UnaryExpressionLogicalNotNode(expr);
        } else {
            RELEASE_ASSERT_NOT_REACHED();
        }
        expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        ctx->m_isAssignmentTarget = ctx->m_isBindingElement = false;
    } else if (matchKeyword(ctx, u"delete") || matchKeyword(ctx, u"void") || matchKeyword(ctx, u"typeof")) {
        // startToken = ctx->m_lookahead;
        token = lex(ctx);
        expr = inheritCoverGrammar(ctx, parseUnaryExpression);
        // expr = new WrappingNode(startToken).finishUnaryExpression(token.value, expr);
        if (token->m_value == u"delete") {
            expr = new escargot::UnaryExpressionDeleteNode(expr);
            expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        } else if (token->m_value == u"void") {
            expr = new escargot::UnaryExpressionVoidNode(expr);
            expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        } else if (token->m_value == u"typeof") {
            expr = new escargot::UnaryExpressionTypeOfNode(expr);
            expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        }

        // TODO
        /*
        if (ctx->m_strict && ((escargot::UnaryExpressionNode *)expr)->readOperator() == escargot::UnaryExpressionNode::Operator::Delete
                && ((escargot::UnaryExpressionNode *)expr)->argument()->type() == escargot::NodeType::Identifier) {
            tolerateError(u"Messages.StrictDelete");
        }
        */
        ctx->m_isAssignmentTarget = ctx->m_isBindingElement = false;
    } else {
        expr = parsePostfixExpression(ctx);
    }

    return expr;
}



int binaryPrecedence(ParseContext* ctx, RefPtr<ParseStatus> token, bool allowIn)
{
    if (token->m_type != Token::PunctuatorToken && token->m_type != Token::KeywordToken) {
        return 0;
    }

    if (token->m_value == u"=") {
        return 0;
    }

    if (token->m_value == u"||") {
        return 1;
    }
    if (token->m_value == u"&&") {
        return 2;
    }
    if (token->m_value == u"|") {
        return 3;
    }
    if (token->m_value == u"^") {
        return 4;
    }
    if (token->m_value == u"&") {
        return 5;
    }
    if (token->m_value == u"==") {
        return 6;
    }
    if (token->m_value == u"!=") {
        return 6;
    }
    if (token->m_value == u"===") {
        return 6;
    }
    if (token->m_value == u"!==") {
        return 6;
    }
    if (token->m_value == u"instanceof") {
        return 7;
    }
    if (token->m_value == u">") {
        return 7;
    }
    if (token->m_value == u"<") {
        return 7;
    }
    if (token->m_value == u"<=") {
        return 7;
    }
    if (token->m_value == u">=") {
        return 7;
    }
    if (token->m_value == u"in") {
        return ctx->m_allowIn ? 7 : 0;
    }
    if (token->m_value == u"<<") {
        return 8;
    }
    if (token->m_value == u">>") {
        return 8;
    }
    if (token->m_value == u">>>") {
        return 8;
    }

    if (token->m_value == u"+") {
        return 9;
    }
    if (token->m_value == u"-") {
        return 9;
    }

    if (token->m_value == u"*") {
        return 11;
    }
    if (token->m_value == u"/") {
        return 11;
    }
    if (token->m_value == u"%") {
        return 11;
    }

    return 0;
}

// ECMA-262 12.6 Multiplicative Operators
// ECMA-262 12.7 Additive Operators
// ECMA-262 12.8 Bitwise Shift Operators
// ECMA-262 12.9 Relational Operators
// ECMA-262 12.10 Equality Operators
// ECMA-262 12.11 Binary Bitwise Operators
// ECMA-262 12.12 Binary Logical Operators

escargot::Node* finishBinaryExpression(ParseContext* ctx, escargot::Node* left, escargot::Node* right, const std::u16string oper)
{
    // Additive Operators
    escargot::Node* nd;
    if (oper == u"+")
        nd = new escargot::BinaryExpressionPlusNode(left, right);
    else if (oper == u"-")
        nd = new escargot::BinaryExpressionMinusNode(left, right);

    // Bitwise Shift Operators
    else if (oper == u"<<")
        nd = new escargot::BinaryExpressionLeftShiftNode(left, right);
    else if (oper == u">>")
        nd = new escargot::BinaryExpressionSignedRightShiftNode(left, right);
    else if (oper == u">>>")
        nd = new escargot::BinaryExpressionUnsignedRightShiftNode(left, right);

    // Multiplicative Operators
    else if (oper == u"*")
        nd = new escargot::BinaryExpressionMultiplyNode(left, right);
    else if (oper == u"/")
        nd = new escargot::BinaryExpressionDivisionNode(left, right);
    else if (oper == u"%")
        nd = new escargot::BinaryExpressionModNode(left, right);

    // Relational Operators
    else if (oper == u"<")
        nd = new escargot::BinaryExpressionLessThanNode(left, right);
    else if (oper == u">")
        nd = new escargot::BinaryExpressionGreaterThanNode(left, right);
    else if (oper == u"<=")
        nd = new escargot::BinaryExpressionLessThanOrEqualNode(left, right);
    else if (oper == u">=")
        nd = new escargot::BinaryExpressionGreaterThanOrEqualNode(left, right);

    // Equality Operators
    else if (oper == u"==")
        nd = new escargot::BinaryExpressionEqualNode(left, right);
    else if (oper == u"!=")
        nd = new escargot::BinaryExpressionNotEqualNode(left, right);
    else if (oper == u"===")
        nd = new escargot::BinaryExpressionStrictEqualNode(left, right);
    else if (oper == u"!==")
        nd = new escargot::BinaryExpressionNotStrictEqualNode(left, right);

    // Binary Bitwise Operator
    else if (oper == u"&")
        nd = new escargot::BinaryExpressionBitwiseAndNode(left, right);
    else if (oper == u"^")
        nd = new escargot::BinaryExpressionBitwiseXorNode(left, right);
    else if (oper == u"|")
        nd = new escargot::BinaryExpressionBitwiseOrNode(left, right);
    else if (oper == u"||")
        nd = new escargot::BinaryExpressionLogicalOrNode(left, right);
    else if (oper == u"&&")
        nd = new escargot::BinaryExpressionLogicalAndNode(left, right);
    else if (oper == u"in")
        nd = new escargot::BinaryExpressionInNode(left, right);
    else if (oper == u"instanceof")
        nd = new escargot::BinaryExpressionInstanceOfNode(left, right);
    // TODO
    else
        RELEASE_ASSERT_NOT_REACHED();

    nd->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
    return nd;
}

escargot::Node* parseBinaryExpression(ParseContext* ctx)
{
    // var marker, markers, expr, token, prec, stack, right, operator, left, i;

    std::u16string operator_;
    RefPtr<ParseStatus> marker = ctx->m_lookahead;
    escargot::Node* left = inheritCoverGrammar(ctx, parseUnaryExpression);

    RefPtr<ParseStatus> token = ctx->m_lookahead;
    int prec = binaryPrecedence(ctx, token, ctx->m_allowIn);
    if (prec == 0) {
        return left;
    }
    ctx->m_isAssignmentTarget = ctx->m_isBindingElement = false;
    token->m_prec = prec;
    lex(ctx);

    std::vector<RefPtr<ParseStatus> > markers;
    markers.push_back(marker);
    markers.push_back(ctx->m_lookahead);
    // markers = [marker, lookahead];
    escargot::Node* right = isolateCoverGrammar(ctx, parseUnaryExpression);

    std::vector<RefPtr<ParseStatus> > refRooter;
    std::vector<void *> stack;
    // stack = [left, token, right];
    stack.push_back(left);
    stack.push_back(token.get());
    refRooter.push_back(token);
    stack.push_back(right);

    while ((prec = binaryPrecedence(ctx, ctx->m_lookahead, ctx->m_allowIn)) > 0) {

        // Reduce: make a binary expression from the three topmost entries.
        while ((stack.size() > 2) && (prec <= ((ParseStatus *)stack[stack.size() - 2])->m_prec)) {
            right = (escargot::Node *)stack[stack.size()-1];
            stack.pop_back();
            // right = stack.pop_back();
            operator_ = ((ParseStatus*)stack[stack.size()-1])->m_value.data();
            stack.pop_back();
            // operator = stack.pop().value;
            left = (escargot::Node *)stack[stack.size()-1];
            stack.pop_back();
            // left = stack.pop();
            markers.pop_back();
            // expr = new WrappingNode(markers[markers.length - 1]).finishBinaryExpression(operator, left, right);
            escargot::Node* expr = finishBinaryExpression(ctx, left, right, operator_);
            stack.push_back(expr);
        }

        // Shift.
        token = lex(ctx);
        token->m_prec = prec;
        stack.push_back(token.get());
        refRooter.push_back(token);
        markers.push_back(ctx->m_lookahead);
        escargot::Node* expr = isolateCoverGrammar(ctx, parseUnaryExpression);
        stack.push_back(expr);
    }

    // Final reduce to clean-up the stack.
    int i = stack.size() - 1;
    escargot::Node* expr = (escargot::Node *)stack[i];
    markers.pop_back();
    while (i > 1) {
        // expr = new WrappingNode(markers.pop()).finishBinaryExpression(, );
        markers.pop_back();
        expr = finishBinaryExpression(ctx, (escargot::Node *)stack[i - 2], expr, ((ParseStatus *)stack[i - 1])->m_value);
        i -= 2;
    }

    return expr;
}


// ECMA-262 12.13 Conditional Operator

escargot::Node* parseConditionalExpression(ParseContext* ctx)
{
    // var expr, previousAllowIn, consequent, alternate, startToken;

    // RefPtr<ParseStatus> startToken = ctx->m_lookahead;
    escargot::Node* consequent;
    escargot::Node* alternate;
    escargot::Node* expr = inheritCoverGrammar(ctx, parseBinaryExpression);
    if (match(ctx, '?')) {
        lex(ctx);
        bool previousAllowIn = ctx->m_allowIn;
        ctx->m_allowIn = true;
        consequent = isolateCoverGrammar(ctx, parseAssignmentExpression);
        ctx->m_allowIn = previousAllowIn;
        expect(ctx, ':');
        alternate = isolateCoverGrammar(ctx, parseAssignmentExpression);

        // expr = new WrappingNode(startToken).finishConditionalExpression(expr, consequent, alternate);
        expr = new escargot::ConditionalExpressionNode(expr, consequent, alternate);
        expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        ctx->m_isAssignmentTarget = ctx->m_isBindingElement = false;
    }

    return expr;
}

// ECMA-262 14.4 Yield expression

escargot::Node* parseYieldExpression(ParseContext* ctx)
{
    // todo
    RELEASE_ASSERT_NOT_REACHED();

   /*
    var argument, expr, delegate, previousAllowYield;

    argument = null;
    expr = new Node();

    expectKeyword('yield');

    if (!hasLineTerminator) {
        previousAllowYield = state.allowYield;
        state.allowYield = false;
        delegate = match('*');
        if (delegate) {
            lex();
            argument = parseAssignmentExpression();
        } else {
            if (!match(';') && !match('}') && !match(')') && lookahead.type !== Token.EOF) {
                argument = parseAssignmentExpression();
            }
        }
        state.allowYield = previousAllowYield;
    }

    return expr.finishYieldExpression(argument, delegate);
    */
}

// ECMA-262 12.14 Assignment Operators

escargot::Node* parseAssignmentExpression(ParseContext* ctx)
{
    // var token, expr, right, list, startToken;

    // RefPtr<ParseStatus> startToken = ctx->m_lookahead;
    RefPtr<ParseStatus> token = ctx->m_lookahead;

    if (!ctx->m_allowYield && matchKeyword(ctx, u"yield")) {
        return parseYieldExpression(ctx);
    }

    escargot::Node* right;
    escargot::Node* expr = parseConditionalExpression(ctx);
    /*
    if (expr->type === PlaceHolders.ArrowParameterPlaceHolder || match('=>')) {
        isAssignmentTarget = isBindingElement = false;
        list = reinterpretAsCoverFormalsList(expr);

        if (list) {
            firstCoverInitializedNameError = null;
            return parseArrowFunctionExpression(list, new WrappingNode(startToken));
        }

        return expr;
    }
    */

    if (matchAssign(ctx)) {
        if (!ctx->m_isAssignmentTarget) {
            tolerateError(u"Messages.InvalidLHSInAssignment");
        }

        // ECMA-262 11.13.1
        if (ctx->m_strict && expr->type() == escargot::NodeType::Identifier && isRestrictedWord(((escargot::IdentifierNode *)expr)->nonAtomicName()->data())) {
            tolerateUnexpectedToken();
        }

        if (!match(ctx, '=')) {
            ctx->m_isAssignmentTarget = ctx->m_isBindingElement = false;
        } else {
            reinterpretExpressionAsPattern(ctx, expr);
        }

        token = lex(ctx);
        right = isolateCoverGrammar(ctx, parseAssignmentExpression);
        // expr = new WrappingNode(startToken).finishAssignmentExpression(token.value, expr, right);
        if (token->m_value == u"=") {
            expr = new escargot::AssignmentExpressionSimpleNode(expr, right);
        } else {
            if (token->m_value == u"+=") {
                expr = new escargot::AssignmentExpressionPlusNode(expr, right);
            } else if (token->m_value == u"-=") {
                expr = new escargot::AssignmentExpressionMinusNode(expr, right);
            } else if (token->m_value == u"*=") {
                expr = new escargot::AssignmentExpressionMultiplyNode(expr, right);
            } else if (token->m_value == u"/=") {
                expr = new escargot::AssignmentExpressionDivisionNode(expr, right);
            } else if (token->m_value == u"%=") {
                expr = new escargot::AssignmentExpressionModNode(expr, right);
            } else if (token->m_value == u"<<=") {
                expr = new escargot::AssignmentExpressionLeftShiftNode(expr, right);
            } else if (token->m_value == u">>=") {
                expr = new escargot::AssignmentExpressionSignedRightShiftNode(expr, right);
            } else if (token->m_value == u">>>=") {
                expr = new escargot::AssignmentExpressionUnsignedShiftNode(expr, right);
            } else if (token->m_value == u"^=") {
                expr = new escargot::AssignmentExpressionBitwiseXorNode(expr, right);
            } else if (token->m_value == u"&=") {
                expr = new escargot::AssignmentExpressionBitwiseAndNode(expr, right);
            } else if (token->m_value == u"|=") {
                expr = new escargot::AssignmentExpressionBitwiseOrNode(expr, right);
            } else {
                RELEASE_ASSERT_NOT_REACHED();
            }
        }
        expr->setSourceLocation(ctx->m_lineNumber, ctx->m_lineStart);
        ctx->m_firstCoverInitializedNameError = NULL;
    }

    return expr;
}

escargot::StatementNodeVector parseScriptBody(ParseContext* ctx)
{
    // var statement, body = [], token, directive, firstRestricted;
    escargot::StatementNodeVector body;
    ctx->m_currentBody = &body;
    RefPtr<ParseStatus> token;
    RefPtr<ParseStatus> firstRestricted;
    while (ctx->m_startIndex < ctx->m_length) {
        token = ctx->m_lookahead;
        if (token->m_type != Token::StringLiteralToken) {
            break;
        }

        escargot::Node* statement = parseStatementListItem(ctx);
        if (statement)
            body.push_back(statement);

        if (((escargot::ExpressionStatementNode *) statement)->expression()->type() != escargot::NodeType::Literal) {
            // this is not directive
            break;
        }

        bool strict = true;
        if (token->m_end - 1 - (token->m_start + 1) == 10) {
            static const char16_t* s = u"use strict";
            for (size_t i = 0 ; i < 10 ; i ++) {
                if (s[i] != ctx->m_source[token->m_start + 1 + i]) {
                    strict = false;
                }
            }
        } else {
            strict = false;
        }
        if (strict) {
            ctx->m_strict = true;
            if (firstRestricted) {
                // tolerateUnexpectedToken(firstRestricted, Messages.StrictOctalLiteral);
                tolerateUnexpectedToken();
            }
        } else {
            if (!firstRestricted && token->m_octal) {
                firstRestricted = token;
            }
        }

        /*
        escargot::u16string directive = ctx->m_source.substr(token->m_start + 1,
            token->m_end - 1 - (token->m_start + 1));
        // directive = source.slice(token.start + 1, token.end - 1);
        if (directive == u"use strict") {
            ctx->m_strict = true;
            if (firstRestricted) {
                // tolerateUnexpectedToken(firstRestricted, Messages.StrictOctalLiteral);
                tolerateUnexpectedToken();
            }
        } else {
            if (!firstRestricted && token->m_octal) {
                firstRestricted = token;
            }
        }
        */
    }

    while (ctx->m_startIndex < ctx->m_length) {
        escargot::Node* statement = parseStatementListItem(ctx);
        /* istanbul ignore if */
        /*
        if (typeof statement === 'undefined') {
            break;
        }
        */
        if (statement)
            body.push_back(statement);
    }

    rearrangeNode(*ctx->m_currentBody);
    return body;
}

escargot::Node* parseProgram(ParseContext* ctx)
{
    /*
    var body, node;

    peek();
    node = new Node();

    body = ;
    return node.finishProgram(body);
     */

    peek(ctx);
    escargot::ProgramNode* node = new escargot::ProgramNode(parseScriptBody(ctx), ctx->m_strict);

    return node;
}

escargot::Node* parse(const escargot::u16string& source)
{
    ParseContext ctx(source);
    ctx.m_index = 0;
    ctx.m_lineNumber = (source.length() > 0) ? 1 : 0;
    ctx.m_lineStart = 0;
    ctx.m_startIndex = ctx.m_index;
    ctx.m_startLineNumber = ctx.m_lineNumber;
    ctx.m_startLineStart = ctx.m_lineStart;
    ctx.m_length = source.length();
    ctx.m_allowIn = true;
    ctx.m_allowYield = true;
    ctx.m_inFunctionBody = false;
    ctx.m_inIteration = false;
    ctx.m_inSwitch = false;
    ctx.m_lastCommentStart = -1;
    ctx.m_strict = false;
    ctx.m_scanning = false;
    ctx.m_isAssignmentTarget = false;
    ctx.m_isBindingElement = false;
    ctx.m_firstCoverInitializedNameError = NULL;
    ctx.m_parenthesizedCount = 0;
    ctx.m_lookahead = nullptr;
    ctx.m_currentBody = nullptr;
    try {
        escargot::Node* node = parseProgram(&ctx);
        return node;
    } catch(...) {
        throw ctx.m_lineNumber;
    }
    return NULL;
}


}
