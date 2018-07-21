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

#include "json_parser.h"

namespace json {

//==================================================================================================
//
//==================================================================================================

namespace charclass {

#if 0
inline int HexDigitValue(char ch)
{
    enum : int8_t { N = -1 };

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

    return kMap[static_cast<unsigned char>(ch)];
}
#else
inline int HexDigitValue(char ch)
{
#if 0
    if ('0' <= ch && ch <= '9') { return ch - '0'; }
    if ('A' <= ch && ch <= 'F') { return ch - 'A' + 10; }
    if ('a' <= ch && ch <= 'f') { return ch - 'a' + 10; }
    return -1;
#else
    auto const i = ch - '0';
    if (0 <= i && i <= 9) { return i; }
    auto const j = (ch | 0x20) - 'a';
    if (0 <= j && j <= 5) { return j + 10; }
    return -1;
#endif
}
#endif

} // namespace charclass

//==================================================================================================
// Unicode support
//==================================================================================================

namespace impl {
namespace unicode {

constexpr char32_t kInvalidCodepoint = 0xFFFFFFFF;

inline bool IsValidCodepoint(char32_t U)
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

template <typename It>
It FindNextUTF8Sequence(It next, It last)
{
    // Skip UTF-8 trail bytes.
    // The first non-trail byte is the start of a (possibly invalid) UTF-8 sequence.
    while (next != last && IsUTF8Trail(*next))
    {
        ++next;
    }
    return next;
}

inline int GetUTF8SequenceLengthFromLeadByte(char ch, char32_t& U)
{
    uint32_t const b = static_cast<uint8_t>(ch);

    if (b <= 0x7F) { U = b;        return 1; } // 01111111 (0xxxxxxx)
    if (b <= 0xC1) {               return 0; }
    if (b <= 0xDF) { U = b & 0x1F; return 2; } // 11011111 (110xxxxx)
    if (b <= 0xEF) { U = b & 0x0F; return 3; } // 11101111 (1110xxxx)
    if (b <= 0xF4) { U = b & 0x07; return 4; } // 11110100 (11110xxx)
    return 0;
}

inline int GetUTF8SequenceLengthFromCodepoint(char32_t U)
{
    JSON_ASSERT(IsValidCodepoint(U));

    if (U <=   0x7F) { return 1; }
    if (U <=  0x7FF) { return 2; }
    if (U <= 0xFFFF) { return 3; }
    return 4;
}

inline bool IsUTF8OverlongSequence(char32_t U, int slen)
{
    return slen != GetUTF8SequenceLengthFromCodepoint(U);
}

template <typename It>
It DecodeUTF8Sequence(It next, It last, char32_t& U)
{
    JSON_ASSERT(next != last);

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
void EncodeUTF8(char32_t U, Put8 put)
{
    JSON_ASSERT(IsValidCodepoint(U));

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
void EncodeUTF16(char32_t U, Put16 put)
{
    JSON_ASSERT(IsValidCodepoint(U));

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
template <typename It>
bool ReadHex16(It& first, It last, char32_t& W)
{
    using namespace json::charclass;

    auto f = first;

    if (last - f < 4) {
        first = last;
        return false;
    }

    auto const h0 = HexDigitValue(*f); ++f;
    auto const h1 = HexDigitValue(*f); ++f;
    auto const h2 = HexDigitValue(*f); ++f;
    auto const h3 = HexDigitValue(*f); ++f;
    first = f;

    if ((h0 | h1 | h2 | h3) >= 0)
    {
        W = static_cast<char32_t>((h0 << 12) | (h1 << 8) | (h2 << 4) | h3);
        return true;
    }

    return false;
}

// Reads a hexadecimal number of the form "\uHHHH".
// Stores the result in W on success.
template <typename It>
bool ReadUCN(It& first, It last, char32_t& W)
{
    using namespace json::charclass;

    auto f = first;

    if (last - f < 6) {
        first = last;
        return false;
    }

    auto const c0 = *f; ++f;
    auto const c1 = *f; ++f;
    auto const h0 = HexDigitValue(*f); ++f;
    auto const h1 = HexDigitValue(*f); ++f;
    auto const h2 = HexDigitValue(*f); ++f;
    auto const h3 = HexDigitValue(*f); ++f;
    first = f;

    if (c0 == '\\' && c1 == 'u' && (h0 | h1 | h2 | h3) >= 0)
    {
        W = static_cast<char32_t>((h0 << 12) | (h1 << 8) | (h2 << 4) | h3);
        return true;
    }

    return false;
}

template <typename It>
It DecodeTrimmedUCNSequence(It next, It last, char32_t& U)
{
    JSON_ASSERT(next != last);

    char32_t W1;
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

    char32_t W2;
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

//==================================================================================================
// Strings
//==================================================================================================

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
    namespace unicode = json::impl::unicode;
    using namespace json::charclass;

    for (;;)
    {
#if 1
#if JSON_USE_SSE42
        const __m128i special_chars = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, '\xFF', '\x80', '\x1F', '\x00', '\\', '\\', '\"', '\"');

        char const* p = next;
        for ( ; last - next >= 16; next += 16)
        {
            const auto bytes = _mm_loadu_si128(reinterpret_cast<__m128i const*>(next));
            const auto index = _mm_cmpestri(special_chars, 8, bytes, 16, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_LEAST_SIGNIFICANT);
            if (index != 16) {
                next += index;
                break;
            }
        }
        for (intptr_t i = 0; i < next - p; ++i)
        {
            yield(p[i]);
        }
#endif
#endif

        for ( ; next != last; ++next)
        {
            auto const m = CharClass(*next);
            if ((m & (CC_StringSpecial | CC_NeedsCleaning)) != 0)
                break;
            yield(*next);
        }

        if (next == last)
            break;

        if (*next == '\\')
        {
            auto const f = next; // The start of the possible UCN sequence

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

                    char32_t U = 0;
                    auto const end = unicode::DecodeTrimmedUCNSequence(next, last, U);
                    JSON_ASSERT(end != next);
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
        else if (static_cast<unsigned char>(*next) < 0x20)
        {
            return {next, Status::unescaped_control_character};
        }
        else
        {
            JSON_ASSERT(static_cast<unsigned char>(*next) >= 0x80);

            auto f = next; // The start of the UTF-8 sequence

            char32_t U = 0;
            next = unicode::DecodeUTF8Sequence(next, last, U);
            JSON_ASSERT(next != f);

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

#if 0
template <typename Fn>
inline char const* SkipNonSpecialEscapeFast(char const* p, char const* end, Fn yield)
{
    using namespace json::charclass;

    static const __m128i chars = _mm_set_epi8(0,0, 0,0, 0,0, '/','/', '\xFF','\x80', '\x1F','\x00', '\\','\\', '\"','\"');

    for ( ; end - p >= 16; p += 16)
    {
        const auto bytes = _mm_loadu_si128(reinterpret_cast<__m128i const*>(p));
        const auto index = _mm_cmpestri(chars, 8, bytes, 16, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_LEAST_SIGNIFICANT);
        if (index != 16)
            return p + index;
        for (int i = 0; i < 16; ++i)
            yield(p[i]);
    }

    for ( ; p != end; ++p)
    {
        auto const m = CharClass(*p);
        if ((m & (CC_StringSpecial | CC_NeedsCleaning)) != 0)
            return p;
        yield(*p);
    }

    return p;
}
#endif

template <typename Fn>
EscapeStringResult EscapeString(char const* next, char const* last, Fn yield)
{
    namespace unicode = json::impl::unicode;

    static constexpr char const kHexDigits[] = "0123456789ABCDEF";

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

            char32_t U = 0;
            next = unicode::DecodeUTF8Sequence(next, last, U);
            JSON_ASSERT(next != f);

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
} // namespace json
