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

#include "json_defs.h"

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

    //
    // TODO:
    // Merge with table in Lexer::Peek?!
    //

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
// Lexer
//==================================================================================================

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
    // Skip the comment starting at p (*p == '/')
    // If the comment is an incomplete block comment, return nullptr.
    // Otherwise return an iterator pointing past the end of the comment.
    static char const* SkipComment(char const* p, char const* end);
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

inline TokenKind Lexer::Peek(Mode mode)
{
    enum : uint8_t {
        Ic = static_cast<uint8_t>(TokenKind::invalid_character),
        Lb = static_cast<uint8_t>(TokenKind::l_brace),  // 3
        Rb = static_cast<uint8_t>(TokenKind::r_brace),  // 4
        Ls = static_cast<uint8_t>(TokenKind::l_square), // 5
        Rs = static_cast<uint8_t>(TokenKind::r_square), // 6
        Ca = static_cast<uint8_t>(TokenKind::comma),    // 7
        Cn = static_cast<uint8_t>(TokenKind::colon),    // 8
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
}

inline Token Lexer::LexString()
{
    using namespace ::json::charclass;

    char const* p = ptr;
    JSON_ASSERT(p != end);
    JSON_ASSERT(*p == '"');

    ++p; // Skip "

    StringClass sc = StringClass::clean;

#if JSON_SSE42
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
        int const mmask = _mm_movemask_epi8(mask0);
        if (mmask != 0)
        {
            p += CountTrailingZeros(mmask);
            if (*p == '"')
                goto L_done;
            sc = StringClass::needs_cleaning;
            break;
        }
    }

    // NB:
    // This loop is not required for correctness.
    //for ( ; end - p >= 16; p += 16)
    //{
    //    __m128i const bytes = _mm_loadu_si128(reinterpret_cast<__m128i const*>(p));
    //    __m128i const mask1 = _mm_cmpeq_epi8(kQuotes, bytes);
    //    __m128i const mask0 = _mm_or_si128(mask1, _mm_cmpeq_epi8(kBackslashes, bytes));
    //    int const mmask = _mm_movemask_epi8(mask0);
    //    if (mmask != 0)
    //    {
    //        p += CountTrailingZeros(mmask);
    //        if (*p == '"' || ++p == end)
    //            goto L_done;
    //        // Skip the escaped character (+1) and subtract 16 to undo the increment
    //        // at the end of loop.
    //        p += -15;
    //    }
    //}

    for ( ; p != end; ++p)
    {
        char ch = *p;
        if (ch == '"') {
            break;
        } else if (ch == '\\') {
            sc = StringClass::needs_cleaning;
            if (++p == end)
                break;
        } else if (' ' > static_cast<int8_t>(ch)) {
            sc = StringClass::needs_cleaning;
        }
    }

L_done:
#else
    // NB:
    // This loop is not required for correctness.
    while (end - p >= 4)
    {
        if ((CharClass(p[0]) & (CC_string_special | CC_needs_cleaning)) != 0) { goto L1; }
        if ((CharClass(p[1]) & (CC_string_special | CC_needs_cleaning)) != 0) { p += 1; goto L1; }
        if ((CharClass(p[2]) & (CC_string_special | CC_needs_cleaning)) != 0) { p += 2; goto L1; }
        if ((CharClass(p[3]) & (CC_string_special | CC_needs_cleaning)) != 0) { p += 3; goto L1; }
        p += 4;
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
            // NB:
            // This loop is not required for correctness.
            //while (end - p >= 4)
            //{
            //    if ((CharClass(p[0]) & CC_string_special) != 0) { goto L2; }
            //    if ((CharClass(p[1]) & CC_string_special) != 0) { p += 1; goto L2; }
            //    if ((CharClass(p[2]) & CC_string_special) != 0) { p += 2; goto L2; }
            //    if ((CharClass(p[3]) & CC_string_special) != 0) { p += 3; goto L2; }
            //    p += 4;
            //}
            while (p != end && (CharClass(*p) & CC_string_special) == 0)
            {
                ++p;
            }
//L2:
            if (p == end || *p == '"')
                break;

            JSON_ASSERT(*p == '\\');
            ++p; // Skip '\\'.
            if (p == end)
                break;
            ++p; // Skip the escaped character.
        }
    }
#endif

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

inline void Lexer::Skip(TokenKind /*kind*/)
{
    ++ptr;
}

inline char const* Lexer::Seek(intptr_t dist)
{
    JSON_ASSERT(dist >= 0);
    JSON_ASSERT(dist <= end - ptr);

    ptr += dist;
    return ptr;
}

JSON_NEVER_INLINE char const* Lexer::SkipComment(char const* p, char const* end)
{
    JSON_ASSERT(p != end);
    JSON_ASSERT(*p == '/');

    ++p; // Skip '/'

    if (p == end)
        return nullptr; // incomplete block- or line-comment

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
                break; // incomplete block-comment
            char const ch = *p;
            ++p;
            if (ch == '*')
            {
                if (p == end)
                    break; // incomplete block-comment
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

//==================================================================================================
// Parser
//==================================================================================================

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
    Lexer lexer;
    Mode mode;

public:
    Parser(ParseCallbacks& cb_, Mode mode);

    void Init(char const* next, char const* last);

public:
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
            else if (mode != Mode::strict && peek == TokenKind::identifier)
            {
                Token const curr = lexer.LexIdentifier();
                JSON_ASSERT(curr.kind == TokenKind::identifier);

                if (Failed ec = cb.HandleKey(curr.ptr, curr.end, curr.string_class))
                    return ParseStatus(ec);
            }
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
                if (mode != Mode::strict && peek == TokenKind::r_brace)
                    break;
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
                if (mode != Mode::strict && peek == TokenKind::r_square)
                    break;
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

template <typename ParseCallbacks>
inline ParseStatus Parser<ParseCallbacks>::ParseNumber()
{
    Token const curr = lexer.LexNumber();
    JSON_ASSERT(curr.kind == TokenKind::number);

    return cb.HandleNumber(curr.ptr, curr.end, curr.number_class);
}

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

    if (len == 3 && std::memcmp(curr.ptr, "NaN", 3) == 0) {
        return cb.HandleNumber(curr.ptr, curr.end, NumberClass::nan);
    }

    if (len == 8 && std::memcmp(curr.ptr, "Infinity", 8) == 0) {
        return cb.HandleNumber(curr.ptr, curr.end, NumberClass::pos_infinity);
    }

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
//json::ParseResult ParseJson(ParseCallbacks& cb, char const* next, char const* last, json::Mode mode = json::Mode::strict)
//{
//    return json::ParseSAX(cb, next, last, mode);
//}
