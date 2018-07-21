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
#include <cstddef>
#include <cstdint>
#include <cstring>

#ifndef JSON_ASSERT
#define JSON_ASSERT(X) assert(X)
#endif

//#if _MSC_VER
//#define JSON_FORCE_INLINE __forceinline
//#define JSON_NEVER_INLINE __declspec(noinline) inline
//#elif __GNUC__
//#define JSON_FORCE_INLINE __attribute__((always_inline)) inline
//#define JSON_NEVER_INLINE __attribute__((noinline)) inline
//#else
//#define JSON_FORCE_INLINE inline
//#define JSON_NEVER_INLINE inline
//#endif

#ifndef JSON_USE_SSE42
#define JSON_USE_SSE42 1
#endif

#if JSON_USE_SSE42
#include <nmmintrin.h>
#endif

namespace json {

//==================================================================================================
// CharClass
//==================================================================================================

namespace charclass {

enum : uint8_t {
    CC_None             = 0,    // nothing special
    CC_StringSpecial    = 0x01, // quote or bs : '"', '\'', '`', '\\'
    CC_Digit            = 0x02, // digit       : '0'...'9'
    CC_IdentifierBody   = 0x04, // ident-body  : IsDigit, IsLetter, '_', '$'
    CC_Whitespace       = 0x08, // whitespace  : '\t', '\n', '\r', ' '
    CC_EscapeSpecial    = 0x10,
    CC_NumberBody       = 0x20,
    CC_NeedsCleaning    = 0x80, // needs cleaning (strings)
};

inline unsigned CharClass(char ch)
{
    enum : uint8_t {
        S = CC_StringSpecial,
        D = CC_Digit,
        I = CC_IdentifierBody,
        W = CC_Whitespace,
        E = CC_EscapeSpecial,
        N = CC_NumberBody,
        C = CC_NeedsCleaning,
    };

    static constexpr uint8_t const kMap[] = {
    //  NUL     SOH     STX     ETX     EOT     ENQ     ACK     BEL     BS      HT      LF      VT      FF      CR      SO      SI
        C,      C,      C,      C,      C,      C,      C,      C,      C,      W|C,    W|C,    C,      C,      W|C,    C,      C,
    //  DLE     DC1     DC2     DC3     DC4     NAK     SYN     ETB     CAN     EM      SUB     ESC     FS      GS      RS      US
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
    //  space   !       "       #       $       %       &       '       (       )       *       +       ,       -       .       /
        W,      0,      S,      0,      I,      0,      0,      0,      0,      0,      0,      N,      0,      N,      N,      E,
    //  0       1       2       3       4       5       6       7       8       9       :       ;       <       =       >       ?
        D|I|N,  D|I|N,  D|I|N,  D|I|N,  D|I|N,  D|I|N,  D|I|N,  D|I|N,  D|I|N,  D|I|N,  0,      0,      0,      0,      0,      0,
    //  @       A       B       C       D       E       F       G       H       I       J       K       L       M       N       O
        0,      I,      I,      I,      I,      I|N,    I,      I,      I,      I|N,    I,      I,      I,      I,      I|N,    I,
    //  P       Q       R       S       T       U       V       W       X       Y       Z       [       \       ]       ^       _
        I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      0,      S|C,    0,      0,      I,
    //  `       a       b       c       d       e       f       g       h       i       j       k       l       m       n       o
        0,      I|N,    I,      I,      I,      I|N,    I|N,    I,      I,      I|N,    I,      I,      I,      I,      I|N,    I,
    //  p       q       r       s       t       u       v       w       x       y       z       {       |       }       ~       DEL
        I,      I,      I,      I,      I|N,    I,      I,      I,      I,      I|N,    I,      0,      0,      0,      0,      0,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
    };

    return kMap[static_cast<unsigned char>(ch)];
}

#if 0
inline bool IsWhitespace      (char ch) { return ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r'; }
#else
inline bool IsWhitespace      (char ch) { return (CharClass(ch) & CC_Whitespace      ) != 0; }
#endif
#if 1
inline bool IsDigit           (char ch) { return '0' <= ch && ch <= '9'; }
#else
inline bool IsDigit           (char ch) { return (CharClass(ch) & CC_Digit           ) != 0; }
#endif
inline bool IsIdentifierBody  (char ch) { return (CharClass(ch) & CC_IdentifierBody  ) != 0; }
#if 0
inline bool IsNumberBody      (char ch) { return (CharClass(ch) & CC_NumberBody      ) != 0; }
inline bool IsStringSpecial   (char ch) { return (CharClass(ch) & CC_StringSpecial   ) != 0; }
inline bool NeedsCleaning     (char ch) { return (CharClass(ch) & CC_NeedsCleaning   ) != 0; }
#endif

} // namespace charclass

//==================================================================================================
// Options
//==================================================================================================

struct Options
{
    // If true, skip line comments (introduced with "//") and block
    // comments like "/* hello */".
    // Default is false.
    bool skip_comments = false;

