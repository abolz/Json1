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

#include "json_options.h"

#include <cassert>
#include <cstdint>

namespace json {
namespace strings {

//==================================================================================================
//
//==================================================================================================

constexpr uint32_t kInvalidCodepoint = 0xFFFFFFFF;

inline bool IsValidCodepoint(uint32_t U)
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

inline int GetUTF8SequenceLengthFromLeadByte(char ch, uint32_t& U)
{
    uint32_t const b = static_cast<uint8_t>(ch);

    if (b <= 0x7F) { U = b;        return 1; } // 01111111 (0xxxxxxx)
    if (b <= 0xC1) {               return 0; }
    if (b <= 0xDF) { U = b & 0x1F; return 2; } // 11011111 (110xxxxx)
    if (b <= 0xEF) { U = b & 0x0F; return 3; } // 11101111 (1110xxxx)
    if (b <= 0xF4) { U = b & 0x07; return 4; } // 11110100 (11110xxx)
    return 0;
}

inline int GetUTF8SequenceLengthFromCodepoint(uint32_t U)
{
    assert(IsValidCodepoint(U));

    if (U <=   0x7F) { return 1; }
    if (U <=  0x7FF) { return 2; }
    if (U <= 0xFFFF) { return 3; }
    return 4;
}

inline bool IsUTF8OverlongSequence(uint32_t U, int slen)
{
    return slen != GetUTF8SequenceLengthFromCodepoint(U);
}

template <typename It>
It DecodeUTF8Sequence(It next, It last, uint32_t& U)
{
    assert(next != last);

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
        ++next;

        if (!IsUTF8Trail(cb))
        {
            U = kInvalidCodepoint;
            return next;
        }

        U = (U << 6) | (static_cast<uint8_t>(cb) & 0x3F);
    }

    if (!IsValidCodepoint(U) || IsUTF8OverlongSequence(U, slen))
    {
        U = kInvalidCodepoint;
        return next;
    }

