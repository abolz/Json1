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

#ifndef JSON_NEVER_INLINE
#if defined(__GNUC__)
#define JSON_NEVER_INLINE __attribute__((noinline)) inline
#elif defined(_MSC_VER)
#define JSON_NEVER_INLINE __declspec(noinline) inline
#else
#define JSON_NEVER_INLINE inline
#endif
#endif

#ifndef JSON_SSE42
#if defined(__SSE4_2__) || defined(__AVX__) || defined(__AVX2__)
#define JSON_SSE42 1
#endif
#endif

#if JSON_SSE42
#if _MSC_VER
#include <intrin.h>
#endif
#include <nmmintrin.h>
#endif

namespace json {

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

struct ScanNumberResult {
    char const* next;
    NumberClass number_class;
};

//template <
//    typename OnSign,
//    typename OnSignificandDigit,
//    typename OnExponentSign,
//    typename OnExponentDigit
//>
inline ScanNumberResult ScanNumber(char const* next, char const* last)
{
    auto IsDigit = [](char ch) {
        return '0' <= ch && ch <= '9';
    };

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
    incomplete_string,
    number,
    identifier,
    comment,
    incomplete_comment,
};

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

struct ParseResult {
    char const* ptr;
    ParseStatus ec = ParseStatus::unknown;
};

} // namespace json