    // If true, parses "NaN" and "Infinity" (without the quotes) as numbers.
    // Default is true.
    bool allow_nan_inf = true;

    // If true, skip UTF-8 byte order mark - if any.
    // Default is true.
    bool skip_bom = true;

    // If true, allows trailing commas in arrays or objects.
    // Default is false.
    bool allow_trailing_comma = false;

    // If true, allow unquoted keys in objects.
    // Default is false.
    bool allow_unquoted_keys = false;

    // If true, assume all strings contain valid UTF-8.
    // Default is false.
    //bool assume_valid_unicode = false;

    // If true, parse numbers as raw strings.
    // Default is false.
    bool parse_numbers_as_strings = false;

    // If true, allow characters after value.
    // Might be used to parse strings like "[1,2,3]{"hello":"world"}" into
    // different values by repeatedly calling parse.
    // Default is false.
    bool allow_trailing_characters = false;

    // If true, ignore "null" values in the "parse" and "stringify" methods.
    // Default is false.
    //bool ignore_null_values = false;

    // If >= 0, pretty-print the JSON.
    // Default is < 0, that is the JSON is rendered as the shortest string possible.
    int indent_width = -1;
};

//==================================================================================================
// ScanNumber
//==================================================================================================

enum class NumberClass : unsigned char {
    invalid,
    nan,
    pos_infinity,
    neg_infinity,
    integer,
    floating_point,
};

template <typename It>
struct ScanNumberResult
{
    It next;
    NumberClass number_class;
};

// PRE: next points at '0', ..., '9', or '-'
template <typename InpIt>
ScanNumberResult<InpIt> ScanNumber(InpIt next, InpIt last, Options const& options = {})
{
    using json::charclass::IsDigit;

    if (next == last)
        return {next, NumberClass::invalid};

// [-]

    bool const is_neg = (*next == '-');
    if (is_neg)
    {
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
    else if (options.allow_nan_inf && last - next >= 3 && std::memcmp(next, "NaN", 3) == 0)
    {
        return {next + 3, NumberClass::nan};
    }
    else if (options.allow_nan_inf && last - next >= 8 && std::memcmp(next, "Infinity", 8) == 0)
    {
        return {next + 8, is_neg ? NumberClass::neg_infinity : NumberClass::pos_infinity};
    }
    else
    {
        return {next, NumberClass::invalid};
    }

// frac

    bool const is_float = (*next == '.');
    if (is_float)
    {
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
            if (next == last || !IsDigit(*next))
                return {next, NumberClass::floating_point};
        }
    }

    return {next, is_float ? NumberClass::floating_point : NumberClass::integer};
}

//==================================================================================================
// ScanString
//==================================================================================================

enum class StringClass : unsigned char {
    plain_ascii,
    needs_cleaning,
};

//template <typename It>
//struct ScanStringResult
//{
//    It next;
//    StringClass string_class;
//};
//
//// PRE: next points at '"'
//template <typename InpIt>
//ScanStringResult<InpIt> ScanNumber(InpIt next, InpIt last)
//{
//}

//==================================================================================================
// Lexer
//==================================================================================================

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
    number,
    identifier,
    comment,
    incomplete, // Incomplete string or C-style comment
};

struct Token
{
    char const* ptr = nullptr;
    char const* end = nullptr;
    TokenKind   kind = TokenKind::unknown;
    union {
        StringClass string_class;
        NumberClass number_class;
    };
};

struct Lexer
{
    char const* ptr = nullptr; // position in [src, end)
    char const* end = nullptr;

    Lexer();
    explicit Lexer(char const* first, char const* last);

    Token Lex(Options const& options);

    Token LexString    (char const* p/*, char quote_char*/);
    Token LexNumber    (char const* p, Options const& options);
    Token LexIdentifier(char const* p);
    Token LexComment   (char const* p);

    Token MakeToken       (char const* p, TokenKind kind);
    Token MakeStringToken (char const* p, StringClass string_class);
    Token MakeNumberToken (char const* p, NumberClass number_class);

