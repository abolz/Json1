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

#include "dtoa.h"
#include "strtod.h"

#include <cassert>
#include <climits>
#include <cmath>
#include <cstring>
#include <limits>
#include <type_traits>

using namespace json;
using namespace json::numbers;

#define FAST_INT 0

//==================================================================================================
// NumberToString
//==================================================================================================

#if FAST_INT

static char* Utoa_9digits(char* buf, uint32_t digits)
{
    using base_conv::Utoa100;

    uint32_t n = digits;
    uint32_t q;

    q = n / 10000000;
    n = n % 10000000;
    buf = Utoa100(buf, q);
    q = n / 100000;
    n = n % 100000;
    buf = Utoa100(buf, q);
    q = n / 1000;
    n = n % 1000;
    buf = Utoa100(buf, q);
    q = n / 10;
    n = n % 10;
    buf = Utoa100(buf, q);
    buf[0] = static_cast<char>('0' + n);
    buf += 1;

    return buf;
}

static char* U32ToString(char* buf, uint32_t n)
{
    using base_conv::Utoa100;

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
        buf++;
        return buf;
    }

    if (n < 100) {
        if (n >= 10)
            goto L_2_digits;
        else
            goto L_1_digit;
    }

    if (n < 10000) {
        if (n >= 1000)
            goto L_4_digits;
        else
            goto L_3_digits;
    }

    if (n < 1000000) {
        if (n >= 100000)
            goto L_6_digits;
        else
            goto L_5_digits;
    }

    //if (n < 100000000)
    {
        if (n >= 10000000)
            goto L_8_digits;
        else
            goto L_7_digits;
    }
}

static char* U64ToString(char* buf, uint64_t n)
{
    using base_conv::Utoa100;

    if (n <= UINT32_MAX)
        return U32ToString(buf, static_cast<uint32_t>(n));

    // n = hi * 10^9 + lo < 10^20,   where hi < 10^11, lo < 10^9
    auto const hi = n / 1000000000;
    auto const lo = n % 1000000000;

#if 1
    assert(hi <= UINT32_MAX);

    buf = U32ToString(buf, static_cast<uint32_t>(hi));
    // lo has exactly 9 digits.
    // (Which might all be zero...)
    buf = Utoa_9digits(buf, static_cast<uint32_t>(lo));

    return buf;
#else
    if (hi <= UINT32_MAX)
    {
        buf = U32ToString(buf, static_cast<uint32_t>(hi));
    }
    else
    {
        // 2^32 < hi = hi1 * 10^9 + hi0 < 10^11,   where hi1 < 10^2, 10^9 <= hi0 < 10^9
        auto const hi1 = static_cast<uint32_t>(hi / 1000000000);
        auto const hi0 = static_cast<uint32_t>(hi % 1000000000);
        if (hi1 >= 10)
        {
            buf = Utoa100(buf, hi1);
        }
        else
        {
            assert(hi1 != 0);
            buf[0] = static_cast<char>('0' + hi1);
            buf++;
        }
        buf = Utoa_9digits(buf, hi0);
    }

    // lo has exactly 9 digits.
    // (Which might all be zero...)
    return Utoa_9digits(buf, static_cast<uint32_t>(lo));
#endif
}

static char* I64ToString(char* buf, int64_t i)
{
    auto n = static_cast<uint64_t>(i);
    if (i < 0)
    {
        buf[0] = '-';
        buf++;
        n = 0 - n;
    }

    return U64ToString(buf, n);
}

#endif

char* json::numbers::NumberToString(char* next, char* last, double value, bool force_trailing_dot_zero)
{
    constexpr double kMinInteger = -9007199254740992.0; // -2^53
    constexpr double kMaxInteger =  9007199254740992.0; //  2^53

    // Print integers in the range [-2^53, +2^53] as integers (without a trailing ".0").
    // These integers are exactly representable as 'double's.
    // However, always print -0 as "-0.0" to increase compatibility with other libraries
    //
    // NB:
    // These tests for work correctly for NaN's and Infinity's (i.e. the DoubleToString branch is taken).
#if FAST_INT
    if (value == 0.0 && std::signbit(value))
    {
        std::memcpy(next, "-0.0", 4);
        return next + 4;
    }
    else if (value >= kMinInteger && value <= kMaxInteger)
    {
        const int64_t i = static_cast<int64_t>(value);
        if (static_cast<double>(i) == value)
        {
            return I64ToString(next, i);
        }
    }
#else
    if (force_trailing_dot_zero && (value >= kMinInteger && value <= kMaxInteger))
    {
        force_trailing_dot_zero = false;
    }
#endif

    return base_conv::Dtoa(next, last, value, force_trailing_dot_zero, "NaN", "Infinity");
}

