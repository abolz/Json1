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

// Returns the decimal value for the given hexadecimal character.
// Returns UINT32_MAX, if the given character is not a hexadecimal character.
inline uint32_t HexDigitValue(char ch)
{
    enum : int8_t { N = -1 };
    static constexpr int8_t kMap[256] = {
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, N, N, N, N, N, N,
        N, 10,11,12,13,14,15,N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, 10,11,12,13,14,15,N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
        N, N, N, N, N, N, N, N, N, N, N, N, N, N, N, N,
    };

    return static_cast<uint32_t>(kMap[static_cast<unsigned char>(ch)]);
}

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
    auto const b = static_cast<uint8_t>(ch);

    return 0x80 == (b & 0xC0); // b == 10xxxxxx???
}

inline char const* FindNextUTF8Sequence(char const* next, char const* last)
{
    // Skip UTF-8 trail bytes.
    // The first non-trail byte is the start of a (possibly invalid) UTF-8 sequence.
    for ( ; next != last && IsUTF8Trail(*next); ++next)
    {
    }

    return next;
}

inline int GetUTF8SequenceLengthFromLeadByte(char ch, char32_t& U)
{
    auto const b = static_cast<uint8_t>(ch);

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

struct DecodeUTF8SequenceResult {
    char const* next;
    bool success;
};

inline DecodeUTF8SequenceResult DecodeUTF8Sequence(char const* next, char const* last, char32_t& U)
{
    JSON_ASSERT(next != last);

    char32_t W = 0;
    int const slen = GetUTF8SequenceLengthFromLeadByte(*next, W);
    ++next;

    if (slen == 0)
    {
        // Invalid lead byte.
#if 0
        // Skip ahead to the start of the next UTF-8 sequence.
        next = FindNextUTF8Sequence(next, last);
#endif
        return {next, false};
    }

    for (int i = 1; i < slen; ++i) // Consume 1,2, or 3 trail bytes.
    {
        if (next == last || !IsUTF8Trail(*next))
        {
            // Incomplete UTF-8 sequence or invalid trail byte.
            // No need to skip to the start of the next UTF-8 sequence:
            // Anything which does not look like a trail byte is
            // considered to be the start of a (possibly invalid) UTF-8
            // sequence.
            return {next, false};
        }

        W = (W << 6) | (static_cast<uint8_t>(*next) & 0x3F);
        ++next;
    }

    // Test for invalid codepoints or overlong sequences.
    // XXX: This check is actually only required for is the lead byte is
    //      E0, ED, F0, or F4. (?!?!)
    if (!IsValidCodepoint(W) || IsUTF8OverlongSequence(W, slen))
    {
        // XXX: Skip to the start of the next UTF-8 sequence here?!?!
        // next = FindNextUTF8Sequence(next, last);
        return {next, false};
    }

    U = W;
    return {next, true};
}

inline DecodeUTF8SequenceResult ValidateUTF8Sequence(char const* next, char const* last)
{
    JSON_ASSERT(next != last);
    JSON_ASSERT(static_cast<unsigned char>(*next) >= 0x80);

#if 0
    char32_t U = 0;
    return DecodeUTF8Sequence(next, last, U);
#else
    // Copyright (c) 2008-2010 Bjoern Hoehrmann <bjoern@hoehrmann.de>
    // See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

    static constexpr uint8_t utf8d[] = {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 00..1f
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 20..3f
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 40..5f
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, // 60..7f
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9,9, // 80..9f
        7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7, // a0..bf
        8,8,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, // c0..df
        0xa,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x3,0x4,0x3,0x3, // e0..ef
        0xb,0x6,0x6,0x6,0x5,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8,0x8, // f0..ff

        0x0,0x1,0x2,0x3,0x5,0x8,0x7,0x1,0x1,0x1,0x4,0x6,0x1,0x1,0x1,0x1, // s0..s0
        1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,0,1,0,1,1,1,1,1,1, // s1..s2
        1,2,1,1,1,1,1,2,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1, // s3..s4
        1,2,1,1,1,1,1,1,1,2,1,1,1,1,1,1,1,1,1,1,1,1,1,3,1,3,1,1,1,1,1,1, // s5..s6
        1,3,1,1,1,1,1,3,1,3,1,1,1,1,1,1,1,3,1,1,1,1,1,1,1,1,1,1,1,1,1,1, // s7..s8
    };

#define UTF8_ACCEPT 0
#define UTF8_REJECT 1

    uint32_t state = UTF8_ACCEPT;
    for (;;)
    {
        state = utf8d[256 + 16 * state + utf8d[static_cast<unsigned char>(*next)]];
        ++next;
        if (next == last || state == UTF8_ACCEPT || state == UTF8_REJECT)
            break;
    }

    return {next, state == UTF8_ACCEPT};

#undef UTF8_REJECT
#undef UTF8_ACCEPT
#endif
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

struct ReadUCNResult {
    char const* next;
    bool success;
};

// Reads a hexadecimal number of the form "HHHH".
// Stores the result in W on success.
inline ReadUCNResult ReadHex16(char const* next, char const* last, char32_t& W)
{
    using namespace json::charclass;

    if (last - next < 4)
        return {last, false};

    uint32_t const h0 = HexDigitValue(next[0]);
    uint32_t const h1 = HexDigitValue(next[1]);
    uint32_t const h2 = HexDigitValue(next[2]);
    uint32_t const h3 = HexDigitValue(next[3]);

    uint32_t const value = (h0 << 12) | (h1 << 8) | (h2 << 4) | h3;
    next += 4;

    W = static_cast<char32_t>(value);
    return {next, value <= 0xFFFF};
}

// Reads a hexadecimal number of the form "\uHHHH".
// Stores the result in W on success.
inline ReadUCNResult ReadUCN(char const* next, char const* last, char32_t& W)
{
    using namespace json::charclass;

    if (last - next < 6)
        return {last, false};

    char const* const u = next;

    uint32_t const h0 = HexDigitValue(next[2]);
    uint32_t const h1 = HexDigitValue(next[3]);
    uint32_t const h2 = HexDigitValue(next[4]);
    uint32_t const h3 = HexDigitValue(next[5]);

    uint32_t const value = (h0 << 12) | (h1 << 8) | (h2 << 4) | h3;
    next += 6;

    W = static_cast<char32_t>(value);
    return {next, std::memcmp(u, "\\u", 2) == 0 && value <= 0xFFFF};
}

struct DecodeUCNSequenceResult {
    char const* next;
    bool success;
};

inline DecodeUCNSequenceResult DecodeTrimmedUCNSequence(char const* next, char const* last, char32_t& U)
{
    JSON_ASSERT(next != last);

    char32_t W1;
    auto const r1 = ReadHex16(next, last, W1);
    next = r1.next;

    if (!r1.success)
    {
        return {next, false};
    }

    if (W1 < 0xD800 || W1 > 0xDFFF)
    {
        U = W1;
        return {next, true};
    }

    if (W1 > 0xDBFF)
    {
        return {next, false};
    }

    char32_t W2;
    auto const r2 = ReadUCN(next, last, W2);
    next = r2.next;

    if (!r2.success)
    {
        return {next, false};
    }

    if (W2 < 0xDC00 || W2 > 0xDFFF)
    {
        return {next, false};
    }

    U = (((W1 & 0x3FF) << 10) | (W2 & 0x3FF)) + 0x10000;
    return {next, true};
}

} // namespace unicode
} // namespace impl

