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

//#define JSON_STRICT 1
//#define JSON_CONVERT_NUMBERS 1

//#define JSON_USE_SSE42 0
#ifndef JSON_USE_SSE42
#if defined(__SSE4_2__) || defined(__AVX__) || defined(__AVX2__)
#define JSON_USE_SSE42 1
#endif
#endif // ^^^ !defined(JSON_USE_SSE42) ^^^

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#if JSON_CONVERT_NUMBERS
#include <limits>
#endif // ^^^ JSON_CONVERT_NUMBERS ^^^

#if _MSC_VER
#include <intrin.h>
#endif
#if JSON_USE_SSE42
#include <nmmintrin.h>
#endif // ^^^ JSON_USE_SSE42 ^^^

#ifndef JSON_ASSERT
#define JSON_ASSERT(X) assert(X)
#endif

#if defined(__GNUC__)
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
        Id = I|A,
        Bs = I|A|C|S,   // backslash
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
        0,    Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,
    //  P     Q     R     S     T     U     V     W     X     Y     Z     [     \     ]     ^     _
        Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   P,    Bs,   P,    0,    Id,
    //  `     a     b     c     d     e     f     g     h     i     j     k     l     m     n     o
        0,    Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,
    //  p     q     r     s     t     u     v     w     x     y     z     {     |     }     ~     DEL
        Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   P,    0,    P,    0,    0,
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

//#if 0
//inline bool IsWhitespace     (char ch) { return ch == ' ' || ch == '\n' || ch == '\r' || ch == '\t'; }
//#else
//inline bool IsWhitespace     (char ch) { return (CharClass(ch) & CC_whitespace      ) != 0; }
//#endif
#if 1
inline bool IsDigit          (char ch) { return '0' <= ch && ch <= '9'; }
#else
inline bool IsDigit          (char ch) { return (CharClass(ch) & CC_digit           ) != 0; }
#endif
inline bool IsIdentifierStart(char ch) { return (CharClass(ch) & CC_identifier_start) != 0; }
inline bool IsIdentifierBody (char ch) { return (CharClass(ch) & CC_identifier_body ) != 0; }
//inline bool IsPunctuation    (char ch) { return (CharClass(ch) & CC_punctuation     ) != 0; }

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
    decimal,
//#if !JSON_STRICT
    nan,
    pos_infinity,
    neg_infinity,
//#endif // ^^^ !JSON_STRICT ^^^
};

inline bool IsFinite(NumberClass nc)
{
    return nc == NumberClass::integer || nc == NumberClass::decimal;
}

struct ScanNumberResult
{
    char const* next;
    NumberClass number_class;
};

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
#if !JSON_STRICT
    else if (last - next >= 3 && std::memcmp(next, "NaN", 3) == 0)
    {
        return {next + 3, NumberClass::nan};
    }
    else if (last - next >= 8 && std::memcmp(next, "Infinity", 8) == 0)
    {
        return {next + 8, is_neg ? NumberClass::neg_infinity : NumberClass::pos_infinity};
    }
#endif // ^^^ !JSON_STRICT ^^^
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

    NumberClass const nc = has_decimal_point || has_exponent
        ? NumberClass::decimal
        : NumberClass::integer;

    return {next, nc};
}

//==================================================================================================
// ScanString
//==================================================================================================

enum class StringClass : uint8_t {
    clean, // => is ASCII (TODO: => is UTF-8)
    needs_cleaning,
};

//==================================================================================================
// Lexer
//==================================================================================================

enum class Mode : uint8_t {
    // In this mode, the parser only accepts JSON as specified in RFC 8259.
    // This is the default.
    strict,
    // In this mode, the parser accepts accepts JSON as specified in RFC 8259 and additionally:
    //  - Block comments starting with /* and ending with */ (which may not be nested),
    //  - Line comments starting with // and ending with a new-line character,
    //  - Trailing commas in arrays and objects,
    //  - Unquoted keys in objects,
    //  - Special values 'NaN' and 'Infinity' when parsing numbers,
    lenient,
};

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
#if !JSON_STRICT
    comment,
    incomplete_comment,
#endif // ^^^ !JSON_STRICT ^^^
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
    char const* ptr = nullptr;
    char const* end = nullptr;

public:
    void SetInput(char const* first, char const* last);

    TokenKind Peek(Mode mode);

    Token LexString();
    Token LexNumber();
    Token LexIdentifier();

    void Skip(TokenKind kind);

    char const* Next() const { return ptr; }
    char const* Last() const { return end; }

    // Advance the read pointer.
    // PRE: dist >= 0
    // PRE: Next() + dist <= Last()
    char const* Seek(intptr_t dist);

private:
#if !JSON_STRICT
    // Skip the comment starting at p (*p == '/')
    // If the comment is an incomplete block comment, return nullptr.
    // Otherwise return an iterator pointing past the end of the comment.
    static char const* SkipComment(char const* p, char const* end);
