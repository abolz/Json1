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
#include "json_unicode.h"

#include <cassert>
#include <cstdint>
#include <iterator>
#include <type_traits>

namespace json {
namespace strings {

namespace impl
{
    template <typename It>
    using IsInputIterator = std::integral_constant<
        bool,
        std::is_convertible<typename std::iterator_traits<It>::iterator_category, std::input_iterator_tag>::value &&
        !std::is_convertible<typename std::iterator_traits<It>::iterator_category, std::forward_iterator_tag >::value
    >;
}

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

enum class UnescapeStringStatus {
    success,
    unescaped_control_character,
    invalid_escaped_character,
    invalid_ucn_sequence,
    invalid_utf8_sequence,
    incomplete,
};

template <typename It>
struct UnescapeStringResult
{
    It next;
    UnescapeStringStatus status;
};

template <typename It, typename Fn>
UnescapeStringResult<It> UnescapeString(It next, It last, /*char quote_char,*/ Fn yield)
{
    while (next != last)
    {
        //if (*next == quote_char)
        //{
        //    ++next;
        //    break;
        //}

        auto const uc = static_cast<unsigned char>(*next);

        if (uc < 0x20) // unescaped control character
        {
            return {next, UnescapeStringStatus::unescaped_control_character};
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
                return {next, UnescapeStringStatus::incomplete};

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
                        return {f, UnescapeStringStatus::incomplete};

                    uint32_t U = 0;
                    auto const end = json::unicode::DecodeTrimmedUCNSequence(next, last, U);
                    //JSON_ASSERT(end != next);
                    next = end;

                    if (U != json::unicode::kInvalidCodepoint)
                    {
                        json::unicode::EncodeUTF8(U, [&](uint8_t code_unit) { yield(static_cast<char>(code_unit)); });
                    }
                    else
                    {
                        return {f, UnescapeStringStatus::invalid_utf8_sequence};
                    }
                }
                break;
            default:
                return {next, UnescapeStringStatus::invalid_escaped_character}; // invalid escaped character
            }
        }
        else // (possibly) the start of a UTF-8 sequence.
        {
            auto f = next; // The start of the UTF-8 sequence

            uint32_t U = 0;
            next = json::unicode::DecodeUTF8Sequence(next, last, U);
            //JSON_ASSERT(next != f);

            if (U != json::unicode::kInvalidCodepoint)
            {
                constexpr bool inpit = json::strings::impl::IsInputIterator<It>::value;
                if (inpit)
                {
                    json::unicode::EncodeUTF8(U, [&](uint8_t code_unit) { yield(static_cast<char>(code_unit)); });
                }
                else
                {
                    // The range [f, next) already contains a valid UTF-8 encoding of U.
                    for ( ; f != next; ++f) {
                        yield(*f);
                    }
                }
            }
            else
            {
                return {f, UnescapeStringStatus::invalid_utf8_sequence};
            }
        }
    }

    return {next, UnescapeStringStatus::success};
}

enum class EscapeStringStatus {
    success,
    invalid_utf8_sequence,
};

template <typename It>
struct EscapeStringResult
{
    It next;
    EscapeStringStatus status;
};

template <typename It, typename Fn>
EscapeStringResult<It> EscapeString(It next, It last, Fn yield)
{
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
                if (ch_prev == '<') {
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

            uint32_t U = 0;
            next = json::unicode::DecodeUTF8Sequence(next, last, U);
            //JSON_ASSERT(next != f);

            if (U != json::unicode::kInvalidCodepoint)
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
                    constexpr bool inpit = json::strings::impl::IsInputIterator<It>::value;
                    if (inpit)
                    {
                        json::unicode::EncodeUTF8(U, [&](uint8_t code_unit) { yield(static_cast<char>(code_unit)); });
                    }
                    else
                    {
                        // The UTF-8 sequence is valid. No need to re-encode.
                        for ( ; f != next; ++f) {
                            yield(*f);
                        }
                    }
                }
            }
            else
            {
                return {next, EscapeStringStatus::invalid_utf8_sequence};
            }
        }
    }

    return {next, EscapeStringStatus::success};
}

} // namespace strings
} // namespace json
