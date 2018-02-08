// Copyright (c) 2017 Alexander Bolz
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

#include "json.h"

#include <climits>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <memory>

#ifdef _MSC_VER
#include <intrin.h>
#endif

#ifndef JSON_HAS_DOUBLE_CONVERSION
#define JSON_HAS_DOUBLE_CONVERSION 0
#endif
#ifndef JSON_HAS_STRTOD_L
#define JSON_HAS_STRTOD_L 0
#endif

#define JSON_SKIP_INVALID_UNICODE 0

using namespace json;

//==================================================================================================
// Int/String conversions
//==================================================================================================

namespace iconv {

static char* Itoa100(char* buf, uint32_t digits)
{
    static constexpr char const* const kDigits100 =
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

    assert(digits < 100);
    std::memcpy(buf, kDigits100 + 2*digits, 2);
    return buf + 2;
}

static char* U32toa(char* buf, uint32_t n)
{
    uint32_t q;

    if (n >= 1000000000)
    {
//L_10_digits:
        q = n / 100000000;
        n = n % 100000000;
        buf = Itoa100(buf, q);
L_8_digits:
        q = n / 1000000;
        n = n % 1000000;
        buf = Itoa100(buf, q);
L_6_digits:
        q = n / 10000;
        n = n % 10000;
        buf = Itoa100(buf, q);
L_4_digits:
        q = n / 100;
        n = n % 100;
        buf = Itoa100(buf, q);
//L_2_digits:
        return Itoa100(buf, n);
    }

    if (n < 100) {
        if (n >= 10) {
            return Itoa100(buf, n);
        } else {
            buf[0] = static_cast<char>('0' + n);
            return buf + 1;
        }
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

    if (n < 100000000) {
        if (n >= 10000000)
            goto L_8_digits;
        else
            goto L_7_digits;
    }

    //if (n >= 100000000)
    {
//L_9_digits:
        q = n / 10000000;
        n = n % 10000000;
        buf = Itoa100(buf, q);
L_7_digits:
        q = n / 100000;
        n = n % 100000;
        buf = Itoa100(buf, q);
L_5_digits:
        q = n / 1000;
        n = n % 1000;
        buf = Itoa100(buf, q);
L_3_digits:
        q = n / 10;
        n = n % 10;
        buf = Itoa100(buf, q);
//L_1_digit:
        buf[0] = static_cast<char>('0' + n);
        return buf + 1;
    }
}

static char* U32toa_n(char* buf, uint32_t n, int num_digits)
{
    uint32_t q;
    switch (num_digits)
    {
// Generate an even number digits
    case 10:
        q = n / 100000000;
        n = n % 100000000;
        buf = Itoa100(buf, q);
        // fall through
    case 8:
        assert(n < 100000000);
        q = n / 1000000;
        n = n % 1000000;
        buf = Itoa100(buf, q);
        // fall through
    case 6:
        assert(n < 1000000);
        q = n / 10000;
        n = n % 10000;
        buf = Itoa100(buf, q);
        // fall through
    case 4:
        assert(n < 10000);
        q = n / 100;
        n = n % 100;
        buf = Itoa100(buf, q);
        // fall through
    case 2:
        assert(n < 100);
        buf = Itoa100(buf, n);
        return buf;

// Generate an odd number of digits:
    case 9:
        assert(n < 1000000000);
        q = n / 10000000;
        n = n % 10000000;
        buf = Itoa100(buf, q);
        // fall through
    case 7:
        assert(n < 10000000);
        q = n / 100000;
        n = n % 100000;
        buf = Itoa100(buf, q);
        // fall through
    case 5:
        assert(n < 100000);
        q = n / 1000;
        n = n % 1000;
        buf = Itoa100(buf, q);
        // fall through
    case 3:
        assert(n < 1000);
        q = n / 10;
        n = n % 10;
        buf = Itoa100(buf, q);
        // fall through
    case 1:
        assert(n < 10);
        buf[0] = static_cast<char>('0' + n);
        return buf + 1;
    }

    return buf;
}

static char* U64toa(char* buf, uint64_t n)
{
    if (n <= UINT32_MAX)
        return U32toa(buf, static_cast<uint32_t>(n));

    // n = hi * 10^9 + lo < 10^20,   where hi < 10^11, lo < 10^9
    const uint64_t hi = n / 1000000000;
    const uint64_t lo = n % 1000000000;

    if (hi <= UINT32_MAX)
    {
        buf = U32toa(buf, static_cast<uint32_t>(hi));
    }
    else
    {
        // 2^32 < hi = hi1 * 10^9 + hi0 < 10^11,   where hi1 < 10^2, 10^9 <= hi0 < 10^9
        const uint32_t hi1 = static_cast<uint32_t>(hi / 1000000000);
        const uint32_t hi0 = static_cast<uint32_t>(hi % 1000000000);
        buf = U32toa_n(buf, hi1, hi1 >= 10 ? 2 : 1);
        buf = U32toa_n(buf, hi0, 9);
    }

    // lo has exactly 9 digits.
    // (Which might all be zero...)
    return U32toa_n(buf, static_cast<uint32_t>(lo), 9);
}

static char* S64toa(char* buf, int64_t i)
{
    uint64_t n = static_cast<uint64_t>(i);
    if (i < 0)
    {
        buf[0] = '-';
        buf++;
        n = 0 - n;
    }

    return U64toa(buf, n);
}

} // namespace iconv

//==================================================================================================
// Double/String conversions
//==================================================================================================

static constexpr char const* const kNaNString = "NaN";
static constexpr char const* const kInfString = "Infinity";

#if JSON_HAS_DOUBLE_CONVERSION
#include <double-conversion/double-conversion.h>

static double Strtod(char const* str, int len)
{
    using double_conversion::StringToDoubleConverter;

    StringToDoubleConverter conv(
        StringToDoubleConverter::ALLOW_LEADING_SPACES |
        StringToDoubleConverter::ALLOW_TRAILING_SPACES,
        0.0,                                        // empty_string_value
        std::numeric_limits<double>::quiet_NaN(),   // junk_string_value,
        kInfString,                                 // infinity_symbol,
        kNaNString);                                // nan_symbol

    int processed = 0;
    return conv.StringToDouble(str, len, &processed);
}

#elif defined(_MSC_VER)
#include <clocale>
#include <cstdio>

namespace
{
    struct ClassicLocale {
        const ::_locale_t loc;
        ClassicLocale() noexcept : loc(::_create_locale(LC_ALL, "C")) {}
       ~ClassicLocale() noexcept { ::_free_locale(loc); }
    };
}
static const ClassicLocale s_clocale;

static double Strtod(char const* c_str, char** end)
{
    return ::_strtod_l(c_str, end, s_clocale.loc);
}

#elif JSON_HAS_STRTOD_L // not tested...
#include <clocale>
#include <cstdio>

namespace
{
    struct ClassicLocale {
        const ::locale_t loc;
        ClassicLocale() noexcept : loc(::newlocale(LC_ALL, "C", 0)) {}
       ~ClassicLocale() noexcept { ::freelocale(loc); }
    };
}
static const ClassicLocale s_clocale;

static double Strtod(char const* c_str, char** end)
{
    return ::strtod_l(c_str, end, s_clocale.loc);
}

#else
#include <cstdio>

// FIXME!
static double Strtod(char const* c_str, char** end)
{
    return ::strtod(c_str, end);
}

#endif

#if !JSON_HAS_DOUBLE_CONVERSION

static double Strtod(char const* str, int len)
{
    static constexpr int const kBufSize = 200;

    assert(str != nullptr);
    assert(len >= 0);

    if (len < kBufSize)
    {
        char buf[kBufSize];
        std::memcpy(buf, str, static_cast<size_t>(len));
        buf[len] = '\0';

        return Strtod(buf, nullptr);
    }

    std::vector<char> buf(str, str + len);
    buf.push_back('\0');

    return Strtod(buf.data(), nullptr);
}

#endif

// The Grisu2 algorithm for binary to decimal floating-point conversion.
//
// This implementation is a slightly modified version of the reference
// implementation by Florian Loitsch which can be obtained from
// http://florian.loitsch.com/publications (bench.tar.gz)
//
// The original license can be found at the end of this file.
//
// References:
//
// [1]  Loitsch, "Printing Floating-Point Numbers Quickly and Accurately with Integers",
//      Proceedings of the ACM SIGPLAN 2010 Conference on Programming Language Design and Implementation, PLDI 2010
// [2]  Burger, Dybvig, "Printing Floating-Point Numbers Quickly and Accurately",
//      Proceedings of the ACM SIGPLAN 1996 Conference on Programming Language Design and Implementation, PLDI 1996
//
namespace dconv {

template <typename Target, typename Source>
static Target ReinterpretBits(Source source)
{
    static_assert(sizeof(Target) == sizeof(Source), "Size mismatch");

    Target target;
    std::memcpy(&target, &source, sizeof(Source));
    return target;
}

struct DiyFp // f * 2^e
{
    static constexpr int kPrecision = 64; // = q

    uint64_t f;
    int e;

    constexpr DiyFp() : f(0), e(0) {}
    constexpr DiyFp(uint64_t f_, int e_) : f(f_), e(e_) {}

    // Returns x - y.
    // PRE: x.e == y.e and x.f >= y.f
    static DiyFp sub(DiyFp x, DiyFp y);

    // Returns x * y.
    // The result is rounded. (Only the upper q bits are returned.)
    static DiyFp mul(DiyFp x, DiyFp y);

    // Normalize x such that the significand is >= 2^(q-1).
    // PRE: x.f != 0
    static DiyFp normalize(DiyFp x);

