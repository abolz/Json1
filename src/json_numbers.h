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

// If non-zero, use the Grisu2 algorithm for converting binary to decimal
// floating-point numbers, otherwise use the Ryu algorithm.
// The results obtained from Grisu2 are not always optimal, but always correct.
// Additionally this uses only ~2KB of tables (instead of ~16KB for Ryu).
#define JSON_NUMBERS_USE_GRISU2 0

#include "json_parser.h" // NumberClass, Options

#include <climits>
#include <limits>

#if JSON_NUMBERS_USE_GRISU2
#include "__charconv_grisu2.h"
#else
#include "__charconv_ryu.h"
#endif

//==================================================================================================
// NumberToString
//==================================================================================================

#if JSON_NUMBERS_USE_GRISU2
namespace charconv_internal {

inline int DecimalLengthDouble(uint64_t v)
{
    CC_ASSERT(v < 100000000000000000ull);

    if (v >= 10000000000000000ull) { return 17; }
    if (v >= 1000000000000000ull) { return 16; }
    if (v >= 100000000000000ull) { return 15; }
    if (v >= 10000000000000ull) { return 14; }
    if (v >= 1000000000000ull) { return 13; }
    if (v >= 100000000000ull) { return 12; }
    if (v >= 10000000000ull) { return 11; }
    if (v >= 1000000000ull) { return 10; }
    if (v >= 100000000ull) { return 9; }
    if (v >= 10000000ull) { return 8; }
    if (v >= 1000000ull) { return 7; }
    if (v >= 100000ull) { return 6; }
    if (v >= 10000ull) { return 5; }
    if (v >= 1000ull) { return 4; }
    if (v >= 100ull) { return 3; }
    if (v >= 10ull) { return 2; }
    return 1;
}

inline int PrintDecimalDigitsDouble(char* buf, uint64_t output)
{
    int const output_length = DecimalLengthDouble(output);
    int i = output_length;

    // We prefer 32-bit operations, even on 64-bit platforms.
    // We have at most 17 digits, and uint32_t can store 9 digits.
    // If output doesn't fit into uint32_t, we cut off 8 digits,
    // so the rest will fit into uint32_t.
    if (static_cast<uint32_t>(output >> 32) != 0)
    {
        CC_ASSERT(i > 8);
        uint64_t const q = output / 100000000;
        uint32_t const r = static_cast<uint32_t>(output % 100000000);
        output = q;
        i -= 8;
        Utoa_8Digits(buf + i, r);
    }

    CC_ASSERT(output <= UINT32_MAX);
    uint32_t output2 = static_cast<uint32_t>(output);

    while (output2 >= 10000)
    {
        CC_ASSERT(i > 4);
        uint32_t const r = output2 % 10000;
        output2 /= 10000;
        i -= 4;
        Utoa_4Digits(buf + i, r);
    }

    if (output2 >= 100)
    {
        CC_ASSERT(i > 2);
        uint32_t const r = output2 % 100;
        output2 /= 100;
        i -= 2;
        Utoa_2Digits(buf + i, r);
    }

    if (output2 >= 10)
    {
        CC_ASSERT(i == 2);
        Utoa_2Digits(buf, output2);
    }
    else
    {
        CC_ASSERT(i == 1);
        buf[0] = static_cast<char>('0' + output2);
    }

    return output_length;
}

} // namespace charconv_internal
#endif

namespace json {
namespace numbers {

// Convert the double-precision number `value` to a decimal floating-point
// number.
// The buffer must be large enough! (size >= 32 is sufficient.)
inline char* NumberToString(char* buffer, int buffer_length, double value, bool force_trailing_dot_zero = true)
{
    JSON_ASSERT(buffer_length >= 32);
    static_cast<void>(buffer_length);

    // Integers in the range [-2^53, +2^53] are exactly repesentable as 'double'.
    // Print these numbers without a trailing ".0".
    // However, always print -0 as "-0.0" to increase compatibility with other
    // libraries, regardless of the value of 'force_trailing_dot_zero'.

    constexpr double kMinInteger = -9007199254740992.0; // -2^53
    constexpr double kMaxInteger =  9007199254740992.0; //  2^53

    charconv_internal::Double const v(value);

    if (!v.IsFinite())
    {
        if (v.IsNaN()) {
            std::memcpy(buffer, "NaN", 3);
            return buffer + 3;
        }

        if (v.SignBit())
            *buffer = '-';

        std::memcpy(buffer, "Infinity", 8);
        return buffer + 8;
    }

    if (v.IsZero())
    {
        if (v.SignBit())
        {
            *buffer++ = '-';
            *buffer++ = '0';
            *buffer++ = '.';
            *buffer++ = '0';
        }
        else
        {
            *buffer++ = '0';
        }

        return buffer;
    }

    if (kMinInteger <= value && value <= kMaxInteger)
    {
        int64_t i = static_cast<int64_t>(value);
        if (static_cast<double>(i) == value)
        {
            if (i < 0)
            {
                *buffer++ = '-';
                i = -i;
            }

            // Reuse PrintDecimalDigits.
            // This routine assumes that 'i' has at most 17 decimal digits.
            // We only get here if 'i' has at most 16 decimal digits.
            return buffer + charconv_internal::PrintDecimalDigitsDouble(buffer, static_cast<uint64_t>(i));
        }
    }

    if (v.SignBit())
    {
        value = v.AbsValue();
        *buffer++ = '-';
    }

    return charconv_internal::PositiveDoubleToString(buffer, value, force_trailing_dot_zero);
}

} // namespace numbers
} // namespace json