//==================================================================================================
// StringToNumber
//==================================================================================================

#if 0
static int DigitValue(char ch)
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
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
    //  P       Q       R       S       T       U       V       W       X       Y       Z       [       \       ]       ^       _
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
    //  `       a       b       c       d       e       f       g       h       i       j       k       l       m       n       o
        N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,      N,
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
static int DigitValue(char ch)
{
    if ('0' <= ch && ch <= '9')
        return ch - '0';
    return -1;
}
#endif

static bool IsDigit(char ch)
{
    return DigitValue(ch) >= 0;
}

static int CountTrailingZeros(char const* buffer, int buffer_length)
{
    assert(buffer_length >= 0);

    int i = buffer_length;
    for ( ; i > 0; --i)
    {
        if (buffer[i - 1] != '0')
            break;
    }

    return buffer_length - i;
}

static bool Strtod(double& result, char const* next, char const* last, Options const& options)
{
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
    constexpr int kBufferSize = kMaxSignificantDigits + 1;

    char buffer[kBufferSize];
    int  length = 0;
    int  exponent = 0;
    bool nonzero_digit_dropped = false;

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
            if (length < kMaxSignificantDigits)
            {
                buffer[length++] = *next;
            }
            else
            {
                ++exponent;
                nonzero_digit_dropped = nonzero_digit_dropped || *next != '0';
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

        if (length == 0)
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
            if (length < kMaxSignificantDigits)
            {
                buffer[length++] = *next;
                --exponent;
            }
            else
            {
                nonzero_digit_dropped = nonzero_digit_dropped || *next != '0';
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
            assert(IsDigit(*next));
            int const digit = *next - '0';

            if (num > kMaxInt / 10 || digit > kMaxInt - 10 * num)
            {
                // Overflow.
                // Skip the rest of the exponent (ignored).
                //for (++next; next != last && IsDigit(*next); ++next)
                for (++next; next != last && DigitValue(*next) >= 0; ++next)
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
#if 1
    if (/*options.allow_trailing_junk &&*/ next != last)
    {
        return false; // trailing junk
    }
#endif

    if (nonzero_digit_dropped)
    {
        // Set the last digit to be non-zero.
        // This is sufficient to guarantee correct rounding.
        assert(length == kMaxSignificantDigits);
        assert(length < kBufferSize);
        buffer[length++] = '1';
        --exponent;
    }
    else
    {
        int const num_zeros = CountTrailingZeros(buffer, length);
        length   -= num_zeros;
        exponent += num_zeros;
    }

    double value = base_conv::StringToIeee<double>(buffer, length, exponent);
    result = is_neg ? -value : value;
    return true;
}

static double ReadDouble_unguarded(char const* buffer, int buffer_length)
{
    int64_t result = 0;

    for (int i = 0; i != buffer_length; ++i)
    {
        assert(IsDigit(buffer[i]));
        result = 10 * result + DigitValue(buffer[i]);
    }

    return static_cast<double>(result);
}

double json::numbers::StringToNumber(char const* first, char const* last, NumberClass nc)
{
    assert(first != last);
    assert(nc != NumberClass::invalid);

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

    char const* first_digit = first;

    bool const is_neg = *first == '-';
    if (is_neg || *first == '+')
    {
        ++first_digit;
    }

#if 1
    // Use a _slightly_ faster method for parsing integers which will fit into a
    // double-precision number without loss of precision. Larger numbers will be
    // handled by strtod.
    //
    // 10^15 < 2^53 = 9007199254740992 < 10^16
    if (nc == NumberClass::integer && last - first_digit <= 16)
    {
        assert(IsDigit(*first_digit));
        if (last - first_digit < 16 || *first_digit <= '8' /*|| std::memcmp(first_digit, "9007199254740992", 16) <= 0*/)
        {
            double const result = ReadDouble_unguarded(first_digit, static_cast<int>(last - first_digit));
            return is_neg ? -result : result;
        }
    }
#endif

    double result;
    if (Strtod(result, first, last, Options{}))
    {
        return result;
    }

    assert(false && "unreachable");
    return std::numeric_limits<double>::quiet_NaN();
}

bool json::numbers::StringToNumber(double& result, char const* first, char const* last, Options const& options)
{
    if (Strtod(result, first, last, options))
        return true;

    result = std::numeric_limits<double>::quiet_NaN();
    return false;
}
