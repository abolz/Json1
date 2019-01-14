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

#include "charconv_bellerophon.h"
#include "charconv_ryu.h"

#include <climits>
#include <limits>

//==================================================================================================
// NumberToString
//==================================================================================================

namespace json {
namespace impl {

// Returns whether x is an integer and in the range [1, 2^53]
// and stores the integral value of x in result.
// PRE: x > 0
inline bool DoubleToInteger(double x, uint64_t& result)
{
    using Flt = charconv::Double;

    Flt const d(x);

    JSON_ASSERT(d.IsFinite());
    JSON_ASSERT(x > 0);

    auto const F = d.PhysicalSignificand();
    auto const E = d.PhysicalExponent();

    constexpr int p = Flt::SignificandSize;
    // F < 2^p
    // e = E - bias
    // x = (1 + F/2^(p-1)) * 2^e
    //   = (2^(p-1) + F) * 2^(e - (p-1))
    //   = (2^(p-1) + F) * 2^k
    //   = I * 2^k
    //   = I / 2^-k
    // 2^(p-1) <= I < 2^p
    auto const I = Flt::HiddenBit | F;
    auto const k = static_cast<int>(E) - Flt::ExponentBias;

    uint64_t value;
    if (0 <= -k && -k < p)
    {
        // 1 <= x < 2^p

        uint64_t const v = I >> -k;
        if ((v << -k) != I) // fractional part is non-zero, i.e. x is not an integer
            return false;
        value = v;
    }
    else if (k == 1 && F == 0)
    {
        // x = 2^p

        value = uint64_t{1} << p;
    }
    else
    {
        return false;
    }

    result = value;
    return true;
}

inline char* Utoa_2Digits(char* buf, uint32_t digits)
{
    static constexpr char const* kDigits100 =
        "00010203040506070809"
        "10111213141516171819"
        "20212223242526272829"
        "30313233343536373839"
        "40414243444546474849"
        "50515253545556575859"
        "60616263646566676869"
        "70717273747576777879"
        "80818283848586878889"
        "90919293949596979899";

    JSON_ASSERT(digits < 100);
    std::memcpy(buf, &kDigits100[2*digits], 2*sizeof(char));
    return buf + 2;
}

inline char* Utoa_4Digits(char* buf, uint32_t digits)
{
    JSON_ASSERT(digits < 10000);
    uint32_t const q = digits / 100;
    uint32_t const r = digits % 100;
    Utoa_2Digits(buf + 0, q);
    Utoa_2Digits(buf + 2, r);
    return buf + 4;
}

inline char* Utoa_8Digits(char* buf, uint32_t digits)
{
    JSON_ASSERT(digits < 100000000);
    uint32_t const q = digits / 10000;
    uint32_t const r = digits % 10000;
    Utoa_4Digits(buf + 0, q);
    Utoa_4Digits(buf + 4, r);
    return buf + 8;
}

inline int DecimalLength(uint64_t v)
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

inline int PrintDecimalDigits(char* buf, uint64_t output)
{
    int const output_length = DecimalLength(output);
    int i = output_length;

    // We prefer 32-bit operations, even on 64-bit platforms.
    // We have at most 17 digits, and uint32_t can store 9 digits.
    // If output doesn't fit into uint32_t, we cut off 8 digits,
    // so the rest will fit into uint32_t.
    if (static_cast<uint32_t>(output >> 32) != 0)
    {
        JSON_ASSERT(i > 8);
#if CC_32_BIT_PLATFORM
        uint64_t const q = charconv::ryu::Div100_000_000(output);
        uint32_t const r = static_cast<uint32_t>(output - 100000000 * q);
#else
        uint64_t const q = output / 100000000;
        uint32_t const r = static_cast<uint32_t>(output % 100000000);
#endif
        output = q;
        i -= 8;
        Utoa_8Digits(buf + i, r);
    }

    JSON_ASSERT(output <= UINT32_MAX);
    uint32_t output2 = static_cast<uint32_t>(output);

    while (output2 >= 10000)
    {
        JSON_ASSERT(i > 4);
        uint32_t const r = output2 % 10000;
        output2 /= 10000;
        i -= 4;
        Utoa_4Digits(buf + i, r);
    }

    if (output2 >= 100)
    {
        JSON_ASSERT(i > 2);
        uint32_t const r = output2 % 100;
        output2 /= 100;
        i -= 2;
        Utoa_2Digits(buf + i, r);
    }

    if (output2 >= 10)
    {
        JSON_ASSERT(i == 2);
        Utoa_2Digits(buf, output2);
    }
    else
    {
        JSON_ASSERT(i == 1);
        buf[0] = static_cast<char>('0' + output2);
    }

    return output_length;
}

inline char* ExponentToString(char* buffer, int value)
{
    JSON_ASSERT(value > -1000);
    JSON_ASSERT(value <  1000);

    int n = 0;

    if (value < 0)
    {
        buffer[n++] = '-';
        value = -value;
    }
    else
    {
        buffer[n++] = '+';
    }

    uint32_t const k = static_cast<uint32_t>(value);
    if (k < 10)
    {
        buffer[n++] = static_cast<char>('0' + k);
    }
    else if (k < 100)
    {
        Utoa_2Digits(buffer + n, k);
        n += 2;
    }
    else
    {
        uint32_t const r = k % 10;
        uint32_t const q = k / 10;
        Utoa_2Digits(buffer + n, q);
        n += 2;
        buffer[n++] = static_cast<char>('0' + r);
    }

    return buffer + n;
}

inline char* FormatFixed(char* buffer, intptr_t num_digits, intptr_t decimal_point, bool force_trailing_dot_zero)
{
    JSON_ASSERT(buffer != nullptr);
    JSON_ASSERT(num_digits >= 1);

    if (num_digits <= decimal_point)
    {
        // digits[000]
        // JSON_ASSERT(buffer_capacity >= decimal_point + (force_trailing_dot_zero ? 2 : 0));

        std::memset(buffer + num_digits, '0', static_cast<size_t>(decimal_point - num_digits));
        buffer += decimal_point;
        if (force_trailing_dot_zero)
        {
            *buffer++ = '.';
            *buffer++ = '0';
        }
        return buffer;
    }
    else if (0 < decimal_point)
    {
        // dig.its
        // JSON_ASSERT(buffer_capacity >= length + 1);

        std::memmove(buffer + (decimal_point + 1), buffer + decimal_point, static_cast<size_t>(num_digits - decimal_point));
        buffer[decimal_point] = '.';
        return buffer + (num_digits + 1);
    }
    else // decimal_point <= 0
    {
        // 0.[000]digits
        // JSON_ASSERT(buffer_capacity >= 2 + (-decimal_point) + length);

        std::memmove(buffer + (2 + -decimal_point), buffer, static_cast<size_t>(num_digits));
        buffer[0] = '0';
        buffer[1] = '.';
        std::memset(buffer + 2, '0', static_cast<size_t>(-decimal_point));
        return buffer + (2 + (-decimal_point) + num_digits);
    }
}

inline char* FormatScientific(char* buffer, intptr_t num_digits, int exponent, bool force_trailing_dot_zero)
{
    JSON_ASSERT(buffer != nullptr);
    JSON_ASSERT(num_digits >= 1);

    if (num_digits == 1)
    {
        // dE+123
        // JSON_ASSERT(buffer_capacity >= num_digits + 5);

        buffer += 1;
        if (force_trailing_dot_zero)
        {
            *buffer++ = '.';
            *buffer++ = '0';
        }
    }
    else
    {
        // d.igitsE+123
        // JSON_ASSERT(buffer_capacity >= num_digits + 1 + 5);

        std::memmove(buffer + 2, buffer + 1, static_cast<size_t>(num_digits - 1));
        buffer[1] = '.';
        buffer += 1 + num_digits;
    }

    buffer[0] = 'e';
    buffer = ExponentToString(buffer + 1, exponent);

    return buffer;
}

inline char* FormatGeneral(char* buffer, int num_digits, int decimal_exponent, bool force_trailing_dot_zero = false)
{
    int const decimal_point = num_digits + decimal_exponent;
    int const scientific_exponent = decimal_point - 1;

    // C/C++:      [-4,P) (P = 6 if omitted)
    // Java:       [-3,7)
    // JavaScript: [-6,21)
    constexpr int kMinExp = -6;
    constexpr int kMaxExp = 21;

    return (kMinExp <= scientific_exponent && scientific_exponent < kMaxExp)
        ? FormatFixed(buffer, num_digits, decimal_point, force_trailing_dot_zero)
        : FormatScientific(buffer, num_digits, scientific_exponent, /*force_trailing_dot_zero*/ false);
}

} // namespace impl
} // namespace json

