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

#include "json_numbers.h"

#include "json_charclass.h"

#include "dtoa.h"
#include "strtod.h"

#include <cassert>
#include <climits>
#include <cmath>
#include <cstring>
#include <limits>
#include <type_traits>

#ifndef JSON_ASSERT
#define JSON_ASSERT(X) assert(X)
#endif

using namespace json;
using namespace json::numbers;

//==================================================================================================
// NumberToString
//==================================================================================================

static char* U32ToString(char* buf, uint32_t n)
{
    using base_conv::dtoa_impl::Utoa100;

    uint32_t q;

    if (n >= 1000000000)
    {
//L_10_digits:
        q = n / 100000000;
        n = n % 100000000;
        buf = Utoa100(buf, q);
L_8_digits:
        q = n / 1000000;
        n = n % 1000000;
        buf = Utoa100(buf, q);
L_6_digits:
        q = n / 10000;
        n = n % 10000;
        buf = Utoa100(buf, q);
L_4_digits:
        q = n / 100;
        n = n % 100;
        buf = Utoa100(buf, q);
L_2_digits:
        buf = Utoa100(buf, n);
        return buf;
    }

    if (n >= 100000000)
    {
//L_9_digits:
        q = n / 10000000;
        n = n % 10000000;
        buf = Utoa100(buf, q);
L_7_digits:
        q = n / 100000;
        n = n % 100000;
        buf = Utoa100(buf, q);
L_5_digits:
        q = n / 1000;
        n = n % 1000;
        buf = Utoa100(buf, q);
L_3_digits:
        q = n / 10;
        n = n % 10;
        buf = Utoa100(buf, q);
L_1_digit:
        buf[0] = static_cast<char>('0' + n);
        return buf + 1;
    }

    if (n < 100)
    {
        if (n >= 10)
            goto L_2_digits;
        else
            goto L_1_digit;
    }
    else if (n < 10000)
    {
        if (n >= 1000)
            goto L_4_digits;
        else
            goto L_3_digits;
    }
    else if (n < 1000000)
    {
        if (n >= 100000)
            goto L_6_digits;
        else
            goto L_5_digits;
    }
    else
    {
        if (n >= 10000000)
            goto L_8_digits;
        else
            goto L_7_digits;
    }
}

static char* U64ToString(char* buf, uint64_t n)
{
    using base_conv::dtoa_impl::Utoa100;

    auto Utoa_9digits = [](char* ptr, uint32_t k)
    {
        uint32_t q;

//L_9_digits:
        q = k / 10000000;
        k = k % 10000000;
        ptr = Utoa100(ptr, q);
//L_7_digits:
        q = k / 100000;
        k = k % 100000;
        ptr = Utoa100(ptr, q);
//L_5_digits:
        q = k / 1000;
        k = k % 1000;
        ptr = Utoa100(ptr, q);
//L_3_digits:
        q = k / 10;
        k = k % 10;
        ptr = Utoa100(ptr, q);
//L_1_digits:
        ptr[0] = static_cast<char>('0' + k);
        return ptr + 1;
    };

    if (n <= UINT32_MAX)
        return U32ToString(buf, static_cast<uint32_t>(n));

    // n = hi * 10^9 + lo < 10^20,   where hi < 10^11, lo < 10^9
    uint64_t const hi = n / 1000000000;
    uint64_t const lo = n % 1000000000;

    if (hi <= UINT32_MAX)
    {
        buf = U32ToString(buf, static_cast<uint32_t>(hi));
    }
    else
    {
        // 2^32 < hi = hi1 * 10^9 + hi0 < 10^11,   where hi1 < 10^2, 10^9 <= hi0 < 10^9
        uint32_t const hi1 = static_cast<uint32_t>(hi / 1000000000);
        uint32_t const hi0 = static_cast<uint32_t>(hi % 1000000000);
        if (hi1 >= 10)
        {
            buf = Utoa100(buf, hi1);
        }
        else
        {
            DTOA_ASSERT(hi1 != 0);
            buf[0] = static_cast<char>('0' + hi1);
            buf++;
        }
        buf = Utoa_9digits(buf, hi0);
    }

    // lo has exactly 9 digits.
    // (Which might all be zero...)
    return Utoa_9digits(buf, static_cast<uint32_t>(lo));
}