#endif // ^^^ !JSON_STRICT ^^^
};

inline void Lexer::SetInput(char const* first, char const* last)
{
    // Skip UTF-8 "BOM".
    if (last - first >= 3 && std::memcmp(first, "\xEF\xBB\xBF", 3) == 0)
    {
        first += 3;
    }

    ptr = first;
    end = last;
}

#if !JSON_STRICT
inline TokenKind Lexer::Peek(Mode mode)
#else // ^^^ !JSON_STRICT ^^^
inline TokenKind Lexer::Peek(Mode /*mode*/)
#endif // ^^^ JSON_STRICT ^^^
{
    enum : uint8_t {
        Ic = static_cast<uint8_t>(TokenKind::invalid_character),
        Lb = static_cast<uint8_t>(TokenKind::l_brace),
        Rb = static_cast<uint8_t>(TokenKind::r_brace),
        Ls = static_cast<uint8_t>(TokenKind::l_square),
        Rs = static_cast<uint8_t>(TokenKind::r_square),
        Ca = static_cast<uint8_t>(TokenKind::comma),
        Cn = static_cast<uint8_t>(TokenKind::colon),
        St = static_cast<uint8_t>(TokenKind::string),
        Nr = static_cast<uint8_t>(TokenKind::number),
        Id = static_cast<uint8_t>(TokenKind::identifier),
    };

    static constexpr uint8_t const kMap[] = {
    //  NUL   SOH   STX   ETX   EOT   ENQ   ACK   BEL   BS    HT    LF    VT    FF    CR    SO    SI
        Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   0,    0,    Ic,   Ic,   0,    Ic,   Ic,
    //  DLE   DC1   DC2   DC3   DC4   NAK   SYN   ETB   CAN   EM    SUB   ESC   FS    GS    RS    US
        Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,
    //  space !     "     #     $     %     &     '     (     )     *     +     ,     -     .     /
        0,    Ic,   St,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Ic,   Nr,   Ca,   Nr,   Nr,   Ic,
    //  0     1     2     3     4     5     6     7     8     9     :     ;     <     =     >     ?
        Nr,   Nr,   Nr,   Nr,   Nr,   Nr,   Nr,   Nr,   Nr,   Nr,   Cn,   Ic,   Ic,   Ic,   Ic,   Ic,
    //  @     A     B     C     D     E     F     G     H     I     J     K     L     M     N     O
        Ic,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,
    //  P     Q     R     S     T     U     V     W     X     Y     Z     [     \     ]     ^     _
        Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Ls,   Id,   Rs,   Ic,   Id,
    //  `     a     b     c     d     e     f     g     h     i     j     k     l     m     n     o
        Ic,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,
    //  p     q     r     s     t     u     v     w     x     y     z     {     |     }     ~     DEL
        Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Lb,   Ic,   Rb,   Ic,   Ic,
        Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,
        Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,
        Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,
        Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,
        Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,
        Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,
        Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,
        Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,   Id,
    };

#if !JSON_STRICT
    char const* p = ptr;
    for (;;)
    {
        uint8_t charclass = 0;

        // Skip whitespace
        for ( ; p != end; ++p)
        {
            charclass = kMap[static_cast<uint8_t>(*p)];
            if (charclass != 0)
                break;
        }

        ptr = p; // Mark start of next token.

        if (p == end)
            return TokenKind::eof;

        if (*p != '/')
            return static_cast<TokenKind>(charclass);

        if (mode == Mode::strict)
            return TokenKind::invalid_character;

        char const* next = SkipComment(p, end);
        if (next == nullptr)
            return TokenKind::incomplete_comment;

        p = next;
    }
#else // ^^^ !JSON_STRICT ^^^
    char const* p = ptr;

    uint8_t charclass = 0;

    // Skip whitespace
    for ( ; p != end; ++p)
    {
        charclass = kMap[static_cast<uint8_t>(*p)];
        if (charclass != 0)
            break;
    }

    ptr = p; // Mark start of next token.

    return (p == end)
        ? TokenKind::eof
        : static_cast<TokenKind>(charclass);
#endif // ^^^ JSON_STRICT ^^^
}