    // Normalize x such that the result has the exponent E.
    // PRE: e >= x.e and the upper e - x.e bits of x.f must be zero.
    static DiyFp normalize_to(DiyFp x, int e);
};

inline DiyFp DiyFp::sub(DiyFp x, DiyFp y)
{
    assert(x.e == y.e);
    assert(x.f >= y.f);

    return DiyFp(x.f - y.f, x.e);
}

inline DiyFp DiyFp::mul(DiyFp x, DiyFp y)
{
    // Computes:
    //  f = round((x.f * y.f) / 2^q)
    //  e = x.e + y.e + q

#if defined(_MSC_VER) && defined(_M_X64)

    uint64_t h = 0;
    uint64_t l = _umul128(x.f, y.f, &h);
    h += l >> 63; // round, ties up: [h, l] += 2^q / 2

    return DiyFp(h, x.e + y.e + 64);

#elif defined(__GNUC__) && defined(__SIZEOF_INT128__)

    __extension__ using Uint128 = unsigned __int128;

    Uint128 const p = Uint128{x.f} * Uint128{y.f};

    uint64_t h = static_cast<uint64_t>(p >> 64);
    uint64_t l = static_cast<uint64_t>(p);
    h += l >> 63; // round, ties up: [h, l] += 2^q / 2

    return DiyFp(h, x.e + y.e + 64);

#else

    // Emulate the 64-bit * 64-bit multiplication:
    //
    // p = u * v
    //   = (u_lo + 2^32 u_hi) (v_lo + 2^32 v_hi)
    //   = (u_lo v_lo         ) + 2^32 ((u_lo v_hi         ) + (u_hi v_lo         )) + 2^64 (u_hi v_hi         )
    //   = (p0                ) + 2^32 ((p1                ) + (p2                )) + 2^64 (p3                )
    //   = (p0_lo + 2^32 p0_hi) + 2^32 ((p1_lo + 2^32 p1_hi) + (p2_lo + 2^32 p2_hi)) + 2^64 (p3                )
    //   = (p0_lo             ) + 2^32 (p0_hi + p1_lo + p2_lo                      ) + 2^64 (p1_hi + p2_hi + p3)
    //   = (p0_lo             ) + 2^32 (Q                                          ) + 2^64 (H                 )
    //   = (p0_lo             ) + 2^32 (Q_lo + 2^32 Q_hi                           ) + 2^64 (H                 )
    //
    // (Since Q might be larger than 2^32 - 1)
    //
    //   = (p0_lo + 2^32 Q_lo) + 2^64 (Q_hi + H)
    //
    // (Q_hi + H does not overflow a 64-bit int)
    //
    //   = p_lo + 2^64 p_hi

    const uint64_t u_lo = x.f & 0xFFFFFFFF;
    const uint64_t u_hi = x.f >> 32;
    const uint64_t v_lo = y.f & 0xFFFFFFFF;
    const uint64_t v_hi = y.f >> 32;

    const uint64_t p0 = u_lo * v_lo;
    const uint64_t p1 = u_lo * v_hi;
    const uint64_t p2 = u_hi * v_lo;
    const uint64_t p3 = u_hi * v_hi;

    const uint64_t p0_hi = p0 >> 32;
    const uint64_t p1_lo = p1 & 0xFFFFFFFF;
    const uint64_t p1_hi = p1 >> 32;
    const uint64_t p2_lo = p2 & 0xFFFFFFFF;
    const uint64_t p2_hi = p2 >> 32;

    uint64_t Q = p0_hi + p1_lo + p2_lo;

    // The full product might now be computed as
    //
    // p_hi = p3 + p2_hi + p1_hi + (Q >> 32)
    // p_lo = p0_lo + (Q << 32)
    //
    // But in this particular case here, the full p_lo is not required.
    // Effectively we only need to add the highest bit in p_lo to p_hi (and
    // Q_hi + 1 does not overflow).

    Q += uint64_t{1} << (63 - 32); // round, ties up

    const uint64_t h = p3 + p2_hi + p1_hi + (Q >> 32);

    return DiyFp(h, x.e + y.e + 64);

#endif
}

inline DiyFp DiyFp::normalize(DiyFp x)
{
    assert(x.f != 0);

#if defined(_MSC_VER) && defined(_M_X64)

    const int leading_zeros = static_cast<int>(__lzcnt64(x.f));
    return DiyFp(x.f << leading_zeros, x.e - leading_zeros);

#elif defined(__GNUC__)

    const int leading_zeros = __builtin_clzll(x.f);
    return DiyFp(x.f << leading_zeros, x.e - leading_zeros);

#else

    while ((x.f >> 63) == 0)
    {
        x.f <<= 1;
        x.e--;
    }
    return x;

#endif
}

inline DiyFp DiyFp::normalize_to(DiyFp x, int target_exponent)
{
    const int delta = x.e - target_exponent;

    assert(delta >= 0);
    assert(((x.f << delta) >> delta) == x.f);

    return DiyFp(x.f << delta, target_exponent);
}

struct Boundaries {
    DiyFp w;
    DiyFp minus;
    DiyFp plus;
};

// Compute the (normalized) DiyFp representing the input number 'value' and its boundaries.
// PRE: value must be finite and positive
template <typename FloatType>
static Boundaries ComputeBoundaries(FloatType value)
{
    assert(std::isfinite(value));
    assert(value > 0);

    // Convert the IEEE representation into a DiyFp.
    //
    // If v is denormal:
    //      value = 0.F * 2^(1 - bias) = (          F) * 2^(1 - bias - (p-1))
    // If v is normalized:
    //      value = 1.F * 2^(E - bias) = (2^(p-1) + F) * 2^(E - bias - (p-1))

    static_assert(std::numeric_limits<FloatType>::is_iec559, "Requires an IEEE-754 floating-point implementation");

    constexpr int      kPrecision = std::numeric_limits<FloatType>::digits; // = p (includes the hidden bit)
    constexpr int      kBias      = std::numeric_limits<FloatType>::max_exponent - 1 + (kPrecision - 1);
    constexpr uint64_t kHiddenBit = uint64_t{1} << (kPrecision - 1); // = 2^(p-1)

    using bits_type = typename std::conditional<kPrecision == 24, uint32_t, uint64_t>::type;

    const uint64_t bits = ReinterpretBits<bits_type>(value);
    const uint64_t E = bits >> (kPrecision - 1);
    const uint64_t F = bits & (kHiddenBit - 1);

    const bool is_denormal = (E == 0);

    const DiyFp v
        = is_denormal
            ? DiyFp(F, 1 - kBias)
            : DiyFp(F + kHiddenBit, static_cast<int>(E) - kBias);

    // Compute the boundaries m- and m+ of the floating-point value
    // v = f * 2^e.
    //
    // Determine v- and v+, the floating-point predecessor and successor if v,
    // respectively.
    //
    //      v- = v - 2^e        if f != 2^(p-1) or e == e_min                (A)
    //         = v - 2^(e-1)    if f == 2^(p-1) and e > e_min                (B)
    //
    //      v+ = v + 2^e
    //
    // Let m- = (v- + v) / 2 and m+ = (v + v+) / 2. All real numbers _strictly_
    // between m- and m+ round to v, regardless of how the input rounding
    // algorithm breaks ties.
    //
    //      ---+-------------+-------------+-------------+-------------+---  (A)
    //         v-            m-            v             m+            v+
    //
    //      -----------------+------+------+-------------+-------------+---  (B)
    //                       v-     m-     v             m+            v+

//  const bool lower_boundary_is_closer = (v.f == kHiddenBit && v.e > kMinExp);
    const bool lower_boundary_is_closer = (F == 0 && E > 1);

    const DiyFp m_plus = DiyFp(2*v.f + 1, v.e - 1);
    const DiyFp m_minus
        = lower_boundary_is_closer
            ? DiyFp(4*v.f - 1, v.e - 2)  // (B)
            : DiyFp(2*v.f - 1, v.e - 1); // (A)

    // Determine the normalized w+ = m+.
    const DiyFp w_plus = DiyFp::normalize(m_plus);

    // Determine w- = m- such that e_(w-) = e_(w+).
    const DiyFp w_minus = DiyFp::normalize_to(m_minus, w_plus.e);

    return {DiyFp::normalize(v), w_minus, w_plus};
}

// Given normalized DiyFp w, Grisu needs to find a (normalized) cached
// power-of-ten c, such that the exponent of the product c * w = f * 2^e lies
// within a certain range [alpha, gamma] (Definition 3.2 from [1])
//
//      alpha <= e = e_c + e_w + q <= gamma
//
// or
//
//      f_c * f_w * 2^alpha <= f_c 2^(e_c) * f_w 2^(e_w) * 2^q
//                          <= f_c * f_w * 2^gamma
//
// Since c and w are normalized, i.e. 2^(q-1) <= f < 2^q, this implies
//
//      2^(q-1) * 2^(q-1) * 2^alpha <= c * w * 2^q < 2^q * 2^q * 2^gamma
//
// or
//
//      2^(q - 2 + alpha) <= c * w < 2^(q + gamma)
//
// The choice of (alpha,gamma) determines the size of the table and the form of
// the digit generation procedure. Using (alpha,gamma)=(-60,-32) works out well
// in practice:
//
// The idea is to cut the number c * w = f * 2^e into two parts, which can be
// processed independently: An integral part p1, and a fractional part p2:
//
//      f * 2^e = ( (f div 2^-e) * 2^-e + (f mod 2^-e) ) * 2^e
//              = (f div 2^-e) + (f mod 2^-e) * 2^e
//              = p1 + p2 * 2^e
//
// The conversion of p1 into decimal form requires a series of divisions and
// modulos by (a power of) 10. These operations are faster for 32-bit than for
// 64-bit integers, so p1 should ideally fit into a 32-bit integer. This can be
// achieved by choosing
//
//      -e >= 32   or   e <= -32 := gamma
//
// In order to convert the fractional part
//
//      p2 * 2^e = p2 / 2^-e = d[-1] / 10^1 + d[-2] / 10^2 + ...
//
// into decimal form, the fraction is repeatedly multiplied by 10 and the digits
// d[-i] are extracted in order:
//
//      (10 * p2) div 2^-e = d[-1]
//      (10 * p2) mod 2^-e = d[-2] / 10^1 + ...
//
// The multiplication by 10 must not overflow. It is sufficient to choose
//
//      10 * p2 < 16 * p2 = 2^4 * p2 <= 2^64.
//
// Since p2 = f mod 2^-e < 2^-e,
//
//      -e <= 60   or   e >= -60 := alpha

constexpr int kAlpha = -60;
constexpr int kGamma = -32;

struct CachedPower { // c = f * 2^e ~= 10^k
    uint64_t f;
    int e;
    int k;
};

// For a normalized DiyFp w = f * 2^e, this function returns a (normalized)
// cached power-of-ten c = f_c * 2^e_c, such that the exponent of the product
// w * c satisfies (Definition 3.2 from [1])
//
//      alpha <= e_c + e + q <= gamma.
//
static CachedPower GetCachedPowerForBinaryExponent(int e)
{
    // Now
    //
    //      alpha <= e_c + e + q <= gamma                                    (1)
    //      ==> f_c * 2^alpha <= c * 2^e * 2^q
    //
    // and since the c's are normalized, 2^(q-1) <= f_c,
    //
    //      ==> 2^(q - 1 + alpha) <= c * 2^(e + q)
    //      ==> 2^(alpha - e - 1) <= c
    //
    // If c were an exakt power of ten, i.e. c = 10^k, one may determine k as
    //
    //      k = ceil( log_10( 2^(alpha - e - 1) ) )
    //        = ceil( (alpha - e - 1) * log_10(2) )
    //
    // From the paper:
    // "In theory the result of the procedure could be wrong since c is rounded,
    //  and the computation itself is approximated [...]. In practice, however,
    //  this simple function is sufficient."
    //
    // For IEEE double precision floating-point numbers converted into
    // normalized DiyFp's w = f * 2^e, with q = 64,
    //
    //      e >= -1022      (min IEEE exponent)
    //           -52        (p - 1)
    //           -52        (p - 1, possibly normalize denormal IEEE numbers)
    //           -11        (normalize the DiyFp)
    //         = -1137
    //
    // and
    //
    //      e <= +1023      (max IEEE exponent)
    //           -52        (p - 1)
    //           -11        (normalize the DiyFp)
    //         = 960
    //
    // This binary exponent range [-1137,960] results in a decimal exponent
    // range [-307,324]. One does not need to store a cached power for each
    // k in this range. For each such k it suffices to find a cached power
    // such that the exponent of the product lies in [alpha,gamma].
    // This implies that the difference of the decimal exponents of adjacent
    // table entries must be less than or equal to
    //
    //      floor( (gamma - alpha) * log_10(2) ) = 8.
    //
    // (A smaller distance gamma-alpha would require a larger table.)

    // NB:
    // Actually this function returns c, such that -60 <= e_c + e + 64 <= -34.

    constexpr int kCachedPowersSize         =   79;
    constexpr int kCachedPowersMinDecExp    = -300;
    constexpr int kCachedPowersDecExpStep   =    8;

    static constexpr CachedPower kCachedPowers[] = {
        { 0xAB70FE17C79AC6CA, -1060, -300 },
        { 0xFF77B1FCBEBCDC4F, -1034, -292 },
        { 0xBE5691EF416BD60C, -1007, -284 },
        { 0x8DD01FAD907FFC3C,  -980, -276 },
        { 0xD3515C2831559A83,  -954, -268 },
        { 0x9D71AC8FADA6C9B5,  -927, -260 },
        { 0xEA9C227723EE8BCB,  -901, -252 },
        { 0xAECC49914078536D,  -874, -244 },
        { 0x823C12795DB6CE57,  -847, -236 },
        { 0xC21094364DFB5637,  -821, -228 },
        { 0x9096EA6F3848984F,  -794, -220 },
        { 0xD77485CB25823AC7,  -768, -212 },
        { 0xA086CFCD97BF97F4,  -741, -204 },
        { 0xEF340A98172AACE5,  -715, -196 },
        { 0xB23867FB2A35B28E,  -688, -188 },
        { 0x84C8D4DFD2C63F3B,  -661, -180 },
        { 0xC5DD44271AD3CDBA,  -635, -172 },
        { 0x936B9FCEBB25C996,  -608, -164 },
        { 0xDBAC6C247D62A584,  -582, -156 },
        { 0xA3AB66580D5FDAF6,  -555, -148 },
        { 0xF3E2F893DEC3F126,  -529, -140 },
        { 0xB5B5ADA8AAFF80B8,  -502, -132 },
        { 0x87625F056C7C4A8B,  -475, -124 },
        { 0xC9BCFF6034C13053,  -449, -116 },
        { 0x964E858C91BA2655,  -422, -108 },
        { 0xDFF9772470297EBD,  -396, -100 },
        { 0xA6DFBD9FB8E5B88F,  -369,  -92 },
        { 0xF8A95FCF88747D94,  -343,  -84 },
        { 0xB94470938FA89BCF,  -316,  -76 },
        { 0x8A08F0F8BF0F156B,  -289,  -68 },
        { 0xCDB02555653131B6,  -263,  -60 },
        { 0x993FE2C6D07B7FAC,  -236,  -52 },
        { 0xE45C10C42A2B3B06,  -210,  -44 },
        { 0xAA242499697392D3,  -183,  -36 },
        { 0xFD87B5F28300CA0E,  -157,  -28 },
        { 0xBCE5086492111AEB,  -130,  -20 },
        { 0x8CBCCC096F5088CC,  -103,  -12 },
        { 0xD1B71758E219652C,   -77,   -4 },
        { 0x9C40000000000000,   -50,    4 },
        { 0xE8D4A51000000000,   -24,   12 },
        { 0xAD78EBC5AC620000,     3,   20 },
        { 0x813F3978F8940984,    30,   28 },
        { 0xC097CE7BC90715B3,    56,   36 },
        { 0x8F7E32CE7BEA5C70,    83,   44 },
        { 0xD5D238A4ABE98068,   109,   52 },
        { 0x9F4F2726179A2245,   136,   60 },
        { 0xED63A231D4C4FB27,   162,   68 },
        { 0xB0DE65388CC8ADA8,   189,   76 },
        { 0x83C7088E1AAB65DB,   216,   84 },
        { 0xC45D1DF942711D9A,   242,   92 },
        { 0x924D692CA61BE758,   269,  100 },
        { 0xDA01EE641A708DEA,   295,  108 },
        { 0xA26DA3999AEF774A,   322,  116 },
        { 0xF209787BB47D6B85,   348,  124 },
        { 0xB454E4A179DD1877,   375,  132 },
        { 0x865B86925B9BC5C2,   402,  140 },
        { 0xC83553C5C8965D3D,   428,  148 },
        { 0x952AB45CFA97A0B3,   455,  156 },
        { 0xDE469FBD99A05FE3,   481,  164 },
        { 0xA59BC234DB398C25,   508,  172 },
        { 0xF6C69A72A3989F5C,   534,  180 },
        { 0xB7DCBF5354E9BECE,   561,  188 },
        { 0x88FCF317F22241E2,   588,  196 },
        { 0xCC20CE9BD35C78A5,   614,  204 },
        { 0x98165AF37B2153DF,   641,  212 },
        { 0xE2A0B5DC971F303A,   667,  220 },
        { 0xA8D9D1535CE3B396,   694,  228 },
        { 0xFB9B7CD9A4A7443C,   720,  236 },
        { 0xBB764C4CA7A44410,   747,  244 },
        { 0x8BAB8EEFB6409C1A,   774,  252 },
        { 0xD01FEF10A657842C,   800,  260 },
        { 0x9B10A4E5E9913129,   827,  268 },
        { 0xE7109BFBA19C0C9D,   853,  276 },
        { 0xAC2820D9623BF429,   880,  284 },
        { 0x80444B5E7AA7CF85,   907,  292 },
        { 0xBF21E44003ACDD2D,   933,  300 },
        { 0x8E679C2F5E44FF8F,   960,  308 },
        { 0xD433179D9C8CB841,   986,  316 },
        { 0x9E19DB92B4E31BA9,  1013,  324 },
    };

    // This computation gives exactly the same results for k as
    //      k = ceil((kAlpha - e - 1) * 0.30102999566398114)
    // for |e| <= 1500, but doesn't require floating-point operations.
    // NB: log_10(2) ~= 78913 / 2^18
    assert(e >= -1500);
    assert(e <=  1500);
    const int f = kAlpha - e - 1;
    const int k = (f * 78913) / (1 << 18) + (f > 0);

    const int index = (-kCachedPowersMinDecExp + k + (kCachedPowersDecExpStep - 1)) / kCachedPowersDecExpStep;
    assert(index >= 0);
    assert(index < kCachedPowersSize);
    static_cast<void>(kCachedPowersSize);

    const CachedPower cached = kCachedPowers[index];
    assert(kAlpha <= cached.e + e + 64);
    assert(kGamma >= cached.e + e + 64);

    return cached;
}

static void Grisu2Round(char* buf, int len, uint64_t dist, uint64_t delta, uint64_t rest, uint64_t ten_k)
{
    assert(len >= 1);
    assert(dist <= delta);
    assert(rest <= delta);
    assert(ten_k > 0);

    //               <--------------------------- delta ---->
    //                                  <---- dist --------->
    // --------------[------------------+-------------------]--------------
    //               M-                 w                   M+
    //
    //                                  ten_k
    //                                <------>
    //                                       <---- rest ---->
    // --------------[------------------+----+--------------]--------------
    //                                  w    V
    //                                       = buf * 10^k
    //
    // ten_k represents a unit-in-the-last-place in the decimal representation
    // stored in buf.
    // Decrement buf by ten_k while this takes buf closer to w.

    // The tests are written in this order to avoid overflow in unsigned
    // integer arithmetic.

    while (rest < dist
        && delta - rest >= ten_k
        && (rest + ten_k < dist || dist - rest > rest + ten_k - dist))
    {
        assert(buf[len - 1] != '0');
        buf[len - 1]--;
        rest += ten_k;
    }
}

// Generates V = buffer * 10^decimal_exponent, such that M- <= V <= M+.
// M- and M+ must be normalized and share the same exponent -60 <= e <= -32.
static void Grisu2DigitGen(char* buffer, int& length, int& decimal_exponent, DiyFp M_minus, DiyFp w, DiyFp M_plus)
{
    static_assert(DiyFp::kPrecision == 64, "invalid config");
    static_assert(kAlpha >= -60, "invalid config");
    static_assert(kGamma <= -32, "invalid config");

    // Generates the digits (and the exponent) of a decimal floating-point
    // number V = buffer * 10^decimal_exponent in the range [M-, M+]. The DiyFp's
    // w, M- and M+ share the same exponent e, which satisfies alpha <= e <= gamma.
    //
    //               <--------------------------- delta ---->
    //                                  <---- dist --------->
    // --------------[------------------+-------------------]--------------
    //               M-                 w                   M+
    //
    // Grisu2 generates the digits of M+ from left to right and stops as soon as
    // V is in [M-,M+].

    assert(M_plus.e >= kAlpha);
    assert(M_plus.e <= kGamma);

    uint64_t delta = DiyFp::sub(M_plus, M_minus).f; // (significand of (M+ - M-), implicit exponent is e)
    uint64_t dist  = DiyFp::sub(M_plus, w      ).f; // (significand of (M+ - w ), implicit exponent is e)

    // Split M+ = f * 2^e into two parts p1 and p2 (note: e < 0):
    //
    //      M+ = f * 2^e
    //         = ((f div 2^-e) * 2^-e + (f mod 2^-e)) * 2^e
    //         = ((p1        ) * 2^-e + (p2        )) * 2^e
    //         = p1 + p2 * 2^e

    const DiyFp one(uint64_t{1} << -M_plus.e, M_plus.e);

    uint32_t p1 = static_cast<uint32_t>(M_plus.f >> -one.e); // p1 = f div 2^-e (Since -e >= 32, p1 fits into a 32-bit int.)
    uint64_t p2 = M_plus.f & (one.f - 1);                    // p2 = f mod 2^-e

    // 1)
    //
    // Generate the digits of the integral part p1 = d[n-1]...d[1]d[0]

    assert(p1 > 0);

    //      10^(k-1) <= p1 < 10^k, pow10 = 10^(k-1)
    //
    //      p1 = (p1 div 10^(k-1)) * 10^(k-1) + (p1 mod 10^(k-1))
    //         = (d[k-1]         ) * 10^(k-1) + (p1 mod 10^(k-1))
    //
    //      M+ = p1                                             + p2 * 2^e
    //         = d[k-1] * 10^(k-1) + (p1 mod 10^(k-1))          + p2 * 2^e
    //         = d[k-1] * 10^(k-1) + ((p1 mod 10^(k-1)) * 2^-e + p2) * 2^e
    //         = d[k-1] * 10^(k-1) + (                         rest) * 2^e
    //
    // Now generate the digits d[n] of p1 from left to right (n = k-1,...,0)
    //
    //      p1 = d[k-1]...d[n] * 10^n + d[n-1]...d[0]
    //
    // but stop as soon as
    //
    //      rest * 2^e = (d[n-1]...d[0] * 2^-e + p2) * 2^e <= delta * 2^e

    // The common case is that all the digits of p1 are needed.
    // Optimize for this case and correct later if required.
    char const* last = iconv::U32toa(buffer, p1);
    length = static_cast<int>(last - buffer);

    if (p2 <= delta)
    {
        // In this case: Too many digits of p1 might have been generated.
        //
        // Find the largest 0 <= n < k, such that
        //
        //      w+ = (p1 div 10^n) * 10^n + ((p1 mod 10^n) * 2^-e + p2) * 2^e
        //         = (p1 div 10^n) * 10^n + (                     rest) * 2^e
        //
        // and rest <= delta.
        //
        // Compute rest * 2^e = w+ mod 10^n = p1 + p2 * 2^e = (p1 * 2^-e + p2) * 2^e
        // and check if enough digits have been generated:
        //
        //      rest * 2^e <= delta * 2^e
        //
        // This test can be slightly simplified, since
        //
        //      rest = (p1 mod 10^n) * 2^-e + p2 <= delta
        //      <==>    r * 2^-e + p2 <= delta
        //      <==>    r * 2^-e      <= delta - p2 = D = D1 * 2^-e + D2
        //      <==>    r < D1 or (r == D1 and 0 <= D2)
        //      <==>    r <= D1
        //

        const uint32_t D1 = static_cast<uint32_t>((delta - p2) >> -one.e);

        int k = length;
        int n = 0;

        uint32_t r = 0;
        uint32_t pow10 = 1; // 10^n
        for (;;)
        {
            assert(k >= n + 1);
            assert(r <= D1);
            assert(n <= 9);
            assert(static_cast<uint32_t>(buffer[k - (n + 1)] - '0') <= UINT32_MAX / pow10);

            const uint32_t r_next = pow10 * static_cast<uint32_t>(buffer[k - (n + 1)] - '0') + r;
            if (r_next > D1)
                break;
            r = r_next;
            n++;
            pow10 *= 10;
        }
        length = k - n;

        // V = buffer * 10^n, with M- <= V <= M+.

        decimal_exponent += n;

        const uint64_t rest = (uint64_t{r} << -one.e) + p2;
        assert(rest <= delta);

        // We may now just stop. But instead look if the buffer could be
        // decremented to bring V closer to w.
        //
        // pow10 = 10^n is now 1 ulp in the decimal representation V.
        // The rounding procedure works with DiyFp's with an implicit
        // exponent of e.
        //
        //      10^n = (10^n * 2^-e) * 2^e = ulp * 2^e
        //
        const uint64_t ten_n = uint64_t{pow10} << -one.e;
        Grisu2Round(buffer, length, dist, delta, rest, ten_n);

        return;
    }

    // 2)
    //
    // The digits of the integral part have been generated:
    //
    //      M+ = d[k-1]...d[1]d[0] + p2 * 2^e
    //         = buffer            + p2 * 2^e
    //
    // Now generate the digits of the fractional part p2 * 2^e.
    //
    // Note:
    // No decimal point is generated: the exponent is adjusted instead.
    //
    // p2 actually represents the fraction
    //
    //      p2 * 2^e
    //          = p2 / 2^-e
    //          = d[-1] / 10^1 + d[-2] / 10^2 + ...
    //
    // Now generate the digits d[-m] of p1 from left to right (m = 1,2,...)
    //
    //      p2 * 2^e = d[-1]d[-2]...d[-m] * 10^-m
    //                      + 10^-m * (d[-m-1] / 10^1 + d[-m-2] / 10^2 + ...)
    //
    // using
    //
    //      10^m * p2 = ((10^m * p2) div 2^-e) * 2^-e + ((10^m * p2) mod 2^-e)
    //                = (                   d) * 2^-e + (                   r)
    //
    // or
    //      10^m * p2 * 2^e = d + r * 2^e
    //
    // i.e.
    //
    //      M+ = buffer + p2 * 2^e
    //         = buffer + 10^-m * (d + r * 2^e)
    //         = (buffer * 10^m + d) * 10^-m + 10^-m * r * 2^e
    //
    // and stop as soon as 10^-m * r * 2^e <= delta * 2^e

    assert(p2 > delta);

    int m = 0;
    for (;;)
    {
        // Invariant:
        //      M+ = buffer * 10^-m + 10^-m * (d[-m-1] / 10 + d[-m-2] / 10^2 + ...) * 2^e
        //         = buffer * 10^-m + 10^-m * (p2                                 ) * 2^e
        //         = buffer * 10^-m + 10^-m * (1/10 * (10 * p2)                   ) * 2^e
        //         = buffer * 10^-m + 10^-m * (1/10 * ((10*p2 div 2^-e) * 2^-e + (10*p2 mod 2^-e)) * 2^e
        //
        assert(p2 <= UINT64_MAX / 10);
        p2 *= 10;
        const uint64_t d = p2 >> -one.e;     // d = (10 * p2) div 2^-e
        const uint64_t r = p2 & (one.f - 1); // r = (10 * p2) mod 2^-e
        //
        //      M+ = buffer * 10^-m + 10^-m * (1/10 * (d * 2^-e + r) * 2^e
        //         = buffer * 10^-m + 10^-m * (1/10 * (d + r * 2^e))
        //         = (buffer * 10 + d) * 10^(-m-1) + 10^(-m-1) * r * 2^e
        //
        assert(d <= 9);
        buffer[length++] = static_cast<char>('0' + d); // buffer := buffer * 10 + d
        //
        //      M+ = buffer * 10^(-m-1) + 10^(-m-1) * r * 2^e
        //
        p2 = r;
        m++;
        //
        //      M+ = buffer * 10^-m + 10^-m * p2 * 2^e
        // Invariant restored.

        // Check if enough digits have been generated.
        //
        //      10^-m * p2 * 2^e <= delta * 2^e
        //              p2 * 2^e <= 10^m * delta * 2^e
        //                    p2 <= 10^m * delta
        delta *= 10;
        dist  *= 10;
        if (p2 <= delta)
        {
            break;
        }
    }

    // V = buffer * 10^-m, with M- <= V <= M+.

    decimal_exponent -= m;

    // 1 ulp in the decimal representation is now 10^-m.
    // Since delta and dist are now scaled by 10^m, we need to do the
    // same with ulp in order to keep the units in sync.
    //
    //      10^m * 10^-m = 1 = 2^-e * 2^e = ten_m * 2^e
    //
    const uint64_t ten_m = one.f;
    Grisu2Round(buffer, length, dist, delta, p2, ten_m);

    // By construction this algorithm generates the shortest possible decimal
    // number (Loitsch, Theorem 6.2) which rounds back to w.
    // For an input number of precision p, at least
    //
    //      N = 1 + ceil(p * log_10(2))
    //
    // decimal digits are sufficient to identify all binary floating-point
    // numbers (Matula, "In-and-Out conversions").
    // This implies that the algorithm does not produce more than N decimal
    // digits.
    //
    //      N = 17 for p = 53 (IEEE double precision)
    //      N = 9  for p = 24 (IEEE single precision)
}

// v = buf * 10^decimal_exponent
// len is the length of the buffer (number of decimal digits)
// The buffer must be large enough, i.e. >= max_digits10.
static void Grisu2(char* buf, int& len, int& decimal_exponent, DiyFp m_minus, DiyFp v, DiyFp m_plus)
{
    assert(m_plus.e == m_minus.e);
    assert(m_plus.e == v.e);

    //  --------(-----------------------+-----------------------)--------    (A)
    //          m-                      v                       m+
    //
    //  --------------------(-----------+-----------------------)--------    (B)
    //                      m-          v                       m+
    //
    // First scale v (and m- and m+) such that the exponent is in the range
    // [alpha, gamma].

    const CachedPower cached = GetCachedPowerForBinaryExponent(m_plus.e);

    const DiyFp c_minus_k(cached.f, cached.e); // = c ~= 10^-k

    // The exponent of the products is = v.e + c_minus_k.e + q and is in the range [alpha,gamma]
    const DiyFp w       = DiyFp::mul(v,       c_minus_k);
    const DiyFp w_minus = DiyFp::mul(m_minus, c_minus_k);
    const DiyFp w_plus  = DiyFp::mul(m_plus,  c_minus_k);

    //  ----(---+---)---------------(---+---)---------------(---+---)----
    //          w-                      w                       w+
    //          = c*m-                  = c*v                   = c*m+
    //
    // DiyFp::mul rounds its result and c_minus_k is approximated too. w, w- and
    // w+ are now off by a small amount.
    // In fact:
    //
    //      w - v * 10^k < 1 ulp
    //
    // To account for this inaccuracy, add resp. subtract 1 ulp.
    //
    //  --------+---[---------------(---+---)---------------]---+--------
    //          w-  M-                  w                   M+  w+
    //
    // Now any number in [M-, M+] (bounds included) will round to w when input,
    // regardless of how the input rounding algorithm breaks ties.
    //
    // And digit_gen generates the shortest possible such number in [M-, M+].
    // Note that this does not mean that Grisu2 always generates the shortest
    // possible number in the interval (m-, m+).
    const DiyFp M_minus(w_minus.f + 1, w_minus.e);
    const DiyFp M_plus (w_plus.f  - 1, w_plus.e );

    decimal_exponent = -cached.k; // = -(-k) = k

    Grisu2DigitGen(buf, len, decimal_exponent, M_minus, w, M_plus);
}

// v = buf * 10^decimal_exponent
// len is the length of the buffer (number of decimal digits)
// The buffer must be large enough, i.e. >= max_digits10.
template <typename FloatType>
static void Grisu2(char* buf, int& len, int& decimal_exponent, FloatType value)
{
    assert(std::isfinite(value));
    assert(value > 0);

#if 0
    // If the neighbors (and boundaries) of 'value' are always computed for
    // double-precision numbers, all float's can be recovered using strtod
    // (and strtof). However, the resulting decimal representations are not
    // exactly "short".
    //
    // If the neighbors are computed for single-precision numbers, there is a
    // single float (7.0385307e-26f) which can't be recovered using strtod.
    // (The resulting double precision is off by 1 ulp.)
    const Boundaries w = ComputeBoundaries(static_cast<double>(value));
#else
    const Boundaries w = ComputeBoundaries(value);
#endif

    Grisu2(buf, len, decimal_exponent, w.minus, w.w, w.plus);
}

// Appends a decimal representation of e to buf.
// Returns a pointer to the element following the exponent.
// PRE: -1000 < e < 1000
static char* AppendExponent(char* buf, int e)
{
    assert(e > -1000);
    assert(e <  1000);

    if (e < 0)
    {
        e = -e;
        *buf++ = '-';
    }
    else
    {
        *buf++ = '+';
    }

    uint32_t k = static_cast<uint32_t>(e);
    if (k < 10)
    {
        buf[0] = static_cast<char>('0' + k);
        buf++;
    }
    else if (k < 100)
    {
        buf = iconv::Itoa100(buf, k);
    }
    else
    {
        uint32_t q = k / 100;
        uint32_t r = k % 100;
        buf[0] = static_cast<char>('0' + q);
        buf = iconv::Itoa100(buf + 1, r);
    }

    return buf;
}

// Prettify v = buf * 10^decimal_exponent
// If v is in the range [10^min_exp, 10^max_exp) it will be printed in fixed-point notation.
// Otherwise it will be printed in exponential notation.
// PRE: min_exp < 0
// PRE: max_exp > 0
static char* FormatBuffer(char* buf, int len, int decimal_exponent, int min_exp, int max_exp)
{
    assert(min_exp < 0);
    assert(max_exp > 0);

    int k = len;
    int n = len + decimal_exponent;
    // v = buf * 10^(n-k)
    // k is the length of the buffer (number of decimal digits)
    // n is the position of the decimal point relative to the start of the buffer.

    if (k <= n && n <= max_exp)
    {
        // digits[000]
        // len <= max_exp

        std::memset(buf + k, '0', static_cast<size_t>(n - k));
        buf[n++] = '.';
        buf[n++] = '0';
        return buf + n;
    }

    if (0 < n && n <= max_exp)
    {
        // dig.its
        // len <= max_digits10 + 1

        assert(k > n);

        std::memmove(buf + (n + 1), buf + n, static_cast<size_t>(k - n));
        buf[n] = '.';
        return buf + (k + 1);
    }

    if (min_exp < n && n <= 0)
    {
        // 0.[000]digits
        // len <= 2 + (-min_exp - 1) + max_digits10

        std::memmove(buf + (2 + -n), buf, static_cast<size_t>(k));
        buf[0] = '0';
        buf[1] = '.';
        std::memset(buf + 2, '0', static_cast<size_t>(-n));
        return buf + (2 + (-n) + k);
    }

    if (k == 1)
    {
        // dE+123
        // len <= 1 + 5

        buf += 1;
    }
    else
    {
        // d.igitsE+123
        // len <= max_digits10 + 1 + 5

        std::memmove(buf + 2, buf + 1, static_cast<size_t>(k - 1));
        buf[1] = '.';
        buf += 1 + k;
    }

    *buf++ = 'e';
    return AppendExponent(buf, n - 1);
}

// Generates a decimal representation of the floating-point number 'value' in
// [first, last).
//
// The input number must be finite, i.e. NaN's and Inf's are not supported.
// The buffer must be large enough.
//
// The result is _not_ null-terminated.
template <typename FloatType>
static char* DtoaShort(char* first, char* last, FloatType value)
{
    static_assert(DiyFp::kPrecision >= std::numeric_limits<FloatType>::digits + 3, "Not enough precision");

    static_cast<void>(last); // maybe unused - fix warning
    assert(std::isfinite(value));

    // Use signbit(value) instead of (value < 0) since signbit works for -0.
    if (std::signbit(value))
    {
        value = -value;
        *first++ = '-';
    }

    if (value == 0)
    {
        *first++ = '0';
        *first++ = '.';
        *first++ = '0';
        return first;
    }

    assert(last - first >= std::numeric_limits<FloatType>::max_digits10);

    // Compute v = buffer * 10^decimal_exponent.
    // The decimal digits are stored in the buffer, which needs to be interpreted
    // as an unsigned decimal integer.
    // len is the length of the buffer, i.e. the number of decimal digits.
    int len = 0;
    int decimal_exponent = 0;
    Grisu2(first, len, decimal_exponent, value);

    assert(len <= std::numeric_limits<FloatType>::max_digits10);

    constexpr int kMinExp = -6;
    constexpr int kMaxExp = 21;

    assert(last - first >= kMaxExp);
    assert(last - first >= 2 + (-kMinExp - 1) + std::numeric_limits<FloatType>::max_digits10);
    assert(last - first >= std::numeric_limits<FloatType>::max_digits10 + 6);

    return FormatBuffer(first, len, decimal_exponent, kMinExp, kMaxExp);
}

} // namespace dconv

static char* Dtoa(char* next, char* last, double value)
{
    return dconv::DtoaShort(next, last, value);
}

//==================================================================================================
// Unicode support
//==================================================================================================

namespace {
namespace unicode {

using Codepoint = uint32_t;

static constexpr Codepoint const kInvalidCodepoint = 0xFFFFFFFF;
//static constexpr Codepoint const kReplacementCharacter = 0xFFFD;

enum class ErrorCode {
    success,
    insufficient_data,
    invalid_codepoint,
    invalid_lead_byte,
    invalid_continuation_byte,
    overlong_sequence,
    invalid_high_surrogate,
    invalid_low_surrogate,
};

struct DecodedCodepoint
{
    Codepoint U;
    ErrorCode ec;
};

static bool IsValidCodePoint(Codepoint U)
{
    // 1. Characters with values greater than 0x10FFFF cannot be encoded in
    //    UTF-16.
    // 2. Values between 0xD800 and 0xDFFF are specifically reserved for use
    //    with UTF-16, and don't have any characters assigned to them.
    return U <= 0x10FFFF && (U < 0xD800 || U > 0xDFFF);
}

static int GetUTF8SequenceLengthFromCodepoint(Codepoint U)
{
    assert(IsValidCodePoint(U));

    if (U <= 0x7F) return 1;
    if (U <= 0x7FF) return 2;
    if (U <= 0xFFFF) return 3;
    return 4;
}

static bool IsUTF8OverlongSequence(Codepoint U, int slen)
{
    return slen != GetUTF8SequenceLengthFromCodepoint(U);
}

static int GetUTF8SequenceLengthFromLeadByte(char ch, Codepoint& U)
{
    uint8_t const b = static_cast<uint8_t>(ch);

    if (b < 0x80) { U = b;        return 1; }
    if (b < 0xC0) {               return 0; }
    if (b < 0xE0) { U = b & 0x1F; return 2; }
    if (b < 0xF0) { U = b & 0x0F; return 3; }
    if (b < 0xF8) { U = b & 0x07; return 4; }
    return 0;
}

#if JSON_SKIP_INVALID_UNICODE
static int GetUTF8SequenceLengthFromLeadByte(char ch)
{
    uint8_t const b = static_cast<uint8_t>(ch);

    if (b < 0x80) return 1;
    if (b < 0xC0) return 0; // 10xxxxxx, i.e. continuation byte
    if (b < 0xE0) return 2;
    if (b < 0xF0) return 3;
    if (b < 0xF8) return 4;
    return 0; // invalid
}

static bool IsUTF8LeadByte(char ch)
{
    return 0 != GetUTF8SequenceLengthFromLeadByte(ch);
}

static char const* FindNextUTF8LeadByte(char const* first, char const* last)
{
    for ( ; first != last && !IsUTF8LeadByte(*first); ++first)
    {
    }

    return first;
}
#endif

static bool IsUTF8ContinuationByte(char ch)
{
    uint8_t const b = static_cast<uint8_t>(ch);

    return 0x80 == (b & 0xC0); // b == 10xxxxxx ???
}

static DecodedCodepoint DecodeUTF8Sequence(char const*& next, char const* last)
{
    //
    // Char. number range  |        UTF-8 octet sequence
    //    (hexadecimal)    |              (binary)
    // --------------------+---------------------------------------------
    // 0000 0000-0000 007F | 0xxxxxxx
    // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    //
    // Decoding a UTF-8 character proceeds as follows:
    //
    // 1.  Initialize a binary number with all bits set to 0.  Up to 21 bits
    // may be needed.
    //
    // 2.  Determine which bits encode the character number from the number
    // of octets in the sequence and the second column of the table
    // above (the bits marked x).
    //
    // 3.  Distribute the bits from the sequence to the binary number, first
    // the lower-order bits from the last octet of the sequence and
    // proceeding to the left until no x bits are left.  The binary
    // number is now equal to the character number.
    //

    if (next == last)
        return { kInvalidCodepoint, ErrorCode::insufficient_data };

    Codepoint U = 0;
    const int slen = GetUTF8SequenceLengthFromLeadByte(*next, U);
    ++next;

    if (slen == 0)
        return { kInvalidCodepoint, ErrorCode::invalid_lead_byte };
    if (last - next < slen - 1)
        return { kInvalidCodepoint, ErrorCode::insufficient_data };

    const auto end = next + (slen - 1);
    for ( ; next != end; ++next)
    {
        if (!IsUTF8ContinuationByte(*next))
            return { U, ErrorCode::invalid_continuation_byte };

        U = (U << 6) | (static_cast<uint8_t>(*next) & 0x3F);
    }

    //
    // Implementations of the decoding algorithm above MUST protect against
    // decoding invalid sequences.  For instance, a naive implementation may
    // decode the overlong UTF-8 sequence C0 80 into the character U+0000,
    // or the surrogate pair ED A1 8C ED BE B4 into U+233B4.  Decoding
    // invalid sequences may have security consequences or cause other
    // problems.
    //

    if (!IsValidCodePoint(U))
        return { U, ErrorCode::invalid_codepoint };
    if (IsUTF8OverlongSequence(U, slen))
        return { U, ErrorCode::overlong_sequence };

    return { U, ErrorCode::success };
}

template <typename Put8>
static bool EncodeUTF8(Codepoint U, Put8 put)
{
    assert(IsValidCodePoint(U));

    //
    // Char. number range  |        UTF-8 octet sequence
    //    (hexadecimal)    |              (binary)
    // --------------------+---------------------------------------------
    // 0000 0000-0000 007F | 0xxxxxxx
    // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    //
    // Encoding a character to UTF-8 proceeds as follows:
    //
    // 1.  Determine the number of octets required from the character number
    // and the first column of the table above.  It is important to note
    // that the rows of the table are mutually exclusive, i.e., there is
    // only one valid way to encode a given character.
    //
    // 2.  Prepare the high-order bits of the octets as per the second
    // column of the table.
    //
    // 3.  Fill in the bits marked x from the bits of the character number,
    // expressed in binary.  Start by putting the lowest-order bit of
    // the character number in the lowest-order position of the last
    // octet of the sequence, then put the next higher-order bit of the
    // character number in the next higher-order position of that octet,
    // etc.  When the x bits of the last octet are filled in, move on to
    // the next to last octet, then to the preceding one, etc. until all
    // x bits are filled in.
    //

    if (U <= 0x7F)
    {
        return put(static_cast<char>(U));
    }
    else if (U <= 0x7FF)
    {
        return put(static_cast<char>(0xC0 | ((U >> 6)       )))
            && put(static_cast<char>(0x80 | ((U     ) & 0x3F)));
    }
    else if (U <= 0xFFFF)
    {
        return put(static_cast<char>(0xE0 | ((U >> 12)       )))
            && put(static_cast<char>(0x80 | ((U >>  6) & 0x3F)))
            && put(static_cast<char>(0x80 | ((U      ) & 0x3F)));
    }
    else /*if (U <= 0x10FFFF)*/
    {
        return put(static_cast<char>(0xF0 | ((U >> 18)       )))
            && put(static_cast<char>(0x80 | ((U >> 12) & 0x3F)))
            && put(static_cast<char>(0x80 | ((U >>  6) & 0x3F)))
            && put(static_cast<char>(0x80 | ((U      ) & 0x3F)));
    }
}

template <typename Get16>
static DecodedCodepoint DecodeUTF16Sequence(Get16 get)
{
    //
    // Decoding of a single character from UTF-16 to an ISO 10646 character
    // value proceeds as follows. Let W1 be the next 16-bit integer in the
    // sequence of integers representing the text. Let W2 be the (eventual)
    // next integer following W1.
    //

    uint16_t W1 = 0;

    if (!get(W1))
        return { 0, ErrorCode::insufficient_data };

    //
    // 1) If W1 < 0xD800 or W1 > 0xDFFF, the character value U is the value
    // of W1. Terminate.
    //

    if (W1 < 0xD800 || W1 > 0xDFFF)
        return { uint32_t{W1}, ErrorCode::success };

    //
    // 2) Determine if W1 is between 0xD800 and 0xDBFF. If not, the sequence
    // is in error and no valid character can be obtained using W1.
    // Terminate.
    //

    if (W1 > 0xDBFF)
        return { uint32_t{W1}, ErrorCode::invalid_high_surrogate };

    //
    // 3) If there is no W2 (that is, the sequence ends with W1), or if W2
    // is not between 0xDC00 and 0xDFFF, the sequence is in error.
    // Terminate.
    //

    uint16_t W2 = 0;

    if (!get(W2))
        return { uint32_t{W1}, ErrorCode::insufficient_data };

    if (W2 < 0xDC00 || W2 > 0xDFFF)
        return { uint32_t{W2}, ErrorCode::invalid_low_surrogate }; // Note: returns W2

    //
    // 4) Construct a 20-bit unsigned integer U', taking the 10 low-order
    // bits of W1 as its 10 high-order bits and the 10 low-order bits of
    // W2 as its 10 low-order bits.
    //
    //
    // 5) Add 0x10000 to U' to obtain the character value U. Terminate.
    //

    Codepoint const Up = ((Codepoint{W1} & 0x3FF) << 10) | (Codepoint{W2} & 0x3FF);
    Codepoint const U = Up + 0x10000;

    return { U, ErrorCode::success };
}

} // namespace unicode
} // namespace

//==================================================================================================
// Value
//==================================================================================================

Value const Value::kUndefined = {};

Value::Value(Value const& rhs)
{
    switch (rhs.type_)
    {
    case Type::undefined:
    case Type::null:
    case Type::boolean:
    case Type::number:
        data_ = rhs.data_;
        type_ = rhs.type_;
        break;
    case Type::string:
        data_.string = new String(*rhs.data_.string);
        type_ = Type::string;
        break;
    case Type::array:
        data_.array = new Array(*rhs.data_.array);
        type_ = Type::array;
        break;
    case Type::object:
        data_.object = new Object(*rhs.data_.object);
        type_ = Type::object;
        break;
    }
}

Value& Value::operator=(Value const& rhs)
{
    if (this != &rhs)
    {
        switch (rhs.type())
        {
        case Type::undefined:
            assign(undefined_tag);
            break;
        case Type::null:
            assign(null_tag);
            break;
        case Type::boolean:
            assign(boolean_tag, rhs.get_boolean());
            break;
        case Type::number:
            assign(number_tag, rhs.get_number());
            break;
        case Type::string:
            assign(string_tag, rhs.get_string());
            break;
        case Type::array:
            assign(array_tag, rhs.get_array());
            break;
        case Type::object:
            assign(object_tag, rhs.get_object());
            break;
        }
    }

    return *this;
}

Value::Value(Type t)
{
    switch (t)
    {
    case Type::undefined:
    case Type::null:
        break;
    case Type::boolean:
        data_.boolean = {};
        break;
    case Type::number:
        data_.number = {};
        break;
    case Type::string:
        data_.string = new String{};
        break;
    case Type::array:
        data_.array = new Array{};
        break;
    case Type::object:
        data_.object = new Object{};
        break;
    default:
        assert(false && "invalid type"); // LCOV_EXCL_LINE
        t = Type::undefined;
        break;
    }

    type_ = t; // Don't move to constructor initializer list!
}

void Value::assign(Tag_undefined) noexcept
{
    _clear();

    type_ = Type::undefined;
}

void Value::assign(Tag_null, Null) noexcept
{
    _clear();

    type_ = Type::null;
}

bool& Value::assign(Tag_boolean, bool v) noexcept
{
    _clear();

    data_.boolean = v;
    type_ = Type::boolean;

    return get_boolean();
}

double& Value::assign(Tag_number, double v) noexcept
{
    _clear();

    data_.number = v;
    type_ = Type::number;

    return get_number();
}

String& Value::assign(Tag_string)
{
    return _assign_string(String{});
}

String& Value::assign(Tag_string, String const& v)
{
    return _assign_string(v);
}

String& Value::assign(Tag_string, String&& v)
{
    return _assign_string(std::move(v));
}

Array& Value::assign(Tag_array)
{
    return _assign_array(Array{});
}

Array& Value::assign(Tag_array, Array const& v)
{
    return _assign_array(v);
}

Array& Value::assign(Tag_array, Array&& v)
{
    return _assign_array(std::move(v));
}

Object& Value::assign(Tag_object)
{
    return _assign_object(Object{});
}

Object& Value::assign(Tag_object, Object const& v)
{
    return _assign_object(v);
}

Object& Value::assign(Tag_object, Object&& v)
{
    return _assign_object(std::move(v));
}

void Value::_clear()
{
    switch (type_)
    {
    case Type::undefined:
    case Type::null:
    case Type::boolean:
    case Type::number:
        break;
    case Type::string:
        delete data_.string;
        break;
    case Type::array:
        delete data_.array;
        break;
    case Type::object:
        delete data_.object;
        break;
    }

    type_ = Type::undefined;
}

template <typename T>
String& Value::_assign_string(T&& value)
{
    switch (type_)
    {
    case Type::undefined:
    case Type::null:
    case Type::boolean:
    case Type::number:
        data_.string = new String(std::forward<T>(value));
        type_ = Type::string;
        break;
    case Type::string:
        *data_.string = std::forward<T>(value);
        break;
    case Type::array:
        {
            auto p = new String(std::forward<T>(value));
            // noexcept ->
            delete data_.array;
            data_.string = p;
            type_ = Type::string;
        }
        break;
    case Type::object:
        {
            auto p = new String(std::forward<T>(value));
            // noexcept ->
            delete data_.object;
            data_.string = p;
            type_ = Type::string;
        }
        break;
    }

    return get_string();
}

template <typename T>
Array& Value::_assign_array(T&& value)
{
    switch (type_)
    {
    case Type::undefined:
    case Type::null:
    case Type::boolean:
    case Type::number:
        data_.array = new Array(std::forward<T>(value));
        type_ = Type::array;
        break;
    case Type::string:
        {
            auto p = new Array(std::forward<T>(value));
            // noexcept ->
            delete data_.string;
            data_.array = p;
            type_ = Type::array;
        }
        break;
    case Type::array:
        *data_.array = std::forward<T>(value);
        break;
    case Type::object:
        {
            auto p = new Array(std::forward<T>(value));
            // noexcept ->
            delete data_.object;
            data_.array = p;
            type_ = Type::array;
        }
        break;
    }

    return get_array();
}

template <typename T>
Object& Value::_assign_object(T&& value)
{
    switch (type_)
    {
    case Type::undefined:
    case Type::null:
    case Type::boolean:
    case Type::number:
        data_.object = new Object(std::forward<T>(value));
        type_ = Type::object;
        break;
    case Type::string:
        {
            auto p = new Object(std::forward<T>(value));
            // noexcept ->
            delete data_.string;
            data_.object = p;
            type_ = Type::object;
        }
        break;
    case Type::array:
        {
            auto p = new Object(std::forward<T>(value));
            // noexcept ->
            delete data_.array;
            data_.object = p;
            type_ = Type::object;
        }
        break;
    case Type::object:
        *data_.object = std::forward<T>(value);
        break;
    }

    return get_object();
}

bool Value::equal_to(Value const& rhs) const noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (is_undefined() || rhs.is_undefined())
        return false;
#endif

