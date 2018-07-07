// Copyright 2018 Alexander Bolz
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#pragma once

#include <cassert>
#include <cstdint>
#include <cstring>

#ifndef JSON1_ASSERT
#define JSON1_ASSERT(X) assert(X)
#endif

//======================================================================================================================
// Parser
//======================================================================================================================

namespace json1 {

struct Options
{
    // If true, skip line comments (introduced with "//") and block
    // comments like "/* hello */".
    // Default is false.
    bool strip_comments = false;

    // If true, parses "NaN" and "Infinity" (without the quotes) as numbers.
    // Default is true.
    bool allow_nan_inf = true;

    // If true, skip UTF-8 byte order mark - if any.
    // Default is true.
    bool skip_bom = true;

    // If true, allows trailing commas in arrays or objects.
    // Default is false.
    bool allow_trailing_comma = false;

    // If true, parse numbers as raw strings.
    // Default is false.
    bool parse_numbers_as_strings = false;

    // If true, allow characters after value.
    // Might be used to parse strings like "[1,2,3]{"hello":"world"}" into
    // different values by repeatedly calling parse.
    // Default is false.
    bool allow_trailing_characters = false;

    // If >= 0, pretty-print the JSON.
    // Default is < 0, that is the JSON is rendered as the shortest string possible.
    int8_t indent_width = -1;
};

enum class ParseStatus {
    success = 0,
    duplicate_key,
    expected_colon_after_key,
    expected_comma_or_closing_brace,
    expected_comma_or_closing_bracket,
    expected_eof,
    expected_key,
    invalid_key,
    invalid_number,
    invalid_string,
    invalid_value,
    max_depth_reached,
    unexpected_eof,
    unexpected_token,
    unrecognized_identifier,
};

enum class NumberClass : unsigned char {
    invalid,
    integer,
    floating_point,
    nan,
    pos_infinity,
    neg_infinity,
};

//struct ParseCallbacks
//{
//    ParseStatus HandleNull(Options const& options);
//    ParseStatus HandleBoolean(bool value, Options const& options);
//    ParseStatus HandleNumber(char const* first, char const* last, NumberClass nc, Options const& options);
//    ParseStatus HandleString(char const* first, char const* last, bool needs_cleaning, Options const& options);
//    ParseStatus HandleBeginArray(Options const& options);
//    ParseStatus HandleEndArray(size_t count, Options const& options);
//    ParseStatus HandleEndElement(size_t& count, Options const& options);
//    ParseStatus HandleBeginObject(Options const& options);
//    ParseStatus HandleEndObject(size_t count, Options const& options);
//    ParseStatus HandleEndMember(size_t& count, Options const& options);
//    ParseStatus HandleKey(char const* first, char const* last, bool needs_cleaning, Options const& options);
//};

struct ParseResult
{
    ParseStatus ec;
    // On return, PTR denotes the position after the parsed value, or if an
    // error occurred, denotes the position of the invalid token.
    char const* ptr;
    // If an error occurred, END denotes the position after the invalid token.
    // This field is unused on success.
    char const* end;
};

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

namespace impl {
namespace charclass {

enum ECharClass : uint8_t {
    CC_None             = 0,    // nothing special
    CC_Whitespace       = 0x01, // whitespace  : '\t', '\n', '\r', ' '
    CC_Digit            = 0x02, // digit       : '0'...'9'
    CC_AsciiControl     = 0x04,
    CC_IdentifierBody   = 0x10, // ident-body  : IsDigit, IsLetter, '_', '$'
    CC_ValidEscapedChar = 0x20, // valid escaped chars: '"', '\\', '/', 'b', 'f', 'n', 'r', 't', 'u'
    CC_StringSpecial    = 0x40, // quote or bs : '"', '\'', '`', '\\'
    CC_NeedsCleaning    = 0x80, // needs cleaning (strings)
};

inline unsigned CharClass(char ch)
{
#define W CC_Whitespace
#define D CC_Digit
#define A CC_AsciiControl
#define I CC_IdentifierBody
#define E CC_ValidEscapedChar
#define S CC_StringSpecial
#define C CC_NeedsCleaning

    static constexpr uint8_t const kMap[] = {
    //  NUL     SOH     STX     ETX     EOT     ENQ     ACK     BEL     BS      HT      LF      VT      FF      CR      SO      SI
        A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|W|C,  A|W|C,  A|C,    A|C,    A|W|C,  A|C,    A|C,
    //  DLE     DC1     DC2     DC3     DC4     NAK     SYN     ETB     CAN     EM      SUB     ESC     FS      GS      RS      US
        A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,    A|C,
    //  space   !       "       #       $       %       &       '       (       )       *       +       ,       -       .       /
        W,      0,      S|E,    0,      I,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      E,
    //  0       1       2       3       4       5       6       7       8       9       :       ;       <       =       >       ?
        D|I,    D|I,    D|I,    D|I,    D|I,    D|I,    D|I,    D|I,    D|I,    D|I,    0,      0,      0,      0,      0,      0,
    //  @       A       B       C       D       E       F       G       H       I       J       K       L       M       N       O
        0,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,
    //  P       Q       R       S       T       U       V       W       X       Y       Z       [       \       ]       ^       _
        I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      0,      E|S|C,  0,      0,      I,
    //  `       a       b       c       d       e       f       g       h       i       j       k       l       m       n       o
        0,      I,      E|I,    I,      I,      I  ,    E|I,    I,      I,      I,      I,      I,      I,      I,      E|I,    I,
    //  p       q       r       s       t       u       v       w       x       y       z       {       |       }       ~       DEL
        I,      I,      E|I,    I,      E|I,    E|I,    I,      I,      I,      I,      I,      0,      0,      0,      0,      0,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
    };

#undef C
#undef S
#undef E
#undef I
#undef A
#undef D
#undef W

    return kMap[static_cast<unsigned char>(ch)];
}

inline bool IsWhitespace      (char ch) { return (CharClass(ch) & CC_Whitespace      ) != 0; }
inline bool IsDigit           (char ch) { return (CharClass(ch) & CC_Digit           ) != 0; }
inline bool IsIdentifierBody  (char ch) { return (CharClass(ch) & CC_IdentifierBody  ) != 0; }
inline bool IsValidEscapedChar(char ch) { return (CharClass(ch) & CC_ValidEscapedChar) != 0; }
inline bool IsStringSpecial   (char ch) { return (CharClass(ch) & CC_StringSpecial   ) != 0; }
inline bool NeedsCleaning     (char ch) { return (CharClass(ch) & CC_NeedsCleaning   ) != 0; }

#if 1
inline int HexDigitValue(char ch)
{
#define N -1
    static constexpr int8_t const kMap[256] = {
    //  NUL     SOH     STX     ETX     EOT     ENQ     ACK     BEL     BS      HT      LF      VT      FF      CR      SO      SI
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
    //  DLE     DC1     DC2     DC3     DC4     NAK     SYN     ETB     CAN     EM      SUB     ESC     FS      GS      RS      US
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
    //  space   !       "       #       $       %       &       '       (       )       *       +       ,       -       .       /
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
    //  0       1       2       3       4       5       6       7       8       9       :       ;       <       =       >       ?
        0,      1,      2,      3,      4,      5,      6,      7,      8,      9,      N,      N,      N,      N,      N,      N,
    //  @       A       B       C       D       E       F       G       H       I       J       K       L       M       N       O
        N,      10,     11,     12,     13,     14,     15,     N,      N,      N,      N,      N,      N,      N,      N,      N,
    //  P       Q       R       S       T       U       V       W       X       Y       Z       [       \       ]       ^       _
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
    //  `       a       b       c       d       e       f       g       h       i       j       k       l       m       n       o
        N,      10,     11,     12,     13,     14,     15,     N,      N,      N,      N,      N,      N,      N,      N,      N,
    //  p       q       r       s       t       u       v       w       x       y       z       {       |       }       ~       DEL
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
    };
#undef N

    return kMap[static_cast<unsigned char>(ch)];
}
#else
inline int HexDigitValue(char ch)
{
    if ('0' <= ch && ch <= '9') { return ch - '0'; }
    if ('A' <= ch && ch <= 'F') { return ch - 'A' + 10; }
    if ('a' <= ch && ch <= 'f') { return ch - 'a' + 10; }
    return -1;
}
#endif

} // namespace charclass
} // namespace impl

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

namespace impl {

inline char const* SkipWhitespace(char const* f, char const* l)
{
    using namespace json1::impl::charclass;

#if 0
    while (l - f >= 4)
    {
        if (!IsWhitespace(f[0])) return f + 0;
        if (!IsWhitespace(f[1])) return f + 1;
        if (!IsWhitespace(f[2])) return f + 2;
        if (!IsWhitespace(f[3])) return f + 3;
        f += 4;
    }
#endif

    for ( ; f != l && IsWhitespace(*f); ++f)
    {
    }

    return f;
}

} // namespace impl

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

namespace impl {

struct ScanNumberResult
{
    char const* next;
    NumberClass number_class;
};

inline ScanNumberResult ScanNumber(char const* next, char const* last, Options const& options)
{
    using namespace json1::impl::charclass;

    if (next == last)
        return {next, NumberClass::invalid};

    bool is_neg = false;
    bool is_float = false;

// [-]

    if (*next == '-')
    {
        is_neg = true;

        ++next;
        if (next == last)
            return {next, NumberClass::invalid};
    }

// int

    if (*next == '0')
    {
        ++next;
        if (next == last)
            return {next, NumberClass::integer};
        if (IsDigit(*next))
            return {next, NumberClass::invalid};
    }
    else if (IsDigit(*next)) // non '0'
    {
        for (;;)
        {
            ++next;
            if (next == last)
                return {next, NumberClass::integer};
            if (!IsDigit(*next))
                break;
        }
    }
    else
    {
        // NaN/Infinity

        //
        // XXX:
        // Requires It = char [const]*
        //
        if (options.allow_nan_inf && last - next >= 3 && std::memcmp(next, "NaN", 3) == 0)
        {
            return {next + 3, NumberClass::nan};
        }
        if (options.allow_nan_inf && last - next >= 8 && std::memcmp(next, "Infinity", 8) == 0)
        {
            return {next + 8, is_neg ? NumberClass::neg_infinity : NumberClass::pos_infinity};
        }

        return {next, NumberClass::invalid};
    }

// frac

    if (*next == '.')
    {
        is_float = true;

        ++next;
        if (next == last || !IsDigit(*next))
            return {next, NumberClass::invalid};

        for (;;)
        {
            ++next;
            if (next == last)
                return {next, NumberClass::floating_point};
            if (!IsDigit(*next))
                break;
        }
    }

// exp

    if (*next == 'e' || *next == 'E')
    {
        is_float = true;

        ++next;
        if (next == last)
            return {next, NumberClass::invalid};

        if (*next == '+' || *next == '-')
        {
            ++next;
            if (next == last)
                return {next, NumberClass::invalid};
        }

        if (!IsDigit(*next))
            return {next, NumberClass::invalid};

        for (;;)
        {
            ++next;
            if (next == last)
                return {next, NumberClass::floating_point};
            if (!IsDigit(*next))
                break;
        }
    }

    return {next, is_float ? NumberClass::floating_point : NumberClass::integer};
}

} // namespace impl

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

namespace impl {

enum class TokenKind : unsigned char {
    unknown,
    eof,
    l_brace,
    r_brace,
    l_square,
    r_square,
    comma,
    colon,
    string,
    incomplete_string,
    number,
    identifier,
    comment,
    incomplete_comment,
};

struct Token
{
    char const* ptr = nullptr;
    char const* end = nullptr;
    TokenKind   kind = TokenKind::unknown;
    bool        needs_cleaning = false;
    NumberClass number_class = NumberClass::invalid;
};

struct Lexer
{
    char const* src = nullptr;
    char const* end = nullptr;
    char const* ptr = nullptr; // position in [src, end)

