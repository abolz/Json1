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

#define JSON_USE_SSE42 1
#ifndef JSON_USE_SSE42
#if defined(__SSE_4_2__) || (/* for MSVC: */ defined(__AVX__) || defined(__AVX2__))
#define JSON_USE_SSE42 1
#endif
#endif

#if JSON_USE_SSE42
#include <nmmintrin.h>
#endif

namespace json {

// UTF-8 code unit
//using char8_t = char;

//==================================================================================================
// CharClass
//==================================================================================================

namespace charclass {

enum : uint8_t {
    CC_None             = 0,    // nothing special
    CC_StringSpecial    = 0x01, // quote or bs : '"', '\\'
    CC_Digit            = 0x02, // digit       : '0'...'9'
    CC_IdentifierBody   = 0x04, // ident-body  : IsDigit, IsLetter, '_', '$'
    CC_Whitespace       = 0x08, // whitespace  : '\t', '\n', '\r', ' '
    CC_Punctuation      = 0x40, // punctuation : '[', ']', '{', '}', ',', ':'
    CC_NeedsCleaning    = 0x80, // needs cleaning (strings)
};

inline uint32_t CharClass(char ch)
{
    enum : uint8_t {
        S = CC_StringSpecial,
        D = CC_Digit,
        I = CC_IdentifierBody,
        W = CC_Whitespace,
        P = CC_Punctuation,
        C = CC_NeedsCleaning,
    };

    static constexpr uint8_t const kMap[] = {
    //  NUL     SOH     STX     ETX     EOT     ENQ     ACK     BEL     BS      HT      LF      VT      FF      CR      SO      SI
        C,      C,      C,      C,      C,      C,      C,      C,      C,      W|C,    W|C,    C,      C,      W|C,    C,      C,
    //  DLE     DC1     DC2     DC3     DC4     NAK     SYN     ETB     CAN     EM      SUB     ESC     FS      GS      RS      US
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
    //  space   !       "       #       $       %       &       '       (       )       *       +       ,       -       .       /
        W,      0,      S,      0,      I,      0,      0,      0,      0,      0,      0,      0,      P,      0,      0,      0,
    //  0       1       2       3       4       5       6       7       8       9       :       ;       <       =       >       ?
        D|I,    D|I,    D|I,    D|I,    D|I,    D|I,    D|I,    D|I,    D|I,    D|I,    P,      0,      0,      0,      0,      0,
    //  @       A       B       C       D       E       F       G       H       I       J       K       L       M       N       O
        0,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,
    //  P       Q       R       S       T       U       V       W       X       Y       Z       [       \       ]       ^       _
        I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      P,      S|C,    P,      0,      I,
    //  `       a       b       c       d       e       f       g       h       i       j       k       l       m       n       o
        0,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,
    //  p       q       r       s       t       u       v       w       x       y       z       {       |       }       ~       DEL
        I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      I,      P,      0,      P,      0,      0,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
        C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,      C,
    };

    return kMap[static_cast<uint8_t>(ch)];
}

inline bool IsWhitespace     (char ch) { return (CharClass(ch) & CC_Whitespace    ) != 0; }
inline bool IsDigit          (char ch) { return (CharClass(ch) & CC_Digit         ) != 0; }
inline bool IsIdentifierBody (char ch) { return (CharClass(ch) & CC_IdentifierBody) != 0; }
inline bool IsPunctuation    (char ch) { return (CharClass(ch) & CC_Punctuation   ) != 0; }

inline bool IsSeparator(char ch)
{
    return (CharClass(ch) & (CC_Whitespace | CC_Punctuation)) != 0;
}

} // namespace charclass

//==================================================================================================
// Options
//==================================================================================================

struct Options {
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

