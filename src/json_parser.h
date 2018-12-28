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

#if defined(__GNUC__) || defined(__clang__)
#define JSON_FORCE_INLINE __attribute__((always_inline)) inline
#define JSON_NEVER_INLINE __attribute__((noinline)) inline
#elif defined(_MSC_VER)
#define JSON_FORCE_INLINE __forceinline
#define JSON_NEVER_INLINE __declspec(noinline) inline
#else
#define JSON_FORCE_INLINE inline
#define JSON_NEVER_INLINE inline
#endif

namespace json {

//==================================================================================================
// CharClass
//==================================================================================================

namespace charclass {

enum /*class*/ ECharClass : uint8_t {
    CC_none             = 0,    // nothing special
    CC_string_special   = 0x01, // quote or bs : '"', '\\'
    CC_digit            = 0x02, // digit       : '0'...'9'
    CC_identifier_body  = 0x04, // ident-body  : IsDigit, IsLetter, '_', '$'
    CC_whitespace       = 0x08, // whitespace  : '\t', '\n', '\r', ' '
    CC_identifier_start = 0x20,
    CC_punctuation      = 0x40, // punctuation : '[', ']', '{', '}', ',', ':'
    CC_needs_cleaning   = 0x80, // needs cleaning (strings)
};

inline uint32_t CharClass(char ch)
{
    enum : uint8_t {
        S  = CC_string_special,
        D  = CC_digit,
        I  = CC_identifier_body,
        W  = CC_whitespace,
        A  = CC_identifier_start,
        P  = CC_punctuation,
        C  = CC_needs_cleaning,
        ID = I|A,
        BS = I|A|C|S,   // backslash
        U8 = I|A|C,     // >= 0x80
    };

    static constexpr uint8_t const kMap[] = {
    //  NUL   SOH   STX   ETX   EOT   ENQ   ACK   BEL   BS    HT    LF    VT    FF    CR    SO    SI
        C,    C,    C,    C,    C,    C,    C,    C,    C,    W|C,  W|C,  C,    C,    W|C,  C,    C,
    //  DLE   DC1   DC2   DC3   DC4   NAK   SYN   ETB   CAN   EM    SUB   ESC   FS    GS    RS    US
        C,    C,    C,    C,    C,    C,    C,    C,    C,    C,    C,    C,    C,    C,    C,    C,
    //  space !     "     #     $     %     &     '     (     )     *     +     ,     -     .     /
        W,    0,    S,    0,    I,    0,    0,    0,    0,    0,    0,    0,    P,    0,    0,    0,
    //  0     1     2     3     4     5     6     7     8     9     :     ;     <     =     >     ?
        D|I,  D|I,  D|I,  D|I,  D|I,  D|I,  D|I,  D|I,  D|I,  D|I,  P,    0,    0,    0,    0,    0,
    //  @     A     B     C     D     E     F     G     H     I     J     K     L     M     N     O
        0,    ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,
    //  P     Q     R     S     T     U     V     W     X     Y     Z     [     \     ]     ^     _
        ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   P,    BS,   P,    0,    ID,
    //  `     a     b     c     d     e     f     g     h     i     j     k     l     m     n     o
        0,    ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,
    //  p     q     r     s     t     u     v     w     x     y     z     {     |     }     ~     DEL
        ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   ID,   P,    0,    P,    0,    0,
        U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,
        U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,
        U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,
        U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,
        U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,
        U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,
        U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,
        U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,   U8,
    };

    return kMap[static_cast<uint8_t>(ch)];
}

inline bool IsWhitespace     (char ch) { return (CharClass(ch) & CC_whitespace      ) != 0; }
#if 1
inline bool IsDigit          (char ch) { return '0' <= ch && ch <= '9'; }
#else
inline bool IsDigit          (char ch) { return (CharClass(ch) & CC_digit           ) != 0; }
#endif
inline bool IsIdentifierStart(char ch) { return (CharClass(ch) & CC_identifier_start) != 0; }
inline bool IsIdentifierBody (char ch) { return (CharClass(ch) & CC_identifier_body ) != 0; }
inline bool IsPunctuation    (char ch) { return (CharClass(ch) & CC_punctuation     ) != 0; }

inline bool IsSeparator(char ch)
{
    return (CharClass(ch) & (CC_whitespace | CC_punctuation)) != 0;
}

} // namespace charclass

//==================================================================================================
// Util
//==================================================================================================

namespace impl {

inline bool StrEqual(char const* str, char const* expected, intptr_t n)
{
    JSON_ASSERT(str != nullptr);
    JSON_ASSERT(expected != nullptr);
    JSON_ASSERT(n >= 0);

#if 1
    return std::memcmp(str, expected, static_cast<size_t>(n)) == 0;
#else
    for (intptr_t i = 0; i < n; ++i) {
        if (str[i] != expected[i])
            return false;
    }
    return true;
#endif
}

} // namespace impl

//==================================================================================================
// Options
//==================================================================================================

struct Options
{
    // If true, skip UTF-8 byte order mark - if any.
    // Default is true.
    bool skip_bom = true;

