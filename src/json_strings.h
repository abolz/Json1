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

    return static_cast<uint32_t>(kMap[static_cast<uint8_t>(ch)]);
}

} // namespace charclass

//==================================================================================================
// Unicode support
//==================================================================================================

namespace impl {
namespace unicode {

inline bool IsValidCodepoint(char32_t U)
{
    // 1. Characters with values greater than 0x10FFFF cannot be encoded in
    //    UTF-16.
    // 2. Values between 0xD800 and 0xDFFF are specifically reserved for use
    //    with UTF-16, and don't have any characters assigned to them.
    return U < 0xD800 || (U > 0xDFFF && U <= 0x10FFFF);
}

struct DecodeUTF8SequenceResult {
    char const* ptr;
    bool success;
};

constexpr uint32_t kUTF8Accept = 0;
constexpr uint32_t kUTF8Reject = 1;

inline uint32_t DecodeUTF8Step(uint32_t state, uint8_t byte, char32_t& U)
{
    // Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
    // See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.

    static constexpr uint8_t kUTF8Decoder[] = {
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

    uint8_t const type = kUTF8Decoder[byte];

    // NB:
    // The conditional here will likely be optimized out in the loop below.

    if (state != kUTF8Accept)
        U = (U << 6) | (byte & 0x3Fu);
    else
        U = byte & (0xFFu >> type);

    state = kUTF8Decoder[256 + state * 16 + type];
    return state;
}

inline DecodeUTF8SequenceResult DecodeUTF8Sequence(char const* next, char const* last, char32_t& U)
{
    JSON_ASSERT(next != last);

    // Always consume the first byte.
    // The following bytes will only be consumed while the UTF-8 sequence is still valid.
    uint8_t const b1 = static_cast<uint8_t>(*next);
    ++next;

    char32_t W = 0;
    uint32_t state = DecodeUTF8Step(kUTF8Accept, b1, W);
    if (state == kUTF8Reject)
    {
        return {next, false};
    }

    while (state != kUTF8Accept)
    {
        if (next == last)
        {
            return {next, false};
        }
        state = DecodeUTF8Step(state, static_cast<uint8_t>(*next), W);
        if (state == kUTF8Reject)
        {
            return {next, false};
        }
        ++next;
    }

    U = W;
    return {next, true};
}

inline DecodeUTF8SequenceResult ValidateUTF8Sequence(char const* next, char const* last)
{
    char32_t U; // unused
    return DecodeUTF8Sequence(next, last, U);
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

enum class DecodeUCNSequenceStatus {
    success,
    invalid_utf16_encoding,
    incomplete_or_invalid_ucn,
};

struct DecodeUCNSequenceResult {
    char const* ptr;
    DecodeUCNSequenceStatus status;
};

inline bool ReadHex16_unsafe(char const* next, char32_t& W)
{
    using namespace ::json::charclass;

    char32_t const h0 = HexDigitValue(next[0]);
    char32_t const h1 = HexDigitValue(next[1]);
    char32_t const h2 = HexDigitValue(next[2]);
    char32_t const h3 = HexDigitValue(next[3]);

    W = (h0 << 12) | (h1 << 8) | (h2 << 4) | h3;
    return W <= 0xFFFF;
}

inline DecodeUCNSequenceResult DecodeUCNSequence(char const* next, char const* last, char32_t& U)
{
    JSON_ASSERT(last - next >= 2);
    JSON_ASSERT(next[0] == '\\');
    JSON_ASSERT(next[1] == 'u');

    char32_t W1;
    if (last - next < 6 /*|| std::memcmp(next, "\\u", 2) != 0*/ || !unicode::ReadHex16_unsafe(next + 2, W1))
    {
        // Incomplete or syntax error.
        // Return the position of the first (invalid) UCN.
        return {next, DecodeUCNSequenceStatus::incomplete_or_invalid_ucn};
    }

    // Always consume the first UCN.
    // The second UCN - if any - will only be consumed if the UTF16-sequence is valid.
    next += 6;

    if (W1 < 0xD800 || W1 > 0xDFFF)
    {
        U = W1;
        return {next, DecodeUCNSequenceStatus::success};
    }

    if (W1 > 0xDBFF)
    {
        // Illegal high surrogate.
        // Return the position following the invalid UCN.
        return {next, DecodeUCNSequenceStatus::invalid_utf16_encoding};
    }

    char32_t W2;
    if (last - next < 6 || std::memcmp(next, "\\u", 2) != 0 || !unicode::ReadHex16_unsafe(next + 2, W2))
    {
        // Incomplete or syntax error.
        // Return the position of the second (invalid) UCN.
        return {next, DecodeUCNSequenceStatus::incomplete_or_invalid_ucn};
    }

    if (W2 < 0xDC00 || W2 > 0xDFFF)
    {
        // Illegal low surrogate.
        // Return the position of the second UCN, which might be the start of a valid UTF-16 sequence.
        return {next, DecodeUCNSequenceStatus::invalid_utf16_encoding};
    }

    // Consume the second UCN.
    // The UTF-16 is valid.
    next += 6;

    U = (((W1 & 0x3FF) << 10) | (W2 & 0x3FF)) + 0x10000;
    return {next, DecodeUCNSequenceStatus::success};
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
    invalid_utf8_encoding,
    invalid_utf16_encoding,
    incomplete,
    invalid_ucn,
};

struct UnescapeStringResult {
    char const* next;
    Status status;
};

template <typename Fn>
UnescapeStringResult UnescapeString(char const* curr, char const* last, Fn yield, bool allow_invalid_unicode = false)
{
    namespace unicode = ::json::impl::unicode;
    using namespace ::json::charclass;

#if JSON_USE_SSE42
    auto const kSpecialChars = _mm_set_epi8(0, 0, 0, 0, 0, 0, 0, 0, 0, 0, '\xFF', '\x80', '\x1F', '\x00', '\\', '\\');
#endif

    auto skip_non_special = [=](char const* f, char const* l)
    {
#if JSON_USE_SSE42
        for ( ; l - f >= 16; f += 16)
        {
            auto const bytes = _mm_loadu_si128(reinterpret_cast<__m128i const*>(f));
            int const index = _mm_cmpestri(kSpecialChars, 6, bytes, 16, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_LEAST_SIGNIFICANT);
            if (index != 16)
                return f + index;
        }
#endif
        for ( ; f != l; ++f)
        {
            if (static_cast<uint8_t>(*f) >= 0x80 || static_cast<uint8_t>(*f) <= 0x1F || *f == '\\')
                break;
        }

        return f;
    };

    auto yield_n = [&](char const* p, intptr_t n)
    {
        for (intptr_t i = 0; i < n; ++i) {
            yield(p[i]);
        }
        return p + n;
    };

    for (;;)
    {
        char const* const next = skip_non_special(curr, last);
        yield_n(curr, next - curr);
        curr = next;

        if (curr == last)
        {
            break;
        }

        if (static_cast<uint8_t>(*curr) >= 0x80)
        {
            auto const res = unicode::ValidateUTF8Sequence(curr, last);
            JSON_ASSERT(curr != res.ptr);

            if (!res.success)
            {
                if (!allow_invalid_unicode)
                    return {curr, Status::invalid_utf8_encoding};

                // Replace invalid UTF-8 sequences with a single Unicode replacement character.
                yield_n("\xEF\xBF\xBD", 3);
            }
            else
            {
                yield_n(curr, res.ptr - curr);
            }

            curr = res.ptr;
        }
        else if (*curr == '\\')
        {
            char const* const f = curr; // The start of the possible UCN sequence

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
                    char32_t U;
                    auto const res = unicode::DecodeUCNSequence(f, last, U);
                    curr = res.ptr;

                    if (res.status == unicode::DecodeUCNSequenceStatus::incomplete_or_invalid_ucn)
                    {
                        // Syntax error.
                        return {curr, Status::invalid_ucn};
                    }
                    else if (res.status == unicode::DecodeUCNSequenceStatus::invalid_utf16_encoding)
                    {
                        if (!allow_invalid_unicode)
                            return {curr, Status::invalid_utf16_encoding};

                        // Replace invalid UTF-16 sequences with a single Unicode replacement character.
                        yield_n("\xEF\xBF\xBD", 3);
                    }
                    else
                    {
                        unicode::EncodeUTF8(U, [&](uint8_t code_unit) { yield(static_cast<char>(code_unit)); });
                    }
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
EscapeStringResult EscapeString(char const* curr, char const* last, Fn yield, bool allow_invalid_unicode = false)
{
    namespace unicode = json::impl::unicode;
    using namespace ::json::charclass;

    static constexpr char const kHexDigits[] = "0123456789ABCDEF";

#if JSON_USE_SSE42
    auto const kSpecialChars = _mm_set_epi8(0, 0, 0, 0, 0, 0, '/','/', '\xFF','\x80', '\x1F','\x00', '\\','\\', '"','"');
#endif

    auto skip_non_special = [=](char const* f, char const* l)
    {
#if JSON_USE_SSE42
        for ( ; l - f >= 16; f += 16)
        {
            auto const bytes = _mm_loadu_si128(reinterpret_cast<__m128i const*>(f));
            int const index = _mm_cmpestri(kSpecialChars, 10, bytes, 16, _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES | _SIDD_LEAST_SIGNIFICANT);
            if (index != 16)
                return f + index;
        }
#endif
        for ( ; f != l; ++f)
        {
            if (static_cast<uint8_t>(*f) >= 0x80 || static_cast<uint8_t>(*f) <= 0x1F || *f == '\\' || *f == '"' || *f == '/')
                break;
        }

        return f;
    };

    auto yield_n = [&](char const* p, intptr_t n)
    {
        for (intptr_t i = 0; i < n; ++i) {
            yield(p[i]);
        }
        return p + n;
    };

    char const* const first = curr;
    for (;;)
    {
        char const* const next = skip_non_special(curr, last);
        yield_n(curr, next - curr);
        curr = next;

        if (curr == last)
        {
            break;
        }

        if (static_cast<uint8_t>(*curr) >= 0x80)
        {
            char32_t U;
            auto const res = unicode::DecodeUTF8Sequence(curr, last, U);
            JSON_ASSERT(curr != res.ptr);

            if (!res.success)
            {
                if (!allow_invalid_unicode)
                    return {curr, Status::invalid_utf8_encoding};

                yield_n("\\uFFFD", 6);
            }
            else
            {
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
                    yield_n("\\u2028", 6);
                    break;
                case 0x2029:
                    yield_n("\\u2029", 6);
                    break;
                default:
                    // The UTF-8 sequence is valid. No need to re-encode.
                    yield_n(curr, res.ptr - curr);
                    break;
                }
            }

            curr = res.ptr;
        }
        else if (static_cast<uint8_t>(*curr) >= 0x20)
        {
            auto do_escape = [&](char ch) {
                switch (ch) {
                case '"':
                case '\\':
                    return true;
                case '/':
                    return curr != first && curr[-1] == '<';
                }
                return false;
            };

            if (do_escape(*curr))
            {
                yield('\\');
            }
            yield(*curr);
            ++curr;
        }
        else // (ASCII) control character
        {
            yield('\\');
            switch (*curr)
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
                yield(kHexDigits[static_cast<uint8_t>(*curr) >> 4]);
                yield(kHexDigits[static_cast<uint8_t>(*curr) & 0xF]);
                break;
            }
            ++curr;
        }
    }

    return {curr, Status::success};
}

} // namespace strings
} // namespace json

#if JSON_STRINGS_COMPILE_TEST
#if 0
size_t UnescapedStringLength(char const* next, char const* last)
{
    size_t num_bytes = 0;
    json::strings::UnescapeString(next, last, [&](char) { ++num_bytes; });
    return num_bytes;
}
#endif
#if 1
size_t EscapedStringLength(char const* next, char const* last)
{
    size_t num_bytes = 0;
    json::strings::EscapeString(next, last, [&](char) { ++num_bytes; });
    return num_bytes;
}
#endif
#endif
