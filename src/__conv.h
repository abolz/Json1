#pragma once

#include <cassert>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <limits>
#include <type_traits>

#if _MSC_VER
#include <intrin.h>
#endif

//==================================================================================================
//
//==================================================================================================

namespace iconv {

inline char* itoa100(char* buf, uint32_t digits)
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

inline char* u32toa(char* buf, uint32_t n)
{
    uint32_t q;

    if (n >= 1000000000)
    {
//L_10_digits:
        q = n / 100000000;
        n = n % 100000000;
        buf = itoa100(buf, q);
L_8_digits:
        q = n / 1000000;
        n = n % 1000000;
        buf = itoa100(buf, q);
L_6_digits:
        q = n / 10000;
        n = n % 10000;
        buf = itoa100(buf, q);
L_4_digits:
        q = n / 100;
        n = n % 100;
        buf = itoa100(buf, q);
//L_2_digits:
        return itoa100(buf, n);
    }

    if (n < 100) {
        if (n >= 10) {
            return itoa100(buf, n);
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
        buf = itoa100(buf, q);
L_7_digits:
        q = n / 100000;
        n = n % 100000;
        buf = itoa100(buf, q);
L_5_digits:
        q = n / 1000;
        n = n % 1000;
        buf = itoa100(buf, q);
L_3_digits:
        q = n / 10;
        n = n % 10;
        buf = itoa100(buf, q);
//L_1_digit:
        buf[0] = static_cast<char>('0' + n);
        return buf + 1;
    }
}

inline char* u32toa_n(char* buf, uint32_t n, int num_digits)
{
    uint32_t q;
    switch (num_digits)
    {
// Generate an even number digits
    case 10:
        q = n / 100000000;
        n = n % 100000000;
        buf = itoa100(buf, q);
        // fall through
    case 8:
        assert(n < 100000000);
        q = n / 1000000;
        n = n % 1000000;
        buf = itoa100(buf, q);
        // fall through
    case 6:
        assert(n < 1000000);
        q = n / 10000;
        n = n % 10000;
        buf = itoa100(buf, q);
        // fall through
    case 4:
        assert(n < 10000);
        q = n / 100;
        n = n % 100;
        buf = itoa100(buf, q);
        // fall through
    case 2:
        assert(n < 100);
        buf = itoa100(buf, n);
        return buf;

// Generate an odd number of digits:
    case 9:
        assert(n < 1000000000);
        q = n / 10000000;
        n = n % 10000000;
        buf = itoa100(buf, q);
        // fall through
    case 7:
        assert(n < 10000000);
        q = n / 100000;
        n = n % 100000;
        buf = itoa100(buf, q);
        // fall through
    case 5:
        assert(n < 100000);
        q = n / 1000;
        n = n % 1000;
        buf = itoa100(buf, q);
        // fall through
    case 3:
        assert(n < 1000);
        q = n / 10;
        n = n % 10;
        buf = itoa100(buf, q);
        // fall through
    case 1:
        assert(n < 10);
        buf[0] = static_cast<char>('0' + n);
        return buf + 1;
    }

    return buf;
}

inline char* u64toa(char* buf, uint64_t n)
{
    if (n <= UINT32_MAX)
        return u32toa(buf, static_cast<uint32_t>(n));

    // n = hi * 10^9 + lo < 10^20,   where hi < 10^11, lo < 10^9
    const uint64_t hi = n / 1000000000;
    const uint64_t lo = n % 1000000000;

    if (hi <= UINT32_MAX)
    {
        buf = u32toa(buf, static_cast<uint32_t>(hi));
    }
    else
    {
        // 2^32 < hi = hi1 * 10^9 + hi0 < 10^11,   where hi1 < 10^2, 10^9 <= hi0 < 10^9
        const uint32_t hi1 = static_cast<uint32_t>(hi / 1000000000);
        const uint32_t hi0 = static_cast<uint32_t>(hi % 1000000000);
        buf = u32toa_n(buf, hi1, hi1 >= 10 ? 2 : 1);
        buf = u32toa_n(buf, hi0, 9);
    }

    // lo has exactly 9 digits.
    // (Which might all be zero...)
    return u32toa_n(buf, static_cast<uint32_t>(lo), 9);
}

inline char* s32toa(char* buf, int32_t i)
{
    uint32_t n = static_cast<uint32_t>(i);
    if (i < 0)
    {
        buf[0] = '-';
        buf++;
        n = 0 - n;
    }

    return u32toa(buf, n);
}

inline char* s64toa(char* buf, int64_t i)
{
    uint64_t n = static_cast<uint64_t>(i);
    if (i < 0)
    {
        buf[0] = '-';
        buf++;
        n = 0 - n;
    }

    return u64toa(buf, n);
}

} // namespace iconv

//==================================================================================================
//
//==================================================================================================

// Implements the Grisu2 algorithm for binary to decimal floating-point
// conversion.
//
// This implementation is a slightly modified version of the reference
// implementation by Florian Loitsch which can be obtained from
// http://florian.loitsch.com/publications (bench.tar.gz)
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
inline Target ReinterpretBits(Source source)
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
inline Boundaries ComputeBoundaries(FloatType value)
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
inline CachedPower GetCachedPowerForBinaryExponent(int e)
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

#if 0
// For n != 0, returns k, such that pow10 := 10^(k-1) <= n < 10^k.
// For n == 0, returns 1 and sets pow10 := 1.
inline int FindLargestPow10(uint32_t n, uint32_t& pow10)
{
    if (n >= 1000000000) { pow10 = 1000000000; return 10; }
    if (n >=  100000000) { pow10 =  100000000; return  9; }
    if (n >=   10000000) { pow10 =   10000000; return  8; }
    if (n >=    1000000) { pow10 =    1000000; return  7; }
    if (n >=     100000) { pow10 =     100000; return  6; }
    if (n >=      10000) { pow10 =      10000; return  5; }
    if (n >=       1000) { pow10 =       1000; return  4; }
    if (n >=        100) { pow10 =        100; return  3; }
    if (n >=         10) { pow10 =         10; return  2; }

    pow10 = 1; return 1;
}
#endif

inline void Grisu2Round(char* buf, int len, uint64_t dist, uint64_t delta, uint64_t rest, uint64_t ten_k)
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
inline void Grisu2DigitGen(char* buffer, int& length, int& decimal_exponent, DiyFp M_minus, DiyFp w, DiyFp M_plus)
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

#if 0
    uint32_t pow10;
    const int k = FindLargestPow10(p1, pow10);
#endif

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

#if 1
    // The common case is that all the digits of p1 are needed.
    // Optimize for this case and correct later if required.
    char const* last = iconv::u32toa(buffer, p1);
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