    static char const* SkipWhitespace(char const* f, char const* l);
};

inline Lexer::Lexer()
{
}

inline Lexer::Lexer(char const* first, char const* last)
    : ptr(first)
    , end(last)
{
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
        if (options.skip_comments)
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

inline Token Lexer::LexString(char const* p/*, char quote_char*/)
{
    using namespace json::charclass;

    JSON_ASSERT(p != end);
    JSON_ASSERT(*p == '"');

    ptr = ++p; // skip " or '

    unsigned mask = 0;
    for (;;)
    {
#if 1
#if JSON_USE_SSE42
        const __m128i special_chars = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, '\xFF', '\x80', '\x1F', '\x00', '\\', '\\', '\"', '\"');

        for ( ; end - p >= 16; p += 16)
        {
            const auto bytes = _mm_loadu_si128(reinterpret_cast<__m128i const*>(p));
            const auto index = _mm_cmpestri(special_chars, 8, bytes, 16, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_LEAST_SIGNIFICANT);
            if (index != 16) {
                p += index;
                break;
            }
        }
#else
        for ( ; end - p >= 4; p += 4)
        {
            auto const m = CharClass(p[0]) | CharClass(p[1]) | CharClass(p[2]) | CharClass(p[3]);
            if ((m & CC_StringSpecial) != 0)
                break;
            mask |= m;
        }
#endif
#endif

        for ( ; p != end; ++p)
        {
            auto const m = CharClass(*p);
            mask |= m;
            if ((m & CC_StringSpecial) != 0)
                break;
        }

        if (p == end)
            break;

        if (*p == '"')
        {
            auto tok = MakeStringToken(p, (mask & CC_NeedsCleaning) == 0 ? StringClass::plain_ascii : StringClass::needs_cleaning);
            ptr = ++p; // skip " or '
            return tok;
        }

        JSON_ASSERT(*p == '\\');
        ++p;
        if (p == end)
            break;
        ++p; // Skip the escaped character.
    }

    return MakeToken(p, TokenKind::incomplete); // incomplete string
}

inline Token Lexer::LexNumber(char const* p, Options const& options)
{
    auto const res = json::ScanNumber(p, end, options);

    return MakeNumberToken(res.next, res.number_class);
}

inline Token Lexer::LexIdentifier(char const* p)
{
    using namespace json::charclass;

#if 1
    for ( ; end - p >= 4; p += 4)
    {
        auto const m0 = CharClass(p[0]);
        auto const m1 = CharClass(p[1]);
        auto const m2 = CharClass(p[2]);
        auto const m3 = CharClass(p[3]);
        auto const m = m0 & m1 & m2 & m3;
        if ((m & CC_IdentifierBody) == 0)
            break;
    }
#endif

    for ( ; p != end && IsIdentifierBody(*p); ++p)
    {
    }

    return MakeToken(p, TokenKind::identifier);
}

inline Token Lexer::LexComment(char const* p)
{
    JSON_ASSERT(p != end);
    JSON_ASSERT(*p == '/');

    TokenKind kind = TokenKind::unknown;

    ++p; // Skip '/'

    if (p == end)
    {
    }
    else if (*p == '/')
    {
        kind = TokenKind::comment;

        for (;;)
        {
            ++p;
            if (p == end)
                break;
            if (*p == '\n' || *p == '\r')
                break;
        }
    }
    else if (*p == '*')
    {
        kind = TokenKind::incomplete;

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
    }

    return MakeToken(p, kind);
}

inline Token Lexer::MakeToken(char const* p, TokenKind kind)
{
    Token tok;

    tok.ptr = ptr;
    tok.end = p;
    tok.kind = kind;

    ptr = p;

    return tok;
}

inline Token Lexer::MakeStringToken(char const* p, StringClass string_class)
{
    Token tok;

    tok.ptr = ptr;
    tok.end = p;
    tok.kind = TokenKind::string;
    tok.string_class = string_class;

    ptr = p;

    return tok;
}

inline Token Lexer::MakeNumberToken(char const* p, NumberClass number_class)
{
    Token tok;

    tok.ptr = ptr;
    tok.end = p;
    tok.kind = TokenKind::number;
    tok.number_class = number_class;

    ptr = p;

    return tok;
}

inline char const* Lexer::SkipWhitespace(char const* f, char const* l)
{
    using namespace json::charclass;

    for ( ; f != l && IsWhitespace(*f); ++f)
    {
    }

    return f;
}

//==================================================================================================
// Parser
//==================================================================================================

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

namespace impl {
    struct Failed
    {
        ParseStatus ec;

        Failed(ParseStatus ec_) : ec(ec_) {}
        operator ParseStatus() const noexcept { return ec; }