    // If true, skip line comments (introduced with "//") and block
    // comments like "/* hello */".
    // Default is false.
    bool skip_comments = false;

    // If true, parses "NaN" and "Infinity" (without the quotes) as numbers.
    // Default is false.
    bool allow_nan_inf = false;

    // If true, allows trailing commas in arrays or objects.
    // Default is false.
    bool allow_trailing_commas = false;

    // If true, allow unquoted keys in objects.
    // Default is false.
    bool allow_unquoted_keys = false;

    // If true, allow characters after value.
    // Might be used to parse strings like "[1,2,3]{"hello":"world"}" into
    // different values by repeatedly calling parse.
    // Default is false.
    bool allow_trailing_characters = false;

    // If true, don't require commas to separate object members or array elements.
    // Default is false.
    bool ignore_missing_commas = false;
};

//==================================================================================================
// ScanNumber
//==================================================================================================

enum class NumberClass : uint8_t {
    invalid,
    integer,
    integer_with_exponent,
    decimal,
    decimal_with_exponent,
    nan,
    pos_infinity,
    neg_infinity,
};

struct ScanNumberResult
{
    char const* next;
    NumberClass number_class;
};

// PRE: next points at '0', ..., '9', or '-'
inline ScanNumberResult ScanNumber(char const* next, char const* last, Options const& options = {})
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
    else if (options.allow_nan_inf && last - next >= 8 && json::impl::StrEqual(next, "Infinity", 8))
    {
        return {next + 8, is_neg ? NumberClass::neg_infinity : NumberClass::pos_infinity};
    }
    else if (options.allow_nan_inf && last - next >= 3 && json::impl::StrEqual(next, "NaN", 3))
    {
        return {next + 3, NumberClass::nan};
    }
    else
    {
        return {next, NumberClass::invalid};
    }

// frac

    bool const has_decimal_point = (*next == '.');
    if (has_decimal_point)
    {
        ++next;
        if (next == last || !IsDigit(*next))
            return {next, NumberClass::invalid};

        for (;;)
        {
            ++next;
            if (next == last)
                return {next, NumberClass::decimal};
            if (!IsDigit(*next))
                break;
        }
    }

// exp

    bool const has_exponent = (*next == 'e' || *next == 'E');
    if (has_exponent)
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
                break;
        }
    }

    NumberClass nc;
    if (has_decimal_point)
        nc = has_exponent ? NumberClass::decimal_with_exponent : NumberClass::decimal;
    else
        nc = has_exponent ? NumberClass::integer_with_exponent : NumberClass::integer;

    return {next, nc};
}

//==================================================================================================
// ScanString
//==================================================================================================

enum class StringClass : uint8_t {
    plain_ascii,
    needs_cleaning,
};