namespace json {
namespace numbers {

// Convert the double-precision number `value` to a decimal floating-point
// number.
// The buffer must be large enough! (size >= 32 is sufficient.)
inline char* NumberToString(char* buffer, int buffer_length, double value, bool force_trailing_dot_zero = true)
{
    using Flt = charconv::Double;

    JSON_ASSERT(buffer_length >= 32);
    static_cast<void>(buffer_length);

    Flt const v(value);
    bool const is_neg = v.SignBit();

    if (!v.IsFinite())
    {
        if (v.IsNaN()) {
            std::memcpy(buffer, "NaN", 3);
            return buffer + 3;
        }

        if (is_neg)
            *buffer++ = '-';

        std::memcpy(buffer, "Infinity", 8);
        return buffer + 8;
    }

    // Integers in the range [-2^53, +2^53] are exactly repesentable as 'double'.
    // Print these numbers without a trailing ".0".
    // However, always print -0 as "-0.0" to increase compatibility with other
    // libraries, regardless of the value of 'force_trailing_dot_zero'.

    if (v.IsZero())
    {
        if (is_neg)
        {
            std::memcpy(buffer, "-0.0", 4);
            buffer += 4;
        }
        else
        {
            *buffer++ = '0';
        }

        return buffer;
    }

    if (is_neg)
    {
        value = -value;
        *buffer++ = '-';
    }

    uint64_t digits;
    int decimal_exponent = 0;

    bool const is_int = json::impl::DoubleToInteger(value, digits);
    if (!is_int)
    {
        // value is not an integer in the range [1, 2^53].
        // Use Ryu to convert value to decimal.
        auto const res = charconv::ryu::DoubleToDecimal(value);

        digits = res.digits;
        decimal_exponent = res.exponent;
    }

    // Convert the digits to decimal.
    int const num_digits = json::impl::PrintDecimalDigits(buffer, digits);

    if (!is_int)
    {
        // Reformat the buffer similar to printf("%g").
        return json::impl::FormatGeneral(buffer, num_digits, decimal_exponent, force_trailing_dot_zero);
    }
    else
    {
        // Done.
        // Never append a trailing ".0" in this case.
        return buffer + num_digits;
    }
}

} // namespace numbers
} // namespace json