inline Token Lexer::LexString()
{
    using namespace ::json::charclass;

    char const* p = ptr;
    JSON_ASSERT(p != end);
    JSON_ASSERT(*p == '"');

    ++p; // Skip "

    StringClass sc = StringClass::clean;

#if JSON_USE_SSE42
    auto CountTrailingZeros = [](int bits)
    {
#if _MSC_VER
        unsigned long index;
        _BitScanForward(&index, static_cast<unsigned long>(bits));
        return static_cast<int>(index);
#else
        return __builtin_ctz(static_cast<unsigned>(bits));
#endif
    };

    /*static*/ __m128i const kQuotes = _mm_set1_epi8('"');
    /*static*/ __m128i const kBackslashes = _mm_set1_epi8('\\');
    /*static*/ __m128i const kSpaces = _mm_set1_epi8(' ');

    for ( ; end - p >= 16; p += 16)
    {
        __m128i const bytes = _mm_loadu_si128(reinterpret_cast<__m128i const*>(p));
        __m128i const mask2 = _mm_cmpeq_epi8(kQuotes, bytes);
        __m128i const mask1 = _mm_or_si128(mask2, _mm_cmpeq_epi8(kBackslashes, bytes));
        __m128i const mask0 = _mm_or_si128(mask1, _mm_cmpgt_epi8(kSpaces, bytes));
        if (! _mm_testz_si128(mask0, mask0))
        {
            p += CountTrailingZeros(_mm_movemask_epi8(mask0));
            goto L1;
        }
    }
    while (p != end && (CharClass(*p) & (CC_string_special | CC_needs_cleaning)) == 0)
    {
        ++p;
    }

L1:
    if (p != end && *p != '"')
    {
        sc = StringClass::needs_cleaning;
        for (;;)
        {
            for ( ; end - p >= 16; p += 16)
            {
                __m128i const bytes = _mm_loadu_si128(reinterpret_cast<__m128i const*>(p));
                __m128i const mask1 = _mm_cmpeq_epi8(kQuotes, bytes);
                __m128i const mask0 = _mm_or_si128(mask1, _mm_cmpeq_epi8(kBackslashes, bytes));
                if (! _mm_testz_si128(mask0, mask0))
                {
                    p += CountTrailingZeros(_mm_movemask_epi8(mask0));
                    goto L2;
                }
            }
            while (p != end && (CharClass(*p) & CC_string_special) == 0)
            {
                ++p;
            }

L2:
            if (p == end || *p == '"')
                break;

            JSON_ASSERT(*p == '\\');
            ++p; // Skip '\\'. The escaped character will be skipped below.
            if (p == end)
                break;
            ++p;
        }
    }
#else // ^^^ JSON_USE_SSE42 ^^^
    while (p != end && (CharClass(*p) & (CC_string_special | CC_needs_cleaning)) == 0)
    {
        ++p;
    }

    if (p != end && *p != '"')
    {
        sc = StringClass::needs_cleaning;
        for (;;)
        {
            while (p != end && (CharClass(*p) & CC_string_special) == 0)
            {
                ++p;
            }

            if (p == end || *p == '"')
                break;

            JSON_ASSERT(*p == '\\');
            ++p; // Skip '\\'. The escaped character will be skipped below.
            if (p == end)
                break;
            ++p;
        }
    }
#endif // ^^^ !JSON_USE_SSE42 ^^^

    JSON_ASSERT(p == end || *p == '"');

    bool const is_incomplete = (p == end);
    if (!is_incomplete)
        ++p; // Skip '"'

    Token tok;

    tok.ptr = ptr;
    tok.end = p;
    tok.kind = is_incomplete ? TokenKind::incomplete_string : TokenKind::string;
    tok.string_class = sc;
//  tok.number_class = 0;

    ptr = p;

    return tok;
}