        // Test for failure.
        explicit operator bool() const noexcept { return ec != ParseStatus::success; }
    };
}

//struct ParseCallbacks
//{
//    ParseStatus HandleNull(Options const& options);
//    ParseStatus HandleBoolean(bool value, Options const& options);
//    ParseStatus HandleNumber(char const* first, char const* last, NumberClass nc, Options const& options);
//    ParseStatus HandleString(char const* first, char const* last, StringClass sc, Options const& options);
//    ParseStatus HandleBeginArray(Options const& options);
//    ParseStatus HandleEndArray(size_t count, Options const& options);
//    ParseStatus HandleEndElement(size_t& count, Options const& options);
//    ParseStatus HandleBeginObject(Options const& options);
//    ParseStatus HandleEndObject(size_t count, Options const& options);
//    ParseStatus HandleEndMember(size_t& count, Options const& options);
//    ParseStatus HandleKey(char const* first, char const* last, StringClass sc, Options const& options);
//};

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
    JSON_ASSERT(token.kind == TokenKind::string);

    if (impl::Failed ec = cb.HandleString(token.ptr, token.end, token.string_class, options))
        return ec;

    // skip string
    token = lexer.Lex(options);

    return ParseStatus::success;
}

template <typename ParseCallbacks>
ParseStatus Parser<ParseCallbacks>::ParseNumber()
{
    JSON_ASSERT(token.kind == TokenKind::number);

    if (token.number_class == NumberClass::invalid)
        return ParseStatus::invalid_number;

    if (impl::Failed ec = cb.HandleNumber(token.ptr, token.end, token.number_class, options))
        return ec;

    // skip number
    token = lexer.Lex(options);

    return ParseStatus::success;
}

template <typename ParseCallbacks>
ParseStatus Parser<ParseCallbacks>::ParseIdentifier()
{
    JSON_ASSERT(token.kind == TokenKind::identifier);
    JSON_ASSERT(token.end - token.ptr > 0 && "internal error");

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

    if (impl::Failed(ec))
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
    JSON_ASSERT(token.kind == TokenKind::l_brace);

    if (stack_size >= kMaxDepth)
        return ParseStatus::max_depth_reached;

    stack[stack_size++] = {Structure::object, 0};

    // skip '{'
    token = lexer.Lex(options);

    if (impl::Failed ec = cb.HandleBeginObject(options))
        return ec;

    if (token.kind != TokenKind::r_brace)
    {
        for (;;)
        {
            if (token.kind != TokenKind::string && (!options.allow_unquoted_keys || token.kind != TokenKind::identifier))
                return ParseStatus::expected_key;

            if (impl::Failed ec = cb.HandleKey(token.ptr, token.end, token.string_class, options))
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

            if (impl::Failed ec = ParsePrimitive())
                return ec;

L_end_member:
            JSON_ASSERT(stack_size != 0);
            JSON_ASSERT(stack[stack_size - 1].structure == Structure::object);
            stack[stack_size - 1].count++;

            if (impl::Failed ec = cb.HandleEndMember(stack[stack_size - 1].count, options))
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

    if (impl::Failed ec = cb.HandleEndObject(stack[stack_size - 1].count, options))
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
    JSON_ASSERT(token.kind == TokenKind::l_square);

    if (stack_size >= kMaxDepth)
        return ParseStatus::max_depth_reached;

    stack[stack_size++] = {Structure::array, 0};

    // skip '['
    token = lexer.Lex(options);

    if (impl::Failed ec = cb.HandleBeginArray(options))
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

            if (impl::Failed ec = ParsePrimitive())
                return ec;

L_end_element:
            JSON_ASSERT(stack_size != 0);
            JSON_ASSERT(stack[stack_size - 1].structure == Structure::array);
            stack[stack_size - 1].count++;

            if (impl::Failed ec = cb.HandleEndElement(stack[stack_size - 1].count, options))
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

    if (impl::Failed ec = cb.HandleEndArray(stack[stack_size - 1].count, options))
        return ec;

    // skip ']'
    token = lexer.Lex(options);
    goto L_end_structured;

L_end_structured:
    JSON_ASSERT(stack_size != 0);
    stack_size--;

    if (stack_size == 0)
        return ParseStatus::success;

    if (stack[stack_size - 1].structure == Structure::object)
        goto L_end_member;
    else
        goto L_end_element;
}

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

template <typename ParseCallbacks>
ParseResult ParseSAX(ParseCallbacks& cb, char const* next, char const* last, Options const& options = {})
{
    JSON_ASSERT(next != nullptr);
    JSON_ASSERT(last != nullptr);

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

    auto ec = parser.ParseValue();

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

} // namespace json