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

#include "json_parse.h"

#include <cassert>
#include <cstdint>
#include <cstring>

using namespace json;

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

#define W 0x01  // whitespace  : ' ', '\n', '\r', '\t'
#define D 0x02  // digit       : '0'...'9'
                // letter      : 'a'...'z', 'A'...'Z'
#define X 0x04  //               'N', 'n', 'A', 'a', 'I', 'i', 'F', 'f', 'T', 't', 'Y', 'y'
#define N 0x08  // number-body : IsDigit, 'E', 'e', '.', '+', '-'
#define I 0x10  // ident-body  : IsDigit, IsLetter, '_', '$'
                // hex-digit   : IsDigit, 'a'...'f', 'A'...'F'
                // esc-char    : '"', '\\', '/', 'b', 'f', 'n', 'r', 't', 'u'

static constexpr uint8_t const kCharClass[256] = {
//  NUL     SOH     STX     ETX     EOT     ENQ     ACK     BEL     BS      HT      LF      VT      FF      CR      SO      SI
    0,      0,      0,      0,      0,      0,      0,      0,      0,      W,      W,      0,      0,      W,      0,      0,
//  DLE     DC1     DC2     DC3     DC4     NAK     SYN     ETB     CAN     EM      SUB     ESC     FS      GS      RS      US
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
//  space   !       "       #       $       %       &       '       (       )       *       +       ,       -       .       /
    W,      0,      0,      0,      I,      0,      0,      0,      0,      0,      0,      N,      0,      N,      N,      0,
//  0       1       2       3       4       5       6       7       8       9       :       ;       <       =       >       ?
    D|N|I,  D|N|I,  D|N|I,  D|N|I,  D|N|I,  D|N|I,  D|N|I,  D|N|I,  D|N|I,  D|N|I,  0,      0,      0,      0,      0,      0,
//  @       A       B       C       D       E       F       G       H       I       J       K       L       M       N       O
    0,      X|I,    I,      I,      I,      N|I,    X|I,    I,      I,      X|I,    I,      I,      I,      I,      X|I,    I,
//  P       Q       R       S       T       U       V       W       X       Y       Z       [       \       ]       ^       _
    I,      I,      I,      I,      X|I,    I,      I,      I,      I,      X|I,    I,      0,      0,      0,      0,      I,
//  `       a       b       c       d       e       f       g       h       i       j       k       l       m       n       o
    0,      X|I,    I,      I,      I,      N|I,    X|I,    I,      I,      X|I,    I,      I,      I,      I,      X|I,    I,
//  p       q       r       s       t       u       v       w       x       y       z       {       |       }       ~       DEL
    I,      I,      I,      I,      X|I,    I,      I,      I,      I,      X|I,    I,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
    0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
};

inline bool IsWhitespace     (char ch) { return (kCharClass[static_cast<unsigned char>(ch)] & W) != 0; }
inline bool IsDigit          (char ch) { return (kCharClass[static_cast<unsigned char>(ch)] & D) != 0; }
inline bool IsNumberBody     (char ch) { return (kCharClass[static_cast<unsigned char>(ch)] & N) != 0; }
inline bool IsIdentifierBody (char ch) { return (kCharClass[static_cast<unsigned char>(ch)] & I) != 0; }

#undef I
#undef N
#undef X
#undef D
#undef W