//==================================================================================================
// Strings
//==================================================================================================

namespace impl {
namespace strings {

template <typename It>
struct SkipCharsResult {
    It next;
    bool success;
};

inline SkipCharsResult<char const*> SkipNonSpecialLatin1_unescape(char const* next, char const* last)
{
#if JSON_USE_SSE42
    __m128i const kSpecialChars = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\xFF', '\x80', '\x1F', '\x00', '\\', '\\');

    for ( ; last - next >= 16; next += 16)
    {
        auto const bytes = _mm_loadu_si128(reinterpret_cast<__m128i const*>(next));
        auto const index = _mm_cmpestri(kSpecialChars, 6, bytes, 16, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_LEAST_SIGNIFICANT);
        if (index != 16) {
            return {next + index, true};
        }
    }
#endif
    for ( ; next != last; ++next)
    {
        if (*next == '\\' || static_cast<uint8_t>(*next) >= 0x80 || static_cast<uint8_t>(*next) <= 0x1F)
            break;
    }

    return {next, true};
}

inline SkipCharsResult<char const*> SkipLongUTF8Sequences_unescape(char const* next, char const* last)
{
    using namespace json::impl::unicode;

    JSON_ASSERT(next != last);
    JSON_ASSERT(static_cast<uint8_t>(*next) >= 0x80);

    for (;;)
    {
        auto const seq = ValidateUTF8Sequence(next, last);
        JSON_ASSERT(next != seq.next); // XXX: Does not work for InputIterator's

        if (!seq.success)
            return {next, false};

        next = seq.next;

        if (next == last || static_cast<uint8_t>(*next) < 0x80)
            break;
    }

    return {next, true};
}

} // namespace strings
} // namespace impl

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

struct UnescapeStringResult {
    char const* next;
    Status status;
};