//==================================================================================================
// StringToNumber
//==================================================================================================

namespace json {
namespace impl {

// Inputs larger than kMaxInt (currently) can not be handled.
// To avoid overflow in integer arithmetic.
constexpr int const kMaxStringToDoubleLen = 99999999; // < INT_MAX / 4

inline double InternalStringToDouble(char const* next, char const* last, NumberClass nc)
{
    using namespace charconv::bellerophon;

    char        buffer[kDoubleMaxSignificantDigits];
    char const* digits     = nullptr;
    int         num_digits = 0;
    int         exponent   = 0;
    bool        zero_tail  = true;

    JSON_ASSERT(next != last);
    JSON_ASSERT(last - next <= kMaxStringToDoubleLen);
    JSON_ASSERT(IsDigit(*next));

    if (nc == NumberClass::integer)
    {
        // No need to copy the digits into a temporary buffer.
        // DigitsToDouble will trim the input to kMaxSignificantDigits.

        digits     = next;
        num_digits = static_cast<int>(last - next);

        // Use a faster method for integers which will fit into a uint64_t.
        // Let static_cast do the conversion.
        if (num_digits <= 20)
        {
            uint64_t const u = ReadInt<uint64_t>(next, num_digits <= 19 ? num_digits : 19);

            if (num_digits <= 19)
            {
                return static_cast<double>(u);
            }

            if (u <= UINT64_MAX / 10)
            {
                uint32_t const d = static_cast<uint32_t>(next[19] - '0');
                if (d <= UINT64_MAX - 10 * u)
                {
                    return static_cast<double>(10 * u + d);
                }
            }
        }
    }
    else
    {
        //
        // TODO:
        // - Avoid a copy if the number is of the form DDD[.000]e+nnn?
        // - Use memcpy if the number is of the form DDD.DDD[e+nnn]
        //   and the length excluding the exponent-part fits into kMaxSignificantDigits?
        // - Rewrite DigitsToDouble to allow a decimal separator?
        //

        digits     = buffer;
        num_digits = 0;

        if (*next == '0')
        {
            // Number is of the form 0[.xxx][e+nnn].
            // The leading zero here is not a significant digit.
            ++next;
        }
        else
        {
            for (;;)
            {
                if (num_digits < kDoubleMaxSignificantDigits)
                {
                    buffer[num_digits++] = *next;
                }
                else
                {
                    ++exponent;
                    zero_tail &= *next == '0';
                }
                ++next;
                JSON_ASSERT(next != last);
                if (!IsDigit(*next))
                    break;
            }
        }

        JSON_ASSERT(next != last);
        JSON_ASSERT(*next == '.' || *next == 'e' || *next == 'E');

        if (*next == '.')
        {
            ++next;
            JSON_ASSERT(next != last);
            JSON_ASSERT(IsDigit(*next));

            if (num_digits == 0)
            {
                // Number is of the form 0.xxx[e+nnn].
                // Skip leading zeros in the fractional part and adjust the exponent.
                while (*next == '0')
                {
                    --exponent;
                    ++next;
                    if (next == last)
                        return 0.0;
                }
            }

            JSON_ASSERT(next != last);

            while (IsDigit(*next))
            {
                if (num_digits < kDoubleMaxSignificantDigits)
                {
                    buffer[num_digits++] = *next;
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
            if (num_digits == 0)
            {
                // Number is of the form 0[.000]e+nnn.
                return 0.0;
            }

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
            while (*next == '0' && ++next != last)
            {
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
                return exp_is_neg
                    ? 0.0
                    : std::numeric_limits<double>::infinity();
            }
        }
    }

    return DigitsToDouble(digits, num_digits, exponent, !zero_tail);
}

} // namespace impl
} // namespace json

namespace json {
namespace numbers {

// Convert the string `[first, last)` to a double-precision value.
// The string must be valid according to the JSON grammar and match the number
// class defined by `nc` (which must not be `NumberClass::invalid`).
inline double StringToNumber(char const* next, char const* last, NumberClass nc)
{
    if (next == last)
        return 0.0;

    if (last - next > json::impl::kMaxStringToDoubleLen)
        return std::numeric_limits<double>::quiet_NaN();

    switch (nc) {
    case NumberClass::invalid:
        return std::numeric_limits<double>::quiet_NaN();
    case NumberClass::nan:
        return std::numeric_limits<double>::quiet_NaN();
    case NumberClass::pos_infinity:
        return +std::numeric_limits<double>::infinity();
    case NumberClass::neg_infinity:
        return -std::numeric_limits<double>::infinity();
    default:
        break;
    }

    bool const is_neg = (*next == '-');
    if (is_neg)
    {
        ++next;
    }

    double const value = json::impl::InternalStringToDouble(next, last, nc);
    return is_neg ? -value : value;
}

// Convert the string `[next, last)` to a double-precision value.
// Returns true if the string is a valid number according to the JSON grammar.
// Otherwise returns false and stores 'NaN' in `result`.
inline bool StringToNumber(double& result, char const* next, char const* last)
{
    if (next == last)
    {
        result = 0.0;
        return true;
    }

    auto const res = json::ScanNumber(next, last);

    if (res.next == last && res.number_class != NumberClass::invalid)
    {
        result = json::numbers::StringToNumber(next, last, res.number_class);
        return true;
    }

    result = std::numeric_limits<double>::quiet_NaN();
    return false;
}

} // namespace numbers
} // namespace json
