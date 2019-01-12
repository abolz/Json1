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

//#if defined(__GNUC__) || defined(__clang__)
//#define JSON_FORCE_INLINE __attribute__((always_inline)) inline
//#define JSON_NEVER_INLINE __attribute__((noinline)) inline
//#elif defined(_MSC_VER)
//#define JSON_FORCE_INLINE __forceinline
//#define JSON_NEVER_INLINE __declspec(noinline) inline
//#else
//#define JSON_FORCE_INLINE inline
//#define JSON_NEVER_INLINE inline
//#endif

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
// ScanNumber
//==================================================================================================

enum class NumberClass : uint8_t {
    invalid,
    integer,
    integer_with_exponent,
    decimal,
    decimal_with_exponent,
    //nan,
    //pos_infinity,
    //neg_infinity,
};

struct ScanNumberResult
{
    char const* next;
    NumberClass number_class;
};

// PRE: next points at '0', ..., '9', or '-'
inline ScanNumberResult ScanNumber(char const* next, char const* last)
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

    NumberClass const nc = has_decimal_point
        ? (has_exponent ? NumberClass::decimal_with_exponent : NumberClass::decimal)
        : (has_exponent ? NumberClass::integer_with_exponent : NumberClass::integer);

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
    incomplete_string,
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

    Token Lex();

private:
    Token LexString     (char const* p);
    Token LexNumber     (char const* p);
    Token LexIdentifier (char const* p);

    Token MakeToken(char const* p, TokenKind kind);
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

inline Token Lexer::Lex()
{
    using namespace json::charclass;

    char const* p = ptr;
    for ( ; p != end && IsWhitespace(*p); ++p)
    {
    }

    ptr = p; // Mark start of next token.

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
        return LexNumber(p);
    default:
        if (IsIdentifierStart(ch))
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

    ++p; // Skip '"'

    uint32_t mask = 0;
    while (p != end)
    {
        uint32_t const m = CharClass(*p);
        mask |= m;
        if ((m & CC_string_special) != 0)
        {
            if (*p == '"' || ++p == end)
                break;
        }
        ++p;
    }

    JSON_ASSERT(p == end || *p == '"');

    bool const is_incomplete = (p == end);
    if (!is_incomplete)
        ++p; // Skip '"'

    Token tok;

    tok.ptr = ptr;
    tok.end = p;
    tok.kind = is_incomplete ? TokenKind::incomplete_string : TokenKind::string;
    tok.string_class = ((mask & CC_needs_cleaning) != 0) ? StringClass::needs_cleaning : StringClass::plain_ascii;
//  tok.number_class = 0;

    ptr = p;

    return tok;
}

inline Token Lexer::LexNumber(char const* p)
{
    using json::charclass::IsSeparator;

    ScanNumberResult const res = json::ScanNumber(p, end);

    p = res.next;
    NumberClass nc = res.number_class;

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
        uint32_t const m = CharClass(*p);
        mask |= m;
        if ((m & CC_identifier_body) == 0)
            break;
        ++p;
    }

    Token tok;

    tok.ptr = ptr;
    tok.end = p;
    tok.kind = TokenKind::identifier;
    tok.string_class = ((mask & CC_needs_cleaning) != 0) ? StringClass::needs_cleaning : StringClass::plain_ascii;
//  tok.number_class = 0;

    ptr = p;

    return tok;
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

//==================================================================================================
// Parser
//==================================================================================================

enum class ParseStatus : uint8_t {
    success,
    expected_colon_after_key,
    expected_comma_or_closing_brace,
    expected_comma_or_closing_bracket,
    expected_eof,
    expected_key,
    expected_value,
    invalid_key,
    invalid_number,
    invalid_string,
    max_depth_reached,
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
    Lexer           lexer;
    Token           peek; // The next token.

public:
    Parser(ParseCallbacks& cb_);

    void Init(char const* next, char const* last);

    // Returns the current token.
    Token GetPeekToken() const;

    // Read the next token from the input stream.
    Token Lex();

    // Extract the next JSON value from the input
    // and check whether EOF has been reached.
    ParseStatus Parse();