template <typename Fn>
UnescapeStringResult UnescapeString(char const* curr, char const* last, Fn yield)
{
    namespace unicode = ::json::impl::unicode;
    using namespace charclass;

    auto yield_n = [&](char const* p, intptr_t n)
    {
        for (intptr_t i = 0; i < n; ++i) {
            yield(p[i]);
        }
        return p + n;
    };

    for (;;)
    {
        {
            auto const res = ::json::impl::strings::SkipNonSpecialLatin1_unescape(curr, last);
            curr = yield_n(curr, res.next - curr);
        }

        if (curr == last)
        {
            break;
        }

        if (static_cast<uint8_t>(*curr) >= 0x80)
        {
            auto const res = ::json::impl::strings::SkipLongUTF8Sequences_unescape(curr, last);
            curr = yield_n(curr, res.next - curr);

            if (!res.success)
            {
                return {curr, Status::invalid_utf8_sequence};
            }
        }
        else if (*curr == '\\')
        {
            auto const f = curr; // The start of the possible UCN sequence

            ++curr; // skip '\'
            if (curr == last)
            {
                return {curr, Status::incomplete};
            }

            switch (*curr)
            {
            case '"':
                yield('\"');
                ++curr;
                break;
            case '\\':
                yield('\\');
                ++curr;
                break;
            case '/':
                yield('/');
                ++curr;
                break;
            case 'b':
                yield('\b');
                ++curr;
                break;
            case 'f':
                yield('\f');
                ++curr;
                break;
            case 'n':
                yield('\n');
                ++curr;
                break;
            case 'r':
                yield('\r');
                ++curr;
                break;
            case 't':
                yield('\t');
                ++curr;
                break;
            case 'u':
                {
                    ++curr;
                    if (curr == last)
                    {
                        return {f, Status::incomplete};
                    }

                    char32_t U = 0;
                    auto const seq = unicode::DecodeTrimmedUCNSequence(curr, last, U);
                    JSON_ASSERT(seq.next != curr); // XXX: Does not work for InputIterator's
                    curr = seq.next;

                    if (!seq.success)
                    {
                        return {f, Status::invalid_utf8_sequence};
                    }

                    unicode::EncodeUTF8(U, [&](uint8_t code_unit) { yield(static_cast<char>(code_unit)); });
                }
                break;
            default:
                return {curr, Status::invalid_escaped_character}; // invalid escaped character
            }
        }
        else
        {
            JSON_ASSERT(static_cast<uint8_t>(*curr) <= 0x1F);
            return {curr, Status::unescaped_control_character};
        }
    }

    return {curr, Status::success};
}

struct EscapeStringResult {
    char const* next;
    Status status;
};

template <typename Fn>
EscapeStringResult EscapeString(char const* next, char const* last, Fn yield)
{
    namespace unicode = json::impl::unicode;

    static constexpr char const kHexDigits[] = "0123456789ABCDEF";

#if JSON_USE_SSE42
    __m128i const kSpecialChars = _mm_set_epi8(0, 0, 0, 0, 0, 0, '/','/', '\xFF','\x80', '\x1F','\x00', '\\','\\', '"','"');
#endif

    auto const first = next;
    for (;;)
    {
#if JSON_USE_SSE42
        // Fast path for non-special Latin1
        if (last - next >= 16)
        {
            auto const p = next;
            for ( ; last - next >= 16; next += 16)
            {
                auto const bytes = _mm_loadu_si128(reinterpret_cast<__m128i const*>(next));
                auto const index = _mm_cmpestri(kSpecialChars, 10, bytes, 16, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_LEAST_SIGNIFICANT);
                if (index != 16) {
                    next += index;
                    break;
                }
            }
            for (intptr_t i = 0; i < next - p; ++i)
            {
                yield(p[i]);
            }
        }
#endif

        if (next == last)
            break;

        char const ch = *next;
        auto const uc = static_cast<uint8_t>(ch);

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
                if (next != first && next[-1] == '<')
                {
                    yield('\\');
                }
                break;
            //case '\x7F': // DEL
            //    if (escape_del)
            //    {
            //        yield('\\');
            //    }
            //    break;
            }
            yield(ch);
            ++next;
        }
        else // (possibly) the start of a UTF-8 sequence.
        {
            char32_t U;
            auto const seq = unicode::DecodeUTF8Sequence(next, last, U);
            JSON_ASSERT(next != seq.next);

            if (!seq.success)
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
                //if (ascii_only_output)
                //{
                //    unicode::EncodeUTF16(U, [&](uint16_t W) {
                //        yield('\\');
                //        yield('u');
                //        yield(kHexDigits[(W >> 12)      ]);
                //        yield(kHexDigits[(W >>  8) & 0xF]);
                //        yield(kHexDigits[(W >>  4) & 0xF]);
                //        yield(kHexDigits[(W      ) & 0xF]);
                //    });
                //}
                //else
                {
                    // The UTF-8 sequence is valid. No need to re-encode.
                    for (intptr_t i = 0; i < seq.next - next; ++i)
                    {
                        yield(next[i]);
                    }
                }
                break;
            }

            next = seq.next;
        }
    }

    return {next, Status::success};
}

} // namespace strings
} // namespace json