    if (this == &rhs)
        return true;

    if (type() != rhs.type())
        return false;

    switch (type())
    {
    case Type::undefined:
        return true;
    case Type::null:
        return true;
    case Type::boolean:
        return get_boolean() == rhs.get_boolean();
    case Type::number:
        return get_number() == rhs.get_number();
    case Type::string:
        return get_string() == rhs.get_string();
    case Type::array:
        return get_array() == rhs.get_array();
    case Type::object:
        return get_object() == rhs.get_object();
    default:
        assert(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

bool Value::less_than(Value const& rhs) const noexcept
{
#if JSON_VALUE_UNDEFINED_IS_UNORDERED
    if (is_undefined() || rhs.is_undefined())
        return false;
#endif

    if (this == &rhs)
        return false;

    if (type() < rhs.type())
        return true;
    if (type() > rhs.type())
        return false;

    switch (type())
    {
    case Type::undefined:
        return false;
    case Type::null:
        return false;
    case Type::boolean:
        return get_boolean() < rhs.get_boolean();
    case Type::number:
        return get_number() < rhs.get_number();
    case Type::string:
        return get_string() < rhs.get_string();
    case Type::array:
        return get_array() < rhs.get_array();
    case Type::object:
        return get_object() < rhs.get_object();
    default:
        assert(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

static size_t HashCombine(size_t h1, size_t h2) noexcept
{
    h1 ^= h2 + 0x9E3779B9 + (h1 << 6) + (h1 >> 2);
//  h1 ^= h2 + 0x9E3779B97F4A7C15 + (h1 << 6) + (h1 >> 2);
    return h1;
}

size_t Value::hash() const noexcept
{
    switch (type())
    {
    case Type::undefined:
        assert(false && "cannot compute hash value for 'undefined'");
        return std::numeric_limits<size_t>::max(); // -1
    case Type::null:
        return 0;
    case Type::boolean:
        return std::hash<bool>()(get_boolean());
    case Type::number:
        return std::hash<double>()(get_number());
    case Type::string:
        return std::hash<String>()(get_string());
    case Type::array:
        {
            size_t h = std::hash<char>()('['); // initial value for empty arrays
            for (auto const& v : get_array())
            {
                h = HashCombine(h, v.hash());
            }
            return h;
        }
    case Type::object:
        {
            size_t h = std::hash<char>()('{'); // initial value for empty objects
            for (auto const& v : get_object())
            {
                auto const h1 = std::hash<String>()(v.first);
                auto const h2 = v.second.hash();
                h ^= HashCombine(h1, h2); // Permutation resistant to support unordered maps.
            }
            return h;
        }
    default:
        assert(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

void Value::swap(Value& rhs) noexcept
{
    std::swap(data_, rhs.data_);
    std::swap(type_, rhs.type_);
}

size_t Value::size() const noexcept
{
    switch (type())
    {
    case Type::undefined:
    case Type::null:
    case Type::boolean:
    case Type::number:
        assert(false && "cannot read property 'size' of undefined, null, boolean or number"); // LCOV_EXCL_LINE
        return 0;
    case Type::string:
        return get_string().size();
    case Type::array:
        return get_array().size();
    case Type::object:
        return get_object().size();
    default:
        assert(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

bool Value::empty() const noexcept
{
    switch (type())
    {
    case Type::undefined:
    case Type::null:
    case Type::boolean:
    case Type::number:
        assert(false && "cannot read property 'empty' of undefined, null, boolean or number"); // LCOV_EXCL_LINE
        return true; // i.e. size() == 0
    case Type::string:
        return get_string().empty();
    case Type::array:
        return get_array().empty();
    case Type::object:
        return get_object().empty();
    default:
        assert(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

Array& Value::_get_or_assign_array()
{
    assert(is_undefined() || is_array());
    if (!is_array()) {
        assign(array_tag);
    }
    return get_array();
}

Value& Value::operator[](size_t index)
{
    auto& arr = _get_or_assign_array();
    if (arr.size() <= index)
    {
        assert(index < SIZE_MAX);
        arr.resize(index + 1);
    }
    return arr[index];
}

Value const& Value::operator[](size_t index) const noexcept
{
#if JSON_VALUE_ALLOW_UNDEFINED_ACCESS
    assert(is_undefined() || is_array());
    if (is_array())
#endif
    {
        auto& arr = get_array();
        if (index < arr.size()) {
            return arr[index];
        }
    }
    return kUndefined;
}

Value* Value::get_ptr(size_t index)
{
    if (is_array())
    {
        auto& arr = get_array();
        if (index < arr.size()) {
            return &arr[index];
        }
    }
    return nullptr;
}

Value const* Value::get_ptr(size_t index) const
{
    if (is_array())
    {
        auto& arr = get_array();
        if (index < arr.size()) {
            return &arr[index];
        }
    }
    return nullptr;
}

void Value::pop_back()
{
    get_array().pop_back();
}

Value::element_iterator Value::erase(size_t index)
{
    auto& arr = get_array();
    assert(index < arr.size());
    return arr.erase(arr.begin() + static_cast<intptr_t>(index));
}

Object& Value::_get_or_assign_object()
{
    assert(is_undefined() || is_object());
    if (!is_object()) {
        assign(object_tag);
    }
    return get_object();
}

Value& Value::operator[](Object::key_type const& key)
{
    return _get_or_assign_object()[key];
}

Value& Value::operator[](Object::key_type&& key)
{
    return _get_or_assign_object()[std::move(key)];
}

Value::item_iterator Value::erase(const_item_iterator pos)
{
    auto& obj = get_object();
    return obj.erase(pos);
}

Value::item_iterator Value::erase(const_item_iterator first, const_item_iterator last)
{
    auto& obj = get_object();
    return obj.erase(first, last);
}

bool Value::to_boolean() const noexcept
{
    switch (type())
    {
    case Type::undefined:
        return false;
    case Type::null:
        return false;
    case Type::boolean:
        return get_boolean();
    case Type::number:
        {
            auto v = get_number();
            return !std::isnan(v) && v != 0.0;
        }
    case Type::string:
        return !get_string().empty();
    case Type::array:
    case Type::object:
        assert(false && "to_boolean must not be called for arrays or objects"); // LCOV_EXCL_LINE
        return {};
    default:
        assert(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

double Value::to_number() const noexcept
{
    switch (type())
    {
    case Type::undefined:
        return std::numeric_limits<double>::quiet_NaN();
    case Type::null:
        return 0.0;
    case Type::boolean:
        return get_boolean() ? 1.0 : 0.0;
    case Type::number:
        return get_number();
    case Type::string:
        {
            auto const& v = get_string();
#if JSON_HAS_DOUBLE_CONVERSION
            return Strtod(v.c_str(), static_cast<int>(v.size()));
#else
            return Strtod(v.c_str(), nullptr);
#endif
        }
    case Type::array:
    case Type::object:
        assert(false && "to_number must not be called for arrays or objects"); // LCOV_EXCL_LINE
        return {};
    default:
        assert(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

double Value::to_integer() const noexcept
{
    auto v = to_number();
    if (std::isnan(v)) {
        return 0.0;
    }
    if (std::isinf(v) || v == 0.0) { // NB: -0 => +0
        return v;
    }

    return std::trunc(v);
}

// The notation "x modulo y" (y must be finite and nonzero) computes
// a value k of the same sign as y (or zero)
// such that abs(k) < abs(y) and x-k = q * y for some integer q.
static double Modulo(double x, double y)
{
    assert(std::isfinite(x));
    assert(std::isfinite(y) && y > 0.0);

    double m = std::fmod(x, y);
    if (m < 0.0) {
        m += y;
    }

    return m; // m in [-0.0, y)
}

int32_t Value::to_int32() const noexcept
{
    constexpr double kTwo32 = 4294967296.0;
    constexpr double kTwo31 = 2147483648.0;

    auto v = to_number();
    if (!std::isfinite(v) || v == 0.0) { // NB: -0 => +0
        return 0;
    }

    auto i = std::trunc(v);
    auto k = Modulo(i, kTwo32);

    if (k >= kTwo31) {
        k -= kTwo32;
    }

    return static_cast<int32_t>(k);
}

uint32_t Value::to_uint32() const noexcept
{
    constexpr double kTwo32 = 4294967296.0;

    auto v = to_number();
    if (!std::isfinite(v) || v == 0.0) { // NB: -0 => +0
        return 0;
    }

    auto i = std::trunc(v);
    auto k = Modulo(i, kTwo32);

    return static_cast<uint32_t>(k);
}

int16_t Value::to_int16() const noexcept
{
    constexpr double kTwo16 = 65536.0;
    constexpr double kTwo15 = 32768.0;

    auto v = to_number();
    if (!std::isfinite(v) || v == 0.0) { // NB: -0 => +0
        return 0;
    }

    auto i = std::trunc(v);
    auto k = Modulo(i, kTwo16);

    if (k >= kTwo15) {
        k -= kTwo16;
    }

    return static_cast<int16_t>(k);
}

uint16_t Value::to_uint16() const noexcept
{
    constexpr double kTwo16 = 65536.0;

    auto v = to_number();
    if (!std::isfinite(v) || v == 0.0) { // NB: -0 => +0
        return 0;
    }

    auto i = std::trunc(v);
    auto k = Modulo(i, kTwo16);

    return static_cast<uint16_t>(k);
}

int8_t Value::to_int8() const noexcept
{
    constexpr double kTwo8 = 256.0;
    constexpr double kTwo7 = 128.0;

    auto v = to_number();
    if (!std::isfinite(v) || v == 0.0) { // NB: -0 => +0
        return 0;
    }

    auto i = std::trunc(v);
    auto k = Modulo(i, kTwo8);

    if (k >= kTwo7) {
        k -= kTwo8;
    }

    return static_cast<int8_t>(k);
}

uint8_t Value::to_uint8() const noexcept
{
    constexpr double kTwo8 = 256.0;

    auto v = to_number();
    if (!std::isfinite(v) || v == 0.0) { // NB: -0 => +0
        return 0;
    }

    auto i = std::trunc(v);
    auto k = Modulo(i, kTwo8);

    return static_cast<uint8_t>(k);
}

uint8_t Value::to_uint8_clamped() const noexcept
{
    auto v = to_number();
    if (std::isnan(v)) {
        return 0;
    }

    if (v <= 0.0) { // NB: -0 => +0
        v = 0.0;
    }
    if (v >= 255.0) {
        v = 255.0;
    }

    auto f = std::floor(v);
    if (f + 0.5 < v) {
        // round up
        f += 1.0;
    }
    else if (v < f + 0.5) {
        // round down
    }
    else if ((static_cast<int>(f) & 1) != 0) {
        // round to even
        f += 1.0;
    }

    return static_cast<uint8_t>(f);
}

String Value::to_string() const
{
    switch (type())
    {
    case Type::undefined:
        return "undefined";
    case Type::null:
        return "null";
    case Type::boolean:
        return get_boolean() ? "true" : "false";
    case Type::number:
        {
            char buf[32];
            char* first = &buf[0];
            char* last = Dtoa(first, first + 32, get_number());
            return String(first, last);
        }
    case Type::string:
        return get_string();
    case Type::array:
    case Type::object:
        assert(false && "to_string must not be called for arrays or objects"); // LCOV_EXCL_LINE
        return {};
    default:
        assert(false && "invalid type"); // LCOV_EXCL_LINE
        return {};
    }
}

//==================================================================================================
// parse
//==================================================================================================

namespace
{
    struct Token
    {
        enum Kind : unsigned char {
            unknown,
            incomplete,
            eof,
            l_brace,
            r_brace,
            l_square,
            r_square,
            comma,
            colon,
            string,
            number,
            identifier,
            line_comment,
            block_comment,
            binary_data,
        };

        char const* ptr = nullptr;
        char const* end = nullptr;
        Kind        kind = Kind::unknown;
        bool        needs_cleaning = false;
    };
}

// 0x01: ' ', '\n', '\r', '\t'
// 0x02: '0'...'9'
// 0x04: 'a'...'z', 'A'...'Z'
// 0x08: IsDigit, IsLetter, '.', '+', '-', '#'
// 0x10: IsDigit, IsLetter, '_', '$'
// 0x20: IsDigit, 'a'...'f', 'A'...'F'
// 0x40: '"', '\\', '/', 'b', 'f', 'n', 'r', 't', 'u'

static constexpr uint8_t const kCharClass[256] = {
//      0       1       2       3       4       5       6       7       8       9       A       B       C       D       E       F
//      NUL     SOH     STX     ETX     EOT     ENQ     ACK     BEL     BS      HT      LF      VT      FF      CR      SO      SI
/*0*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0x01,   0x01,   0,      0,      0x01,   0,      0,
//      DLE     DC1     DC2     DC3     DC4     NAK     SYN     ETB     CAN     EM      SUB     ESC     FS      GS      RS      US
/*1*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
//      space   !       "       #       $       %       &       '       (       )       *       +       ,       -       .       /
/*2*/   0x01,   0,      0x40,   0x08,   0x10,   0,      0,      0,      0,      0,      0,      0x08,   0,      0x08,   0x08,   0x40,
//      0       1       2       3       4       5       6       7       8       9       :       ;       <       =       >       ?
/*3*/   0x3A,   0x3A,   0x3A,   0x3A,   0x3A,   0x3A,   0x3A,   0x3A,   0x3A,   0x3A,   0,      0,      0,      0,      0,      0,
//      @       A       B       C       D       E       F       G       H       I       J       K       L       M       N       O
/*4*/   0,      0x3C,   0x3C,   0x3C,   0x3C,   0x3C,   0x3C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,
//      P       Q       R       S       T       U       V       W       X       Y       Z       [       \       ]       ^       _
/*5*/   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0,      0x40,   0,      0,      0x10,
//      `       a       b       c       d       e       f       g       h       i       j       k       l       m       n       o
/*6*/   0,      0x3C,   0x7C,   0x3C,   0x3C,   0x3C,   0x7C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0x5C,   0x1C,
//      p       q       r       s       t       u       v       w       x       y       z       {       |       }       ~       DEL
/*7*/   0x1C,   0x1C,   0x5C,   0x1C,   0x5C,   0x5C,   0x1C,   0x1C,   0x1C,   0x1C,   0x1C,   0,      0,      0,      0,      0,
/*8*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
/*9*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
/*A*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
/*B*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
/*C*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
/*D*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
/*E*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
/*F*/   0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,      0,
};

static constexpr bool IsWhitespace        (char ch) { return (kCharClass[static_cast<unsigned char>(ch)] & 0x01) != 0; } // ' ', '\n', '\r', '\t'
static constexpr bool IsDigit             (char ch) { return (kCharClass[static_cast<unsigned char>(ch)] & 0x02) != 0; } // '0'...'9'
static constexpr bool IsNumberBody        (char ch) { return (kCharClass[static_cast<unsigned char>(ch)] & 0x08) != 0; } // IsDigit, IsLetter, '.', '+', '-', '#'
static constexpr bool IsIdentifierBody    (char ch) { return (kCharClass[static_cast<unsigned char>(ch)] & 0x10) != 0; } // IsDigit, IsLetter, '_', '$'
// static constexpr bool IsValidEscapedChar  (char ch) { return (kCharClass[static_cast<unsigned char>(ch)] & 0x40) != 0; } // '"', '\\', '/', 'b', 'f', 'n', 'r', 't', 'u'

static char const* SkipWhitespace(char const* f, char const* l)
{
    for ( ; f != l && IsWhitespace(*f); ++f)
    {
    }

    return f;
}

namespace
{
    struct Lexer
    {
        char const* src = nullptr;
        char const* end = nullptr;
        char const* ptr = nullptr; // position in [src, end)

        Lexer();
        explicit Lexer(char const* first, char const* last);

        Token Lex(ParseOptions const& options);

        Token MakeToken(char const* p, Token::Kind kind, bool needs_cleaning = false);

        Token LexString(char const* p, char quote_char);
        Token LexNumber(char const* p);
        Token LexIdentifier(char const* p);
        Token LexComment(char const* p);
    };
}

Lexer::Lexer()
{
}

Lexer::Lexer(char const* first, char const* last)
    : src(first)
    , end(last)
    , ptr(first)
{
}

Token Lexer::Lex(ParseOptions const& options)
{
L_again:
    ptr = SkipWhitespace(ptr, end);

    char const* p = ptr;

    if (p == end)
        return MakeToken(p, Token::eof);

    Token::Kind kind = Token::unknown;

    switch (*p++)
    {
    case '{':
        kind = Token::l_brace;
        break;
    case '}':
        kind = Token::r_brace;
        break;
    case '[':
        kind = Token::l_square;
        break;
    case ']':
        kind = Token::r_square;
        break;
    case ',':
        kind = Token::comma;
        break;
    case ':':
        kind = Token::colon;
        break;
    case '\'':
        if (options.allow_single_quoted_strings)
            return LexString(p, '\'');
        break;
    case '"':
        return LexString(p, '"');
    case '+':
        if (options.allow_leading_plus)
            return LexNumber(p);
        break;
    case '.':
        if (options.allow_leading_dot)
            return LexNumber(p);
        break;
    case '-':
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
        return LexNumber(p);
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case '_':
        return LexIdentifier(p);
    case '/':
        if (options.allow_comments)
        {
            Token tok = LexComment(p);
            if (tok.kind == Token::unknown)
                return tok;
            goto L_again;
        }
        break;
    case '$':
    default:
        break;
    }

    return MakeToken(p, kind);
}

Token Lexer::MakeToken(char const* p, Token::Kind kind, bool needs_cleaning)
{
    Token tok;

    tok.ptr            = ptr;
    tok.end            = p;
    tok.kind           = kind;
    tok.needs_cleaning = needs_cleaning;

    ptr = p;

    return tok;
}

Token Lexer::LexString(char const* p, char quote_char)
{
    bool needs_cleaning = false;

    ptr = p; // skip " or '

    if (p == end)
        return MakeToken(p, Token::incomplete, needs_cleaning);

    unsigned char uch = static_cast<unsigned char>(*p++);
    for (;;)
    {
        if (uch == quote_char)
            break;

        if (uch < 0x20) // Unescaped control character
        {
            needs_cleaning = true;
        }
        else if (uch >= 0x80) // Possibly a UTF-8 lead byte (sequence length >= 2)
        {
            needs_cleaning = true;
        }
        else if (uch == '\\')
        {
            if (p != end) // Skip the escaped character.
                ++p;
            needs_cleaning = true;
        }

        if (p == end)
            return MakeToken(p, Token::incomplete, needs_cleaning);

        uch = static_cast<unsigned char>(*p++);
    }

    Token tok = MakeToken(p - 1, Token::string, needs_cleaning);

    ptr = p; // skip " or '

    return tok;
}

Token Lexer::LexNumber(char const* p)
{
    // Lex everything which might possibly occur in numbers.
    //
    // No need to lex "1.2e+3+4" as ["1.2e+3", "+", "4"] as the exact format
    // of the number will be checked later.

    for ( ; p != end && IsNumberBody(*p); ++p)
    {
    }

    return MakeToken(p, Token::number);
}

Token Lexer::LexIdentifier(char const* p)
{
    for ( ; p != end && IsIdentifierBody(*p); ++p)
    {
    }

    return MakeToken(p, Token::identifier);
}

Token Lexer::LexComment(char const* p)
{
    assert(p[-1] == '/');

    if (p == end)
        return MakeToken(p, Token::unknown);

    if (*p == '/')
    {
        ptr = ++p; // skip leading "//"
        for (;;)
        {
            if (p == end)
                break;
            if (*p == '\n' || *p == '\r')
                break;
            ++p;
        }

        return MakeToken(p, Token::line_comment);
    }

    if (*p == '*')
    {
        ptr = ++p; // skip leading "/*"
        for (;;)
        {
            if (p == end)
                return MakeToken(p, Token::unknown);
            if (*p == '*' && ++p != end && *p == '/')
                break;
            ++p;
        }

        Token tok = MakeToken(p - 1, Token::block_comment);
        ptr = ++p; // skip trailing "*/"
        return tok;
    }

    return MakeToken(p, Token::unknown);
}

//------------------------------------------------------------------------------
// Parser
//------------------------------------------------------------------------------

namespace
{
    struct Parser
    {
        static const int kMaxDepth = 500;

        Lexer lexer;
        Token token;

        ErrorCode ParseObject    (Value& value, int depth, ParseOptions const& options);
        ErrorCode ParsePair      (String& key, Value& value, int depth, ParseOptions const& options);
        ErrorCode ParseArray     (Value& value, int depth, ParseOptions const& options);
        ErrorCode ParseString    (Value& value, int depth, ParseOptions const& options);
        ErrorCode ParseNumber    (Value& value, int depth, ParseOptions const& options);
        ErrorCode ParseIdentifier(Value& value, int depth, ParseOptions const& options);
        ErrorCode ParseValue     (Value& value, int depth, ParseOptions const& options);

        static ErrorCode GetCleanString(String& out, char const*& first, char const* last, ParseOptions const& options);

        enum class NumberClass {
            invalid,
            neg_integer,
            pos_integer,
            floating_point,
            pos_nan,
            neg_nan,
            pos_inf,
            neg_inf,
        };

        NumberClass ClassifyNumber(char const* first, char const* last, ParseOptions const& options);
    };
}

//
//  object
//      {}
//      { members }
//  members
//      pair
//      pair , members
//
ErrorCode Parser::ParseObject(Value& value, int depth, ParseOptions const& options)
{
    assert(token.kind == Token::l_brace);

    if (depth >= kMaxDepth)
        return ErrorCode::max_depth_reached;

    token = lexer.Lex(options); // skip '{'

    auto& obj = value.assign(object_tag);

    if (token.kind != Token::r_brace)
    {
        for (;;)
        {
            String K;
            Value V;

            ErrorCode ec = ParsePair(K, V, depth + 1, options);
            if (ec != ErrorCode::success)
                return ec;

            if (options.reject_duplicate_keys)
            {
#if __cplusplus >= 201703 || (_MSC_VER >= 1911 && _HAS_CXX17)
                auto const p = obj.try_emplace(std::move(K), std::move(V));
                if (!p.second)
                    return ErrorCode::duplicate_key_in_object;
#else
                auto const it = obj.find(K);
                if (it != obj.end())
                    return ErrorCode::duplicate_key_in_object;

                obj.emplace_hint(it, std::move(K), std::move(V));
#endif
            }
            else
            {
                obj[std::move(K)] = std::move(V);
            }

            if (token.kind != Token::comma)
                break;

            token = lexer.Lex(options); // skip ','

            if (options.allow_trailing_comma && token.kind == Token::r_brace)
                break;
        }

        if (token.kind != Token::r_brace)
            return ErrorCode::expected_comma_or_closing_brace;
    }

    token = lexer.Lex(options); // skip '}'

    return ErrorCode::success;
}

//
//  pair
//      string : value
//
ErrorCode Parser::ParsePair(String& key, Value& value, int depth, ParseOptions const& options)
{
    if (depth >= kMaxDepth)
        return ErrorCode::max_depth_reached;

    if (token.kind != Token::string && (!options.allow_unquoted_keys || token.kind != Token::identifier))
        return ErrorCode::expected_key;

    if (token.needs_cleaning)
    {
        ErrorCode ec = GetCleanString(key, token.ptr, token.end, options);
        if (ec != ErrorCode::success)
            return ec;
    }
    else
    {
        key.assign(token.ptr, token.end);
    }

    // skip 'key'
    token = lexer.Lex(options);

    if (token.kind != Token::colon)
        return ErrorCode::expected_colon_after_key;

    // skip ':'
    token = lexer.Lex(options);

    // Convert string to VALUE
    return ParseValue(value, depth, options);
}

//
//  array
//      []
//      [ elements ]
//  elements
//      value
//      value , elements
//
ErrorCode Parser::ParseArray(Value& value, int depth, ParseOptions const& options)
{
    assert(token.kind == Token::l_square);

    if (depth >= kMaxDepth)
        return ErrorCode::max_depth_reached;

    token = lexer.Lex(options); // skip '['

    auto& arr = value.assign(array_tag);

    if (token.kind != Token::r_square)
    {
        for (;;)
        {
            Value V;

            ErrorCode ec = ParseValue(V, depth + 1, options);
            if (ec != ErrorCode::success)
                return ec;

            arr.push_back(std::move(V));

            if (token.kind != Token::comma)
                break;

            // skip ','
            token = lexer.Lex(options);

            if (options.allow_trailing_comma && token.kind == Token::r_square)
                break;
        }

        if (token.kind != Token::r_square)
            return ErrorCode::expected_comma_or_closing_bracket;
    }

    token = lexer.Lex(options); // skip ']'

    return ErrorCode::success;
}

ErrorCode Parser::ParseString(Value& value, int /*depth*/, ParseOptions const& options)
{
    if (token.needs_cleaning)
    {
        String out;

        ErrorCode ec = GetCleanString(out, token.ptr, token.end, options);
        if (ec != ErrorCode::success)
            return ec;

        value.assign(string_tag, std::move(out));
    }
    else
    {
        value.assign(string_tag, String(token.ptr, token.end));
    }

    token = lexer.Lex(options); // skip string

    return ErrorCode::success;
}

static bool IsNaNString(char const* f, char const* l)
{
    auto const len = l - f;
    switch (len)
    {
    case 3:
        return memcmp(f, "NaN", 3) == 0
            || memcmp(f, "nan", 3) == 0
            || memcmp(f, "NAN", 3) == 0;
    case 6:
        return memcmp(f, "1.#IND", 6) == 0;
    case 7:
        return memcmp(f, "1.#QNAN", 7) == 0
            || memcmp(f, "1.#SNAN", 7) == 0;
    default:
        return false;
    }
}

static bool IsInfinityString(char const* f, char const* l)
{
    auto const len = l - f;
    switch (len)
    {
    case 3:
        return memcmp(f, "Inf", 3) == 0
            || memcmp(f, "inf", 3) == 0
            || memcmp(f, "INF", 3) == 0;
    case 6:
        return memcmp(f, "1.#INF", 6) == 0;
    case 8:
        return memcmp(f, "Infinity", 8) == 0
            || memcmp(f, "infinity", 8) == 0
            || memcmp(f, "INFINITY", 8) == 0;
    default:
        return false;
    }
}

//
//  number
//     int
//     int frac
//     int exp
//     int frac exp
//  int
//     digit
//     digit1-9 digits
//     - digit
//     - digit1-9 digits
//  frac
//     . digits
//  exp
//     e digits
//  digits
//     digit
//     digit digits
//  e
//     e
//     e+
//     e-
//     E
//     E+
//     E-
//
Parser::NumberClass Parser::ClassifyNumber(char const* f, char const* l, ParseOptions const& options)
{
    if (f == l)
        return NumberClass::invalid;

// [-]

    bool is_neg = false;

    if (*f == '-')
    {
        is_neg = true;
        ++f;
        if (f == l)
            return NumberClass::invalid;
    }
    else if (options.allow_leading_plus && *f == '+')
    {
        ++f;
        if (f == l)
            return NumberClass::invalid;
    }

// NaN/Infinity

    if (options.allow_nan_inf)
    {
        if (IsNaNString(f, l))
            return is_neg ? NumberClass::neg_nan : NumberClass::pos_nan;

        if (IsInfinityString(f, l))
            return is_neg ? NumberClass::neg_inf : NumberClass::pos_inf;
    }

// int

    if (*f == '0')
    {
        ++f;
        if (f == l)
            return is_neg ? NumberClass::neg_integer : NumberClass::pos_integer;
        if (IsDigit(*f))
            return NumberClass::invalid;
    }
    else if (IsDigit(*f)) // non '0'
    {
        for (;;)
        {
            ++f;
            if (f == l)
                return is_neg ? NumberClass::neg_integer : NumberClass::pos_integer;
            if (!IsDigit(*f))
                break;
        }
    }
    else if (options.allow_leading_dot && *f == '.')
    {
        // Parsed again below.
    }
    else
    {
        return NumberClass::invalid;
    }

// frac

    bool is_float = false;

    if (*f == '.')
    {
        is_float = true;
        ++f;
        if (f == l)
            return NumberClass::invalid;

        if (!IsDigit(*f))
            return NumberClass::invalid;

        for (;;)
        {
            ++f;
            if (f == l)
                return NumberClass::floating_point;
            if (!IsDigit(*f))
                break;
        }
    }

// exp

    if (*f == 'e' || *f == 'E')
    {
        is_float = true;
        ++f;
        if (f == l)
            return NumberClass::invalid;

        if (*f == '+' || *f == '-')
        {
            ++f;
            if (f == l)
                return NumberClass::invalid;
        }

        if (!IsDigit(*f))
            return NumberClass::invalid;

        for (;;)
        {
            ++f;
            if (f == l)
                return NumberClass::floating_point;
            if (!IsDigit(*f))
                break;
        }
    }

    if (f != l)
        return NumberClass::invalid;

    if (is_float)
        return NumberClass::floating_point;

    return is_neg ? NumberClass::neg_integer : NumberClass::pos_integer;
}

static int HexDigitValue(char ch)
{
    if ('0' <= ch && ch <= '9') return ch - '0';
    if ('A' <= ch && ch <= 'F') return ch - 'A' + 10;
    if ('a' <= ch && ch <= 'f') return ch - 'a' + 10;
    return -1;
}

// 2^53 = 9007199254740992 is the largest integer which can be represented
// without loss of precision in an IEEE double. That's 16 digits, so an integer
// with at most 15 digits always can be converted to double without loss of
// precision.
static constexpr int const kIntDigits10 = 15;

static int64_t ParseInteger(char const* f, char const* l)
{
    assert(l - f > 0); // internal error
    assert(l - f <= kIntDigits10); // internal error

    int64_t val = 0;
    for ( ; f != l; ++f)
    {
        assert(*f >= '0'); // internal error
        assert(*f <= '9'); // internal error
        val = val * 10 + (*f - '0');
    }

    return val;
}

ErrorCode Parser::ParseNumber(Value& value, int /*depth*/, ParseOptions const& options)
{
    // Validate number even if parsing numbers as strings!
    NumberClass const nc = ClassifyNumber(token.ptr, token.end, options);

    if (nc == NumberClass::invalid)
        return ErrorCode::invalid_numeric_literal;

    if (options.parse_numbers_as_strings)
    {
        value.assign(string_tag, String(token.ptr, token.end));

        token = lexer.Lex(options); // skip number

        return ErrorCode::success;
    }

    double num = 0.0;

    // Use a _slightly_ faster method for parsing integers which will fit into a
    // double-precision number without loss of precision. Larger numbers will be
    // handled by strtod.
    bool const is_pos_int = (nc == NumberClass::pos_integer) && (token.end - token.ptr) <= kIntDigits10;
    bool const is_neg_int = (nc == NumberClass::neg_integer) && (token.end - token.ptr) <= kIntDigits10 + 1/*minus sign*/;

    if (is_pos_int)
    {
        num = static_cast<double>(ParseInteger(token.ptr, token.end));
    }
    else if (is_neg_int)
    {
        assert(token.end - token.ptr >= 1); // internal error
        assert(token.ptr[0] == '-'); // internal error

        // NB:
        // Works for signed zeros.
        num = -static_cast<double>(ParseInteger(token.ptr + 1, token.end));
    }
    else if (nc == NumberClass::neg_nan)
    {
        num = -std::numeric_limits<double>::quiet_NaN();
    }
    else if (nc == NumberClass::pos_nan)
    {
        num = +std::numeric_limits<double>::quiet_NaN();
    }
    else if (nc == NumberClass::neg_inf)
    {
        num = -std::numeric_limits<double>::infinity();
    }
    else if (nc == NumberClass::pos_inf)
    {
        num = +std::numeric_limits<double>::infinity();
    }
    else
    {
        num = Strtod(token.ptr, static_cast<int>(token.end - token.ptr));
    }

    value.assign(number_tag, num);

    token = lexer.Lex(options); // skip number

    return ErrorCode::success;
}

ErrorCode Parser::ParseIdentifier(Value& value, int /*depth*/, ParseOptions const& options)
{
    assert(token.end - token.ptr > 0 && "internal error");

    auto const f = token.ptr;
    auto const l = token.end;
    auto const len = l - f;

    if (len == 4 && memcmp(f, "null", 4) == 0)
    {
        value.assign(null_tag);
    }
    else if (len == 4 && memcmp(f, "true", 4) == 0)
    {
        value.assign(boolean_tag, true);
    }
    else if (len == 5 && memcmp(f, "false", 5) == 0)
    {
        value.assign(boolean_tag, false);
    }
    else if (options.allow_nan_inf && IsNaNString(f, l))
    {
        value.assign(number_tag, std::numeric_limits<double>::quiet_NaN());
    }
    else if (options.allow_nan_inf && IsInfinityString(f, l))
    {
        value.assign(number_tag, std::numeric_limits<double>::infinity());
    }
    else
    {
        return ErrorCode::unrecognized_identifier;
    }

    token = lexer.Lex(options); // skip 'identifier'

    return ErrorCode::success;
}

//
//  value
//      string
//      number
//      object
//      array
//      true
//      false
//      null
//
ErrorCode Parser::ParseValue(Value& value, int depth, ParseOptions const& options)
{
    if (depth >= kMaxDepth)
        return ErrorCode::max_depth_reached;

    switch (token.kind)
    {
    case Token::l_brace:
        return ParseObject(value, depth, options);
    case Token::l_square:
        return ParseArray(value, depth, options);
    case Token::string:
        return ParseString(value, depth, options);
    case Token::number:
        return ParseNumber(value, depth, options);
    case Token::identifier:
        return ParseIdentifier(value, depth, options);
    default:
        return ErrorCode::unexpected_token;
    }
}

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
ErrorCode Parser::GetCleanString(String& out, char const*& first, char const* last, ParseOptions const& options)
{
    out.clear();
    out.reserve(static_cast<size_t>(last - first));

    char const* f = first;
    while (f != last)
    {
        auto const uch = static_cast<unsigned char>(*f);

        if (uch < 0x20) // unescaped control character
        {
            first = f;
            return ErrorCode::unescaped_control_character_in_string;
        }
        else if (uch < 0x80) // ASCII printable or DEL
        {
            if (*f != '\\')
            {
                out += *f;
                ++f;
                continue;
            }

            ++f; // skip '\'
            if (f == last)
            {
                first = f;
                return ErrorCode::unexpected_end_of_string;
            }

            switch (*f)
            {
            case '"':
                out += '"';
                ++f;
                break;
            case '\\':
                out += '\\';
                ++f;
                break;
            case '/':
                out += '/';
                ++f;
                break;
            case 'b':
                out += '\b';
                ++f;
                break;
            case 'f':
                out += '\f';
                ++f;
                break;
            case 'n':
                out += '\n';
                ++f;
                break;
            case 'r':
                out += '\r';
                ++f;
                break;
            case 't':
                out += '\t';
                ++f;
                break;
            case 'u':
                {
                    --f; // Put back '\'

                    auto const res = unicode::DecodeUTF16Sequence([&](uint16_t& W)
                    {
                        if (last - f < 6)
                            return false;

                        if (*f++ != '\\' || *f++ != 'u')
                            return false;

                        int const h0 = HexDigitValue(*f++);
                        int const h1 = HexDigitValue(*f++);
                        int const h2 = HexDigitValue(*f++);
                        int const h3 = HexDigitValue(*f++);

                        int const value = (h0 << 12) | (h1 << 8) | (h2 << 4) | h3;
                        if (value >= 0)
                        {
                            assert(value <= 0xFFFF);
                            W = static_cast<uint16_t>(value);
                            return true;
                        }

                        return false;
                    });

                    switch (res.ec)
                    {
                    case unicode::ErrorCode::success:
                        unicode::EncodeUTF8(res.U, [&](char ch) { out += ch; return true; });
                        break;

                    case unicode::ErrorCode::insufficient_data:
                        // This is either an "end of file" or an "invalid UCN".
                        // In both cases this is not strictly an "invalid unicode" error.
                        first = f;
                        return ErrorCode::invalid_unicode_sequence_in_string;

                    default:
                        if (!options.allow_invalid_unicode)
                        {
                            first = f;
                            return ErrorCode::invalid_unicode_sequence_in_string;
                        }

//                      unicode::EncodeUTF8(unicode::kReplacementCharacter, [&](char ch) { out += ch; return true; });
                        out += "\xEF\xBF\xBD";
                        break;
                    }
                }
                break;
            default:
                first = f;
                return ErrorCode::invalid_escaped_character_in_string;
            }
        }
        else // (possibly) the start of a UTF-8 sequence.
        {
            auto f0 = f;
            auto const res = unicode::DecodeUTF8Sequence(f, last);

            switch (res.ec)
            {
            case unicode::ErrorCode::success:
                // The range [f0, f) already contains a valid UTF-8 encoding of U.
                switch (f - f0) {
                case 4: out += *f0++; // fall through
                case 3: out += *f0++; // fall through
                case 2: out += *f0++; // fall through
                        out += *f0++;
                    break;
                default:
                    assert(false && "internal error");
                    break;
                }
                break;

            default:
                if (!options.allow_invalid_unicode)
                {
                    first = f;
                    return ErrorCode::invalid_unicode_sequence_in_string;
                }

//              unicode::EncodeUTF8(unicode::kReplacementCharacter, [&](char ch) { out += ch; return true; });
                out += "\xEF\xBF\xBD";

#if JSON_SKIP_INVALID_UNICODE
                // Skip to the start of the next sequence (if not already done yet)
                f = unicode::FindNextUTF8LeadByte(f, last);
#endif
                break;
            }
        }
    }

    first = f;
    return ErrorCode::success;
}

ParseResult json::parse(Value& value, char const* next, char const* last, ParseOptions const& options)
{
    assert(next != nullptr);
    assert(last != nullptr);
//  assert(reinterpret_cast<uintptr_t>(next) <= reinterpret_cast<uintptr_t>(last));

    value.assign(undefined_tag); // clear!

    if (options.skip_bom && last - next >= 3)
    {
        if (static_cast<unsigned char>(next[0]) == 0xEF &&
            static_cast<unsigned char>(next[1]) == 0xBB &&
            static_cast<unsigned char>(next[2]) == 0xBF)
        {
            next += 3;
        }
    }

    Parser parser;

    parser.lexer = Lexer(next, last);
    parser.token = parser.lexer.Lex(options);

    ErrorCode ec = parser.ParseValue(value, 0, options);

    if (ec == ErrorCode::success)
    {
        if (!options.allow_trailing_characters && parser.token.kind != Token::eof)
        {
            ec = ErrorCode::expected_eof;
        }
    }

    return {ec, parser.token.ptr, parser.token.end};
}

ErrorCode json::parse(Value& value, std::string const& str, ParseOptions const& options)
{
    char const* next = str.data();
    char const* last = str.data() + str.size();

    return ::json::parse(value, next, last, options).ec;
}

//==================================================================================================
// stringify
//==================================================================================================

static bool StringifyValue(std::string& str, Value const& value, StringifyOptions const& options, int curr_indent);

static bool StringifyNull(std::string& str)
{
    str += "null";
    return true;
}

static bool StringifyBoolean(std::string& str, bool value)
{
    str += value ? "true" : "false";
    return true;
}

static bool StringifyNumber(std::string& str, double value, StringifyOptions const& options)
{
    if (!std::isfinite(value))
    {
        if (!options.allow_nan_inf)
        {
            str += "null";
            return true;
        }

        if (value < 0)
            str += '-';

        if (std::isnan(value))
            str += kNaNString;
        else
            str += kInfString;
        return true;
    }

    char buf[32];

    // Handle +-0.
    // Interpret -0 as a floating-point number and +0 as an integer.
    if (value == 0.0)
    {
        if (std::signbit(value))
            str += "-0.0";
        else
            str += '0';
        return true;
    }

    // The test for |value| <= 2^53 is not strictly required, but ensures that a number
    // is only printed as an integer if it is exactly representable as double.
    if (value >= -9007199254740992.0 && value <= 9007199254740992.0)
    {
        const int64_t i = static_cast<int64_t>(value);
        if (static_cast<double>(i) == value)
        {
            char* end = iconv::S64toa(buf, i);
            str.append(buf, end);
            return true;
        }
    }

    char* end = Dtoa(buf, buf + 32, value);
    str.append(buf, end);
    return true;
}

static bool StringifyString(std::string& str, String const& value, StringifyOptions const& options)
{
    static constexpr char const kHexDigits[] = "0123456789ABCDEF";

    char const*       first = value.data();
    char const* const last  = value.data() + value.size();

    str += '"';

    char ch_prev = '\0';
    char ch = '\0';

    while (first != last)
    {
        ch_prev = ch;
        ch = *first;

        auto const uch = static_cast<unsigned char>(ch);

        if (uch < 0x20) // (ASCII) control character
        {
            switch (ch)
            {
            case '\b':
                str += '\\';
                str += 'b';
                break;
            case '\f':
                str += '\\';
                str += 'f';
                break;
            case '\n':
                str += '\\';
                str += 'n';
                break;
            case '\r':
                str += '\\';
                str += 'r';
                break;
            case '\t':
                str += '\\';
                str += 't';
                break;
            default:
                str += "\\u00";
                str += kHexDigits[uch >> 4];
                str += kHexDigits[uch & 0xF];
                break;
            }
            ++first;
        }
        else if (uch < 0x80) // ASCII printable or DEL
        {
            switch (ch)
            {
            case '"':   // U+0022
                str += '\\';
                break;
            case '/':   // U+002F
                if (options.escape_slash && ch_prev == '<') {
                    str += '\\';
                }
                break;
            case '\\':  // U+005C
                str += '\\';
                break;
            }
            str += ch;
            ++first;
        }
        else // (possibly) the start of a UTF-8 sequence.
        {
            auto f0 = first;
            auto const res = unicode::DecodeUTF8Sequence(first, last);

            switch (res.ec)
            {
            case unicode::ErrorCode::success:
                //
                // Always escape U+2028 (LINE SEPARATOR) and U+2029 (PARAGRAPH
                // SEPARATOR). No string in JavaScript can contain a literal
                // U+2028 or a U+2029.
                //
                // (See http://timelessrepo.com/json-isnt-a-javascript-subset)
                //
                if (res.U == 0x2028)
                {
                    str += "\\u2028";
                }
                else if (res.U == 0x2029)
                {
                    str += "\\u2029";
                }
                else
                {
                    // The UTF-8 sequence is valid. No need to re-encode.
                    switch (first - f0) {
                    case 4: str += *f0++; // fall through
                    case 3: str += *f0++; // fall through
                    case 2: str += *f0++; // fall through
                            str += *f0++;
                        break;
                    default:
                        assert(false && "internal error");
                        break;
                    }
                }
                break;

            default:
                if (!options.allow_invalid_unicode)
                {
                    // TODO:
                    // Caller should be notified of the position of the invalid sequence
                    return false;
                }

                str += "\\uFFFD";

#if JSON_SKIP_INVALID_UNICODE
                // Scan to the start of the next UTF-8 sequence.
                // This includes ASCII characters.
                first = unicode::FindNextUTF8LeadByte(first, last);
#endif
                break;
            }
        }
    }

    str += '"';

    return true;
}

static bool StringifyArray(std::string& str, Array const& value, StringifyOptions const& options, int curr_indent)
{
    str += '[';

    auto       I = value.begin();
    auto const E = value.end();
    if (I != E)
    {
        if (options.indent_width > 0)
        {
            assert(curr_indent <= INT_MAX - options.indent_width);
            curr_indent += options.indent_width;

            for (;;)
            {
                str += '\n';
                str.append(static_cast<size_t>(curr_indent), ' ');

                if (!StringifyValue(str, *I, options, curr_indent))
                    return false;

                if (++I == E)
                    break;

                str += ',';
            }

            curr_indent -= options.indent_width;

            str += '\n';
            str.append(static_cast<size_t>(curr_indent), ' ');
        }
        else
        {
            for (;;)
            {
                if (!StringifyValue(str, *I, options, curr_indent))
                    return false;

                if (++I == E)
                    break;

                str += ',';
                if (options.indent_width == 0)
                    str += ' ';
            }
        }
    }

    str += ']';

    return true;
}

static bool StringifyObject(std::string& str, Object const& value, StringifyOptions const& options, int curr_indent)
{
    str += '{';

    auto       I = value.begin();
    auto const E = value.end();
    if (I != E)
    {
        if (options.indent_width > 0)
        {
            assert(curr_indent <= INT_MAX - options.indent_width);
            curr_indent += options.indent_width;

            for (;;)
            {
                str += '\n';
                str.append(static_cast<size_t>(curr_indent), ' ');

                if (!StringifyString(str, I->first, options))
                    return false;
                str += ':';
                str += ' ';
                if (!StringifyValue(str, I->second, options, curr_indent))
                    return false;

                if (++I == E)
                    break;

                str += ',';
            }

            curr_indent -= options.indent_width;

            str += '\n';
            str.append(static_cast<size_t>(curr_indent), ' ');
        }
        else
        {
            for (;;)
            {
                if (!StringifyString(str, I->first, options))
                    return false;
                str += ':';
                if (options.indent_width == 0)
                    str += ' ';
                if (!StringifyValue(str, I->second, options, curr_indent))
                    return false;

                if (++I == E)
                    break;

                str += ',';
                if (options.indent_width == 0)
                    str += ' ';
            }
        }
    }

    str += '}';

    return true;
}

static bool StringifyValue(std::string& str, Value const& value, StringifyOptions const& options, int curr_indent)
{
    switch (value.type())
    {
    case Type::undefined:
        assert(false && "cannot stringify 'undefined'");
        return StringifyNull(str);
    case Type::null:
        return StringifyNull(str);
    case Type::boolean:
        return StringifyBoolean(str, value.get_boolean());
    case Type::number:
        return StringifyNumber(str, value.get_number(), options);
    case Type::string:
        return StringifyString(str, value.get_string(), options);
    case Type::array:
        return StringifyArray(str, value.get_array(), options, curr_indent);
    case Type::object:
        return StringifyObject(str, value.get_object(), options, curr_indent);
    default:
        assert(false && "invalid type");
        return {};
    }
}

bool json::stringify(std::string& str, Value const& value, StringifyOptions const& options)
{
    return StringifyValue(str, value, options, 0);
}

/*
Copyright (c) 2009 Florian Loitsch

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/
