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

#include "charconv/bellerophon.h"
#include "charconv/ryu.h"

#include <climits>
#include <limits>

//==================================================================================================
// Number conversions
//==================================================================================================

namespace json {
namespace numbers {

inline uint32_t ToUint32(double value)
{
    //
    // https://tc39.github.io/ecma262/#sec-touint32
    //
    // The abstract operation ToUint32 converts argument to one of 2^32 integer values in the range 0 through 2^32 - 1, inclusive.
    // This abstract operation functions as follows:
    //
    //  1.  Let number be ? ToNumber(argument).
    //  2.  If number is NaN, +0, -0, +Infinity, or -Infinity, return +0.
    //  3.  Let int be the mathematical value that is the same sign as number and whose magnitude is floor(abs(number)).
    //  4.  Let int32bit be int modulo 2^32.
    //  5.  Return int32bit.
    //

    using Double = charconv::Double;
    Double const d(value);

    uint64_t const F = d.PhysicalSignificand();
    uint64_t const E = d.PhysicalExponent();

    // Assume that x is a normalized floating-point number.
    // The special cases subnormal/zero and nan/inf are actually handled below
    // in the branches 'exponent <= -p' and 'exponent >= 32'.

    auto const significand = Double::HiddenBit | F;
    auto const exponent = static_cast<int>(E) - Double::ExponentBias;

    uint32_t bits32; // = floor(abs(x)) mod 2^32
    if (exponent <= -Double::SignificandSize)
    {
        // x = significand / 2^-exponent < 1.
        return 0;
    }
    else if (exponent < 0)
    {
        // x = significand / 2^-exponent.
        // Discard the fractional bits (floor).
        // Since 0 < -exponent < 53, the (64-bit) right shift is well defined.
        bits32 = static_cast<uint32_t>(significand >> -exponent);
    }
    else if (exponent < 32)
    {
        // x = significand * 2^exponent.
        // Since 0 <= exponent < 32, the (32-bit) left shift is well defined.
        bits32 = static_cast<uint32_t>(significand) << exponent;
    }
    else
    {
        // x = significand * 2^exponent.
        // The lower 32 bits of x are definitely 0.
        return 0;
    }

    return d.SignBit() ? 0 - bits32 : bits32;
}

// TODO:
// Technically, casting unsigned to signed integers is implementation-defined (if out of range).

inline int32_t ToInt32(double value)
{
    return static_cast<int32_t>(ToUint32(value));
}

inline uint16_t ToUint16(double value)
{
    return static_cast<uint16_t>(ToUint32(value));
}

inline int16_t ToInt16(double value)
{
    return static_cast<int16_t>(ToUint16(value));
}

inline uint8_t ToUint8(double value)
{
    return static_cast<uint8_t>(ToUint32(value));
}

inline int8_t ToInt8(double value)
{
    return static_cast<int8_t>(ToUint8(value));
}

inline uint8_t ToUint8Clamp(double value)
{
    //
    // https://tc39.github.io/ecma262/#sec-touint8clamp
    //
    // The abstract operation ToUint8Clamp converts argument to one of 2^8 integer values in the
    // range 0 through 255, inclusive.
    // This abstract operation functions as follows:
    //
    //  1.  Let number be ? ToNumber(argument).
    //  2.  If number is NaN, return +0.
    //  3.  If number <= 0, return +0.
    //  4.  If number >= 255, return 255.
    //  5.  Let f be floor(number).
    //  6.  If f + 0.5 < number, return f + 1.
    //  7.  If number < f + 0.5, return f.
    //  8.  If f is odd, return f + 1.
    //  9.  Return f.
    //

#if 1
    // The following test is equivalent to (std::isnan(value) || value <= 0.5),
    // i.e. it catches NaN's and subnormal numbers.
    if (!(value > 0.5))
        return 0;
    if (value > 254.5)
        return 255;

    // Truncate to integer.
    int32_t t = static_cast<int32_t>(value);

    // Then do the rounding.
    // Note: (value - t) is exact here (as would be (t + 0.5)).
    double const fraction = value - t;

    // Round towards +Inf
    t += (fraction >= 0.5);
    if (fraction == 0.5)
    {
        // Round to nearest-even
        t &= ~1;
    }

    JSON_ASSERT(t >= 0);
    JSON_ASSERT(t <= 255);
    return static_cast<uint8_t>(static_cast<uint32_t>(t));
#else
    using Double = charconv::Double;
    Double const d(value);

    if (d.bits <= 0x3FE0000000000000ull) // +0.0 <= x <= 1/2
    {
        return 0;
    }
    else if (d.bits <= 0x406FD00000000000ull) // 0.5 < x <= 254.5
    {
        // x = significand * 2^exponent is a normalized floating-point number.
        uint64_t const F = d.PhysicalSignificand();
        uint64_t const E = d.PhysicalExponent();

        auto const significand = Double::HiddenBit | F;
        auto const exponent = static_cast<int>(E) - Double::ExponentBias;
        JSON_ASSERT(exponent >= -1 - (Double::SignificandSize - 1));
        JSON_ASSERT(exponent <   8 - (Double::SignificandSize - 1));
        // 44 < -exponent <= 53

        uint64_t const high = uint64_t{1} << -exponent;
        uint64_t const mask = high - 1;
        uint64_t const half = high >> 1;
        uint64_t const fraction = significand & mask;

        uint32_t t = static_cast<uint32_t>(significand >> -exponent);

        // Round towards +Inf
        t += (fraction >= half);
        if (fraction == half)
        {
            // Round towards nearest-even.
            t &= ~1u;
        }

        JSON_ASSERT(t <= 255);
        return static_cast<uint8_t>(t);
    }
    else if (d.bits <= 0x7FF0000000000000ull) // 254.5 < x <= +Inf
    {
        return 255;
    }
    else // x <= -0.0 or NaN(x)
    {
        return 0;
    }
#endif
}

} // namespace numbers
} // namespace json

