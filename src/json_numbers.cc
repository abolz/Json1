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

#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>
#include <type_traits>

#ifdef _MSC_VER
#include <intrin.h>
#endif

using namespace json;
using namespace json::numbers;

//==================================================================================================
// NumberToString
//==================================================================================================

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

namespace {

char* Itoa100(char* buf, uint32_t digits)
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

char* U32ToString(char* buf, uint32_t n)
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

char* U32ToString_n(char* buf, uint32_t n, int num_digits)
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

char* U64ToString(char* buf, uint64_t n)
{
    if (n <= UINT32_MAX)
        return U32ToString(buf, static_cast<uint32_t>(n));

    // n = hi * 10^9 + lo < 10^20,   where hi < 10^11, lo < 10^9
    auto const hi = n / 1000000000;
    auto const lo = n % 1000000000;

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
            buf = Itoa100(buf, hi1);
        }
        else
        {
            assert(hi1 != 0);
            buf[0] = static_cast<char>('0' + hi1);
            buf++;
        }
        buf = U32ToString_n(buf, hi0, 9);
    }

    // lo has exactly 9 digits.
    // (Which might all be zero...)
    return U32ToString_n(buf, static_cast<uint32_t>(lo), 9);
}

char* I64ToString(char* buf, int64_t i)
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

} // namespace

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

namespace {

template <typename Target, typename Source>
Target ReinterpretBits(Source source)
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

DiyFp DiyFp::sub(DiyFp x, DiyFp y)
{
    assert(x.e == y.e);
    assert(x.f >= y.f);

    return DiyFp(x.f - y.f, x.e);
}

DiyFp DiyFp::mul(DiyFp x, DiyFp y)
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

    uint64_t const u_lo = x.f & 0xFFFFFFFF;
    uint64_t const u_hi = x.f >> 32;
    uint64_t const v_lo = y.f & 0xFFFFFFFF;
    uint64_t const v_hi = y.f >> 32;

    uint64_t const p0 = u_lo * v_lo;
    uint64_t const p1 = u_lo * v_hi;
    uint64_t const p2 = u_hi * v_lo;
    uint64_t const p3 = u_hi * v_hi;

    uint64_t const p0_hi = p0 >> 32;
    uint64_t const p1_lo = p1 & 0xFFFFFFFF;
    uint64_t const p1_hi = p1 >> 32;
    uint64_t const p2_lo = p2 & 0xFFFFFFFF;
    uint64_t const p2_hi = p2 >> 32;

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

    uint64_t const h = p3 + p2_hi + p1_hi + (Q >> 32);

    return DiyFp(h, x.e + y.e + 64);

#endif
}