    // If true, parse numbers as raw strings.
    // Default is false.
    bool parse_numbers_as_strings = false;

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
    nan,
    pos_infinity,
    neg_infinity,
    integer,
    floating_point,
};

struct ScanNumberResult {
    char const* next;
    NumberClass number_class;
};

//inline bool StartsWith(char const* next, char const* last, char const* prefix)
//{
//}
//
//inline bool StartsWithCaseInsensitive(char const* next, char const* last, char const* lower_case_prefix)
//{
//}
//
//inline ScanNumberResult ScanInfinity(char const* next, char const* last)
//{
//}
//
//inline ScanNumberResult ScanNaN(char const* next, char const* last)
//{
//}

// PRE: next points at '0', ..., '9', or '-'
inline ScanNumberResult ScanNumber(char const* next, char const* last, Options const& options)
{
    using ::json::charclass::IsDigit;
    using ::json::charclass::IsSeparator;

    bool is_neg = false;
    bool is_float = false;

    if (next == last)
        goto L_invalid;

// [-]

    if (*next == '-')
    {
        is_neg = true;

        ++next;
        if (next == last)
            goto L_invalid;
    }
    //else if (options.allow_leading_plus && *next == '+')
    //{
    //    ++next;
    //    if (next == last)
    //        goto L_invalid;
    //}

// int

    if (*next == '0')
    {
        ++next;
        if (next == last)
            goto L_success;
        if (IsDigit(*next))
            goto L_invalid;
    }
    else if (IsDigit(*next)) // non '0'
    {
        for (;;)
        {
            ++next;
            if (next == last)
                goto L_success;
            if (!IsDigit(*next))
                break;
        }
    }
    //else if (options.allow_leading_dot && *next == '.')
    //{
    //    // Will be scanned again below.
    //}
    else if (options.allow_nan_inf && last - next >= 8 && std::memcmp(next, "Infinity", 8) == 0)
    {
        return {next + 8, is_neg ? NumberClass::neg_infinity : NumberClass::pos_infinity};
    }
    //else if (options.allow_nan_inf && last - next >= 3 && std::memcmp(next, "Inf", 3) == 0)
    //{
    //    return {next + 8, is_neg ? NumberClass::neg_infinity : NumberClass::pos_infinity};
    //}
    else if (options.allow_nan_inf && last - next >= 3 && std::memcmp(next, "NaN", 3) == 0)
    {
        return {next + 3, NumberClass::nan};
    }
    else
    {
        goto L_invalid;
    }

// frac

    if (*next == '.')
    {
        is_float = true;

        ++next;
        if (next == last || !IsDigit(*next))
            goto L_invalid;

        for (;;)
        {
            ++next;
            if (next == last)
                goto L_success;
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
            goto L_invalid;

        if (*next == '+' || *next == '-')
        {
            ++next;
            if (next == last)
                goto L_invalid;
        }

        if (!IsDigit(*next))
            goto L_invalid;

        for (;;)
        {
            ++next;
            if (next == last || !IsDigit(*next))
                break;
        }
    }

L_success:
    if (next == last || IsSeparator(*next))
    {
        return {next, is_float ? NumberClass::floating_point : NumberClass::integer};
    }

L_invalid:
    // Skip everything which looks like a number. For slightly nicer error messages.
    // Everything which is not whitespace or punctuation will be skipped.
    for ( ; next != last && !IsSeparator(*next); ++next)
    {
    }

    return {next, NumberClass::invalid};
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
    TokenKind kind = TokenKind::unknown;
    union {
//      uint8_t _init_class_ = 0;
        StringClass string_class;
        NumberClass number_class;
    };
};

class Lexer
{
private:
//  char const* src = nullptr;
    char const* ptr = nullptr; // position in [src, end)
    char const* end = nullptr;

public:
    Lexer();
    explicit Lexer(char const* first, char const* last);

    Token Lex(Options const& options);

private:
    Token LexString     (char const* p, char quote_char);
    Token LexNumber     (char const* p, Options const& options);
    Token LexIdentifier (char const* p, Options const& options);
    Token LexComment    (char const* p);

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

    char const* p = ptr;

    if (p == end)
        return MakeToken(p, TokenKind::eof);

    TokenKind kind = TokenKind::unknown;

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
    //case '\'':
    //    if (!options.allow_single_quoted_strings)
    //        break;
    //    [[fallthrough]];
    case '"':
        return LexString(p, ch);
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
        return LexIdentifier(p, options);
    case '/':
        if (options.skip_comments)
        {
            auto const tok = LexComment(p);
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

inline Token Lexer::LexString(char const* p, char quote_char)
{
    using namespace ::json::charclass;

    JSON_ASSERT(p != end);
    JSON_ASSERT(*p == '"' || *p == '\'');
    JSON_ASSERT(*p == quote_char);

    ptr = ++p; // skip " or '

#if JSON_USE_SSE42
    auto const kSpecialChars = _mm_set_epi8(0, 0, 0, 0, 0, 0, '\xFF', '\x80', '\x1F', '\x00', '\\', '\\', '\'', '\'', '"', '"');
    int special_chars_length = 10;
#endif

    uint32_t mask = 0;
    for (;;)
    {
#if JSON_USE_SSE42
        // XXX:
        // Consider _mm_cmpestrm and skip UTF-8 sequences while setting the NeedsCleaning flag.

        for ( ; end - p >= 16; p += 16)
        {
            auto const bytes = _mm_loadu_si128(reinterpret_cast<__m128i const*>(p));
            int const index = _mm_cmpestri(kSpecialChars, special_chars_length, bytes, 16, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_LEAST_SIGNIFICANT);
            if (index != 16)
            {
                p += index;
                break;
            }
        }

        if (p == end)
            return MakeToken(p, TokenKind::incomplete);
        if (*p == quote_char)
            break;
#endif

        // Scan inputs smaller than 16 characters (or the rest of small inputs)
        // or initial runs of UTF-8 sequences.
        for ( ; p != end; ++p)
        {
            uint32_t const m = CharClass(*p);
            mask |= m;
            if ((m & CC_StringSpecial) != 0)
                break;
        }

        if (p == end)
            return MakeToken(p, TokenKind::incomplete);

        if (*p == quote_char)
        {
            // Don't skip the quote character.
            // Will be skipped in MakeStringToken.
            break;
        }

        if (*p == '\\')
        {
            ++p;
            if (p == end)
                return MakeToken(p, TokenKind::incomplete);
            // The escaped character will be skipped below.
        }

        ++p;

#if JSON_USE_SSE42
        // Once the NeedsCleaning flag is set, we only need to look at '\' and '"' (or '\'').
        special_chars_length = ((mask & CC_NeedsCleaning) != 0) ? 6 : special_chars_length;
#endif
    }

    return MakeStringToken(p, (mask & CC_NeedsCleaning) != 0 ? StringClass::needs_cleaning : StringClass::plain_ascii);
}

inline Token Lexer::LexNumber(char const* p, Options const& options)
{
    auto const res = ScanNumber(p, end, options);

    return MakeNumberToken(res.next, res.number_class);
}

inline Token Lexer::LexIdentifier(char const* p, Options const& options)
{
    using namespace ::json::charclass;

#if 0 && JSON_USE_SSE42
    if (options.allow_unquoted_keys)
    {
        auto const kSpecialChars = _mm_set_epi8(0, 0, 0, 0, 0, 0, '$', '$', 'z', 'a', '_', '_', 'Z', 'A', '9', '0');

        for ( ; end - p >= 16; p += 16)
        {
            auto const bytes = _mm_loadu_si128(reinterpret_cast<__m128i const*>(p));
            int const index = _mm_cmpestri(kSpecialChars, 10, bytes, 16, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_LEAST_SIGNIFICANT | _SIDD_NEGATIVE_POLARITY);
            if (index != 16)
            {
                p += index;
                return MakeToken(p, TokenKind::identifier);
            }
        }
    }
#else
    static_cast<void>(options); // unused
#endif

    // XXX: Add '$' here?!?!
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
            kind = TokenKind::incomplete;

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

    JSON_ASSERT(p != end);
    JSON_ASSERT(*p == '"');
    ptr = ++p; // skip " or '

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
    using ::json::charclass::IsWhitespace;

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

struct Failed
{
    ParseStatus ec;

    Failed(ParseStatus ec_) : ec(ec_) {}
    operator ParseStatus() const noexcept { return ec; }

    // Test for failure.
    explicit operator bool() const noexcept { return ec != ParseStatus::success; }
};

//struct ParseCallbacks
//{
//    ParseStatus HandleNull(char const* first, char const* last, Options const& options);
//    ParseStatus HandleTrue(char const* first, char const* last, Options const& options);
//    ParseStatus HandleFalse(char const* first, char const* last, Options const& options);
//    ParseStatus HandleNumber(char const* first, char const* last, NumberClass nc, Options const& options);
//    ParseStatus HandleString(char const* first, char const* last, StringClass sc, Options const& options);
//    ParseStatus HandleKey(char const* first, char const* last, StringClass sc, Options const& options);
//    ParseStatus HandleBeginArray(Options const& options);
//    ParseStatus HandleEndArray(size_t count, Options const& options);
//    ParseStatus HandleEndElement(size_t& count, Options const& options);
//    ParseStatus HandleBeginObject(Options const& options);
//    ParseStatus HandleEndObject(size_t count, Options const& options);
//    ParseStatus HandleEndMember(size_t& count, Options const& options);
//};

// XXX:
// Move to json::impl?!?!

template <typename ParseCallbacks>
struct Parser
{
    static constexpr uint32_t kMaxDepth = 500;

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

    if (Failed ec = cb.HandleString(token.ptr, token.end, token.string_class, options))
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

    if (Failed ec = cb.HandleNumber(token.ptr, token.end, token.number_class, options))
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

    char const* const f = token.ptr;
    char const* const l = token.end;
    auto const len = l - f;

    ParseStatus ec;
    if (len == 4 && std::memcmp(f, "null", 4) == 0)
    {
        ec = cb.HandleNull(f, l, options);
    }
    else if (len == 4 && std::memcmp(f, "true", 4) == 0)
    {
        ec = cb.HandleTrue(f, l, options);
    }
    else if (len == 5 && std::memcmp(f, "false", 5) == 0)
    {
        ec = cb.HandleFalse(f, l, options);
    }
    else if (options.allow_nan_inf && len == 8 && std::memcmp(f, "Infinity", 8) == 0)
    {
        ec = cb.HandleNumber(f, l, NumberClass::pos_infinity, options);
    }
    //else if (options.allow_nan_inf && len == 3 && std::memcmp(f, "Inf", 3) == 0)
    //{
    //    ec = cb.HandleNumber(f, l, NumberClass::pos_infinity, options);
    //}
    else if (options.allow_nan_inf && len == 3 && std::memcmp(f, "NaN", 3) == 0)
    {
        ec = cb.HandleNumber(f, l, NumberClass::nan, options);
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

    // skip '{'
    token = lexer.Lex(options);

    if (Failed ec = cb.HandleBeginObject(options))
        return ec;

    if (token.kind != TokenKind::r_brace)
    {
        for (;;)
        {
            if (token.kind != TokenKind::string && (!options.allow_unquoted_keys || token.kind != TokenKind::identifier))
                return ParseStatus::expected_key;

            if (Failed ec = cb.HandleKey(token.ptr, token.end, token.string_class, options))
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
            ++stack[stack_size - 1].count;

            if (Failed ec = cb.HandleEndMember(stack[stack_size - 1].count, options))
                return ec;

            if (token.kind == TokenKind::r_brace)
            {
                break;
            }
            else if (token.kind == TokenKind::comma)
            {
                // skip ','
                token = lexer.Lex(options);

                if (options.allow_trailing_commas && token.kind == TokenKind::r_brace)
                    break;
            }
            else
            {
                if (!options.ignore_missing_commas)
                    break;
            }
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

    stack[stack_size] = {0, Structure::array};
    ++stack_size;

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
            ++stack[stack_size - 1].count;

            if (Failed ec = cb.HandleEndElement(stack[stack_size - 1].count, options))
                return ec;

            if (token.kind == TokenKind::r_square)
            {
                break;
            }
            else if (token.kind == TokenKind::comma)
            {
                // skip ','
                token = lexer.Lex(options);

                if (options.allow_trailing_commas && token.kind == TokenKind::r_square)
                    break;
            }
            else
            {
                if (!options.ignore_missing_commas)
                    break;
            }
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
    --stack_size;

    if (stack_size == 0)
        return ParseStatus::success;

    if (stack[stack_size - 1].structure == Structure::object)
        goto L_end_member;
    else
        goto L_end_element;
}

struct ParseResult {
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
        if (static_cast<uint8_t>(next[0]) == 0xEF &&
            static_cast<uint8_t>(next[1]) == 0xBB &&
            static_cast<uint8_t>(next[2]) == 0xBF)
        {
            next += 3;
        }
    }

    Parser<ParseCallbacks> parser(cb, options);

    parser.lexer = Lexer(next, last);
    parser.token = parser.lexer.Lex(options); // Get the first token

    ParseStatus ec = parser.ParseValue();

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

//==================================================================================================
// Utility
//==================================================================================================

namespace util {

struct LineInfo {
    size_t line = 1;
    size_t column = 1;
};

inline LineInfo GetLineInfo(char const* start, char const* pos)
{
    //JSON_ASSERT(start != nullptr);
    //JSON_ASSERT(pos != nullptr);
    //JSON_ASSERT(start <= pos);

    size_t line = 1;
    size_t column = 1;

    for (char const* next = start; next != pos; )
    {
        ++column;

        char const ch = *next;
        ++next;
#if 0
        // Skip UTF-8 trail bytes.
        for ( ; next != pos && (0x80 == (static_cast<uint8_t>(*next) & 0xC0)); ++next)
        {
        }
#endif

        if (ch == '\n' || ch == '\r')
        {
            // If this is '\r\n', skip the other half.
            if (ch == '\r' && next != pos && *next == '\n') {
                ++next;
            }

            ++line;
            column = 1;
        }
    }

    return {line, column};
}

} // namespace util

} // namespace json

//==================================================================================================
//
//==================================================================================================

#if 0
struct ParseCallbacks
{
    virtual json::ParseStatus HandleNull(char const* first, char const* last, json::Options const& options) = 0;
    virtual json::ParseStatus HandleTrue(char const* first, char const* last, json::Options const& options) = 0;
    virtual json::ParseStatus HandleFalse(char const* first, char const* last, json::Options const& options) = 0;
    virtual json::ParseStatus HandleNumber(char const* first, char const* last, json::NumberClass nc, json::Options const& options) = 0;
    virtual json::ParseStatus HandleString(char const* first, char const* last, json::StringClass sc, json::Options const& options) = 0;
    virtual json::ParseStatus HandleKey(char const* first, char const* last, json::StringClass sc, json::Options const& options) = 0;
    virtual json::ParseStatus HandleBeginArray(json::Options const& options) = 0;
    virtual json::ParseStatus HandleEndArray(size_t count, json::Options const& options) = 0;
    virtual json::ParseStatus HandleEndElement(size_t& count, json::Options const& options) = 0;
    virtual json::ParseStatus HandleBeginObject(json::Options const& options) = 0;
    virtual json::ParseStatus HandleEndObject(size_t count, json::Options const& options) = 0;
    virtual json::ParseStatus HandleEndMember(size_t& count, json::Options const& options) = 0;
};

json::ParseResult ParseJSON(ParseCallbacks& cb, char const* next, char const* last, json::Options const& options = {})
{
    return json::ParseSAX(cb, next, last, options);
}
#endif