static char* I64ToString(char* buf, int64_t i)
{
    uint64_t n = static_cast<uint64_t>(i);
    if (i < 0)
    {
        buf[0] = '-';
        buf++;
        n = 0u - n;
    }

    return U64ToString(buf, n);
}

char* json::numbers::NumberToString(char* next, char* last, double value, bool emit_trailing_dot_zero)
{
#if 1
    constexpr double kMinInteger = -9007199254740992.0; // -2^53
    constexpr double kMaxInteger =  9007199254740992.0; //  2^53

    // Print integers in the range [-2^53, +2^53] as integers (without a trailing ".0").
    // These integers are exactly representable as 'double's.
    // However, always print -0 as "-0.0" to increase compatibility with other libraries
    //
    // NB:
    // These tests for work correctly for NaN's and Infinity's (i.e. the DoubleToString branch is taken).
#if 1
    if (value == 0)
    {
        if (std::signbit(value))
        {
            std::memcpy(next, "-0.0", 4);
            return next + 4;
        }
        else
        {
            next[0] = '0';
            return next + 1;
        }
    }
    else if (kMinInteger <= value && value <= kMaxInteger)
    {
        const int64_t i = static_cast<int64_t>(value);
        if (static_cast<double>(i) == value)
        {
            return I64ToString(next, i);
        }
    }
#else
    if (emit_trailing_dot_zero && (kMinInteger <= value && value <= kMaxInteger))
    {
        emit_trailing_dot_zero = false;
    }
#endif
#endif

    return base_conv::Dtoa(next, last, value, emit_trailing_dot_zero, "NaN", "Infinity");
}

//==================================================================================================
// StringToNumber
//==================================================================================================