inline Token Lexer::LexNumber()
{
    using json::charclass::IsSeparator;

    char const* p = ptr;

    auto const res = json::ScanNumber(p, end);
    p = res.next;

    NumberClass nc = res.number_class;
    if (nc == NumberClass::invalid || (p != end && !IsSeparator(*p)))
    {
        // Invalid number,
        // or valid number with trailing garbage, which is also an invalid number.
        nc = NumberClass::invalid;
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

inline Token Lexer::LexIdentifier()
{
    using namespace json::charclass;

    char const* p = ptr;
    JSON_ASSERT(p != end);
    JSON_ASSERT(IsIdentifierStart(*p));
    JSON_ASSERT(IsIdentifierBody(*p));

    // Don't skip identifier start here.
    // Might have to set needs_cleaning flag below.

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
    tok.string_class = ((mask & CC_needs_cleaning) != 0) ? StringClass::needs_cleaning : StringClass::clean;
//  tok.number_class = 0;

    ptr = p;

    return tok;
}

inline void Lexer::Skip(TokenKind kind)
{
#ifndef NDEBUG
    switch (kind)
    {
    case TokenKind::l_brace:
    case TokenKind::r_brace:
    case TokenKind::l_square:
    case TokenKind::r_square:
    case TokenKind::comma:
    case TokenKind::colon:
        break;
    default:
        JSON_ASSERT(false && "internal error: skipping is only allowed for punctuation");
        break;
    }
#else
    static_cast<void>(kind); // Fix warning
#endif

    ++ptr;
}

inline char const* Lexer::Seek(intptr_t dist)
{
    JSON_ASSERT(dist >= 0);
    JSON_ASSERT(dist <= end - ptr);

    ptr += dist;
    return ptr;
}

#if !JSON_STRICT
JSON_NEVER_INLINE char const* Lexer::SkipComment(char const* p, char const* end)
{
    JSON_ASSERT(p != end);
    JSON_ASSERT(*p == '/');

    ++p; // Skip '/'

    if (p == end)
        return nullptr;

    if (*p == '/')
    {
        ++p;
        for (;;)
        {
            if (p == end || *p == '\n' || *p == '\r')
                return p;
            ++p;
        }
    }

    if (*p == '*')
    {
        ++p;
        for (;;)
        {
            if (p == end)
                break; // incomplete block comment
            char const ch = *p;
            ++p;
            if (ch == '*')
            {
                if (p == end)
                    break; // incomplete block comment
                if (*p == '/')
                {
                    ++p;
                    return p;
                }
            }
        }
    }

    return nullptr;
}
#endif // ^^^ !JSON_STRICT ^^^

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

struct ParseResult {
    char const* ptr;
    ParseStatus ec = ParseStatus::unknown;
};

template <typename ParseCallbacks>
class Parser
{
    static constexpr uint32_t kMaxDepth = 500;

    ParseCallbacks& cb;
    Lexer lexer;
    Mode mode;

public:
    Parser(ParseCallbacks& cb_, Mode mode);

    void Init(char const* next, char const* last);

    // Extract the next JSON value from the input
    // and check whether EOF has been reached.
    ParseResult Parse();

    // Extract the next JSON value from the input.
    ParseStatus ParseValue();

private:
    ParseStatus ParseString();
    ParseStatus ParseNumber();
    ParseStatus ParseIdentifier();
};

template <typename ParseCallbacks>
inline Parser<ParseCallbacks>::Parser(ParseCallbacks& cb_, Mode mode_)
    : cb(cb_)
    , mode(mode_)
{
}

template <typename ParseCallbacks>
inline void Parser<ParseCallbacks>::Init(char const* next, char const* last)
{
    lexer.SetInput(next, last);
}

template <typename ParseCallbacks>
inline ParseResult Parser<ParseCallbacks>::Parse()
{
    ParseStatus ec = ParseValue();

    if (ec == ParseStatus::success)
    {
        if (lexer.Peek(mode) != TokenKind::eof)
        {
            ec = ParseStatus::expected_eof;
        }
    }

    return {lexer.Next(), ec};
}

template <typename ParseCallbacks>
JSON_NEVER_INLINE ParseStatus Parser<ParseCallbacks>::ParseValue()
{
    struct StackElement {
        size_t count; // number of elements or members in the current array resp. object
        TokenKind close;
    };

    uint32_t stack_size = 0;
    StackElement stack[kMaxDepth];

    // The first token has been read in SetInput()
    // or in the last call to ParseValue().

    TokenKind peek = lexer.Peek(mode);

    goto L_parse_value;

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
    JSON_ASSERT(peek == TokenKind::l_brace);

    if (stack_size >= kMaxDepth)
        return ParseStatus::max_depth_reached;

    stack[stack_size] = {0, TokenKind::r_brace};
    ++stack_size;

    if (Failed ec = cb.HandleBeginObject())
        return ParseStatus(ec);

    // Skip the '{'.
    lexer.Skip(TokenKind::l_brace);

    // Get the possible kind of the next token.
    // This must be either an '}' or a key.
    peek = lexer.Peek(mode);
    if (peek != TokenKind::r_brace)
    {
        for (;;)
        {
            if (peek == TokenKind::string)
            {
                Token const curr = lexer.LexString();
                JSON_ASSERT(curr.kind == TokenKind::string || curr.kind == TokenKind::incomplete_string);

                if (curr.kind == TokenKind::incomplete_string)
                {
                    return ParseStatus::expected_key;
                }

                JSON_ASSERT(curr.end - curr.ptr >= 2);
                JSON_ASSERT(curr.ptr[ 0] == '"');
                JSON_ASSERT(curr.end[-1] == '"');
                if (Failed ec = cb.HandleKey(curr.ptr + 1, curr.end - 1, curr.string_class))
                    return ParseStatus(ec);
            }
#if !JSON_STRICT
            else if (mode != Mode::strict && peek == TokenKind::identifier)
            {
                Token const curr = lexer.LexIdentifier();
                JSON_ASSERT(curr.kind == TokenKind::identifier);

                if (Failed ec = cb.HandleKey(curr.ptr, curr.end, curr.string_class))
                    return ParseStatus(ec);
            }
#endif // ^^^ !JSON_STRICT ^^^
            else
            {
                return ParseStatus::expected_key;
            }

            // Read the token after the key.
            // This must be a ':'.
            peek = lexer.Peek(mode);

            if (peek != TokenKind::colon)
                return ParseStatus::expected_colon_after_key;

            lexer.Skip(TokenKind::colon);

            // Read the token after the ':'.
            // This must be a JSON value.
            peek = lexer.Peek(mode);

            goto L_parse_value;

L_end_member:
            JSON_ASSERT(stack_size != 0);
            JSON_ASSERT(stack[stack_size - 1].close == TokenKind::r_brace);
            ++stack[stack_size - 1].count;

            if (Failed ec = cb.HandleEndMember(stack[stack_size - 1].count))
                return ParseStatus(ec);

            // Expect a ',' (and another member) or a closing '}'.
            peek = lexer.Peek(mode);
            if (peek == TokenKind::comma)
            {
                // Read the token after the ','.
                // This must be a key.
                lexer.Skip(TokenKind::comma);

                peek = lexer.Peek(mode);
#if !JSON_STRICT
                if (mode != Mode::strict && peek == TokenKind::r_brace)
                    break;
#endif // ^^^ !JSON_STRICT ^^^
            }
            else
            {
                break;
            }
        }

        if (peek != TokenKind::r_brace)
            return ParseStatus::expected_comma_or_closing_brace;
    }

    if (Failed ec = cb.HandleEndObject(stack[stack_size - 1].count))
        return ParseStatus(ec);

    lexer.Skip(TokenKind::r_brace);
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
    JSON_ASSERT(peek == TokenKind::l_square);

    if (stack_size >= kMaxDepth)
        return ParseStatus::max_depth_reached;

    stack[stack_size] = {0, TokenKind::r_square};
    ++stack_size;

    if (Failed ec = cb.HandleBeginArray())
        return ParseStatus(ec);

    // Read the token after '['.
    // This must be a ']' or any JSON value.
    lexer.Skip(TokenKind::l_square);

    peek = lexer.Peek(mode);
    if (peek != TokenKind::r_square)
    {
        for (;;)
        {
            goto L_parse_value;

L_end_element:
            JSON_ASSERT(stack_size != 0);
            JSON_ASSERT(stack[stack_size - 1].close == TokenKind::r_square);
            ++stack[stack_size - 1].count;

            if (Failed ec = cb.HandleEndElement(stack[stack_size - 1].count))
                return ParseStatus(ec);

            // Expect a ',' (and another element) or a closing ']'.
            peek = lexer.Peek(mode);
            if (peek == TokenKind::comma)
            {
                // Read the token after the ','.
                // This must be a JSON value.
                lexer.Skip(TokenKind::comma);

                peek = lexer.Peek(mode);
#if !JSON_STRICT
                if (mode != Mode::strict && peek == TokenKind::r_square)
                    break;
#endif // ^^^ !JSON_STRICT ^^^
            }
            else
            {
                break;
            }
        }

        if (peek != TokenKind::r_square)
            return ParseStatus::expected_comma_or_closing_bracket;
    }

    if (Failed ec = cb.HandleEndArray(stack[stack_size - 1].count))
        return ParseStatus(ec);

    lexer.Skip(TokenKind::r_square);
    goto L_end_structured;

L_end_structured:
    JSON_ASSERT(stack_size != 0);
    --stack_size;

    if (0)
    {
L_parse_value:
        ParseStatus ec;
        switch (peek)
        {
        case TokenKind::l_brace:
            goto L_begin_object;
        case TokenKind::l_square:
            goto L_begin_array;
        case TokenKind::string:
            ec = ParseString();
            break;
        case TokenKind::number:
            ec = ParseNumber();
            break;
        case TokenKind::identifier:
            ec = ParseIdentifier();
            break;
        default:
            ec = ParseStatus::expected_value;
            break;
        }

        if (ec != ParseStatus::success)
            return ec;
    }

    if (stack_size == 0)
        return ParseStatus::success;

    if (stack[stack_size - 1].close == TokenKind::r_brace)
        goto L_end_member;
    else
        goto L_end_element;
}

template <typename ParseCallbacks>
inline ParseStatus Parser<ParseCallbacks>::ParseString()
{
    Token const curr = lexer.LexString();
    JSON_ASSERT(curr.kind == TokenKind::string || curr.kind == TokenKind::incomplete_string);

    if (curr.kind == TokenKind::incomplete_string)
    {
        return ParseStatus::expected_value;
    }

    JSON_ASSERT(curr.end - curr.ptr >= 2);
    JSON_ASSERT(curr.ptr[ 0] == '"');
    JSON_ASSERT(curr.end[-1] == '"');
    return cb.HandleString(curr.ptr + 1, curr.end - 1, curr.string_class);
}

#if JSON_CONVERT_NUMBERS
namespace impl {

struct ParsedNumber {
    uint64_t significand = 0;
    int      num_digits = 0;
    int      exponent_adjust = 0;
    int      parsed_exponent = 0;
    int      parsed_exponent_digits = 0;
    bool     is_neg = false;
};

JSON_FORCE_INLINE ScanNumberResult ParseNumber(char const* next, char const* last, ParsedNumber& number)
{
    using json::charclass::IsDigit;

    if (next == last)
        return {next, NumberClass::invalid};

// [-]

    if (*next == '-')
    {
        number.is_neg = true;

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
            if (number.num_digits < 19)
            {
                number.significand = 10 * number.significand + static_cast<uint8_t>(*next - '0');
            }
            ++number.num_digits;
            ++next;
            if (next == last)
                return {next, NumberClass::integer};
            if (!IsDigit(*next))
                break;
        }
    }
#if !JSON_STRICT
    else if (last - next >= 3 && std::memcmp(next, "NaN", 3) == 0)
    {
        return {next + 3, NumberClass::nan};
    }
    else if (last - next >= 8 && std::memcmp(next, "Infinity", 8) == 0)
    {
        return {next + 8, number.is_neg ? NumberClass::neg_infinity : NumberClass::pos_infinity};
    }
#endif // ^^^ !JSON_STRICT ^^^
    else
    {
        return {next, NumberClass::invalid};
    }

// frac

    JSON_ASSERT(next != last);

    bool const has_decimal_point = (*next == '.');
    if (has_decimal_point)
    {
        ++next;
        if (next == last || !IsDigit(*next))
            return {next, NumberClass::invalid};

        if (number.num_digits == 0)
        {
            while (*next == '0')
            {
                --number.exponent_adjust;
                ++next;
                if (next == last)
                    return {next, NumberClass::decimal}; // Actually: 0.0...000
            }
        }

        JSON_ASSERT(next != last);
        if (IsDigit(*next))
        {
            for (;;)
            {
                if (number.num_digits < 19)
                {
                    number.significand = 10 * number.significand + static_cast<uint8_t>(*next - '0');
                }
                ++number.num_digits;
                --number.exponent_adjust;
                ++next;
                if (next == last)
                    return {next, NumberClass::decimal};
                if (!IsDigit(*next))
                    break;
            }
        }
    }

// exp

    JSON_ASSERT(next != last);

    bool const has_exponent = (*next == 'e' || *next == 'E');
    if (has_exponent)
    {
        ++next;
        if (next == last)
            return {next, NumberClass::invalid};

        bool const exp_is_neg = (*next == '-');
        if (exp_is_neg || *next == '+')
        {
            ++next;
            if (next == last)
                return {next, NumberClass::invalid};
        }

        if (!IsDigit(*next))
            return {next, NumberClass::invalid};

        // Skip leading zeros in the exponent.
        // The exponent is always a decimal number.
        while (*next == '0')
        {
            ++next;
            if (next == last)
                return {next, NumberClass::decimal};
        }

        for (;;)
        {
            JSON_ASSERT(next != last);
            if (!IsDigit(*next))
                break;
            if (number.parsed_exponent_digits < 8)
            {
                number.parsed_exponent = 10 * number.parsed_exponent + (*next - '0');
            }
            ++number.parsed_exponent_digits;
            ++next;
            if (next == last)
                break;
        }

        if (exp_is_neg)
            number.parsed_exponent = -number.parsed_exponent;
    }

    NumberClass const nc = has_decimal_point || has_exponent
        ? NumberClass::decimal
        : NumberClass::integer;

    return {next, nc};
}

// NB: Ignore sign!
JSON_FORCE_INLINE double ConvertFiniteParsedNumber(ParsedNumber const& number)
{
    static constexpr double kPow10[] = { // 2472 bytes
        1e+000, 1e+001, 1e+002, 1e+003, 1e+004, 1e+005, 1e+006, 1e+007,
        1e+008, 1e+009, 1e+010, 1e+011, 1e+012, 1e+013, 1e+014, 1e+015,
        1e+016, 1e+017, 1e+018, 1e+019, 1e+020, 1e+021, 1e+022, 1e+023,
        1e+024, 1e+025, 1e+026, 1e+027, 1e+028, 1e+029, 1e+030, 1e+031,
        1e+032, 1e+033, 1e+034, 1e+035, 1e+036, 1e+037, 1e+038, 1e+039,
        1e+040, 1e+041, 1e+042, 1e+043, 1e+044, 1e+045, 1e+046, 1e+047,
        1e+048, 1e+049, 1e+050, 1e+051, 1e+052, 1e+053, 1e+054, 1e+055,
        1e+056, 1e+057, 1e+058, 1e+059, 1e+060, 1e+061, 1e+062, 1e+063,
        1e+064, 1e+065, 1e+066, 1e+067, 1e+068, 1e+069, 1e+070, 1e+071,
        1e+072, 1e+073, 1e+074, 1e+075, 1e+076, 1e+077, 1e+078, 1e+079,
        1e+080, 1e+081, 1e+082, 1e+083, 1e+084, 1e+085, 1e+086, 1e+087,
        1e+088, 1e+089, 1e+090, 1e+091, 1e+092, 1e+093, 1e+094, 1e+095,
        1e+096, 1e+097, 1e+098, 1e+099, 1e+100, 1e+101, 1e+102, 1e+103,
        1e+104, 1e+105, 1e+106, 1e+107, 1e+108, 1e+109, 1e+110, 1e+111,
        1e+112, 1e+113, 1e+114, 1e+115, 1e+116, 1e+117, 1e+118, 1e+119,
        1e+120, 1e+121, 1e+122, 1e+123, 1e+124, 1e+125, 1e+126, 1e+127,
        1e+128, 1e+129, 1e+130, 1e+131, 1e+132, 1e+133, 1e+134, 1e+135,
        1e+136, 1e+137, 1e+138, 1e+139, 1e+140, 1e+141, 1e+142, 1e+143,
        1e+144, 1e+145, 1e+146, 1e+147, 1e+148, 1e+149, 1e+150, 1e+151,
        1e+152, 1e+153, 1e+154, 1e+155, 1e+156, 1e+157, 1e+158, 1e+159,
        1e+160, 1e+161, 1e+162, 1e+163, 1e+164, 1e+165, 1e+166, 1e+167,
        1e+168, 1e+169, 1e+170, 1e+171, 1e+172, 1e+173, 1e+174, 1e+175,
        1e+176, 1e+177, 1e+178, 1e+179, 1e+180, 1e+181, 1e+182, 1e+183,
        1e+184, 1e+185, 1e+186, 1e+187, 1e+188, 1e+189, 1e+190, 1e+191,
        1e+192, 1e+193, 1e+194, 1e+195, 1e+196, 1e+197, 1e+198, 1e+199,
        1e+200, 1e+201, 1e+202, 1e+203, 1e+204, 1e+205, 1e+206, 1e+207,
        1e+208, 1e+209, 1e+210, 1e+211, 1e+212, 1e+213, 1e+214, 1e+215,
        1e+216, 1e+217, 1e+218, 1e+219, 1e+220, 1e+221, 1e+222, 1e+223,
        1e+224, 1e+225, 1e+226, 1e+227, 1e+228, 1e+229, 1e+230, 1e+231,
        1e+232, 1e+233, 1e+234, 1e+235, 1e+236, 1e+237, 1e+238, 1e+239,
        1e+240, 1e+241, 1e+242, 1e+243, 1e+244, 1e+245, 1e+246, 1e+247,
        1e+248, 1e+249, 1e+250, 1e+251, 1e+252, 1e+253, 1e+254, 1e+255,
        1e+256, 1e+257, 1e+258, 1e+259, 1e+260, 1e+261, 1e+262, 1e+263,
        1e+264, 1e+265, 1e+266, 1e+267, 1e+268, 1e+269, 1e+270, 1e+271,
        1e+272, 1e+273, 1e+274, 1e+275, 1e+276, 1e+277, 1e+278, 1e+279,
        1e+280, 1e+281, 1e+282, 1e+283, 1e+284, 1e+285, 1e+286, 1e+287,
        1e+288, 1e+289, 1e+290, 1e+291, 1e+292, 1e+293, 1e+294, 1e+295,
        1e+296, 1e+297, 1e+298, 1e+299, 1e+300, 1e+301, 1e+302, 1e+303,
        1e+304, 1e+305, 1e+306, 1e+307, 1e+308,
    };

    if (number.parsed_exponent_digits > 8)
    {
        // Exponents larger than 99999999 are considered to be +inf.
        if (number.parsed_exponent < 0)
            return 0.0;
        else
            return std::numeric_limits<double>::infinity();
    }

    int num_digits = number.num_digits;
    int exponent = number.parsed_exponent + number.exponent_adjust;

    // Min non-zero double: 4.9406564584124654 * 10^-324
    // Any x <= 10^-324 is interpreted as 0.
    if (num_digits + exponent <= -324)
    {
        return 0.0;
    }

    // Max double: 1.7976931348623157 * 10^308, which has 309 digits.
    // Any x >= 10^309 is interpreted as +infinity.
    if (num_digits + exponent > 309)
    {
        return std::numeric_limits<double>::infinity();
    }

    // Move least significant digits into the exponent.
    if (num_digits > 19)
    {
        exponent += num_digits - 19;
        // exponent > -324 - num_digits + (num_digits - 19) = -324 - 19 = -343
        // exponent <= 309 - num_digits + (num_digits - 19) = 309 - 19 = 290
    }
    else
    {
        // exponent > -324 - num_digits >= -324 - 19 = -343
        // exponent <= 309 - num_digits <= 309 - 1 = 308
    }
    JSON_ASSERT(exponent > -343);
    JSON_ASSERT(exponent <= 308);

    double s = static_cast<double>(number.significand);
    if (exponent >= 0)
        return s * kPow10[exponent];
    else if (exponent >= -308)
        return s / kPow10[-exponent];
    else
        return s / 1e+308 / kPow10[-exponent - 308];
}

JSON_FORCE_INLINE double ConvertParsedNumber(ParsedNumber const& number, NumberClass nc)
{
    double value;
    switch (nc)
    {
    case NumberClass::invalid:
        return 0.0;
    case NumberClass::integer:
        value = static_cast<double>(number.significand);
        break;
    case NumberClass::decimal:
        value = ConvertFiniteParsedNumber(number);
        break;
#if !JSON_STRICT
    case NumberClass::nan:
        return std::numeric_limits<double>::quiet_NaN();
    case NumberClass::neg_infinity:
        return -std::numeric_limits<double>::infinity();
    case NumberClass::pos_infinity:
        return +std::numeric_limits<double>::infinity();
#endif // ^^^ !JSON_STRICT ^^^
    default:
        break;
    }

    return number.is_neg ? -value : value;
}

} // namespace impl
#endif

#if JSON_CONVERT_NUMBERS
template <typename ParseCallbacks>
inline ParseStatus Parser<ParseCallbacks>::ParseNumber()
{
    using json::charclass::IsSeparator;

    char const* base = lexer.Next();
    char const* last = lexer.Last();
    char const* next = base;

    json::impl::ParsedNumber number;
    auto const snr = json::impl::ParseNumber(next, last, number);
    next = snr.next;

    NumberClass nc = snr.number_class;
    if (nc == NumberClass::invalid || (next != last && !IsSeparator(*next)))
    {
        // Invalid number,
        // or valid number with trailing garbage, which is also an invalid number.
        nc = NumberClass::invalid;
    }

    lexer.Seek(next - base);

    return cb.HandleNumber(json::impl::ConvertParsedNumber(number, nc), nc);
}
#else // ^^^ JSON_CONVERT_NUMBERS ^^^
template <typename ParseCallbacks>
inline ParseStatus Parser<ParseCallbacks>::ParseNumber()
{
    Token const curr = lexer.LexNumber();
    JSON_ASSERT(curr.kind == TokenKind::number);

    return cb.HandleNumber(curr.ptr, curr.end, curr.number_class);
}
#endif // ^^^ !JSON_CONVERT_NUMBERS ^^^

template <typename ParseCallbacks>
inline ParseStatus Parser<ParseCallbacks>::ParseIdentifier()
{
    Token const curr = lexer.LexIdentifier();
    JSON_ASSERT(curr.kind == TokenKind::identifier);

    auto const len = curr.end - curr.ptr;
    JSON_ASSERT(len >= 1);

    if (len == 4 && std::memcmp(curr.ptr, "null", 4) == 0) {
        return cb.HandleNull();
    }

    if (len == 4 && std::memcmp(curr.ptr, "true", 4) == 0) {
        return cb.HandleTrue();
    }

    if (len == 5 && std::memcmp(curr.ptr, "false", 5) == 0) {
        return cb.HandleFalse();
    }

#if !JSON_STRICT
    if (len == 3 && std::memcmp(curr.ptr, "NaN", 3) == 0) {
#if JSON_CONVERT_NUMBERS
        return cb.HandleNumber(std::numeric_limits<double>::quiet_NaN(), NumberClass::nan);
#else // ^^^ JSON_CONVERT_NUMBERS ^^^
        return cb.HandleNumber(curr.ptr, curr.end, NumberClass::nan);
#endif // ^^^ !JSON_CONVERT_NUMBERS ^^^
    }

    if (len == 8 && std::memcmp(curr.ptr, "Infinity", 8) == 0) {
#if JSON_CONVERT_NUMBERS
        return cb.HandleNumber(std::numeric_limits<double>::infinity(), NumberClass::pos_infinity);
#else // ^^^ JSON_CONVERT_NUMBERS ^^^
        return cb.HandleNumber(curr.ptr, curr.end, NumberClass::pos_infinity);
#endif // ^^^ !JSON_CONVERT_NUMBERS ^^^
    }
#endif // ^^^ !JSON_STRICT ^^^

    return ParseStatus::unrecognized_identifier;
}

//==================================================================================================
// ParseSAX
//==================================================================================================

template <typename ParseCallbacks>
inline ParseResult ParseSAX(ParseCallbacks& cb, char const* next, char const* last, Mode mode)
{
    JSON_ASSERT(next != nullptr);
    JSON_ASSERT(last != nullptr);

    Parser<ParseCallbacks> parser(cb, mode);

    parser.Init(next, last);

    return parser.Parse();
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
//#if JSON_CONVERT_NUMBERS
//    virtual json::ParseStatus HandleNumber(double value, json::NumberClass nc) = 0;
//#else
//    virtual json::ParseStatus HandleNumber(char const* first, char const* last, json::NumberClass nc) = 0;
//#endif
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
//json::ParseResult ParseJson(ParseCallbacks& cb, char const* next, char const* last, json::Mode mode = json::Mode::strict)
//{
//    return json::ParseSAX(cb, next, last, mode);
//}