DiyFp DiyFp::normalize(DiyFp x)
{
    assert(x.f != 0);

#if defined(_MSC_VER) && defined(_M_X64)

    auto const leading_zeros = static_cast<int>(__lzcnt64(x.f));
    return DiyFp(x.f << leading_zeros, x.e - leading_zeros);

#elif defined(__GNUC__)

    auto const leading_zeros = __builtin_clzll(x.f);
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

DiyFp DiyFp::normalize_to(DiyFp x, int target_exponent)
{
    auto const delta = x.e - target_exponent;

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
Boundaries ComputeBoundaries(FloatType value)
{
    assert(std::isfinite(value));
    assert(value > 0);

    // Convert the IEEE representation into a DiyFp.
    //
    // If v is denormal:
    //      value = 0.F * 2^(1 - bias) = (          F) * 2^(1 - bias - (p-1))
    // If v is normalized:
    //      value = 1.F * 2^(E - bias) = (2^(p-1) + F) * 2^(E - bias - (p-1))

    static_assert(std::numeric_limits<FloatType>::is_iec559,
        "This implementation requires an IEEE-754 floating-point implementation");

    static constexpr int      kPrecision = std::numeric_limits<FloatType>::digits; // = p (includes the hidden bit)
    static constexpr int      kBias      = std::numeric_limits<FloatType>::max_exponent - 1 + (kPrecision - 1);
    static constexpr uint64_t kHiddenBit = uint64_t{1} << (kPrecision - 1); // = 2^(p-1)

    using bits_type = std::conditional_t<kPrecision == 24, uint32_t, uint64_t>;

    auto const bits = ReinterpretBits<bits_type>(value);
    auto const E = bits >> (kPrecision - 1);
    auto const F = bits & (kHiddenBit - 1);

    auto const is_denormal = (E == 0);

    auto const v
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

//  auto const lower_boundary_is_closer = (v.f == kHiddenBit && v.e > kMinExp);
    auto const lower_boundary_is_closer = (F == 0 && E > 1);

    auto const m_plus = DiyFp(2*v.f + 1, v.e - 1);
    auto const m_minus
        = lower_boundary_is_closer
            ? DiyFp(4*v.f - 1, v.e - 2)  // (B)
            : DiyFp(2*v.f - 1, v.e - 1); // (A)

    // Determine the normalized w+ = m+.
    auto const w_plus = DiyFp::normalize(m_plus);

    // Determine w- = m- such that e_(w-) = e_(w+).
    auto const w_minus = DiyFp::normalize_to(m_minus, w_plus.e);

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

static constexpr int kAlpha = -60;
static constexpr int kGamma = -32;

// Now
//
//      alpha <= e_c + e + q <= gamma                                        (1)
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
// "In theory the result of the procedure could be wrong since c is rounded, and
//  the computation itself is approximated [...]. In practice, however, this
//  simple function is sufficient."
//
// For IEEE double precision floating-point numbers converted into normalized
// DiyFp's w = f * 2^e, with q = 64,
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
// This binary exponent range [-1137,960] results in a decimal exponent range
// [-307,324]. One does not need to store a cached power for each k in this
// range. For each such k it suffices to find a cached power such that the
// exponent of the product lies in [alpha,gamma].
// This implies that the difference of the decimal exponents of adjacent table
// entries must be less than or equal to
//
//      floor( (gamma - alpha) * log_10(2) ) = 8.
//
// (A smaller distance gamma-alpha would require a larger table.)

static constexpr int kCachedPowersSize         =   87;
static constexpr int kCachedPowersMinDecExp    = -348;
static constexpr int kCachedPowersMaxDecExp    =  340;
static constexpr int kCachedPowersDecExpStep   =    8;

struct CachedPower { // c = f * 2^e ~= 10^k
    uint64_t f;
    int e;
    int k;
};

static constexpr CachedPower kCachedPowers[] = {
    { 0xFA8FD5A0081C0288, -1220, -348 }, //*
    { 0xBAAEE17FA23EBF76, -1193, -340 }, //*
    { 0x8B16FB203055AC76, -1166, -332 }, //*
    { 0xCF42894A5DCE35EA, -1140, -324 }, //*
    { 0x9A6BB0AA55653B2D, -1113, -316 }, //*
    { 0xE61ACF033D1A45DF, -1087, -308 }, //*
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
    { 0xEB96BF6EBADF77D9,  1039,  332 }, //*
    { 0xAF87023B9BF0EE6B,  1066,  340 }, //*
};

// For a normalized DiyFp w = f * 2^e, this function returns a (normalized)
// cached power-of-ten c = f_c * 2^e_c, such that the exponent of the product
// w * c satisfies (Definition 3.2 from [1])
//
//      alpha <= e_c + e + q <= gamma.
//
CachedPower GetCachedPowerForBinaryExponent(int e)
{
    // NB:
    // Actually this function returns c, such that -60 <= e_c + e + 64 <= -34.

    // This computation gives exactly the same results for k as
    //      k = ceil((kAlpha - e - 1) * 0.30102999566398114)
    // for |e| <= 1500, but doesn't require floating-point operations.
    // NB: log_10(2) ~= 78913 / 2^18
    assert(e >= -1500);
    assert(e <=  1500);
    auto const f = kAlpha - e - 1;
    auto const k = (f * 78913) / (1 << 18) + (f > 0);

    auto const index = (-kCachedPowersMinDecExp + k + (kCachedPowersDecExpStep - 1)) / kCachedPowersDecExpStep;
    assert(index >= 0);
    assert(index < kCachedPowersSize);
    static_cast<void>(kCachedPowersSize);

    auto const cached = kCachedPowers[index];
    assert(kAlpha <= cached.e + e + 64);
    assert(kGamma >= cached.e + e + 64);

    return cached;
}

void Grisu2Round(char* buf, int len, uint64_t dist, uint64_t delta, uint64_t rest, uint64_t ten_k)
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
void Grisu2DigitGen(char* buffer, int& length, int& decimal_exponent, DiyFp M_minus, DiyFp w, DiyFp M_plus)
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

    auto delta = DiyFp::sub(M_plus, M_minus).f; // (significand of (M+ - M-), implicit exponent is e)
    auto dist  = DiyFp::sub(M_plus, w      ).f; // (significand of (M+ - w ), implicit exponent is e)

    // Split M+ = f * 2^e into two parts p1 and p2 (note: e < 0):
    //
    //      M+ = f * 2^e
    //         = ((f div 2^-e) * 2^-e + (f mod 2^-e)) * 2^e
    //         = ((p1        ) * 2^-e + (p2        )) * 2^e
    //         = p1 + p2 * 2^e

    DiyFp const one(uint64_t{1} << -M_plus.e, M_plus.e);

    auto p1 = static_cast<uint32_t>(M_plus.f >> -one.e); // p1 = f div 2^-e (Since -e >= 32, p1 fits into a 32-bit int.)
    auto p2 = M_plus.f & (one.f - 1);                    // p2 = f mod 2^-e

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
    auto const last = U32ToString(buffer, p1);
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

        int k = length;
        int n = 0;

        uint32_t r = 0;
        uint32_t pow10 = 1; // 10^n
        for (;;)
        {
            assert(k >= n + 1);
            assert((uint64_t{r} << -one.e) + p2 <= delta);
            assert(n <= 9);
            assert(static_cast<uint32_t>(buffer[k - (n + 1)] - '0') <= UINT32_MAX / pow10);

            auto const r_next = pow10 * static_cast<uint32_t>(buffer[k - (n + 1)] - '0') + r;
            if ((uint64_t{r_next} << -one.e) + p2 > delta)
                break;
            r = r_next;
            n++;
            pow10 *= 10;
        }
        length = k - n;

        // V = buffer * 10^n, with M- <= V <= M+.

        decimal_exponent += n;

        auto const rest = (uint64_t{r} << -one.e) + p2;
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
        auto const ten_n = uint64_t{pow10} << -one.e;
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
        auto const d = p2 >> -one.e;     // d = (10 * p2) div 2^-e
        auto const r = p2 & (one.f - 1); // r = (10 * p2) mod 2^-e
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
    auto const ten_m = one.f;
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
void Grisu2(char* buf, int& len, int& decimal_exponent, DiyFp m_minus, DiyFp v, DiyFp m_plus)
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

    CachedPower const cached = GetCachedPowerForBinaryExponent(m_plus.e);

    DiyFp const c_minus_k(cached.f, cached.e); // = c ~= 10^-k

    // The exponent of the products is = v.e + c_minus_k.e + q and is in the range [alpha,gamma]
    auto const w       = DiyFp::mul(v,       c_minus_k);
    auto const w_minus = DiyFp::mul(m_minus, c_minus_k);
    auto const w_plus  = DiyFp::mul(m_plus,  c_minus_k);

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
    DiyFp const M_minus(w_minus.f + 1, w_minus.e);
    DiyFp const M_plus (w_plus.f  - 1, w_plus.e );

    decimal_exponent = -cached.k; // = -(-k) = k

    Grisu2DigitGen(buf, len, decimal_exponent, M_minus, w, M_plus);
}

// v = buf * 10^decimal_exponent
// len is the length of the buffer (number of decimal digits)
// The buffer must be large enough, i.e. >= max_digits10.
template <typename FloatType>
void Grisu2(char* buf, int& len, int& decimal_exponent, FloatType value)
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
    auto const w = ComputeBoundaries(static_cast<double>(value));
#else
    auto const w = ComputeBoundaries(value);
#endif

    Grisu2(buf, len, decimal_exponent, w.minus, w.w, w.plus);
}

// Appends a decimal representation of e to buf.
// Returns a pointer to the element following the exponent.
// PRE: -1000 < e < 1000
char* AppendExponent(char* buf, int e)
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

    auto const k = static_cast<uint32_t>(e);
    if (k < 10)
    {
        buf[0] = static_cast<char>('0' + k);
        buf++;
    }
    else if (k < 100)
    {
        buf = Itoa100(buf, k);
    }
    else
    {
        auto const q = k / 100;
        auto const r = k % 100;
        buf[0] = static_cast<char>('0' + q);
        buf = Itoa100(buf + 1, r);
    }

    return buf;
}

