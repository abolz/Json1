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

#include "json_parser.h" // NumberClass, Options

#include <climits>
#include <limits>

#include "__charconv_bellerophon.h"
#include "__charconv_ryu.h"

//==================================================================================================
// NumberToString
//==================================================================================================

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

    charconv_ryu::Double const v(value);

    if (!v.IsFinite())
    {
        if (v.IsNaN()) {
            std::memcpy(buffer, "NaN", 3);
            return buffer + 3;
        }

        if (v.SignBit())
            *buffer++ = '-';

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

            uint64_t const digits = static_cast<uint64_t>(i);

            // Reuse PrintDecimalDigits.
            // This routine assumes that 'i' has at most 17 decimal digits.
            // We only get here if 'i' has at most 16 decimal digits.
            int const num_digits = charconv_ryu::DecimalLengthDouble(digits);
            return buffer + charconv_ryu::PrintDecimalDigitsDouble(buffer, digits, num_digits);
        }
    }

    if (v.SignBit())
    {
        value = v.AbsValue();
        *buffer++ = '-';
    }

    return charconv_ryu::PositiveDoubleToString(buffer, value, force_trailing_dot_zero);
}

} // namespace numbers
} // namespace json

//==================================================================================================
// StringToNumber
//==================================================================================================

namespace charconv_bellerophon {

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

    JSON_ASSERT(next != last);
    JSON_ASSERT(IsDigit(*next));

    if (last - next > kMaxInt)
        return std::numeric_limits<double>::quiet_NaN();

    if (*next == '0')
    {
        ++next;
        if (next == last)
            return 0;

        JSON_ASSERT(!IsDigit(*next));
    }
    else
    {
        for (;;)
        {
            if (num_digits < kMaxSignificantDigits)
            {
                digits[num_digits] = *next;
                ++num_digits;
            }
            else
            {
                ++exponent;
                zero_tail &= *next == '0';
            }
            ++next;
            if (next == last)
                break;
            if (!IsDigit(*next))
                break;
        }
    }

    if (next != last && *next == '.')
    {
        ++next;
        JSON_ASSERT(next != last);
        JSON_ASSERT(IsDigit(*next));

        if (num_digits == 0)
        {
            // 0.xxx
            // Skip leading zeros in the fractional part and adjust the exponent.
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
                digits[num_digits] = *next;
                ++num_digits;
                --exponent;
            }
            else
            {
                zero_tail &= *next == '0';
            }
            ++next;
            if (next == last)
                break;
        }
    }

    if (next != last && (*next == 'e' || *next == 'E'))
    {
        ++next;
        JSON_ASSERT(next != last);

        bool const exp_is_neg = (*next == '-');
        if (exp_is_neg || *next == '+')
        {
            ++next;
            JSON_ASSERT(next != last);
        }

        // Skip leading zeros.
        // The exponent is always decimal, not octal.
        while (next != last && *next == '0')
        {
            ++next;
        }

        if (last - next <= 8)
        {
            // NB:
            // ReadInt returns 0 for empty inputs.
            int const e = ReadInt<int>(next, static_cast<int>(last - next));
            exponent += exp_is_neg ? -e : e;
        }
        else
        {
            // Exponents >= 10^8 are considered to be +Infinity.
            // This is slightly incorrect (since the computed exponent and the
            // parsed exponent might cancel each other out), but still correct
            // for sane inputs.
            return exp_is_neg || num_digits == 0
                ? 0.0
                : std::numeric_limits<double>::infinity();
        }
    }

    return DigitsToDouble(digits, num_digits, exponent, !zero_tail);
}

} // namespace charconv_bellerophon

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

        uint64_t u = charconv_bellerophon::ReadInt<uint64_t>(first, max_digits);
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
    double const value = charconv_bellerophon::InternalStrtod(first, last);
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