    return next;
}

template <typename Put8>
void EncodeUTF8(uint32_t U, Put8 put)
{
    assert(IsValidCodepoint(U));

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

#if 0
template <typename It, typename Put32>
bool ForEachUTF8EncodedCodepoint(It next, It last, Put32 put)
{
    while (next != last)
    {
        uint32_t U = 0;

        auto const next1 = DecodeUTF8Sequence(next, last, U);
        assert(next != next1);
        next = next1;

        if (!put(U))
            return false;
    }

    return true;
}
#endif

#if 0
template <typename It>
It DecodeUTF16Sequence(It next, It last, uint32_t& U)
{
    assert(next != last);

    uint32_t const W1 = static_cast<uint16_t>(*next);
    ++next;

    if (W1 < 0xD800 || W1 > 0xDFFF)
    {
        U = W1;
        return next;
    }

    if (W1 > 0xDBFF || next == last)
    {
        U = kInvalidCodepoint; // Invalid high surrogate or incomplete UTF-16 sequence
        return next;
    }

    uint32_t const W2 = static_cast<uint16_t>(*next);
    ++next;

    if (W2 < 0xDC00 || W2 > 0xDFFF)
    {
        U = kInvalidCodepoint;
        return next;
    }

    U = (((W1 & 0x3FF) << 10) | (W2 & 0x3FF)) + 0x10000;
    return next;
}
#endif

template <typename Put16>
void EncodeUTF16(uint32_t U, Put16 put)
{
    assert(IsValidCodepoint(U));

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

#if 0
template <typename It, typename Put32>
bool ForEachUTF16EncodedCodepoint(It next, It last, Put32 put)
{
    while (next != last)
    {
        uint32_t U = 0;

        auto const next1 = DecodeUTF16Sequence(next, last, U);
        assert(next != next1);
        next = next1;

        if (!put(U))
            return false;
    }

    return true;
}
#endif

#if 1
inline int HexDigitValue(char ch)
{
#define N -1
    static constexpr int8_t const kDigitValue[256] = {
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
#undef N

    return kDigitValue[static_cast<unsigned char>(ch)];
}
#else
inline int HexDigitValue(char ch)
{
    if ('0' <= ch && ch <= '9') { return ch - '0'; }
    if ('A' <= ch && ch <= 'F') { return ch - 'A' + 10; }
    if ('a' <= ch && ch <= 'f') { return ch - 'a' + 10; }
    return -1;
}
#endif

template <typename It>
bool ReadHex16_unguarded(It& first, uint32_t& W)
{
    int const h0 = HexDigitValue(*first++);
    int const h1 = HexDigitValue(*first++);
    int const h2 = HexDigitValue(*first++);
    int const h3 = HexDigitValue(*first++);

    int const value = (h0 << 12) | (h1 << 8) | (h2 << 4) | h3;
    if (value >= 0)
    {
        assert(value <= 0xFFFF);
        W = static_cast<uint16_t>(value);
        return true;
    }

    return false;
}

template <typename It>
bool ReadHex16(It& first, It last, uint32_t& W)
{
    if (last - first < 4)
    {
        first = last;
        return false;
    }

    return ReadHex16_unguarded(first, W);
}

// Reads a hexadecimal number of the form "\uHHHH".
// Stores the result in W on success.
template <typename It>
bool ReadUCN(It& first, It last, uint32_t& W)
{
    if (last - first < 6)
    {
        first = last;
        return false;
    }

    auto const next = first + 6;

    if (*first++ != '\\' || *first++ != 'u')
    {
        first = next;
        return false;
    }

    return ReadHex16_unguarded(first, W);
}

template <typename It>
It DecodeTrimmedUCNSequence(It next, It last, uint32_t& U)
{
    assert(next != last);

    uint32_t W1;
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

    uint32_t W2;
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

#if 0
template <typename It>
It DecodeUCNSequence(It next, It last, uint32_t& U)
{
    assert(next != last);

    uint32_t W1;
    if (!ReadUCN(next, last, W1))
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

    uint32_t W2;
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
#endif

//==================================================================================================
//
//==================================================================================================

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

template <typename It>
struct UnescapeStringResult
{
    bool ok;
    It next;
};

template <typename It, typename Fn>
UnescapeStringResult<It> UnescapeString(
    It             next,
    It             last,
    Options const& options,
    Fn             yield)
{
    while (next != last)
    {
        auto const uc = static_cast<unsigned char>(*next);

        if (uc < 0x20) // unescaped control character
        {
            if (!options.allow_unescaped_control_characters)
                return {false, next};

            yield(*next);
            ++next;
        }
        else if (uc < 0x80) // ASCII printable or DEL
        {
            if (*next != '\\')
            {
                yield(*next);
                ++next;
                continue;
            }

            auto const f = next; // The start of the UCN sequence

            ++next; // skip '\'
            if (next == last)
                return {false, next};

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
                        return {false, f};

                    uint32_t U = 0;
                    auto const end = DecodeTrimmedUCNSequence(next, last, U);
                    assert(end != next);
                    next = end;

                    if (U != kInvalidCodepoint)
                    {
                        EncodeUTF8(U, [&](uint8_t code_unit) { yield(static_cast<char>(code_unit)); });
                    }
                    else
                    {
                        if (!options.allow_invalid_unicode)
                            return {false, f};

                        yield('\xEF');
                        yield('\xBF');
                        yield('\xBD');
                    }
                }
                break;
            default:
                return {false, next}; // invalid escaped character
            }
        }
        else // (possibly) the start of a UTF-8 sequence.
        {
            auto f = next; // The start of the UTF-8 sequence

            uint32_t U = 0;
            next = DecodeUTF8Sequence(next, last, U);
            assert(next != f);

            if (U != kInvalidCodepoint)
            {
                // The range [f, next) already contains a valid UTF-8 encoding of U.
                for ( ; f != next; ++f) {
                    yield(*f);
                }
            }
            else
            {
                if (!options.allow_invalid_unicode)
                    return {false, f};

                yield('\xEF');
                yield('\xBF');
                yield('\xBD');
            }
        }
    }

    return {true, next};
}

template <typename It>
struct EscapeStringResult
{
    bool ok;
    It next;
};

template <typename It, typename Fn>
EscapeStringResult<It> EscapeString(
    It             next,
    It             last,
    Options const& options,
    Fn             yield)
{
    static constexpr char const kHexDigits[] = "0123456789ABCDEF";

    yield('"');

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
                yield('\\');
                break;
            case '/':   // U+002F
                if (options.escape_slash && ch_prev == '<') {
                    yield('\\');
                }
                break;
            case '\\':  // U+005C
                yield('\\');
                break;
            }
            yield(ch);
            ++next;
        }
        else // (possibly) the start of a UTF-8 sequence.
        {
            auto f = next; // The start of the UTF-8 sequence

            uint32_t U = 0;
            next = DecodeUTF8Sequence(next, last, U);
            assert(next != f);

            if (U != kInvalidCodepoint)
            {
                //
                // Always escape U+2028 (LINE SEPARATOR) and U+2029 (PARAGRAPH
                // SEPARATOR). No string in JavaScript can contain a literal
                // U+2028 or a U+2029.
                //
                // (See http://timelessrepo.com/json-isnt-a-javascript-subset)
                //
                if (U == 0x2028)
                {
                    yield('\\');
                    yield('u');
                    yield('2');
                    yield('0');
                    yield('2');
                    yield('8');
                }
                else if (U == 0x2029)
                {
                    yield('\\');
                    yield('u');
                    yield('2');
                    yield('0');
                    yield('2');
                    yield('9');
                }
                else
                {
                    // The UTF-8 sequence is valid. No need to re-encode.
                    for ( ; f != next; ++f) {
                        yield(*f);
                    }
                }
            }
            else
            {
                if (!options.allow_invalid_unicode)
                    return {false, next};

                yield('\\');
                yield('u');
                yield('F');
                yield('F');
                yield('F');
                yield('D');
            }
        }
    }

    yield('"');

    return {true, next};
}

} // namespace strings
} // namespace json