// Prettify v = buf * 10^decimal_exponent
// If v is in the range [10^min_exp, 10^max_exp) it will be printed in fixed-point notation.
// Otherwise it will be printed in exponential notation.
// PRE: min_exp < 0
// PRE: max_exp > 0
char* FormatBuffer(char* buf, int len, int decimal_exponent, int min_exp, int max_exp, bool emit_trailing_dot_zero)
{
    assert(min_exp < 0);
    assert(max_exp > 0);

    int const k = len;
    int /*const*/ n = len + decimal_exponent;
    // v = buf * 10^(n-k)
    // k is the length of the buffer (number of decimal digits)
    // n is the position of the decimal point relative to the start of the buffer.

    if (k <= n && n <= max_exp)
    {
        // digits[000]
        // len <= max_exp

        std::memset(buf + k, '0', static_cast<size_t>(n - k));
        if (emit_trailing_dot_zero)
        {
            buf[n++] = '.';
            buf[n++] = '0';
        }
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

char* StrCopy(char* first, char const* last, char const* source)
{
    assert(source != nullptr);

    auto const len = ::strlen(source);
    assert(first <= last);
    assert(static_cast<size_t>(last - first) >= len);
    static_cast<void>(last);

    std::memcpy(first, source, len);
    return first + len;
}

static constexpr int const kDtoaMaximalLength = 17;

// Generates a decimal representation of the floating-point number 'value' in
// [first, last).
//
// Note: The buffer must be large enough.
// Note: The result is _not_ null-terminated.
char* DoubleToString(char* first, char* last, double value, bool emit_trailing_dot_zero, char const* nan_string, char const* inf_string)
{
    if (std::isnan(value))
    {
        return StrCopy(first, last, nan_string);
    }

    // Use signbit(value) instead of (value < 0) since signbit works for -0.
    if (std::signbit(value))
    {
        value = -value;
        *first++ = '-';
    }

    if (std::isinf(value))
    {
        return StrCopy(first, last, inf_string);
    }

    if (value == 0)
    {
        *first++ = '0';
        if (emit_trailing_dot_zero)
        {
            *first++ = '.';
            *first++ = '0';
        }
        return first;
    }

    assert(last - first >= kDtoaMaximalLength);
    assert(last - first <= INT_MAX);

    // Compute v = buffer * 10^decimal_exponent.
    // The decimal digits are stored in the buffer, which needs to be interpreted
    // as an unsigned decimal integer.
    // len is the length of the buffer, i.e. the number of decimal digits.

    int len = 0;
    int decimal_exponent = 0;
    Grisu2(first, len, decimal_exponent, value);
    assert(len <= kDtoaMaximalLength);

    constexpr int kMinExp = -6;
    constexpr int kMaxExp = 21;

    assert(last - first >= kMaxExp);
    assert(last - first >= 2 + (-kMinExp - 1) + kDtoaMaximalLength);
    assert(last - first >= kDtoaMaximalLength + 6);

    return FormatBuffer(first, len, decimal_exponent, kMinExp, kMaxExp, emit_trailing_dot_zero);
}

} // namespace

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

char* json::numbers::NumberToString(char* next, char* last, double value, bool emit_trailing_dot_zero)
{
    constexpr double kMinInteger = -9007199254740992.0; // -2^53
    constexpr double kMaxInteger =  9007199254740992.0; //  2^53

    // Print integers in the range [-2^53, +2^53] as integers (without a trailing ".0").
    // These integers are exactly representable as 'double's.
    // However, always print -0 as "-0.0" to increase compatibility with other libraries
    //
    // NB:
    // These tests for work correctly for NaN's and Infinity's (i.e. the DoubleToString branch is taken).

    if (value == 0.0 && std::signbit(value))
    {
        std::memcpy(next, "-0.0", 4);
        return next + 4;
    }
    else if (/*!emit_trailing_dot_zero &&*/ value >= kMinInteger && value <= kMaxInteger)
    {
        const int64_t i = static_cast<int64_t>(value);
        if (static_cast<double>(i) == value)
        {
            return I64ToString(next, i);
        }
    }

    return DoubleToString(next, last, value, emit_trailing_dot_zero, "NaN", "Infinity");
}

//==================================================================================================
// StringToNumber
//==================================================================================================

namespace {

//--------------------------------------------------------------------------------------------------
// FastStrtod
//--------------------------------------------------------------------------------------------------

// 2^53 = 9007199254740992.
// Any integer with at most 15 decimal digits will hence fit into a double (which has a 53bit significand) without loss of precision.
static constexpr int const kMaxExactDoubleIntegerDecimalDigits = 15;

// Double operations detection based on target architecture.
// Linux uses a 80bit wide floating point stack on x86. This induces double
// rounding, which in turn leads to wrong results.
// An easy way to test if the floating-point operations are correct is to
// evaluate: 89255.0/1e22. If the floating-point stack is 64 bits wide then
// the result is equal to 89255e-22.
// The best way to test this, is to create a division-function and to compare
// the output of the division with the expected result. (Inlining must be
// disabled.)
// On Linux,x86 89255e-22 != Div_double(89255.0/1e22)
#if defined(_M_X64)              || \
    defined(__x86_64__)          || \
    defined(__ARMEL__)           || \
    defined(__avr32__)           || \
    defined(__hppa__)            || \
    defined(__ia64__)            || \
    defined(__mips__)            || \
    defined(__powerpc__)         || \
    defined(__ppc__)             || \
    defined(__ppc64__)           || \
    defined(_POWER)              || \
    defined(_ARCH_PPC)           || \
    defined(_ARCH_PPC64)         || \
    defined(__sparc__)           || \
    defined(__sparc)             || \
    defined(__s390__)            || \
    defined(__SH4__)             || \
    defined(__alpha__)           || \
    defined(_MIPS_ARCH_MIPS32R2) || \
    defined(__AARCH64EL__)       || \
    defined(__aarch64__)         || \
    defined(__riscv)
#define CORRECT_DOUBLE_OPERATIONS 1
#elif defined(__mc68000__)
#undef CORRECT_DOUBLE_OPERATIONS
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
#if defined(_WIN32)
// Windows uses a 64bit wide floating point stack.
#define CORRECT_DOUBLE_OPERATIONS 1
#else
#undef CORRECT_DOUBLE_OPERATIONS
#endif  // _WIN32
#else
#error Target architecture was not detected as supported by Double-Conversion.
#endif

#if !defined(CORRECT_DOUBLE_OPERATIONS)

bool FastStrtod(char const* /*next*/, char const* /*last*/, int /*exponent*/, double& result)
{
    // On x86 the floating-point stack can be 64 or 80 bits wide. If it is
    // 80 bits wide (as is the case on Linux) then double-rounding occurs and the
    // result is not accurate.
    // We know that Windows32 uses 64 bits and is therefore accurate.
    // Note that the ARM simulator is compiled for 32bits. It therefore exhibits
    // the same problem.

    result = std::numeric_limits<double>::quiet_NaN();
    return false;
}

#else // ^^^ !CORRECT_DOUBLE_OPERATIONS

static constexpr int const kExactPowersOfTenSize = 23;
static constexpr double const kExactPowersOfTen[] = {
    1.0e+00,
    1.0e+01,
    1.0e+02,
    1.0e+03,
    1.0e+04,
    1.0e+05,
    1.0e+06,
    1.0e+07,
    1.0e+08,
    1.0e+09,
    1.0e+10,
    1.0e+11,
    1.0e+12,
    1.0e+13,
    1.0e+14,
    1.0e+15,
    1.0e+16,
    1.0e+17,
    1.0e+18,
    1.0e+19,
    1.0e+20,
    1.0e+21,
    1.0e+22,
};

static double ReadDouble_unguarded(char const* buffer, int buffer_length)
{
    int64_t result = 0;

    for (int i = 0; i != buffer_length; ++i)
    {
        assert(buffer[i] >= '0');
        assert(buffer[i] <= '9');
        result = 10 * result + (buffer[i] - '0');
    }

    return static_cast<double>(result);
}

bool FastStrtod(char const* buffer, int buffer_length, int exponent, double& result)
{
    if (buffer_length > kMaxExactDoubleIntegerDecimalDigits)
        return false;

    // The trimmed input fits into a double.
    // If the 10^exponent (resp. 10^-exponent) fits into a double too then we
    // can compute the result-double simply by multiplying (resp. dividing) the
    // two numbers.
    // This is possible because IEEE guarantees that floating-point operations
    // return the best possible approximation.

    if (exponent < 0 && -exponent < kExactPowersOfTenSize)
    {
        // 10^-exponent fits into a double.
        double d = ReadDouble_unguarded(buffer, buffer_length);
        d /= kExactPowersOfTen[-exponent];
        result = d;
        return true;
    }

    if (0 <= exponent && exponent < kExactPowersOfTenSize)
    {
        // 10^exponent fits into a double.
        double d = ReadDouble_unguarded(buffer, buffer_length);
        d *= kExactPowersOfTen[exponent];
        result = d;
        return true;
    }

    int const remaining_digits = kMaxExactDoubleIntegerDecimalDigits - buffer_length;
    if (0 <= exponent && exponent - remaining_digits < kExactPowersOfTenSize)
    {
        // The trimmed string was short and we can multiply it with
        // 10^remaining_digits. As a result the remaining exponent now fits
        // into a double too.
        double d = ReadDouble_unguarded(buffer, buffer_length);
        d *= kExactPowersOfTen[remaining_digits];
        d *= kExactPowersOfTen[exponent - remaining_digits];
        result = d;
        return true;
    }

    return false;
}

#endif // ^^^ CORRECT_DOUBLE_OPERATIONS

//--------------------------------------------------------------------------------------------------
// DiyFpStrtod
//--------------------------------------------------------------------------------------------------

// 2^64 = 18446744073709551616 > 10^19
static constexpr int const kMaxUint64DecimalDigits = 19; // std::numeric_limits<uint64_t>::digits10;

struct Double
{
    static constexpr uint64_t const kSignificandMask         = 0x000FFFFFFFFFFFFF;
    static constexpr uint64_t const kHiddenBit               = 0x0010000000000000;
    static constexpr int      const kSignificandSize         = 53;  // Includes the hidden bit.
    static constexpr int      const kPhysicalSignificandSize = 52;  // Excludes the hidden bit.
    static constexpr int      const kExponentBias            = 0x3FF + kPhysicalSignificandSize;
    static constexpr int      const kDenormalExponent        = -kExponentBias + 1;
    static constexpr int      const kMaxExponent             = 0x7FF - kExponentBias;
};

// ldexp: Convert x = f * 2^e to IEEE double precision.
double MakeDouble(uint64_t f, int e)
{
    while (f > Double::kHiddenBit + Double::kSignificandMask) {
        f >>= 1;
        assert(e < INT_MAX);
        e++;
    }

    if (e >= Double::kMaxExponent)
        return std::numeric_limits<double>::infinity();

    if (e < Double::kDenormalExponent)
        return 0;

    while (e > Double::kDenormalExponent && (f & Double::kHiddenBit) == 0) {
        f <<= 1;
        e--;
    }

    uint64_t biased_exponent;
    if (e == Double::kDenormalExponent && (f & Double::kHiddenBit) == 0)
        biased_exponent = 0;
    else
        biased_exponent = static_cast<uint64_t>(e + Double::kExponentBias);

    uint64_t const bits = (f & Double::kSignificandMask) | (biased_exponent << Double::kPhysicalSignificandSize);

    return ReinterpretBits<double>(bits);
}

// Reads a DiyFp from the buffer.
// The returned DiyFp is not necessarily normalized.
// If remaining_decimals is zero then the returned DiyFp is accurate.
// Otherwise it has been rounded and has error of at most 1/2 ulp.
DiyFp ReadDiyFp(char const* buffer, int buffer_length, int& remaining_decimals)
{
    uint64_t significand = 0;

    int const max_len = buffer_length <= kMaxUint64DecimalDigits ? static_cast<int>(buffer_length) : kMaxUint64DecimalDigits;
    int i = 0;
    for (; i != max_len; ++i)
    {
        assert(buffer[i] >= '0');
        assert(buffer[i] <= '9');
        auto const digit = static_cast<unsigned>(buffer[i] - '0');
        significand = 10 * significand + digit;
    }

    if (i != buffer_length)
    {
        assert(buffer[i] >= '0');
        assert(buffer[i] <= '9');
        if (buffer[i] >= '5')
        {
            assert(significand != UINT64_MAX);
            ++significand;
        }
    }

    remaining_decimals = buffer_length - i;
    return DiyFp(significand, 0);
}

// Returns a cached power of ten x ~= 10^n such that
//      n <= k < n + kCachedPowersDecimalDistance.
// The given k must satisfy
//      (1)     k >= kCachedPowersMinDecExp
//      (2)     k < kCachedPowersMaxDecExp + kCachedPowersDecExpStep
CachedPower GetCachedPowerForDecimalExponent(int64_t k)
{
    assert(k >= kCachedPowersMinDecExp);
    assert(k < kCachedPowersMaxDecExp + kCachedPowersDecExpStep);
    static_cast<void>(kCachedPowersMaxDecExp);

    auto const index = (-kCachedPowersMinDecExp + k) / kCachedPowersDecExpStep;
    assert(index >= 0);
    assert(index < kCachedPowersSize);
    static_cast<void>(kCachedPowersSize);

//  auto const cached = kCachedPowers[static_cast<size_t>(index)];
    auto const cached = kCachedPowers[index];
    assert(k >= cached.k);
    assert(k < cached.k + kCachedPowersDecExpStep);

    return cached;
}

// Returns 10^exponent as an exact DiyFp.
// The given exponent must be in the range [1, kCachedPowersDecExpStep).
DiyFp AdjustmentPowerOfTen(int64_t exponent)
{
    assert(exponent > 0);
    assert(exponent < kCachedPowersDecExpStep);

    // Simply hardcode the remaining powers for the given decimal exponent distance.
    static_assert(kCachedPowersDecExpStep == 8, "internal error");
    switch (exponent)
    {
    case 1: return DiyFp(0xA000000000000000, -60);
    case 2: return DiyFp(0xC800000000000000, -57);
    case 3: return DiyFp(0xFA00000000000000, -54);
    case 4: return DiyFp(0x9C40000000000000, -50);
    case 5: return DiyFp(0xC350000000000000, -47);
    case 6: return DiyFp(0xF424000000000000, -44);
    case 7: return DiyFp(0x9896800000000000, -40);
    default:
        assert(false && "unreachable");
        return {};
    }
}

// Returns the significand size for a given order of magnitude.
// If v = f*2^e with 2^p-1 <= f <= 2^p then p+e is v's order of magnitude.
// This function returns the number of significant binary digits v will have
// once it's encoded into a double. In almost all cases this is equal to
// kSignificandSize. The only exceptions are denormals. They start with
// leading zeroes and their effective significand-size is hence smaller.
int SignificandSizeForOrderOfMagnitude(int order)
{
    static constexpr int const kSignificandSize  = 53;  // Includes the hidden bit.
    static constexpr int const kDenormalExponent = 1 - (0x3FF + 52);

    if (order >= kDenormalExponent + kSignificandSize)
        return kSignificandSize;

    if (order <= kDenormalExponent)
        return 0;

    return order - kDenormalExponent;
}

// If the function returns true then the result is the correct double.
// Otherwise it is either the correct double or the double that is just below
// the correct double.
static bool DiyFpStrtod(char const* buffer, int buffer_length, int exponent, double& result)
{
    static_assert(DiyFp::kPrecision == 64, "We use uint64_ts. This only works if the DiyFp uses uint64_ts too.");

    int remaining_decimals;
    auto input = ReadDiyFp(buffer, buffer_length, remaining_decimals);

    // Since we may have dropped some digits the input is not accurate.
    // If remaining_decimals is different than 0 than the error is at most
    // .5 ulp (unit in the last place).
    // We don't want to deal with fractions and therefore keep a common
    // denominator.
    constexpr int kDenominatorLog = 3;
    constexpr int kDenominator = 1 << kDenominatorLog;

    // Move the remaining decimals into the exponent.
    exponent += remaining_decimals;
    uint64_t error = (remaining_decimals == 0) ? 0 : kDenominator / 2;

    {
        auto const old_e = input.e;
        input = DiyFp::normalize(input);
        error <<= old_e - input.e;
    }

    assert(exponent <= kCachedPowersMaxDecExp);
    if (exponent < kCachedPowersMinDecExp)
    {
        result = 0.0;
        return true;
    }

    auto const cached = GetCachedPowerForDecimalExponent(exponent);
    auto const cached_power = DiyFp(cached.f, cached.e);
    auto const cached_decimal_exponent = cached.k;

    if (cached_decimal_exponent != exponent)
    {
        auto const adjustment_exponent = exponent - cached_decimal_exponent;
        auto const adjustment_power = AdjustmentPowerOfTen(adjustment_exponent);

        input = DiyFp::mul(input, adjustment_power);

        if (kMaxUint64DecimalDigits - buffer_length >= adjustment_exponent)
        {
            // The product of input with the adjustment power fits into a 64 bit
            // integer.
        }
        else
        {
            // The adjustment power is exact. There is hence only an error of 0.5.
            error += kDenominator / 2;
        }
    }

    input = DiyFp::mul(input, cached_power);

    // The error introduced by a multiplication of a*b equals
    //   error_a + error_b + error_a*error_b/2^64 + 0.5
    // Substituting a with 'input' and b with 'cached_power' we have
    //   error_b = 0.5  (all cached powers have an error of less than 0.5 ulp),
    //   error_ab = 0 or 1 / kDenominator > error_a*error_b/ 2^64
    uint64_t const error_b = kDenominator / 2;
    uint64_t const error_ab = (error == 0) ? 0 : 1; // We round up to 1.
    uint64_t const fixed_error = kDenominator / 2;

    error += error_b + error_ab + fixed_error;

    {
        auto const old_e = input.e;
        input = DiyFp::normalize(input);
        error <<= old_e - input.e;
    }

    // See if the double's significand changes if we add/subtract the error.
    auto const order_of_magnitude = DiyFp::kPrecision + input.e;
    auto const effective_significand_size = SignificandSizeForOrderOfMagnitude(order_of_magnitude);

    int precision_digits_count = DiyFp::kPrecision - effective_significand_size;
    if (precision_digits_count + kDenominatorLog >= DiyFp::kPrecision)
    {
        // This can only happen for very small denormals. In this case the
        // half-way multiplied by the denominator exceeds the range of an uint64.
        // Simply shift everything to the right.

        auto const shift_amount = (precision_digits_count + kDenominatorLog) - DiyFp::kPrecision + 1;
        input.f = input.f >> shift_amount;
        input.e = input.e + shift_amount;

        // We add 1 for the lost precision of error, and kDenominator for
        // the lost precision of input.f().
        error = (error >> shift_amount) + 1 + kDenominator;
        precision_digits_count -= shift_amount;
    }

    // We use uint64_ts now. This only works if the DiyFp uses uint64_ts too.
    assert(precision_digits_count > 0);
    assert(precision_digits_count < 64);

    uint64_t const precision_bits = kDenominator * (input.f & ((uint64_t{1} << precision_digits_count) - 1));
    uint64_t const half_way       = kDenominator * (uint64_t{1} << (precision_digits_count - 1));

    auto rounded_input = DiyFp(input.f >> precision_digits_count, input.e + precision_digits_count);
    if (precision_bits >= half_way + error)
    {
        assert(rounded_input.f != UINT64_MAX);
        rounded_input.f += 1;
    }

    // If the last_bits are too close to the half-way case than we are too
    // inaccurate and round down. In this case we return false so that we can
    // fall back to a more precise algorithm.

    result = MakeDouble(rounded_input.f, rounded_input.e);

    if (half_way - error < precision_bits && precision_bits < half_way + error)
    {
        // Too imprecise. The caller will have to fall back to a slower version.
        // However the returned number is guaranteed to be either the correct
        // double, or the next-lower double.
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

// Max double: 1.7976931348623157 x 10^308
// Min non-zero double: 4.9406564584124654 x 10^-324
// Any x >= 10^309 is interpreted as +infinity.
// Any x <= 10^-324 is interpreted as 0.
// Note that 2.5e-324 (despite being smaller than the min double) will be read
// as non-zero (equal to the min non-zero double).
static constexpr int const kMaxDecimalPower =  309;
static constexpr int const kMinDecimalPower = -324;

// Returns true if the guess is the correct double.
// Returns false, when guess is either correct or the next-lower double.
static bool ComputeGuess(char const* buffer, int buffer_length, int exponent, double& guess)
{
    assert(buffer_length == 0 || (buffer[0] != '0' && buffer[buffer_length - 1] != '0'));

    if (buffer_length == 0) {
        guess = 0.0;
        return true;
    }
    if (exponent + buffer_length - 1 >= kMaxDecimalPower) {
        guess = std::numeric_limits<double>::infinity();
        return true;
    }
    if (exponent + buffer_length <= kMinDecimalPower) {
        guess = 0.0;
        return true;
    }

    if (FastStrtod(buffer, buffer_length, exponent, guess) || DiyFpStrtod(buffer, buffer_length, exponent, guess)) {
        return true;
    }
    if (guess == std::numeric_limits<double>::infinity()) {
        return true;
    }

    // guess has been assigned to in DiyFpStrtod.
    // guess is either correct or the next-lower double.
    return false;
}

double ReadDouble(char const* f, char const* l)
{
    assert(l - f > 0); // internal error
    assert(l - f <= kMaxExactDoubleIntegerDecimalDigits); // internal error

    int64_t val = 0;
    for ( ; f != l; ++f)
    {
        assert(*f >= '0'); // internal error
        assert(*f <= '9'); // internal error
        val = val * 10 + (*f - '0');
    }

    return static_cast<double>(val);
}

int CountTrailingZeros(char const* buffer, int buffer_length)
{
    int i = buffer_length;
    for ( ; i != 0; --i)
    {
        if (buffer[i - 1] != '0')
            break;
    }

    return buffer_length - i;
}

// Maximum number of significant digits in decimal representation.
// The longest possible double in decimal representation is
// (2^53 - 1) * 2^-1074 that is (2^53 - 1) * 5^1074 / 10^1074 (768 digits). If
// we parse a number whose first digits are equal to a mean of 2 adjacent
// doubles (that could have up to 769 digits) the result must be rounded to the
// bigger one unless the tail consists of zeros, so we don't need to preserve
// all the digits.
static constexpr int const kMaxSignificantDigits = 772;

bool IsDigit(char ch)
{
    return '0' <= ch && ch <= '9';
}

bool StringToIeee(double& result, char const* next, char const* last, Options const& options)
{
    // Inputs larger than kMaxInt (currently) can not be handled.
    // To avoid overflow in integer arithmetic.
    constexpr int const kMaxInt = INT_MAX / 4;

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

    static constexpr int const kBufferSize = kMaxSignificantDigits + 1;

    char digits[kBufferSize];
    int  num_digits = 0;
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
            if (num_digits < kMaxSignificantDigits)
            {
                digits[num_digits++] = *next;
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
#if 1
    if (next != last)
    {
        return false; // trailing junk
    }
#endif

    if (nonzero_digit_dropped)
    {
        // Set the last digit to be non-zero. This is sufficient to guarantee
        // correct rounding.
        assert(num_digits == kMaxSignificantDigits);
        assert(num_digits < kBufferSize);
        digits[num_digits++] = '1';
        --exponent;
    }
    else
    {
        int const num_zeros = CountTrailingZeros(digits, num_digits);
        exponent += num_zeros;
        num_digits -= num_zeros;
    }

#if 1
    if (exponent == 0 && num_digits <= kMaxExactDoubleIntegerDecimalDigits)
    {
        double d = ReadDouble(digits, digits + num_digits);
        result = is_neg ? -d : d;
        return true;
    }
#endif

    double guess;
#if 1
    ComputeGuess(digits, num_digits, exponent, guess);
#else
    if (ComputeGuess(digits, num_digits, exponent, guess))
#endif
    {
        result = is_neg ? -guess : guess;
        return true;
    }
}

} // namespace

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------

#if 0
double json::numbers::StringToNumber(char const* first, char const* last, NumberClass nc, Options const& options)
{
    assert(first != last);
    assert(nc != NumberClass::invalid);

    if (first == last)
        return std::numeric_limits<double>::quiet_NaN();

    // Use a _slightly_ faster method for parsing integers which will fit into a
    // double-precision number without loss of precision. Larger numbers will be
    // handled by strtod.
    if (nc == NumberClass::integer && last - first <= kMaxExactDoubleIntegerDecimalDigits)
    {
        bool const is_neg = *first == '-';
        if (is_neg || *first == '+')
        {
            ++first;
        }

        double const result = ReadDouble(first, last);
        return is_neg ? -result : result;
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

    double result;
    if (StringToIeee(result, first, last))
    {
        return result;
    }

    assert(false && "unreachable");
    return std::numeric_limits<double>::quiet_NaN();
}
#endif

bool json::numbers::StringToNumber(double& result, char const* first, char const* last, Options const& options)
{
    if (StringToIeee(result, first, last, options))
        return true;

    result = std::numeric_limits<double>::quiet_NaN();
    return false;
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

/*
Copyright 2006-2011, the V8 project authors. All rights reserved.
Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.
    * Neither the name of Google Inc. nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