    // Extract the next JSON value from the input.
    ParseStatus ParseValue();

private:
    ParseStatus ConsumePrimitive();
};

template <typename ParseCallbacks>
Parser<ParseCallbacks>::Parser(ParseCallbacks& cb_)
    : cb(cb_)
{
}

template <typename ParseCallbacks>
void Parser<ParseCallbacks>::Init(char const* next, char const* last)
{
    lexer.SetInput(next, last, /*skip_bom*/ true);

    // Get the first token.
    Lex();
}

template <typename ParseCallbacks>
Token Parser<ParseCallbacks>::GetPeekToken() const
{
    return peek;
}

template <typename ParseCallbacks>
Token Parser<ParseCallbacks>::Lex()
{
    peek = lexer.Lex();
    return peek;
}

template <typename ParseCallbacks>
ParseStatus Parser<ParseCallbacks>::Parse()
{
    ParseStatus ec = ParseValue();

    if (ec == ParseStatus::success)
    {
        if (peek.kind != TokenKind::eof)
        {
            ec = ParseStatus::expected_eof;
        }
    }

    return ec;
}

template <typename ParseCallbacks>
ParseStatus Parser<ParseCallbacks>::ParseValue()
{
    struct StackElement {
        size_t count; // number of elements or members in the current array resp. object
        TokenKind close;
    };

    uint32_t stack_size = 0;
    StackElement stack[kMaxDepth];

    // The first token has been read in SetInput()
    // or in the last call to ParseValue().

    if (peek.kind == TokenKind::l_brace)
        goto L_begin_object;
    if (peek.kind == TokenKind::l_square)
        goto L_begin_array;

    if (Failed ec = ConsumePrimitive())
        return ParseStatus(ec);

    return ParseStatus::success;

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
    JSON_ASSERT(peek.kind == TokenKind::l_brace);

    if (stack_size >= kMaxDepth)
        return ParseStatus::max_depth_reached;

    stack[stack_size] = {0, TokenKind::r_brace};
    ++stack_size;

    if (Failed ec = cb.HandleBeginObject())
        return ParseStatus(ec);

    // Read the token after the '{'.
    // This must be either an '}' or a key.
    Lex();

    if (peek.kind != TokenKind::r_brace)
    {
        for (;;)
        {
            if (peek.kind != TokenKind::string)
                return ParseStatus::expected_key;

            {
                JSON_ASSERT(peek.end - peek.ptr >= 2);
                JSON_ASSERT(peek.ptr[ 0] == '"');
                JSON_ASSERT(peek.end[-1] == '"');
                char const* I = peek.ptr + 1; // Discard leading '"'
                char const* E = peek.end - 1; // Discard trailing '"'

                if (Failed ec = cb.HandleKey(I, E, peek.string_class))
                    return ParseStatus(ec);
            }

            // Read the token after the key.
            // This must be a ':'.
            Lex();

            if (peek.kind != TokenKind::colon)
                return ParseStatus::expected_colon_after_key;

            // Read the token after the ':'.
            // This must be a JSON value.
            Lex();

            if (peek.kind == TokenKind::l_brace)
                goto L_begin_object;
            if (peek.kind == TokenKind::l_square)
                goto L_begin_array;

            if (Failed ec = ConsumePrimitive())
                return ParseStatus(ec);

L_end_member:
            JSON_ASSERT(stack_size != 0);
            JSON_ASSERT(stack[stack_size - 1].close == TokenKind::r_brace);
            ++stack[stack_size - 1].count;

            if (Failed ec = cb.HandleEndMember(stack[stack_size - 1].count))
                return ParseStatus(ec);

            // Expect a ',' (and another member) or a closing '}'.
            if (peek.kind == TokenKind::comma)
            {
                // Read the token after the ','.
                // This must be a key.
                Lex();
            }
            else
            {
                break;
            }
        }

        if (peek.kind != TokenKind::r_brace)
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
    JSON_ASSERT(peek.kind == TokenKind::l_square);

    if (stack_size >= kMaxDepth)
        return ParseStatus::max_depth_reached;

    stack[stack_size] = {0, TokenKind::r_square};
    ++stack_size;

    if (Failed ec = cb.HandleBeginArray())
        return ParseStatus(ec);

    // Read the token after '['.
    // This must be a ']' or any JSON value.
    Lex();

    if (peek.kind != TokenKind::r_square)
    {
        for (;;)
        {
            if (peek.kind == TokenKind::l_brace)
                goto L_begin_object;
            if (peek.kind == TokenKind::l_square)
                goto L_begin_array;

            if (Failed ec = ConsumePrimitive())
                return ParseStatus(ec);

L_end_element:
            JSON_ASSERT(stack_size != 0);
            JSON_ASSERT(stack[stack_size - 1].close == TokenKind::r_square);
            ++stack[stack_size - 1].count;

            if (Failed ec = cb.HandleEndElement(stack[stack_size - 1].count))
                return ParseStatus(ec);

            // Expect a ',' (and another element) or a closing ']'.
            if (peek.kind == TokenKind::comma)
            {
                // Read the token after the ','.
                // This must be a JSON value.
                Lex();
            }
            else
            {
                break;
            }
        }

        if (peek.kind != TokenKind::r_square)
            return ParseStatus::expected_comma_or_closing_bracket;
    }

    if (Failed ec = cb.HandleEndArray(stack[stack_size - 1].count))
        return ParseStatus(ec);

    goto L_end_structured;

L_end_structured:
    // Read the token after '}' or ']'.
    // Parser::peek refers to the next token the parser needs to look at.
    Lex();

    JSON_ASSERT(stack_size != 0);
    --stack_size;

    if (stack_size == 0)
        return ParseStatus::success;

    if (stack[stack_size - 1].close == TokenKind::r_brace)
        goto L_end_member;
    else
        goto L_end_element;
}

template <typename ParseCallbacks>
ParseStatus Parser<ParseCallbacks>::ConsumePrimitive()
{
    JSON_ASSERT(peek.kind != TokenKind::l_brace && peek.kind != TokenKind::l_square);

    auto const f = peek.ptr;
    auto const l = peek.end;
    auto const len = l - f;

    ParseStatus ec;
    switch (peek.kind)
    {
    case TokenKind::string:
        {
            JSON_ASSERT(peek.end - peek.ptr >= 2);
            JSON_ASSERT(peek.ptr[ 0] == '"');
            JSON_ASSERT(peek.end[-1] == '"');
            char const* I = peek.ptr + 1; // Discard leading '"'
            char const* E = peek.end - 1; // Discard trailing '"'

            ec = cb.HandleString(I, E, peek.string_class);
        }
        break;
    case TokenKind::number:
        if (peek.number_class != NumberClass::invalid)
        {
            ec = cb.HandleNumber(peek.ptr, peek.end, peek.number_class);
        }
        else
        {
            ec = ParseStatus::invalid_number;
        }
        break;
    case TokenKind::identifier:
        if (len == 4 && std::memcmp(f, "null", 4) == 0)
        {
            ec = cb.HandleNull();
        }
        else if (len == 4 && std::memcmp(f, "true", 4) == 0)
        {
            ec = cb.HandleTrue();
        }
        else if (len == 5 && std::memcmp(f, "false", 5) == 0)
        {
            ec = cb.HandleFalse();
        }
        else
        {
            ec = ParseStatus::unrecognized_identifier;
        }
        break;
    default:
        ec = ParseStatus::expected_value;
        break;
    }

    if (ec == ParseStatus::success)
    {
        // Peek the next token after 'string', 'number', or 'identifier'.
        Lex();
    }

    return ec;
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
ParseResult ParseSAX(ParseCallbacks& cb, char const* next, char const* last)
{
    JSON_ASSERT(next != nullptr);
    JSON_ASSERT(last != nullptr);

    Parser<ParseCallbacks> parser(cb);

    parser.Init(next, last);

    auto const ec = parser.Parse();
    auto const token = parser.GetPeekToken();

    return {ec, token};
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
//json::ParseResult ParseJson(ParseCallbacks& cb, char const* next, char const* last)
//{
//    return json::ParseSAX(cb, next, last);
//}