    Lexer();
    explicit Lexer(char const* first, char const* last);

    Token MakeToken(char const* p, TokenKind kind, bool needs_cleaning = false, NumberClass number_class = NumberClass::invalid);

    Token Lex(Options const& options);

    Token LexString    (char const* p);
    Token LexNumber    (char const* p, Options const& options);
    Token LexIdentifier(char const* p);
    Token LexComment   (char const* p);
};

inline Lexer::Lexer()
{
}

inline Lexer::Lexer(char const* first, char const* last)
    : src(first)
    , end(last)
    , ptr(first)
{
}

inline Token Lexer::MakeToken(char const* p, TokenKind kind, bool needs_cleaning, NumberClass number_class)
{
    Token tok;

    tok.ptr = ptr;
    tok.end = p;
    tok.kind = kind;
    tok.needs_cleaning = needs_cleaning;
    tok.number_class = number_class;

    ptr = p;

    return tok;
}

inline Token Lexer::Lex(Options const& options)
{
L_again:
    ptr = SkipWhitespace(ptr, end);

    auto p = ptr;

    if (p == end)
        return MakeToken(p, TokenKind::eof);

    auto kind = TokenKind::unknown;

    char const ch = *p;
    switch (ch)
    {
    case '{':
        kind = TokenKind::l_brace;
        break;
    case '}':
        kind = TokenKind::r_brace;
        break;
    case '[':
        kind = TokenKind::l_square;
        break;
    case ']':
        kind = TokenKind::r_square;
        break;
    case ',':
        kind = TokenKind::comma;
        break;
    case ':':
        kind = TokenKind::colon;
        break;
    case '"':
        return LexString(p);
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return LexNumber(p, options);
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case '_':
        return LexIdentifier(p);
    case '/':
        if (options.strip_comments)
        {
            auto tok = LexComment(p);
            if (tok.kind == TokenKind::comment)
                goto L_again;
        }
        break;
    default:
        break;
    }

    ++p;
    return MakeToken(p, kind);
}

inline Token Lexer::LexString(char const* p)
{
    using namespace json1::impl::charclass;

    JSON1_ASSERT(p != end);
    JSON1_ASSERT(*p == '"');

    ptr = ++p; // skip " or '

    unsigned mask = 0;
    for (;;)
    {
        while (end - p >= 4)
        {
            unsigned const m0 = CharClass(p[0]);
            unsigned const m1 = CharClass(p[1]);
            unsigned const m2 = CharClass(p[2]);
            unsigned const m3 = CharClass(p[3]);

            unsigned const mm = m0 | m1 | m2 | m3;
            if ((mm & CC_StringSpecial) == 0)
            {
                mask |= mm;
                p += 4;
                continue;
            }

            mask |= m0;        if ((m0 & CC_StringSpecial) != 0)   { goto L_check; }
            mask |= m1; ++p;   if ((m1 & CC_StringSpecial) != 0)   { goto L_check; }
            mask |= m2; ++p;   if ((m2 & CC_StringSpecial) != 0)   { goto L_check; }
            mask |= m3; ++p; /*if ((m3 & CC_StringSpecial) != 0)*/ { goto L_check; }
        }

        for (;;)
        {
            if (p == end)
                goto L_incomplete;

            unsigned const m0 = CharClass(*p);
            mask |= m0;
            if ((m0 & CC_StringSpecial) != 0)
                goto L_check;

            ++p;
        }

L_check:
        auto const ch = *p;

        if (ch == '"')
        {
            auto tok = MakeToken(p, TokenKind::string, (mask & CC_NeedsCleaning) != 0);
            ptr = ++p; // skip " or '
            return tok;
        }

        JSON1_ASSERT(ch == '\\');
        ++p;
        if (p == end)
            break;
        ++p; // Skip the escaped character.
    }

L_incomplete:
    return MakeToken(p, TokenKind::incomplete_string, (mask & CC_NeedsCleaning) != 0);
}

inline Token Lexer::LexNumber(char const* p, Options const& options)
{
    auto const res = ScanNumber(p, end, options);

    return MakeToken(res.next, TokenKind::number, /*needs_cleaning*/ false, res.number_class);
}

inline Token Lexer::LexIdentifier(char const* p)
{
    using namespace json1::impl::charclass;

    for ( ; p != end && IsIdentifierBody(*p); ++p)
    {
    }

    return MakeToken(p, TokenKind::identifier);
}

inline Token Lexer::LexComment(char const* p)
{
    JSON1_ASSERT(p != end);
    JSON1_ASSERT(*p == '/');

    ++p;
    if (p == end)
        return MakeToken(p, TokenKind::unknown);

    if (*p == '/')
    {
        for (;;)
        {
            ++p;
            if (p == end)
                break;
            if (*p == '\n' || *p == '\r')
                break;
        }

        return MakeToken(p, TokenKind::comment);
    }

    if (*p == '*')
    {
        TokenKind kind = TokenKind::incomplete_comment;

        for (;;)
        {
            ++p;
            if (p == end)
                break;
            if (*p == '*')
            {
                ++p;
                if (p == end)
                    break;
                if (*p == '/')
                {
                    kind = TokenKind::comment;
                    ++p;
                    break;
                }
            }
        }

        return MakeToken(p, kind);
    }

    return MakeToken(p, TokenKind::unknown);
}

} // namespace impl

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

namespace impl {

struct Failed
{
    ParseStatus ec;

