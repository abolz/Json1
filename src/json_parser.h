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
    nan,
    pos_infinity,
    neg_infinity,
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
    else if (last - next >= 3 && std::memcmp(next, "NaN", 3) == 0)
    {
        return {next + 3, NumberClass::nan};
    }
    else if (last - next >= 8 && std::memcmp(next, "Infinity", 8) == 0)
    {
        return {next + 8, is_neg ? NumberClass::neg_infinity : NumberClass::pos_infinity};
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
// ScanIdentifier
//==================================================================================================

enum class Identifier : uint8_t {
    other,
    null,
    true_,
    false_,
    nan,
    infinity,
};

inline Identifier ScanIdentifer(char const* next, char const* last)
{
    auto const len = last - next;
    JSON_ASSERT(len >= 1);

    if (len == 4 && std::memcmp(next, "null", 4) == 0)
        return Identifier::null;
    if (len == 4 && std::memcmp(next, "true", 4) == 0)
        return Identifier::true_;
    if (len == 5 && std::memcmp(next, "false", 5) == 0)
        return Identifier::false_;
    if (len == 3 && std::memcmp(next, "NaN", 3) == 0)
        return Identifier::nan;
    if (len == 8 && std::memcmp(next, "Infinity", 8) == 0)
        return Identifier::infinity;

    return Identifier::other;
}

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
    comment,
    incomplete_string,
    incomplete_comment,
    discarded,
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
    void SetInput(char const* first, char const* last);

    TokenKind Peek(Mode mode);

    Token Lex(TokenKind kind);

    void Skip(TokenKind kind);

private:
    Token LexString     (char const* p);
    Token LexNumber     (char const* p);
    Token LexIdentifier (char const* p);

    // Skip the comment starting at p (*p == '/')
    // If the comment is an incomplete block comment, return nullptr.
    // Otherwise return an iterator pointing past the end of the comment.
    static char const* SkipComment(char const* p, char const* end);

    Token MakeToken(char const* p, TokenKind kind);
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

JSON_FORCE_INLINE TokenKind Lexer::Peek(Mode mode)
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

    static constexpr uint8_t kMap[] = {
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

    char const*       f = ptr;
    char const* const l = end;
    for (;;)
    {
        uint8_t charclass = 0;

        // Skip whitespace
        for ( ; f != l; ++f)
        {
            charclass = kMap[static_cast<uint8_t>(*f)];
            if (charclass != 0)
                break;
        }

        // Mark start of next token.
        ptr = f;

        if (f == l)
            return TokenKind::eof;

        if (*f != '/')
            return static_cast<TokenKind>(charclass);

        if (mode == Mode::strict)
            return TokenKind::invalid_character;

        char const* const next = SkipComment(f, l);
        if (next == nullptr)
            return TokenKind::incomplete_comment;

        f = next;
    }
}

JSON_FORCE_INLINE Token Lexer::Lex(TokenKind kind)
{
    char const* p = ptr;
    switch (kind)
    {
    case TokenKind::eof:
        break;
    case TokenKind::l_brace:
    case TokenKind::r_brace:
    case TokenKind::l_square:
    case TokenKind::r_square:
    case TokenKind::comma:
    case TokenKind::colon:
        ++p;
        break;
    case TokenKind::string:
        return LexString(p);
    case TokenKind::number:
        return LexNumber(p);
    case TokenKind::incomplete_comment:
        p = end;
        break;
    case TokenKind::identifier:
        return LexIdentifier(p);
    case TokenKind::invalid_character:
        ++p;
        break;
    default:
        JSON_ASSERT(false && "unreachable");
        break;
    }

    return MakeToken(p, kind);
}

JSON_FORCE_INLINE void Lexer::Skip(TokenKind kind)
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

JSON_FORCE_INLINE Token Lexer::LexString(char const* p)
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

    StringClass const sc = (mask & CC_needs_cleaning) != 0
        ? StringClass::needs_cleaning
        : StringClass::clean;

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

JSON_FORCE_INLINE Token Lexer::LexNumber(char const* p)
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

JSON_FORCE_INLINE Token Lexer::LexIdentifier(char const* p)
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
    tok.string_class = ((mask & CC_needs_cleaning) != 0) ? StringClass::needs_cleaning : StringClass::clean;
//  tok.number_class = 0;

    ptr = p;

    return tok;
}

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

JSON_FORCE_INLINE Token Lexer::MakeToken(char const* p, TokenKind kind)
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
    Token           curr; // The current token.
    TokenKind       peek; // The next token. (hint only)
    Mode            mode;