//==================================================================================================
// NumberToString
//==================================================================================================

namespace json {
namespace impl {

// Returns whether x is an integer and in the range [1, 2^53]
// and stores the integral value of x in result.
inline uint64_t DoubleToSmallInt(double x, bool& is_small_int)
{
#if 1
    if (1.0 <= x && x <= 9007199254740992.0) // 1.0 <= x <= 2^53
    {
        int64_t const i = static_cast<int64_t>(x);
        is_small_int = (x == static_cast<double>(i));
        return static_cast<uint64_t>(i);
    }

    is_small_int = false;
    return 0;
#else
    using charconv::Double;
    Double const d(x);

    if (0x3FF0000000000000ull <= d.bits && d.bits < 0x4340000000000000ull) // 1 <= x < 2^53
    {
        // x = significand * 2^exponent is a normalized floating-point number.
        uint64_t const F = d.PhysicalSignificand();
        uint64_t const E = d.PhysicalExponent();

        auto const significand = Double::HiddenBit | F;
        auto const exponent = static_cast<int>(E) - Double::ExponentBias;
        JSON_ASSERT(-exponent >= 0);
        JSON_ASSERT(-exponent < Double::SignificandSize);

        // Test whether the lower -exponent bits are 0, i.e.
        // whether the fractional part of x is 0.
        uint64_t const mask = (uint64_t{1} << -exponent) - 1;
        uint64_t const fraction = significand & mask;

        is_small_int = (fraction == 0);
        return significand >> -exponent;
    }
    else
    {
        // Test whether x == 2^53.
        // TODO: Merge with the branch above? (significand*2, exponent-1)?
        is_small_int = (d.bits == 0x4340000000000000ull);
        return uint64_t{1} << Double::SignificandSize;
    }
#endif
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
        uint64_t const q = charconv::ryu::Div1e8(output);
        uint32_t const r = charconv::ryu::Mod1e8(output, q);
        output = q;
        i -= 8;
        Utoa_8Digits(buf + i, r);
    }

    JSON_ASSERT(output <= UINT32_MAX);
    uint32_t output2 = static_cast<uint32_t>(output);

    while (output2 >= 10000)
    {
        JSON_ASSERT(i > 4);
        uint32_t const q = output2 / 10000;
        uint32_t const r = output2 % 10000;
        output2 = q;
        i -= 4;
        Utoa_4Digits(buf + i, r);
    }

    if (output2 >= 100)
    {
        JSON_ASSERT(i > 2);
        uint32_t const q = output2 / 100;
        uint32_t const r = output2 % 100;
        output2 = q;
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

} // namespace impl
} // namespace json

namespace json {
namespace numbers {

// Convert the double-precision number `value` to a decimal floating-point
// number.
// The buffer must be large enough! (size >= 32 is sufficient.)
inline char* NumberToString(char* buffer, int buffer_length, double value, bool force_trailing_dot_zero = true)
{
    JSON_ASSERT(buffer_length >= 32);
    static_cast<void>(buffer_length);

    using Double = charconv::Double;
    Double const v(value);

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

    bool is_small_int;
    uint64_t digits = json::impl::DoubleToSmallInt(value, is_small_int);

    int decimal_exponent = 0;
    if (is_small_int)
    {
        // value is an integer in the range [1, 2^53].
        // Just print the decimal digits (below).
    }
    else
    {
        // value is not an integer in the range [1, 2^53].
        // Use Ryu to convert value to decimal.
        auto const res = charconv::ryu::DoubleToDecimal(value);

        digits = res.digits;
        decimal_exponent = res.exponent;
    }

    // Convert the digits to decimal.
    int const num_digits = json::impl::PrintDecimalDigits(buffer, digits);

    if (is_small_int)
    {
        // Done.
        // Never append a trailing ".0" in this case.
        return buffer + num_digits;
    }
    else
    {
        // Print values in the range [10^-6, 10^21) in fixed-point notation.
        // All other values will be printed in scientific notation.
        // This is what JavaScript does and it is consistent with the integer formatting above (2^53 < 10^17).

        int const decimal_point = num_digits + decimal_exponent;
        int const scientific_exponent = decimal_point - 1;

        if (-6 <= scientific_exponent && scientific_exponent < 21)
            return json::impl::FormatFixed(buffer, num_digits, decimal_point, force_trailing_dot_zero);
        else
            return json::impl::FormatScientific(buffer, num_digits, scientific_exponent, /*force_trailing_dot_zero*/ false);
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

        digits = next;
        num_digits = static_cast<int>(last - next);

        // Integers with at most 15 decimal digits are exactly representable as 'double'.
        // We could read up to 19 (or 20) decimal digits into an uint64_t and let static_cast
        // do the conversion, but then rounding would be implementation-defined.
        if (num_digits <= 15)
        {
            return static_cast<double>(ReadInt<int64_t>(next, num_digits));
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

        digits = buffer;
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
                if /*very very likely*/ (num_digits < kDoubleMaxSignificantDigits)
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
                if /*very very likely*/ (num_digits < kDoubleMaxSignificantDigits)
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

            if /*very likely*/ (last - next <= 8)
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