    Failed(ParseStatus ec_) : ec(ec_) {}
    operator ParseStatus() const noexcept { return ec; }

    // Test for failure.
    explicit operator bool() const noexcept { return ec != ParseStatus::success; }
};

template <typename ParseCallbacks>
struct Parser
{
    static constexpr int kMaxDepth = 500;

    ParseCallbacks& cb;
    Options         options;
    Lexer           lexer;
    Token           token; // The next token.

    Parser(ParseCallbacks& cb_, Options const& options_);

    ParseStatus ParseString();
    ParseStatus ParseNumber();
    ParseStatus ParseIdentifier();
    ParseStatus ParsePrimitive();
    ParseStatus ParseValue();
};

template <typename ParseCallbacks>
Parser<ParseCallbacks>::Parser(ParseCallbacks& cb_, Options const& options_)
    : cb(cb_)
    , options(options_)
{
}

template <typename ParseCallbacks>
ParseStatus Parser<ParseCallbacks>::ParseString()
{
    JSON1_ASSERT(token.kind == TokenKind::string);

    if (Failed ec = cb.HandleString(token.ptr, token.end, token.needs_cleaning, options))
        return ec;

    // skip string
    token = lexer.Lex(options);

    return ParseStatus::success;
}

template <typename ParseCallbacks>
ParseStatus Parser<ParseCallbacks>::ParseNumber()
{
    JSON1_ASSERT(token.kind == TokenKind::number);

    if (token.number_class == NumberClass::invalid)
        return ParseStatus::invalid_number;

    if (Failed ec = cb.HandleNumber(token.ptr, token.end, token.number_class, options))
        return ec;

    // skip number
    token = lexer.Lex(options);

    return ParseStatus::success;
}

template <typename ParseCallbacks>
ParseStatus Parser<ParseCallbacks>::ParseIdentifier()
{
    JSON1_ASSERT(token.kind == TokenKind::identifier);
    JSON1_ASSERT(token.end - token.ptr > 0 && "internal error");

    auto const f = token.ptr;
    auto const l = token.end;
    auto const len = l - f;

    ParseStatus ec;
    if (len == 4 && /**f == 'n' &&*/ std::memcmp(f, "null", 4) == 0)
    {
        ec = cb.HandleNull(options);
    }
    else if (len == 4 && /**f == 't' &&*/ std::memcmp(f, "true", 4) == 0)
    {
        ec = cb.HandleBoolean(true, options);
    }
    else if (len == 5 && /**f == 'f' &&*/ std::memcmp(f, "false", 5) == 0)
    {
        ec = cb.HandleBoolean(false, options);
    }
    else if (options.allow_nan_inf && len == 3 && std::memcmp(f, "NaN", 3) == 0)
    {
        ec = cb.HandleNumber(f, l, NumberClass::nan, options);
    }
    else if (options.allow_nan_inf && len == 8 && std::memcmp(f, "Infinity", 8) == 0)
    {
        ec = cb.HandleNumber(f, l, NumberClass::pos_infinity, options);
    }
    else
    {
        return ParseStatus::unrecognized_identifier;
    }

    if (Failed(ec))
        return ec;

    // skip 'identifier'
    token = lexer.Lex(options);

    return ParseStatus::success;
}

template <typename ParseCallbacks>
ParseStatus Parser<ParseCallbacks>::ParsePrimitive()
{
    switch (token.kind)
    {
    case TokenKind::string:
        return ParseString();
    case TokenKind::number:
        return ParseNumber();
    case TokenKind::identifier:
        return ParseIdentifier();
    case TokenKind::eof:
        return ParseStatus::unexpected_eof;
    default:
        return ParseStatus::unexpected_token;
    }
}

template <typename ParseCallbacks>
ParseStatus Parser<ParseCallbacks>::ParseValue()
{
    enum class Structure {
        object,
        array,
    };

    struct StackElement
    {
        Structure structure;
        size_t count; // number of elements or members in the current array resp. object

        StackElement() = default;
        StackElement(Structure structure_, size_t count_) : structure(structure_) , count(count_) {}
    };

    StackElement stack[kMaxDepth];
    size_t stack_size = 0;

    // parse 'value'
    if (token.kind == TokenKind::l_brace)
        goto L_begin_object;
    if (token.kind == TokenKind::l_square)
        goto L_begin_array;

    return ParsePrimitive();

L_begin_object:
    //
    //  object
    //      {}
    //      { members }
    //  members
    //      pair
    //      pair , members
    //  pair
    //      string : value
    //
    JSON1_ASSERT(token.kind == TokenKind::l_brace);

    if (stack_size >= kMaxDepth)
        return ParseStatus::max_depth_reached;

    stack[stack_size++] = {Structure::object, 0};

    // skip '{'
    token = lexer.Lex(options);

    if (Failed ec = cb.HandleBeginObject(options))
        return ec;

    if (token.kind != TokenKind::r_brace)
    {
        for (;;)
        {
            if (token.kind != TokenKind::string)
                return ParseStatus::expected_key;

            if (Failed ec = cb.HandleKey(token.ptr, token.end, token.needs_cleaning, options))
                return ec;

            // skip 'key'
            token = lexer.Lex(options);

            if (token.kind != TokenKind::colon)
                return ParseStatus::expected_colon_after_key;

            // skip ':'
            token = lexer.Lex(options);

            // parse 'value'
            if (token.kind == TokenKind::l_brace)
                goto L_begin_object;
            if (token.kind == TokenKind::l_square)
                goto L_begin_array;

            if (Failed ec = ParsePrimitive())
                return ec;

L_end_member:
            JSON1_ASSERT(stack_size != 0);
            JSON1_ASSERT(stack[stack_size - 1].structure == Structure::object);
            stack[stack_size - 1].count++;

            if (Failed ec = cb.HandleEndMember(stack[stack_size - 1].count, options))
                return ec;

            if (token.kind != TokenKind::comma)
                break;

            // skip ','
            token = lexer.Lex(options);

            if (options.allow_trailing_comma && token.kind == TokenKind::r_brace)
                break;
        }

        if (token.kind != TokenKind::r_brace)
            return ParseStatus::expected_comma_or_closing_brace;
    }

    if (Failed ec = cb.HandleEndObject(stack[stack_size - 1].count, options))
        return ec;

    // skip '}'
    token = lexer.Lex(options);
    goto L_end_structured;

L_begin_array:
    //
    //  array
    //      []
    //      [ elements ]
    //  elements
    //      value
    //      value , elements
    //
    JSON1_ASSERT(token.kind == TokenKind::l_square);

    if (stack_size >= kMaxDepth)
        return ParseStatus::max_depth_reached;

    stack[stack_size++] = {Structure::array, 0};

    // skip '['
    token = lexer.Lex(options);

    if (Failed ec = cb.HandleBeginArray(options))
        return ec;

    if (token.kind != TokenKind::r_square)
    {
        for (;;)
        {
            // parse 'value'
            if (token.kind == TokenKind::l_brace)
                goto L_begin_object;
            if (token.kind == TokenKind::l_square)
                goto L_begin_array;

            if (Failed ec = ParsePrimitive())
                return ec;

L_end_element:
            JSON1_ASSERT(stack_size != 0);
            JSON1_ASSERT(stack[stack_size - 1].structure == Structure::array);
            stack[stack_size - 1].count++;

            if (Failed ec = cb.HandleEndElement(stack[stack_size - 1].count, options))
                return ec;

            if (token.kind != TokenKind::comma)
                break;

            // skip ','
            token = lexer.Lex(options);

            if (options.allow_trailing_comma && token.kind == TokenKind::r_square)
                break;
        }

        if (token.kind != TokenKind::r_square)
            return ParseStatus::expected_comma_or_closing_bracket;
    }

    if (Failed ec = cb.HandleEndArray(stack[stack_size - 1].count, options))
        return ec;

    // skip ']'
    token = lexer.Lex(options);
    goto L_end_structured;

L_end_structured:
    JSON1_ASSERT(stack_size != 0);
    stack_size--;

    if (stack_size == 0)
        return ParseStatus::success;

    if (stack[stack_size - 1].structure == Structure::object)
        goto L_end_member;
    else
        goto L_end_element;
}

} // namespace impl

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

template <typename ParseCallbacks>
ParseResult Parse(ParseCallbacks& cb, char const* next, char const* last, Options const& options = {})
{
    using namespace json1::impl;

    JSON1_ASSERT(next != nullptr);
    JSON1_ASSERT(last != nullptr);

    if (options.skip_bom && last - next >= 3)
    {
        if (static_cast<unsigned char>(next[0]) == 0xEF &&
            static_cast<unsigned char>(next[1]) == 0xBB &&
            static_cast<unsigned char>(next[2]) == 0xBF)
        {
            next += 3;
        }
    }

    Parser<ParseCallbacks> parser(cb, options);

    parser.lexer = Lexer(next, last);
    parser.token = parser.lexer.Lex(options); // Get the first token

    auto /*const*/ ec = parser.ParseValue();

    if (ec == ParseStatus::success)
    {
        if (!options.allow_trailing_characters && parser.token.kind != TokenKind::eof)
        {
            ec = ParseStatus::expected_eof;
        }
#if 0
        else if (options.allow_trailing_characters && parser.token.kind == TokenKind::comma)
        {
            // Skip commas at end of value.
            // Allows to parse strings like "true,1,[1]"
            parser.token = parser.lexer.Lex(options);
        }
#endif
    }

    //
    // XXX:
    // Return token.kind on error?!?!
    //

    return {ec, parser.token.ptr, parser.token.end};
}

} // namespace json1