//==================================================================================================
// Lexer
//==================================================================================================

enum class TokenKind : uint8_t {
    unknown,
    invalid_character,
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
    incomplete_string,
    incomplete_comment,
};

struct Token
{
    char const* ptr = nullptr;
    char const* end = nullptr;
    TokenKind kind = TokenKind::unknown;
    union
    {
        StringClass string_class; // Valid iff kind == TokenKind::string || kind == TokenKind::identifier
        NumberClass number_class; // Valid iff kind == TokenKind::number
    };
};

class Lexer
{
private:
    char const* ptr = nullptr;
    char const* end = nullptr;

public:
    void SetInput(char const* first, char const* last, bool skip_bom = true);

    Token Lex(Options const& options);

private:
    Token LexString     (char const* p);
    Token LexNumber     (char const* p, Options const& options);
    Token LexIdentifier (char const* p);
    Token LexComment    (char const* p);

    Token MakeToken(char const* p, TokenKind kind);

    static char const* SkipWhitespace(char const* f, char const* l);
};

inline void Lexer::SetInput(char const* first, char const* last, bool skip_bom)
{
    if (skip_bom && last - first >= 3)
    {
        if (static_cast<uint8_t>(first[0]) == 0xEF &&
            static_cast<uint8_t>(first[1]) == 0xBB &&
            static_cast<uint8_t>(first[2]) == 0xBF)
        {
            first += 3;
        }
    }

    ptr = first;
    end = last;
}

JSON_NEVER_INLINE Token Lexer::Lex(Options const& options)
{
L_again:
    ptr = SkipWhitespace(ptr, end);

    char const* p = ptr;

    if (p == end)
        return MakeToken(p, TokenKind::eof);

    TokenKind kind;

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
    case '.':
    case '+':
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
    case '/':
        {
            auto const tok = LexComment(p);
            if (options.skip_comments && tok.kind == TokenKind::comment)
                goto L_again;

            return tok;
        }
        break;
    default:
        if (json::charclass::IsIdentifierStart(ch))
            return LexIdentifier(p);
        kind = TokenKind::invalid_character;
        break;
    }

    ++p;
    return MakeToken(p, kind);
}

inline Token Lexer::LexString(char const* p)
{
    using namespace json::charclass;

    JSON_ASSERT(p != end);
    JSON_ASSERT(*p == '"');

    ptr = ++p; // skip "

    uint32_t mask = 0;
    while (p != end)
    {
        auto const m = CharClass(*p);
        mask |= m;
        if ((m & CC_string_special) != 0)
        {
            if (*p == '"' || ++p == end)
                break;
        }
        ++p;
    }

    JSON_ASSERT(p == end || *p == '"');

    Token tok;

    tok.ptr = ptr;
    tok.end = p;
    tok.kind = (p == end) ? TokenKind::incomplete_string : TokenKind::string;
    tok.string_class = ((mask & CC_needs_cleaning) != 0) ? StringClass::needs_cleaning : StringClass::plain_ascii;
//  tok.number_class = 0;

    ptr = (p == end)
        ? p
        : p + 1; // skip "

    return tok;
}

inline Token Lexer::LexNumber(char const* p, Options const& options)
{
    using json::charclass::IsSeparator;

    auto const res = ScanNumber(p, end, options);

    p = res.next;
    auto nc = res.number_class;

    if (nc == NumberClass::invalid || (p != end && !IsSeparator(*p)))
    {
        // Invalid number,
        // or valid number with trailing garbage, which is also an invalid number.
        nc = NumberClass::invalid;

        // Skip everything which looks like a number.
        // For slightly nicer error messages.
        // Everything which is not whitespace or punctuation will be skipped.
        for ( ; p != end && !IsSeparator(*p); ++p)
        {
        }
    }

    Token tok;

    tok.ptr = ptr;
    tok.end = p;
    tok.kind = TokenKind::number;
//  tok.string_class = 0;
    tok.number_class = nc;

    ptr = p;

    return tok;
}