inline char const* SkipWhitespace(char const* f, char const* l)
{
    for ( ; f != l && IsWhitespace(*f); ++f)
    {
    }
    return f;
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

inline char const* ScanNaNString(char const* next, char const* last)
{
    auto const len = last - next;

    if (len >= 3 && std::memcmp(next, "NaN", 3) == 0) { return next + 3; }
#if 0
    if (len >= 3 && std::memcmp(next, "nan", 3) == 0) { return next + 3; }
    if (len >= 3 && std::memcmp(next, "NAN", 3) == 0) { return next + 3; }
#endif

    return nullptr;
}

inline char const* ScanInfString(char const* next, char const* last)
{
    auto const len = last - next;

    if (len >= 8 && std::memcmp(next, "Infinity", 8) == 0) { return next + 8; }
#if 0
    if (len >= 8 && std::memcmp(next, "infinity", 8) == 0) { return next + 8; }
    if (len >= 8 && std::memcmp(next, "INFINITY", 8) == 0) { return next + 8; }
    if (len >= 3 && std::memcmp(next, "Inf",      3) == 0) { return next + 3; }
    if (len >= 3 && std::memcmp(next, "inf",      3) == 0) { return next + 3; }
    if (len >= 3 && std::memcmp(next, "INF",      3) == 0) { return next + 3; }
#endif

    return nullptr;
}

inline bool IsNaNString(char const* f, char const* l)
{
    auto const end = ScanNaNString(f, l);
    return end != nullptr && end == l;
}

inline bool IsInfString(char const* f, char const* l)
{
    auto const end = ScanInfString(f, l);
    return end != nullptr && end == l;
}

template <typename It>
struct ScanNumberResult
{
    It end;
    NumberClass number_class;
};

template <typename It>
ScanNumberResult<It> ScanNumber(It next, It last, json::Options const& options)
{
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
    else if (options.allow_leading_plus && *next == '+')
    {
        ++next;
        if (next == last)
            return {next, NumberClass::invalid};
    }

// NaN/Infinity

    if (options.allow_nan_inf)
    {
        if (auto const end = ScanNaNString(next, last))
            return {end, is_neg ? NumberClass::neg_nan : NumberClass::pos_nan};

        if (auto const end = ScanInfString(next, last))
            return {end, is_neg ? NumberClass::neg_infinity : NumberClass::pos_infinity};
    }

// int

    if (*next == '0')
    {
        ++next;
        if (next == last)
            return {next, is_neg ? NumberClass::neg_integer : NumberClass::pos_integer};
        if (IsDigit(*next))
            return {next, NumberClass::invalid};
    }
    else if (IsDigit(*next)) // non '0'
    {
        for (;;)
        {
            ++next;
            if (next == last)
                return {next, is_neg ? NumberClass::neg_integer : NumberClass::pos_integer};
            if (!IsDigit(*next))
                break;
        }
    }
    else if (options.allow_leading_dot && *next == '.')
    {
        // Parsed again below.
    }
    else
    {
        return {next, NumberClass::invalid};
    }

// frac

    if (*next == '.')
    {
        is_float = true;

        ++next;
        if (next == last)
            return {next, NumberClass::invalid};

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

    return {next, is_float ? NumberClass::floating_point
                           : is_neg ? NumberClass::neg_integer
                                    : NumberClass::pos_integer};
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

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

    Token LexString    (char const* p, char quote_char);
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
    case '\'':
        if (options.allow_single_quoted_strings)
            return LexString(p, '\'');
        break;
    case '"':
        return LexString(p, '"');
    case '+':
        if (options.allow_leading_plus)
            return LexNumber(p, options);
        break;
    case '.':
        if (options.allow_leading_dot)
            return LexNumber(p, options);
        break;
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
        if (options.allow_comments)
        {
            auto tok = LexComment(p);
            if (tok.kind != TokenKind::comment)
                return tok;
            goto L_again;
        }
        break;
    default:
        break;
    }

    ++p;
    return MakeToken(p, kind);
}

inline Token Lexer::LexString(char const* p, char quote_char)
{
    assert(p != end);
    assert(*p == quote_char);

    bool needs_cleaning = false;

    ptr = ++p; // skip " or '

    for (;;)
    {
        if (p == end)
            return MakeToken(p, TokenKind::incomplete_string, needs_cleaning);

        auto const ch = *p;
        auto const uc = static_cast<unsigned char>(ch);

        if (uc == quote_char)
            break;

        ++p;
        if (uc < 0x20) // Unescaped control character
        {
            needs_cleaning = true;
        }
        else if (uc >= 0x80) // Possibly a UTF-8 lead byte (sequence length >= 2)
        {
            needs_cleaning = true;
        }
        else if (ch == '\\')
        {
            if (p != end) // Skip the escaped character.
                ++p;

            needs_cleaning = true;
        }
    }

    auto tok = MakeToken(p, TokenKind::string, needs_cleaning);

    ptr = ++p; // skip " or '

    return tok;
}

inline Token Lexer::LexNumber(char const* p, Options const& options)
{
    auto const res = ScanNumber(p, end, options);

    return MakeToken(res.end, TokenKind::number, /*needs_cleaning*/ false, res.number_class);
}

inline Token Lexer::LexIdentifier(char const* p)
{
    for ( ; p != end && IsIdentifierBody(*p); ++p)
    {
    }

    return MakeToken(p, TokenKind::identifier);
}

inline Token Lexer::LexComment(char const* p)
{
    assert(p != end);
    assert(*p == '/');

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

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

struct Failed
{
    ParseStatus ec;

    Failed(ParseStatus ec_) : ec(ec_) {}
    operator ParseStatus() const noexcept { return ec; }

    // Test for failure.
    explicit operator bool() const noexcept { return ec != ParseStatus::success; }
};

struct Parser
{
    static constexpr int kMaxDepth = 500;

    ParseCallbacks& cb;
    Options         options;
    Lexer           lexer;
    Token           token; // The next token.

    Parser(ParseCallbacks& cb_, Options const& options_);

    ParseStatus ParseObject     (int depth);
    ParseStatus ParsePair       (int depth);
    ParseStatus ParseArray      (int depth);
    ParseStatus ParseString     (int depth);
    ParseStatus ParseNumber     (int depth);
    ParseStatus ParseIdentifier (int depth);
    ParseStatus ParseValue      (int depth);
};

inline Parser::Parser(ParseCallbacks& cb_, Options const& options_)
    : cb(cb_)
    , options(options_)
{
}

//
//  object
//      {}
//      { members }
//  members
//      pair
//      pair , members
//
inline ParseStatus Parser::ParseObject(int depth)
{
    assert(token.kind == TokenKind::l_brace);

    if (depth >= kMaxDepth)
        return ParseStatus::max_depth_reached;

    // skip '{'
    token = lexer.Lex(options);

    if (Failed ec = cb.HandleBeginObject(options))
        return ec;

    size_t num_members = 0;
    if (token.kind != TokenKind::r_brace)
    {
        for (;;)
        {
            if (Failed ec = ParsePair(depth + 1))
                return ec;

            ++num_members;

            if (Failed ec = cb.HandleEndMember(num_members, options))
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

    if (Failed ec = cb.HandleEndObject(num_members, options))
        return ec;

    // skip '}'
    token = lexer.Lex(options);

    return ParseStatus::success;
}

//
//  pair
//      string : value
//
inline ParseStatus Parser::ParsePair(int depth)
{
    if (depth >= kMaxDepth)
        return ParseStatus::max_depth_reached;

    if (token.kind != TokenKind::string && (!options.allow_unquoted_keys || token.kind != TokenKind::identifier))
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
    return ParseValue(depth);
}

//
//  array
//      []
//      [ elements ]
//  elements
//      value
//      value , elements
//
inline ParseStatus Parser::ParseArray(int depth)
{
    assert(token.kind == TokenKind::l_square);

    if (depth >= kMaxDepth)
        return ParseStatus::max_depth_reached;

    // skip '['
    token = lexer.Lex(options);

    if (Failed ec = cb.HandleBeginArray(options))
        return ec;

    size_t num_elements = 0;
    if (token.kind != TokenKind::r_square)
    {
        for (;;)
        {
            if (Failed ec = ParseValue(depth + 1))
                return ec;

            ++num_elements;

            if (Failed ec = cb.HandleEndElement(num_elements, options))
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

    if (Failed ec = cb.HandleEndArray(num_elements, options))
        return ec;

    // skip ']'
    token = lexer.Lex(options);

    return ParseStatus::success;
}

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
inline ParseStatus Parser::ParseString(int /*depth*/)
{
    assert(token.kind == TokenKind::string);

    if (Failed ec = cb.HandleString(token.ptr, token.end, token.needs_cleaning, options))
        return ec;

    // skip string
    token = lexer.Lex(options);

    return ParseStatus::success;
}

//
//  number
//     int
//     int frac
//     int exp
//     int frac exp
//  int
//     digit
//     digit1-9 digits
//     - digit
//     - digit1-9 digits
//  frac
//     . digits
//  exp
//     e digits
//  digits
//     digit
//     digit digits
//  e
//     e
//     e+
//     e-
//     E
//     E+
//     E-
//
inline ParseStatus Parser::ParseNumber(int /*depth*/)
{
    assert(token.kind == TokenKind::number);

    if (token.number_class == NumberClass::invalid)
        return ParseStatus::invalid_number;

    if (Failed ec = cb.HandleNumber(token.ptr, token.end, token.number_class, options))
        return ec;

    // skip number
    token = lexer.Lex(options);

    return ParseStatus::success;
}

//
//  true
//  false
//  null
//
inline ParseStatus Parser::ParseIdentifier(int /*depth*/)
{
    assert(token.kind == TokenKind::identifier);
    assert(token.end - token.ptr > 0 && "internal error");

    auto const f = token.ptr;
    auto const l = token.end;
    auto const len = l - f;

    ParseStatus ec;
    if (len == 4 && std::memcmp(f, "null", 4) == 0)
    {
        ec = cb.HandleNull(options);
    }
    else if (len == 4 && std::memcmp(f, "true", 4) == 0)
    {
        ec = cb.HandleBoolean(true, options);
    }
    else if (len == 5 && std::memcmp(f, "false", 5) == 0)
    {
        ec = cb.HandleBoolean(false, options);
    }
    else if (options.allow_nan_inf && IsNaNString(f, l))
    {
        ec = cb.HandleNumber(f, l, NumberClass::pos_nan, options);
    }
    else if (options.allow_nan_inf && IsInfString(f, l))
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

//
//  value
//      string
//      number
//      object
//      array
//      true
//      false
//      null
//
inline ParseStatus Parser::ParseValue(int depth)
{
    if (depth >= kMaxDepth)
        return ParseStatus::max_depth_reached;

    switch (token.kind)
    {
    case TokenKind::l_brace:
        return ParseObject(depth);
    case TokenKind::l_square:
        return ParseArray(depth);
    case TokenKind::string:
        return ParseString(depth);
    case TokenKind::number:
        return ParseNumber(depth);
    case TokenKind::identifier:
        return ParseIdentifier(depth);
    case TokenKind::eof:
        return ParseStatus::unexpected_eof;
    default:
        return ParseStatus::unexpected_token;
    }
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

ParseResult json::parse(ParseCallbacks& cb, char const* next, char const* last, Options const& options)
{
    assert(next != nullptr);
    assert(last != nullptr);

    if (options.skip_bom && last - next >= 3)
    {
        if (static_cast<unsigned char>(next[0]) == 0xEF &&
            static_cast<unsigned char>(next[1]) == 0xBB &&
            static_cast<unsigned char>(next[2]) == 0xBF)
        {
            next += 3;
        }
    }

    Parser parser(cb, options);

    parser.lexer = Lexer(next, last);
    parser.token = parser.lexer.Lex(options); // Get the first token

    auto /*const*/ ec = parser.ParseValue(0);

    if (ec == ParseStatus::success)
    {
        if (!options.allow_trailing_characters && parser.token.kind != TokenKind::eof)
        {
            ec = ParseStatus::expected_eof;
        }
    }

    //
    // XXX:
    // Return token.kind on error?!?!
    //

    return {ec, parser.token.ptr, parser.token.end};
}