//======================================================================================================================
// Strings
//======================================================================================================================

namespace json1 {
namespace impl {
namespace unicode {

constexpr uint32_t kInvalidCodepoint = 0xFFFFFFFF;

inline bool IsValidCodepoint(uint32_t U)
{
    // 1. Characters with values greater than 0x10FFFF cannot be encoded in
    //    UTF-16.
    // 2. Values between 0xD800 and 0xDFFF are specifically reserved for use
    //    with UTF-16, and don't have any characters assigned to them.
    return U < 0xD800 || (U > 0xDFFF && U <= 0x10FFFF);
}

inline bool IsUTF8Trail(char ch)
{
    uint32_t const b = static_cast<uint8_t>(ch);

    return 0x80 == (b & 0xC0); // b == 10xxxxxx???
}

inline char const* FindNextUTF8Sequence(char const* next, char const* last)
{
    // Skip UTF-8 trail bytes.
    // The first non-trail byte is the start of a (possibly invalid) UTF-8 sequence.
    while (next != last && IsUTF8Trail(*next))
    {
        ++next;
    }
    return next;
}

inline int GetUTF8SequenceLengthFromLeadByte(char ch, uint32_t& U)
{
    uint32_t const b = static_cast<uint8_t>(ch);

    if (b <= 0x7F) { U = b;        return 1; } // 01111111 (0xxxxxxx)
    if (b <= 0xC1) {               return 0; }
    if (b <= 0xDF) { U = b & 0x1F; return 2; } // 11011111 (110xxxxx)
    if (b <= 0xEF) { U = b & 0x0F; return 3; } // 11101111 (1110xxxx)
    if (b <= 0xF4) { U = b & 0x07; return 4; } // 11110100 (11110xxx)
    return 0;
}

inline int GetUTF8SequenceLengthFromCodepoint(uint32_t U)
{
    JSON1_ASSERT(IsValidCodepoint(U));

    if (U <=   0x7F) { return 1; }
    if (U <=  0x7FF) { return 2; }
    if (U <= 0xFFFF) { return 3; }
    return 4;
}

inline bool IsUTF8OverlongSequence(uint32_t U, int slen)
{
    return slen != GetUTF8SequenceLengthFromCodepoint(U);
}

inline char const* DecodeUTF8Sequence(char const* next, char const* last, uint32_t& U)
{
    JSON1_ASSERT(next != last);

    int const slen = GetUTF8SequenceLengthFromLeadByte(*next, U);
    ++next;

    if (slen == 0)
    {
        U = kInvalidCodepoint; // Invalid lead byte
        return FindNextUTF8Sequence(next, last);
    }

    for (int i = 1; i < slen; ++i)
    {
        if (next == last)
        {
            U = kInvalidCodepoint; // Incomplete UTF-8 sequence
            return next;
        }

        auto const cb = *next;

        if (!IsUTF8Trail(cb))
        {
            U = kInvalidCodepoint;
            return next;
        }

        U = (U << 6) | (static_cast<uint8_t>(cb) & 0x3F);
        ++next;
    }

    if (!IsValidCodepoint(U) || IsUTF8OverlongSequence(U, slen))
    {
        U = kInvalidCodepoint;
        return next;
    }

    return next;
}

template <typename Put8>
void EncodeUTF8(uint32_t U, Put8 put)
{
    JSON1_ASSERT(IsValidCodepoint(U));

    if (U <= 0x7F)
    {
        put( static_cast<uint8_t>( U ) );
    }
    else if (U <= 0x7FF)
    {
        put( static_cast<uint8_t>( 0xC0 | ((U >>  6)       ) ) );
        put( static_cast<uint8_t>( 0x80 | ((U      ) & 0x3F) ) );
    }
    else if (U <= 0xFFFF)
    {
        put( static_cast<uint8_t>( 0xE0 | ((U >> 12)       ) ) );
        put( static_cast<uint8_t>( 0x80 | ((U >>  6) & 0x3F) ) );
        put( static_cast<uint8_t>( 0x80 | ((U      ) & 0x3F) ) );
    }
    else
    {
        put( static_cast<uint8_t>( 0xF0 | ((U >> 18) & 0x3F) ) );
        put( static_cast<uint8_t>( 0x80 | ((U >> 12) & 0x3F) ) );
        put( static_cast<uint8_t>( 0x80 | ((U >>  6) & 0x3F) ) );
        put( static_cast<uint8_t>( 0x80 | ((U      ) & 0x3F) ) );
    }
}

template <typename Put16>
void EncodeUTF16(uint32_t U, Put16 put)
{
    JSON1_ASSERT(IsValidCodepoint(U));

    if (U < 0x10000)
    {
        put( static_cast<uint16_t>(U) );
    }
    else
    {
        uint32_t const Up = U - 0x10000;

        put( static_cast<uint16_t>(0xD800 + ((Up >> 10) & 0x3FF)) );
        put( static_cast<uint16_t>(0xDC00 + ((Up      ) & 0x3FF)) );
    }
}

// Reads a hexadecimal number of the form "HHHH".
// Stores the result in W on success.
inline bool ReadHex16(char const*& first, char const* last, uint32_t& W)
{
    using namespace json1::impl::charclass;

    auto f = first;

    if (last - f < 4) {
        first = last;
        return false;
    }

    auto const h0 = HexDigitValue(f[0]);
    auto const h1 = HexDigitValue(f[1]);
    auto const h2 = HexDigitValue(f[2]);
    auto const h3 = HexDigitValue(f[3]);
    first = f + 4;

    if ((h0 | h1 | h2 | h3) >= 0)
    {
        W = static_cast<uint32_t>((h0 << 12) | (h1 << 8) | (h2 << 4) | h3);
        return true;
    }

    return false;
}

// Reads a hexadecimal number of the form "\uHHHH".
// Stores the result in W on success.
inline bool ReadUCN(char const*& first, char const* last, uint32_t& W)
{
    using namespace json1::impl::charclass;

    auto f = first;

    if (last - f < 6) {
        first = last;
        return false;
    }

    auto const c0 = f[0];
    auto const c1 = f[1];
    auto const h0 = HexDigitValue(f[2]);
    auto const h1 = HexDigitValue(f[3]);
    auto const h2 = HexDigitValue(f[4]);
    auto const h3 = HexDigitValue(f[5]);
    first = f + 6;

    if (c0 == '\\' && c1 == 'u' && (h0 | h1 | h2 | h3) >= 0)
    {
        W = static_cast<uint32_t>((h0 << 12) | (h1 << 8) | (h2 << 4) | h3);
        return true;
    }

    return false;
}

inline char const* DecodeTrimmedUCNSequence(char const* next, char const* last, uint32_t& U)
{
    JSON1_ASSERT(next != last);

    uint32_t W1;
    if (!ReadHex16(next, last, W1))
    {
        U = kInvalidCodepoint;
        return next;
    }

    if (W1 < 0xD800 || W1 > 0xDFFF)
    {
        U = W1;
        return next;
    }

    if (W1 > 0xDBFF)
    {
        U = kInvalidCodepoint; // Invalid high surrogate
        return next;
    }

    uint32_t W2;
    if (!ReadUCN(next, last, W2))
    {
        U = kInvalidCodepoint;
        return next;
    }

    if (W2 < 0xDC00 || W2 > 0xDFFF)
    {
        U = kInvalidCodepoint;
        return next;
    }

    U = (((W1 & 0x3FF) << 10) | (W2 & 0x3FF)) + 0x10000;
    return next;
}

} // namespace unicode
} // namespace impl

namespace strings {

//
//  string
//      ""
//      " chars "
//  chars
//      char
//      char chars
//  char
//      <any Unicode character except " or \ or control-character>
//      '\"'
//      '\\'
//      '\/'
//      '\b'
//      '\f'
//      '\n'
//      '\r'
//      '\t'
//      '\u' four-hex-digits
//

enum class Status {
    success,
    unescaped_control_character,
    invalid_escaped_character,
    invalid_ucn_sequence,
    invalid_utf8_sequence,
    incomplete,
};

struct UnescapeStringResult
{
    char const* next;
    Status status;
};

template <typename Fn>
UnescapeStringResult UnescapeString(char const* next, char const* last, Fn yield)
{
    namespace unicode = json1::impl::unicode;

    while (next != last)
    {
        auto const uc = static_cast<unsigned char>(*next);

        if (uc < 0x20) // unescaped control character
        {
            return {next, Status::unescaped_control_character};
        }
        else if (uc < 0x80) // ASCII printable or DEL
        {
            if (*next != '\\')
            {
                yield(*next);
                ++next;
                continue;
            }

            auto const f = next; // The start of the UCN sequence

            ++next; // skip '\'
            if (next == last)
            {
                return {next, Status::incomplete};
            }

            switch (*next)
            {
            case '"':
                yield('"');
                ++next;
                break;
            case '\\':
                yield('\\');
                ++next;
                break;
            case '/':
                yield('/');
                ++next;
                break;
            case 'b':
                yield('\b');
                ++next;
                break;
            case 'f':
                yield('\f');
                ++next;
                break;
            case 'n':
                yield('\n');
                ++next;
                break;
            case 'r':
                yield('\r');
                ++next;
                break;
            case 't':
                yield('\t');
                ++next;
                break;
            case 'u':
                {
                    if (++next == last)
                    {
                        return {f, Status::incomplete};
                    }

                    uint32_t U = 0;
                    auto const end = unicode::DecodeTrimmedUCNSequence(next, last, U);
                    JSON1_ASSERT(end != next);
                    next = end;

                    if (U == unicode::kInvalidCodepoint)
                    {
                        return {f, Status::invalid_utf8_sequence};
                    }

                    unicode::EncodeUTF8(U, [&](uint8_t code_unit) { yield(static_cast<char>(code_unit)); });
                }
                break;
            default:
                return {next, Status::invalid_escaped_character}; // invalid escaped character
            }
        }
        else // (possibly) the start of a UTF-8 sequence.
        {
            auto f = next; // The start of the UTF-8 sequence

            uint32_t U = 0;
            next = unicode::DecodeUTF8Sequence(next, last, U);
            JSON1_ASSERT(next != f);

            if (U == unicode::kInvalidCodepoint)
            {
                return {f, Status::invalid_utf8_sequence};
            }

            // The range [f, next) already contains a valid UTF-8 encoding of U.
            for ( ; f != next; ++f)
            {
                yield(*f);
            }
        }
    }

    return {next, Status::success};
}

struct EscapeStringResult
{
    char const* next;
    Status status;
};

template <typename Fn>
EscapeStringResult EscapeString(char const* next, char const* last, Fn yield)
{
    static constexpr char const kHexDigits[] = "0123456789ABCDEF";

    namespace unicode = json1::impl::unicode;

    char ch_prev = '\0';
    char ch = '\0';

    while (next != last)
    {
        ch_prev = ch;
        ch = *next;

        auto const uc = static_cast<unsigned char>(ch);

        if (uc < 0x20) // (ASCII) control character
        {
            yield('\\');
            switch (ch)
            {
            case '\b':
                yield('b');
                break;
            case '\f':
                yield('f');
                break;
            case '\n':
                yield('n');
                break;
            case '\r':
                yield('r');
                break;
            case '\t':
                yield('t');
                break;
            default:
                yield('u');
                yield('0');
                yield('0');
                yield(kHexDigits[uc >> 4]);
                yield(kHexDigits[uc & 0xF]);
                break;
            }
            ++next;
        }
        else if (uc < 0x80) // ASCII printable or DEL
        {
            switch (ch)
            {
            case '"':   // U+0022
            case '\\':  // U+005C
                yield('\\');
                break;
            case '/':   // U+002F
                if (ch_prev == '<')
                {
                    yield('\\');
                }
                break;
            }
            yield(ch);
            ++next;
        }
        else // (possibly) the start of a UTF-8 sequence.
        {
            auto f = next; // The start of the UTF-8 sequence

            uint32_t U = 0;
            next = unicode::DecodeUTF8Sequence(next, last, U);
            JSON1_ASSERT(next != f);

            if (U == unicode::kInvalidCodepoint)
            {
                return {next, Status::invalid_utf8_sequence};
            }

            //
            // Always escape U+2028 (LINE SEPARATOR) and U+2029 (PARAGRAPH
            // SEPARATOR). No string in JavaScript can contain a literal
            // U+2028 or a U+2029.
            //
            // (See http://timelessrepo.com/json-isnt-a-javascript-subset)
            //
            switch (U)
            {
            case 0x2028:
                yield('\\');
                yield('u');
                yield('2');
                yield('0');
                yield('2');
                yield('8');
                break;
            case 0x2029:
                yield('\\');
                yield('u');
                yield('2');
                yield('0');
                yield('2');
                yield('9');
                break;
            default:
                // The UTF-8 sequence is valid. No need to re-encode.
                for ( ; f != next; ++f)
                {
                    yield(*f);
                }
                break;
            }
        }
    }

    return {next, Status::success};
}

} // namespace strings
} // namespace json1

//======================================================================================================================
// Numbers
//======================================================================================================================