inline Token Lexer::LexIdentifier(char const* p)
{
    using namespace json::charclass;

    JSON_ASSERT(p != end);
    JSON_ASSERT(IsIdentifierStart(*p));
    JSON_ASSERT(IsIdentifierBody(*p));

    // Don't skip identifier start here.
    // Might have the set needs_cleaning flag below.

    uint32_t mask = 0;
    while (p != end)
    {
        auto const m = CharClass(*p);
        mask |= m;
        if ((m & CC_identifier_body) == 0)
            break;
        ++p;
    }

    Token tok;

    tok.ptr = ptr;
    tok.end = p;
    tok.kind = TokenKind::identifier;
    tok.string_class = (mask & CC_needs_cleaning) != 0 ? StringClass::needs_cleaning : StringClass::plain_ascii;
//  tok.number_class = 0;

    ptr = p;

    return tok;
}

inline Token Lexer::LexComment(char const* p)
{
    JSON_ASSERT(p != end);
    JSON_ASSERT(*p == '/');

    TokenKind kind = TokenKind::invalid_character;

    ++p; // Skip '/'

    if (p != end)
    {
        if (*p == '/')
        {
            kind = TokenKind::comment;

            for (++p; p != end; ++p)
            {
                if (*p == '\n' || *p == '\r')
                    break;
            }
        }
        else if (*p == '*')
        {
            kind = TokenKind::incomplete_comment;

            for (++p; p != end; /**/)
            {
                char const ch = *p;
                ++p;
                if (ch == '*')
                {
                    if (p == end)
                        break;
                    if (*p == '/')
                    {
                        ++p;
                        kind = TokenKind::comment;
                        break;
                    }
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
//  tok.string_class = 0;
//  tok.number_class = 0;

    ptr = p;

    return tok;
}

inline char const* Lexer::SkipWhitespace(char const* f, char const* l)
{
    using json::charclass::IsWhitespace;

    for ( ; f != l && IsWhitespace(*f); ++f)
    {
    }

    return f;
}

//==================================================================================================
// Parser
//==================================================================================================

enum class ParseStatus : uint8_t {
    success,
    duplicate_key,
    expected_colon_after_key,
    expected_comma_or_closing_brace,
    expected_comma_or_closing_bracket,
    expected_eof,
    expected_key,
    expected_value,
    invalid_key,
    invalid_number,
    invalid_string,
    invalid_value,
    max_depth_reached,
    unexpected_eof,
    unknown,
    unrecognized_identifier,
};

struct Failed
{
    ParseStatus ec;

    Failed(ParseStatus ec_) : ec(ec_) {}
    explicit operator bool() const noexcept { return ec != ParseStatus::success; }
    explicit operator ParseStatus() const noexcept { return ec; }
};

template <typename ParseCallbacks>
class Parser
{
    static constexpr uint32_t kMaxDepth = 500;

    ParseCallbacks& cb;
    Options         options;
    Lexer           lexer;
    Token           token; // The next token.

public:
    Parser(ParseCallbacks& cb_, Options const& options_);

    void SetInput(char const* next, char const* last);

    Token CurrToken() const;
    Token Lex();

private:
    ParseStatus ParsePrimitive();

public:
    ParseStatus ParseValue();

private:
    bool AdvanceToNextValue(TokenKind close);
};

template <typename ParseCallbacks>
Parser<ParseCallbacks>::Parser(ParseCallbacks& cb_, Options const& options_)
    : cb(cb_)
    , options(options_)
{
}

template <typename ParseCallbacks>
void Parser<ParseCallbacks>::SetInput(char const* next, char const* last)
{
    lexer.SetInput(next, last, options.skip_bom);
    token = lexer.Lex(options); // Get the first token
}

template <typename ParseCallbacks>
Token Parser<ParseCallbacks>::CurrToken() const
{
    return token;
}

template <typename ParseCallbacks>
Token Parser<ParseCallbacks>::Lex()
{
    token = lexer.Lex(options);
    return token;
}

template <typename ParseCallbacks>
ParseStatus Parser<ParseCallbacks>::ParsePrimitive()
{
    JSON_ASSERT(token.kind != TokenKind::l_brace && token.kind != TokenKind::l_square);

    auto const f = token.ptr;
    auto const l = token.end;
    auto const len = l - f;

    ParseStatus ec;
    switch (token.kind)
    {
    case TokenKind::string:
        ec = cb.HandleString(token.ptr, token.end, token.string_class);
        break;
    case TokenKind::number:
        if (token.number_class != NumberClass::invalid)
        {
            ec = cb.HandleNumber(token.ptr, token.end, token.number_class);
        }
        else
        {
            ec = ParseStatus::invalid_number;
        }
        break;
    case TokenKind::identifier:
        if (len == 4 && json::impl::StrEqual(f, "null", 4))
        {
            ec = cb.HandleNull();
        }
        else if (len == 4 && json::impl::StrEqual(f, "true", 4))
        {
            ec = cb.HandleTrue();
        }
        else if (len == 5 && json::impl::StrEqual(f, "false", 5))
        {
            ec = cb.HandleFalse();
        }
        else if (options.allow_nan_inf && len == 8 && json::impl::StrEqual(f, "Infinity", 8))
        {
            ec = cb.HandleNumber(f, l, NumberClass::pos_infinity);
        }
        else if (options.allow_nan_inf && len == 3 && json::impl::StrEqual(f, "NaN", 3))
        {
            ec = cb.HandleNumber(f, l, NumberClass::nan);
        }
        else
        {
            ec = ParseStatus::unrecognized_identifier;
        }
        break;
    case TokenKind::eof:
        ec = ParseStatus::unexpected_eof;
        break;
    default:
        ec = ParseStatus::expected_value;
        break;
    }

    if (ec == ParseStatus::success)
    {
        // Skip 'string', 'number', or 'identifier'
        Lex();
    }

    return ec;
}

template <typename ParseCallbacks>
ParseStatus Parser<ParseCallbacks>::ParseValue()
{
    enum class Structure : uint8_t {
        object,
        array,
    };

    struct StackElement {
        size_t count; // number of elements or members in the current array resp. object
        Structure structure;
    };

    uint32_t stack_size = 0;
    StackElement stack[kMaxDepth];

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

    stack[stack_size] = {0, Structure::object};
    ++stack_size;

    if (Failed ec = cb.HandleBeginObject())
        return ParseStatus(ec);

    Lex(); // skip '{'

    if (token.kind != TokenKind::r_brace)
    {
        for (;;)
        {
            if (token.kind != TokenKind::string && (!options.allow_unquoted_keys || token.kind != TokenKind::identifier))
                return ParseStatus::expected_key;

            if (Failed ec = cb.HandleKey(token.ptr, token.end, token.string_class))
                return ParseStatus(ec);

            Lex(); // skip 'key'

            if (token.kind != TokenKind::colon)
                return ParseStatus::expected_colon_after_key;

            Lex(); // skip ':'

            // parse 'value'
            if (token.kind == TokenKind::l_brace)
                goto L_begin_object;
            if (token.kind == TokenKind::l_square)
                goto L_begin_array;

            if (Failed ec = ParsePrimitive())
                return ParseStatus(ec);

L_end_member:
            JSON_ASSERT(stack_size != 0);
            JSON_ASSERT(stack[stack_size - 1].structure == Structure::object);
            ++stack[stack_size - 1].count;

            if (Failed ec = cb.HandleEndMember(stack[stack_size - 1].count))
                return ParseStatus(ec);

            if (!AdvanceToNextValue(TokenKind::r_brace))
                break;
        }

        if (token.kind != TokenKind::r_brace)
            return ParseStatus::expected_comma_or_closing_brace;
    }

    if (Failed ec = cb.HandleEndObject(stack[stack_size - 1].count))
        return ParseStatus(ec);

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

    stack[stack_size] = {0, Structure::array};
    ++stack_size;

    if (Failed ec = cb.HandleBeginArray())
        return ParseStatus(ec);

    Lex(); // skip '['

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
                return ParseStatus(ec);

L_end_element:
            JSON_ASSERT(stack_size != 0);
            JSON_ASSERT(stack[stack_size - 1].structure == Structure::array);
            ++stack[stack_size - 1].count;

            if (Failed ec = cb.HandleEndElement(stack[stack_size - 1].count))
                return ParseStatus(ec);

            if (!AdvanceToNextValue(TokenKind::r_square))
                break;
        }

        if (token.kind != TokenKind::r_square)
            return ParseStatus::expected_comma_or_closing_bracket;
    }

    if (Failed ec = cb.HandleEndArray(stack[stack_size - 1].count))
        return ParseStatus(ec);

    goto L_end_structured;

L_end_structured:
    Lex(); // skip '}' or ']'

    JSON_ASSERT(stack_size != 0);
    --stack_size;

    if (stack_size == 0)
        return ParseStatus::success;

    if (stack[stack_size - 1].structure == Structure::object)
        goto L_end_member;
    else
        goto L_end_element;
}

template <typename ParseCallbacks>
bool Parser<ParseCallbacks>::AdvanceToNextValue(TokenKind close)
{
    if (token.kind == TokenKind::comma)
    {
        Lex(); // skip ','
        if (options.allow_trailing_commas)
            return token.kind != close;
        return true;
    }
    else
    {
        if (options.ignore_missing_commas)
            return token.kind != close;
        return false;
    }
}

//==================================================================================================
// ParseSAX
//==================================================================================================

struct ParseResult
{
    ParseStatus ec = ParseStatus::unknown;
    // On return, TOKEN contains the last token which has been read by the parser.
    // In case of an error, this contains the invalid token.
    Token token;
};

template <typename ParseCallbacks>
ParseResult ParseSAX(ParseCallbacks& cb, char const* next, char const* last, Options const& options = {})
{
    JSON_ASSERT(next != nullptr);
    JSON_ASSERT(last != nullptr);

    Parser<ParseCallbacks> parser(cb, options);

    parser.SetInput(next, last);
    ParseStatus ec = parser.ParseValue();

    Token last_read = parser.CurrToken();

    if (ec == ParseStatus::success)
    {
        if (!options.allow_trailing_characters && last_read.kind != TokenKind::eof)
        {
            ec = ParseStatus::expected_eof;
        }
    }

    return {ec, last_read};
}

} // namespace json

//==================================================================================================
//
//==================================================================================================

//struct ParseCallbacks
//{
//    virtual json::ParseStatus HandleNull() = 0;
//    virtual json::ParseStatus HandleTrue() = 0;
//    virtual json::ParseStatus HandleFalse() = 0;
//    virtual json::ParseStatus HandleNumber(char const* first, char const* last, json::NumberClass nc) = 0;
//    virtual json::ParseStatus HandleString(char const* first, char const* last, json::StringClass sc) = 0;
//    virtual json::ParseStatus HandleKey(char const* first, char const* last, json::StringClass sc) = 0;
//    virtual json::ParseStatus HandleBeginArray() = 0;
//    virtual json::ParseStatus HandleEndArray(size_t count) = 0;
//    virtual json::ParseStatus HandleEndElement(size_t& count) = 0;
//    virtual json::ParseStatus HandleBeginObject() = 0;
//    virtual json::ParseStatus HandleEndObject(size_t count) = 0;
//    virtual json::ParseStatus HandleEndMember(size_t& count) = 0;
//};
//
//json::ParseResult ParseJson(ParseCallbacks& cb, char const* next, char const* last, json::Options const& options = {})
//{
//    return json::ParseSAX(cb, next, last, options);
//}