public:
    Parser(ParseCallbacks& cb_, Mode mode);

    void Init(char const* next, char const* last);

    // Returns the current token.
    Token GetPeekToken();

    // Extract the next JSON value from the input
    // and check whether EOF has been reached.
    ParseStatus Parse();

    // Extract the next JSON value from the input.
    ParseStatus ParseValue();

private:
    ParseStatus ConsumePrimitive(TokenKind kind);

    ParseStatus ConsumeString();
    ParseStatus ConsumeNumber();
    ParseStatus ConsumeIdentifier();

    TokenKind Peek(); // idemp

    TokenKind Lex(TokenKind kind);

    void Discard(); // idemp

    void Skip(TokenKind kind);
};

template <typename ParseCallbacks>
Parser<ParseCallbacks>::Parser(ParseCallbacks& cb_, Mode mode_)
    : cb(cb_)
    , mode(mode_)
{
}

template <typename ParseCallbacks>
void Parser<ParseCallbacks>::Init(char const* next, char const* last)
{
    lexer.SetInput(next, last);
}

template <typename ParseCallbacks>
Token Parser<ParseCallbacks>::GetPeekToken()
{
    JSON_ASSERT(curr.kind != TokenKind::discarded);
    return curr;
}

template <typename ParseCallbacks>
ParseStatus Parser<ParseCallbacks>::Parse()
{
    ParseStatus ec = ParseValue();

    // We want to return the last token in ParseSAX() below.
    // If curr has been successfully consumed in the ParseValue() function above,
    // read one more token (and use this to check for EOF).
    if (curr.kind == TokenKind::discarded)
    {
        curr = lexer.Lex(Peek());
        peek = TokenKind::discarded;
    }

    JSON_ASSERT(curr.kind != TokenKind::discarded);
    JSON_ASSERT(peek == TokenKind::discarded);

    if (ec == ParseStatus::success)
    {
        if (curr.kind != TokenKind::eof)
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

    peek = lexer.Peek(mode);
    curr.kind = TokenKind::discarded;

    // Parse 'value'
    if (peek == TokenKind::l_brace)
        goto L_begin_object;
    if (peek == TokenKind::l_square)
        goto L_begin_array;

    if (Failed ec = ConsumePrimitive(peek))
        return ParseStatus(ec);

    Discard();

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
    JSON_ASSERT(peek == TokenKind::l_brace);

    if (stack_size >= kMaxDepth)
        return ParseStatus::max_depth_reached;

    stack[stack_size] = {0, TokenKind::r_brace};
    ++stack_size;

    if (Failed ec = cb.HandleBeginObject())
        return ParseStatus(ec);

    // Skip the '{'.
    Skip(TokenKind::l_brace);

    // Get the possible kind of the next token.
    // This must be either an '}' or a key.
    Peek();
    if (peek != TokenKind::r_brace)
    {
        for (;;)
        {
            if (peek == TokenKind::string)
            {
                Lex(TokenKind::string);

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
                Lex(TokenKind::identifier);

                if (Failed ec = cb.HandleKey(curr.ptr, curr.end, curr.string_class))
                    return ParseStatus(ec);
            }
            else
            {
                return ParseStatus::expected_key;
            }

            // Mark the current token (the key) as used.
            Discard();

            // Read the token after the key.
            // This must be a ':'.
            Peek();

            if (peek != TokenKind::colon)
                return ParseStatus::expected_colon_after_key;

            Skip(TokenKind::colon);

            // Read the token after the ':'.
            // This must be a JSON value.
            Peek();

            if (peek == TokenKind::l_brace)
                goto L_begin_object;
            if (peek == TokenKind::l_square)
                goto L_begin_array;

            if (Failed ec = ConsumePrimitive(peek))
                return ParseStatus(ec);

            Discard();

L_end_member:
            JSON_ASSERT(stack_size != 0);
            JSON_ASSERT(stack[stack_size - 1].close == TokenKind::r_brace);
            ++stack[stack_size - 1].count;

            if (Failed ec = cb.HandleEndMember(stack[stack_size - 1].count))
                return ParseStatus(ec);

            // Expect a ',' (and another member) or a closing '}'.
            Peek();
            if (peek == TokenKind::comma)
            {
                // Read the token after the ','.
                // This must be a key.
                Skip(TokenKind::comma);

                Peek();
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

    Skip(TokenKind::r_brace);
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
    Skip(TokenKind::l_square);

    Peek();
    if (peek != TokenKind::r_square)
    {
        for (;;)
        {
            if (peek == TokenKind::l_brace)
                goto L_begin_object;
            if (peek == TokenKind::l_square)
                goto L_begin_array;

            if (Failed ec = ConsumePrimitive(peek))
                return ParseStatus(ec);

            Discard();

L_end_element:
            JSON_ASSERT(stack_size != 0);
            JSON_ASSERT(stack[stack_size - 1].close == TokenKind::r_square);
            ++stack[stack_size - 1].count;

            if (Failed ec = cb.HandleEndElement(stack[stack_size - 1].count))
                return ParseStatus(ec);

            // Expect a ',' (and another element) or a closing ']'.
            Peek();
            if (peek == TokenKind::comma)
            {
                // Read the token after the ','.
                // This must be a JSON value.
                Skip(TokenKind::comma);

                Peek();
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

    Skip(TokenKind::r_square);
    goto L_end_structured;

L_end_structured:
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
JSON_FORCE_INLINE ParseStatus Parser<ParseCallbacks>::ConsumePrimitive(TokenKind kind)
{
    JSON_ASSERT(curr.kind == TokenKind::discarded);
    JSON_ASSERT(peek == kind);
    JSON_ASSERT(kind != TokenKind::l_brace && kind != TokenKind::l_square);

    ParseStatus ec;
    switch (kind)
    {
    case TokenKind::string:
        ec = ConsumeString();
        break;
    case TokenKind::number:
        ec = ConsumeNumber();
        break;
    case TokenKind::identifier:
        ec = ConsumeIdentifier();
        break;
    default:
        ec = ParseStatus::expected_value;
        break;
    }

    return ec;
}

template <typename ParseCallbacks>
ParseStatus Parser<ParseCallbacks>::ConsumeString()
{
    Lex(TokenKind::string);
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
ParseStatus Parser<ParseCallbacks>::ConsumeNumber()
{
    Lex(TokenKind::number);
    JSON_ASSERT(curr.kind == TokenKind::number);

    return cb.HandleNumber(curr.ptr, curr.end, curr.number_class);
}

template <typename ParseCallbacks>
ParseStatus Parser<ParseCallbacks>::ConsumeIdentifier()
{
    Lex(TokenKind::identifier);
    JSON_ASSERT(curr.kind == TokenKind::identifier);

    char const* next = curr.ptr;
    char const* last = curr.end;

    switch (ScanIdentifer(next, last))
    {
    case Identifier::null:
        return cb.HandleNull();
    case Identifier::true_:
        return cb.HandleTrue();
    case Identifier::false_:
        return cb.HandleFalse();
    case Identifier::nan:
        curr.kind = TokenKind::number;
        curr.number_class = NumberClass::nan;
        return cb.HandleNumber(curr.ptr, curr.end, curr.number_class);
    case Identifier::infinity:
        curr.kind = TokenKind::number;
        curr.number_class = NumberClass::pos_infinity;
        return cb.HandleNumber(curr.ptr, curr.end, curr.number_class);
    default:
        return ParseStatus::unrecognized_identifier;
    }
}

template <typename ParseCallbacks>
JSON_FORCE_INLINE TokenKind Parser<ParseCallbacks>::Peek()
{
    //JSON_ASSERT(curr.kind == TokenKind::discarded);
    //JSON_ASSERT(peek == TokenKind::discarded);

    curr.kind = TokenKind::discarded;
    peek = lexer.Peek(mode);

    return peek;
}

template <typename ParseCallbacks>
JSON_FORCE_INLINE TokenKind Parser<ParseCallbacks>::Lex(TokenKind kind)
{
    JSON_ASSERT(curr.kind == TokenKind::discarded);
    JSON_ASSERT(kind != TokenKind::discarded);

    curr = lexer.Lex(kind);
    peek = TokenKind::discarded;

    return curr.kind;
}

template <typename ParseCallbacks>
JSON_FORCE_INLINE void Parser<ParseCallbacks>::Discard()
{
    curr.kind = TokenKind::discarded;
    peek = TokenKind::discarded;
}

template <typename ParseCallbacks>
JSON_FORCE_INLINE void Parser<ParseCallbacks>::Skip(TokenKind kind)
{
    lexer.Skip(kind);
    Discard();
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
ParseResult ParseSAX(ParseCallbacks& cb, char const* next, char const* last, Mode mode)
{
    JSON_ASSERT(next != nullptr);
    JSON_ASSERT(last != nullptr);

    Parser<ParseCallbacks> parser(cb, mode);

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
//json::ParseResult ParseJson(ParseCallbacks& cb, char const* next, char const* last, json::Mode mode = json::Mode::strict)
//{
//    return json::ParseSAX(cb, next, last, mode);
//}
