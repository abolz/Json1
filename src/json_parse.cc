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

#include "json_charclass.h"

#include <cassert>
#include <cstdint>
#include <cstring>

#ifndef JSON_ASSERT
#define JSON_ASSERT(X) assert(X)
#endif

using namespace json;

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

template <typename It>
static It SkipWhitespace(It f, It l)
{
    using namespace json::charclass;

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

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

namespace {

template <typename It>
struct ScanNumberResult
{
    It next;
    NumberClass number_class;
};

template <typename It>
static ScanNumberResult<It> ScanNumber(It next, It last, Options const& options)
{
    using namespace json::charclass;

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

} // namespace

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

namespace {

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

Lexer::Lexer()
{
}

Lexer::Lexer(char const* first, char const* last)
    : src(first)
    , end(last)
    , ptr(first)
{
}

Token Lexer::MakeToken(char const* p, TokenKind kind, bool needs_cleaning, NumberClass number_class)
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

Token Lexer::Lex(Options const& options)
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

Token Lexer::LexString(char const* p)
{
    using namespace json::charclass;

    JSON_ASSERT(p != end);
    JSON_ASSERT(*p == '"');

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

        JSON_ASSERT(ch == '\\');
        ++p;
        if (p == end)
            break;
        ++p; // Skip the escaped character.
    }

L_incomplete:
    return MakeToken(p, TokenKind::incomplete_string, (mask & CC_NeedsCleaning) != 0);
}

Token Lexer::LexNumber(char const* p, Options const& options)
{
    auto const res = ScanNumber(p, end, options);

    return MakeToken(res.next, TokenKind::number, /*needs_cleaning*/ false, res.number_class);
}

Token Lexer::LexIdentifier(char const* p)
{
    using namespace json::charclass;

    for ( ; p != end && IsIdentifierBody(*p); ++p)
    {
    }

    return MakeToken(p, TokenKind::identifier);
}

Token Lexer::LexComment(char const* p)
{
    JSON_ASSERT(p != end);
    JSON_ASSERT(*p == '/');

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

} // namespace

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

namespace {

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

    ParseStatus ParseString();
    ParseStatus ParseNumber();
    ParseStatus ParseIdentifier();
    ParseStatus ParsePrimitive();
    ParseStatus ParseValue();
};

Parser::Parser(ParseCallbacks& cb_, Options const& options_)
    : cb(cb_)
    , options(options_)
{
}

ParseStatus Parser::ParseString()
{
    JSON_ASSERT(token.kind == TokenKind::string);

    if (Failed ec = cb.HandleString(token.ptr, token.end, token.needs_cleaning, options))
        return ec;

    // skip string
    token = lexer.Lex(options);

    return ParseStatus::success;
}

ParseStatus Parser::ParseNumber()
{
    JSON_ASSERT(token.kind == TokenKind::number);

    if (token.number_class == NumberClass::invalid)
        return ParseStatus::invalid_number;

    if (Failed ec = cb.HandleNumber(token.ptr, token.end, token.number_class, options))
        return ec;

    // skip number
    token = lexer.Lex(options);

    return ParseStatus::success;
}

ParseStatus Parser::ParseIdentifier()
{
    JSON_ASSERT(token.kind == TokenKind::identifier);
    JSON_ASSERT(token.end - token.ptr > 0 && "internal error");

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

ParseStatus Parser::ParsePrimitive()
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

ParseStatus Parser::ParseValue()
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
            JSON_ASSERT(stack_size != 0);
            JSON_ASSERT(stack[stack_size - 1].structure == Structure::object);
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
    JSON_ASSERT(token.kind == TokenKind::l_square);

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
            JSON_ASSERT(stack_size != 0);
            JSON_ASSERT(stack[stack_size - 1].structure == Structure::array);
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
    JSON_ASSERT(stack_size != 0);
    stack_size--;

    if (stack_size == 0)
        return ParseStatus::success;

    if (stack[stack_size - 1].structure == Structure::object)
        goto L_end_member;
    else
        goto L_end_element;
}

} // namespace

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

ParseResult json::parse(ParseCallbacks& cb, char const* next, char const* last, Options const& options)
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

    Parser parser(cb, options);

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