static bool StringToDouble(double& result, char const* next, char const* last, Options const& options)
{
#if 0
    if (next == last)
    {
        result = 0.0;
        return true;
    }

    return base_conv::Strtod(result, next, last) == base_conv::StrtodStatus::success;
#else
    using namespace json::charclass;

    // Inputs larger than kMaxInt (currently) can not be handled.
    // To avoid overflow in integer arithmetic.
    static constexpr int const kMaxInt = INT_MAX / 4;

    if (next == last)
    {
        result = 0.0; // [Recover.]
        return true;
    }

    if (last - next >= kMaxInt)
    {
        result = std::numeric_limits<double>::quiet_NaN();
        return false;
    }

    constexpr int kMaxSignificantDigits = base_conv::kMaxSignificantDigits;

    char digits[kMaxSignificantDigits];
    int  num_digits = 0;
    int  exponent = 0;
    bool nonzero_tail = false;

    bool is_neg = false;
    if (*next == '-')
    {
        is_neg = true;

        ++next;
        if (next == last)
        {
            result = is_neg ? -0.0 : +0.0; // Recover.
            return false;
        }
    }
#if 0
    else if (options.allow_leading_plus && *next == '+')
    {
        ++next;
        if (next == last)
        {
            result = is_neg ? -0.0 : +0.0; // Recover.
            return false;
        }
    }
#endif

    if (*next == '0')
    {
        ++next;
        if (next == last)
        {
            result = is_neg ? -0.0 : +0.0;
            return true;
        }
    }
    else if (IsDigit(*next))
    {
        // Copy significant digits of the integer part (if any) to the buffer.
        for (;;)
        {
            if (num_digits < kMaxSignificantDigits)
            {
                digits[num_digits++] = *next;
            }
            else
            {
                ++exponent;
                nonzero_tail = nonzero_tail || *next != '0';
            }
            ++next;
            if (next == last)
            {
                goto L_parsing_done;
            }
            if (!IsDigit(*next))
            {
                break;
            }
        }
    }
#if 0
    else if (options.allow_leading_dot && *next == '.')
    {
    }
#endif
    else
    {
        if (options.allow_nan_inf && last - next >= 3 && std::memcmp(next, "NaN", 3) == 0)
        {
            result = std::numeric_limits<double>::quiet_NaN();
            return true;
        }

        if (options.allow_nan_inf && last - next >= 8 && std::memcmp(next, "Infinity", 8) == 0)
        {
            result = is_neg ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
            return true;
        }

        return false;
    }

    if (*next == '.')
    {
        ++next;
        if (next == last)
        {
            // XXX: Recover? Continue with exponent?
            return false;
        }

        if (num_digits == 0)
        {
            // Integer part consists of 0 (or is absent).
            // Significant digits start after leading zeros (if any).
            while (*next == '0')
            {
                ++next;
                if (next == last)
                {
                    result = is_neg ? -0.0 : +0.0;
                    return true;
                }

                // Move this 0 into the exponent.
                --exponent;
            }
        }

        // There is a fractional part.
        // We don't emit a '.', but adjust the exponent instead.
        while (IsDigit(*next))
        {
            if (num_digits < kMaxSignificantDigits)
            {
                digits[num_digits++] = *next;
                --exponent;
            }
            else
            {
                nonzero_tail = nonzero_tail || *next != '0';
            }
            ++next;
            if (next == last)
            {
                goto L_parsing_done;
            }
        }
    }

    // Parse exponential part.
    if (*next == 'e' || *next == 'E')
    {
        ++next;
        if (next == last)
        {
            // XXX:
            // Recover? Parse as if exponent = 0?
            return false;
        }

        bool const exp_is_neg = (*next == '-');

        if (*next == '+' || exp_is_neg)
        {
            ++next;
            if (next == last)
            {
                // XXX:
                // Recover? Parse as if exponent = 0?
                return false;
            }
        }

        if (!IsDigit(*next))
        {
            // XXX:
            // Recover? Parse as if exponent = 0?
            return false;
        }

        int num = 0;
        for (;;)
        {
            int const digit = HexDigitValue(*next);

//          if (num > kMaxInt / 10 || digit > kMaxInt - 10 * num)
            if (num > kMaxInt / 10 - 9)
            {
                // Overflow.
                // Skip the rest of the exponent (ignored).
                for (++next; next != last && IsDigit(*next); ++next)
                {
                }
                num = kMaxInt;
                break;
            }

            num = num * 10 + digit;
            ++next;
            if (next == last)
            {
                break;
            }
            if (!IsDigit(*next))
            {
                break; // trailing junk
            }
        }

        exponent += exp_is_neg ? -num : num;
    }

L_parsing_done:
    double const value = base_conv::DecimalToDouble(digits, num_digits, exponent, nonzero_tail);
    JSON_ASSERT(!std::signbit(value));

    result = is_neg ? -value : value;
    return true;
#endif
}

inline double ReadDouble_unguarded(char const* digits, int num_digits)
{
    using namespace json::charclass;

    int64_t result = 0;

    for (int i = 0; i != num_digits; ++i)
    {
        JSON_ASSERT(IsDigit(digits[i]));
        result = 10 * result + HexDigitValue(digits[i]);
    }

    return static_cast<double>(result);
}