        uint32_t const D1 = static_cast<uint32_t>((delta - p2) >> -one.e);

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

            uint32_t r_next = pow10 * static_cast<uint32_t>(buffer[k - (n + 1)] - '0') + r;
            if (r_next > D1)
                break;
            r = r_next;
            n++;
            pow10 *= 10;
        }
        length = k - n;

        // V = buffer * 10^n, with M- <= V <= M+.

        decimal_exponent += n;

        uint64_t const rest = (uint64_t{r} << -one.e) + p2;
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
        uint64_t const ten_n = uint64_t{pow10} << -one.e;
        Grisu2Round(buffer, length, dist, delta, rest, ten_n);

        return;
    }
#else
    int n = k;
    while (n > 0)
    {
        // Invariants:
        //      M+ = buffer * 10^n + (p1 + p2 * 2^e)    (buffer = 0 for n = k)
        //      pow10 = 10^(n-1) <= p1 < 10^n
        //
        const uint32_t d = p1 / pow10;  // d = p1 div 10^(n-1)
        const uint32_t r = p1 % pow10;  // r = p1 mod 10^(n-1)
        //
        //      M+ = buffer * 10^n + (d * 10^(n-1) + r) + p2 * 2^e
        //         = (buffer * 10 + d) * 10^(n-1) + (r + p2 * 2^e)
        //
        assert(d <= 9);
        buffer[length++] = static_cast<char>('0' + d); // buffer := buffer * 10 + d
        //
        //      M+ = buffer * 10^(n-1) + (r + p2 * 2^e)
        //
        p1 = r;
        n--;
        //
        //      M+ = buffer * 10^n + (p1 + p2 * 2^e)
        //      pow10 = 10^n
        //

        // Now check if enough digits have been generated.
        // Compute
        //
        //      p1 + p2 * 2^e = (p1 * 2^-e + p2) * 2^e = rest * 2^e
        //
        // Note:
        // Since rest and delta share the same exponent e, it suffices to
        // compare the significands.
        const uint64_t rest = (uint64_t{p1} << -one.e) + p2;
        if (rest <= delta)
        {
            // V = buffer * 10^n, with M- <= V <= M+.

            decimal_exponent += n;

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

        pow10 /= 10;
        //
        //      pow10 = 10^(n-1) <= p1 < 10^n
        // Invariants restored.
    }
#endif

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
inline void Grisu2(char* buf, int& len, int& decimal_exponent, DiyFp m_minus, DiyFp v, DiyFp m_plus)
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
inline void Grisu2(char* buf, int& len, int& decimal_exponent, FloatType value)
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
inline char* AppendExponent(char* buf, int e)
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
        buf = iconv::itoa100(buf, k);
    }
    else
    {
        uint32_t q = k / 100;
        uint32_t r = k % 100;
        buf[0] = static_cast<char>('0' + q);
        buf = iconv::itoa100(buf + 1, r);
    }

    return buf;
}

// Prettify v = buf * 10^decimal_exponent
// If v is in the range [10^min_exp, 10^max_exp) it will be printed in fixed-point notation.
// Otherwise it will be printed in exponential notation.
// PRE: min_exp < 0
// PRE: max_exp > 0
inline char* FormatBuffer(char* buf, int len, int decimal_exponent, int min_exp, int max_exp)
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
inline char* ToShort(char* first, char* last, FloatType value)
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

    // Format the buffer like printf("%.*g", prec, value)
    constexpr int kMinExp = -4;
#if 0
    // Using digits10 here for compatibility with version 2.
    constexpr int kMaxExp = std::numeric_limits<FloatType>::digits10;
#else
    constexpr int kMaxExp = std::numeric_limits<FloatType>::max_digits10;
#endif

    assert(last - first >=
        std::max({
            kMaxExp,
            2 + (-kMinExp - 1) + std::numeric_limits<FloatType>::max_digits10,
            std::numeric_limits<FloatType>::max_digits10 + 6}));

    return FormatBuffer(first, len, decimal_exponent, kMinExp, kMaxExp);
}

} // namespace dconv

/*
Copyright (c) 2009 Florian Loitsch
Copyright (c) 2017 Alexander Bolz

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