//==================================================================================================
// StringToNumber
//==================================================================================================

namespace charconv_internal {

// PRE: NumberClass = floating_point
inline double InternalStrtod(char const* next, char const* last)
{
    using json::charclass::IsDigit;

    // Inputs larger than kMaxInt (currently) can not be handled.
    // To avoid overflow in integer arithmetic.
    constexpr int const kMaxInt = 99999999; // < INT_MAX / 4

    char digits[kMaxSignificantDigits];
    int  num_digits   = 0;
    int  exponent     = 0;
    bool zero_tail    = true;

    if (last - next > kMaxInt)
        return std::numeric_limits<double>::quiet_NaN();

    if (*next == '0')
    {
        ++next;
        if (next == last)
            return 0;

        JSON_ASSERT(!IsDigit(*next));
    }
    else if (IsDigit(*next))
    {
        for (;;)
        {
            if (num_digits < kMaxSignificantDigits)
            {
                digits[num_digits++] = *next;
            }
            else
            {
                ++exponent;
                zero_tail &= *next == '0';
            }
            ++next;
            if (next == last)
                goto L_convert;
            if (!IsDigit(*next))
                break;
        }
    }
    //else if (*next == '.')
    //{
    //}
    else
    {
        JSON_ASSERT(false);
    }

    if (*next == '.')
    {
        ++next;
        JSON_ASSERT(next != last);
        JSON_ASSERT(IsDigit(*next));

        if (num_digits == 0)
        {
            while (*next == '0')
            {
                --exponent;
                ++next;
                if (next == last)
                    return 0;
            }
        }

        JSON_ASSERT(next != last);

        while (IsDigit(*next))
        {
            if (num_digits < kMaxSignificantDigits)
            {
                digits[num_digits++] = *next;
                --exponent;
            }
            else
            {
                zero_tail &= *next == '0';
            }
            ++next;
            if (next == last)
                goto L_convert;
        }
    }

    if (*next == 'e' || *next == 'E')
    {
        ++next;
        JSON_ASSERT(next != last);

        bool const exp_is_neg = (*next == '-');
        if (exp_is_neg || *next == '+')
        {
            ++next;
            JSON_ASSERT(next != last);
        }

        int const max_exp_len = static_cast<int>(last - next);
        int const exp_val = ReadInt<int>(next, max_exp_len < 8 ? max_exp_len : 8);

        exponent += exp_is_neg ? -exp_val : exp_val;
    }

L_convert:
    return DigitsToDouble(digits, num_digits, exponent, !zero_tail);
}

} // namespace charconv_internal

namespace json {
namespace numbers {

// Convert the string `[first, last)` to a double-precision value.
// The string must be valid according to the JSON grammar and match the number
// class defined by `nc` (which must not be `NumberClass::invalid`).
inline double StringToNumber(char const* first, char const* last, NumberClass nc)
{
    JSON_ASSERT(first != last);
    JSON_ASSERT(nc != NumberClass::invalid);

    if (first == last) {
        return 0.0;
    }

    switch (nc) {
    case NumberClass::invalid:
    case NumberClass::nan:
        return std::numeric_limits<double>::quiet_NaN();
    case NumberClass::pos_infinity:
        return +std::numeric_limits<double>::infinity();
    case NumberClass::neg_infinity:
        return -std::numeric_limits<double>::infinity();
    default:
        break;
    }

    bool const is_neg = *first == '-';
    if (is_neg)
    {
        ++first;
    }
    //else if (*first == '+')
    //{
    //    ++first;
    //}

    // Use a faster method for integers which will fit into a uint64_t.
    // Let static_cast do the conversion.
    if (nc == NumberClass::integer && last - first <= 20)
    {
        int const num_digits = static_cast<int>(last - first);
        int const max_digits = num_digits <= 19 ? num_digits : 19;

        uint64_t u = charconv_internal::ReadInt<uint64_t>(first, max_digits);
        if (num_digits == 20)
        {
            uint32_t const digit = static_cast<uint32_t>(first[19] - '0');
            if (u > UINT64_MAX / 10 || digit > UINT64_MAX - 10 * u)
            {
                goto L_use_strtod;
            }

            u = 10 * u + digit;
        }

        double const value = static_cast<double>(u);
        return is_neg ? -value : value;
    }

L_use_strtod:
    double const value = charconv_internal::InternalStrtod(first, last);
    return is_neg ? -value : value;
}

// Convert the string `[first, last)` to a double-precision value.
// Returns true if the string is a valid number according to the JSON grammar.
// Otherwise returns false and stores 'NaN' in `result`.
inline bool StringToNumber(double& result, char const* first, char const* last, Options const& options = {})
{
    if (first == last)
    {
        result = 0.0;
        return true;
    }

    auto const res = json::ScanNumber(first, last, options);

    if (res.next == last && res.number_class != NumberClass::invalid)
    {
        result = json::numbers::StringToNumber(first, last, res.number_class);
        return true;
    }

    result = std::numeric_limits<double>::quiet_NaN();
    return false;
}

} // namespace numbers
} // namespace json