double json::numbers::StringToNumber(char const* first, char const* last, NumberClass nc)
{
    JSON_ASSERT(first != last);
    JSON_ASSERT(nc != NumberClass::invalid);

    if (first == last || last - first > INT_MAX) {
        return std::numeric_limits<double>::quiet_NaN();
    }

    if (nc == NumberClass::nan) {
        return std::numeric_limits<double>::quiet_NaN();
    }
    if (nc == NumberClass::pos_infinity) {
        return +std::numeric_limits<double>::infinity();
    }
    if (nc == NumberClass::neg_infinity) {
        return -std::numeric_limits<double>::infinity();
    }

#if 1
    // Use a _slightly_ faster method for parsing integers which will fit into a
    // double-precision number without loss of precision. Larger numbers will be
    // handled by strtod.
    if (nc == NumberClass::integer)
    {
        char const* first_digit = first;

        bool const is_neg = *first == '-';
        if (is_neg || *first == '+')
        {
            ++first_digit;
        }

#if 0
        JSON_ASSERT(last - first_digit <= INT_MAX);
        double const result = base_conv::DecimalToDouble(first_digit, static_cast<int>(last - first_digit), /*exponent*/ 0);
        return is_neg ? -result : result;
#else
        // 10^15 < 2^53 = 9007199254740992 < 10^16
        if (last - first_digit <= 16)
        {
            using namespace json::charclass;

            JSON_ASSERT(IsDigit(*first_digit));

            if (last - first_digit < 16 || HexDigitValue(*first_digit) <= 8 /*|| std::memcmp(first_digit, "9007199254740992", 16) <= 0*/)
            {
                double const result = ReadDouble_unguarded(first_digit, static_cast<int>(last - first_digit));
                return is_neg ? -result : result;
            }
        }
#endif
    }
#endif

    double result;
    if (StringToDouble(result, first, last, Options{}))
    {
        return result;
    }

    JSON_ASSERT(false && "unreachable");
    return std::numeric_limits<double>::quiet_NaN();
}

bool json::numbers::StringToNumber(double& result, char const* first, char const* last, Options const& options)
{
    if (StringToDouble(result, first, last, options))
    {
        return true;
    }

    result = std::numeric_limits<double>::quiet_NaN();
    return false;
}

double json::numbers::ParseFloat(char const* first, char const* last)
{
    double d;
    if (StringToDouble(d, first, last, Options{}))
    {
        return d;
    }

    return std::numeric_limits<double>::quiet_NaN();
}

// https://tc39.github.io/ecma262/#sec-parseint-string-radix
//
// When the parseInt function is called, the following steps are taken:
//
//  1. Let inputString be ? ToString(string).
//  2. Let S be a newly created substring of inputString consisting of the first
//     code unit that is not a StrWhiteSpaceChar and all code units following
//     that code unit. (In other words, remove leading white space.) If
//     inputString does not contain any such code unit, let S be the empty
//     string.
//  3. Let sign be 1.
//  4. If S is not empty and the first code unit of S is the code unit 0x002D
//     (HYPHEN-MINUS), let sign be -1.
//  5. If S is not empty and the first code unit of S is the code unit 0x002B
//     (PLUS SIGN) or the code unit 0x002D (HYPHEN-MINUS), remove the first code
//     unit from S.
//  6. Let R be ? ToInt32(radix).
//  7. Let stripPrefix be true.
//  8. If R != 0, then
//       a. If R < 2 or R > 36, return NaN.
//       b. If R != 16, let stripPrefix be false.
//  9. Else R = 0,
//       a. Let R be 10.
// 10. If stripPrefix is true, then
//       a. If the length of S is at least 2 and the first two code units of S
//          are either "0x" or "0X", remove the first two code units from S and
//          let R be 16.
// 11. If S contains a code unit that is not a radix-R digit, let Z be the
//     substring of S consisting of all code units before the first such code
//     unit; otherwise, let Z be S.
// 12. If Z is empty, return NaN.
// 13. Let mathInt be the mathematical integer value that is represented by Z
//     in radix-R notation, using the letters A-Z and a-z for digits with values
//     10 through 35. (However, if R is 10 and Z contains more than 20
//     significant digits, every significant digit after the 20th may be
//     replaced by a 0 digit, at the option of the implementation; and if R is
//     not 2, 4, 8, 10, 16, or 32, then mathInt may be an implementation-
//     dependent approximation to the mathematical integer value that is
//     represented by Z in radix-R notation.)
// 14. If mathInt = 0, then
//       a. If sign = -1, return -0.
//       b. Return +0.
// 15. Let number be the Number value for mathInt.
// 16. Return sign * number.

double json::numbers::ParseInt(char const* /*first*/, char const* /*last*/, int /*radix*/)
{
    JSON_ASSERT(false && "not implemented");
    return 0.0;
}
