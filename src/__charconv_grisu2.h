// Copyright 2017 Alexander Bolz
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

// sizeof(tables) = 201 + 184 + 40 + 56 + (680 + 64) = 2225

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <limits>

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#ifndef CC_ASSERT
#define CC_ASSERT(X) assert(X)
#endif

namespace charconv_internal {

//==================================================================================================
// IEEE double-/single-precision inspection
//==================================================================================================

template <typename Dest, typename Source>
inline Dest ReinterpretBits(Source source)
{
    static_assert(sizeof(Dest) == sizeof(Source), "size mismatch");

    Dest dest;
    std::memcpy(&dest, &source, sizeof(Source));
    return dest;
}

struct Double
{
    static_assert(std::numeric_limits<double>::is_iec559
                  && std::numeric_limits<double>::digits == 53
                  && std::numeric_limits<double>::max_exponent == 1024,
        "IEEE-754 double-precision implementation required");

    static constexpr int      SignificandSize         = std::numeric_limits<double>::digits;    // = p   (includes the hidden bit)
    static constexpr int      PhysicalSignificandSize = SignificandSize - 1;                    // = p-1 (excludes the hidden bit)
    static constexpr int      UnbiasedMinExponent     = 1;
    static constexpr int      UnbiasedMaxExponent     = 2 * std::numeric_limits<double>::max_exponent - 1 - 1;
    static constexpr int      ExponentBias            = 2 * std::numeric_limits<double>::max_exponent / 2 - 1 + (SignificandSize - 1);
    static constexpr int      MinExponent             = UnbiasedMinExponent - ExponentBias;
    static constexpr int      MaxExponent             = UnbiasedMaxExponent - ExponentBias;
    static constexpr uint64_t HiddenBit               = uint64_t{1} << (SignificandSize - 1);   // = 2^(p-1)
    static constexpr uint64_t SignificandMask         = HiddenBit - 1;                          // = 2^(p-1) - 1
    static constexpr uint64_t ExponentMask            = uint64_t{2 * std::numeric_limits<double>::max_exponent - 1} << PhysicalSignificandSize;
    static constexpr uint64_t SignMask                = ~(~uint64_t{0} >> 1);

    uint64_t /*const*/ bits;

    explicit Double(uint64_t bits_) : bits(bits_) {}
    explicit Double(double value) : bits(ReinterpretBits<uint64_t>(value)) {}

    uint64_t PhysicalSignificand() const {
        return bits & SignificandMask;
    }

    uint64_t PhysicalExponent() const {
        return (bits & ExponentMask) >> PhysicalSignificandSize;
    }

    bool IsFinite() const {
        return (bits & ExponentMask) != ExponentMask;
    }

    bool IsInf() const {
        return (bits & ExponentMask) == ExponentMask && (bits & SignificandMask) == 0;
    }

    bool IsNaN() const {
        return (bits & ExponentMask) == ExponentMask && (bits & SignificandMask) != 0;
    }

    bool IsZero() const {
        return (bits & ~SignMask) == 0;
    }

    bool SignBit() const {
        return (bits & SignMask) != 0;
    }

    double Value() const {
        return ReinterpretBits<double>(bits);
    }

    double AbsValue() const {
        return ReinterpretBits<double>(bits & ~SignMask);
    }

    double NextValue() const {
        CC_ASSERT(!SignBit());
        return ReinterpretBits<double>(IsInf() ? bits : bits + 1);
    }
};

//==================================================================================================
// Itoa helper
//==================================================================================================

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

    CC_ASSERT(digits < 100);
    std::memcpy(buf, kDigits100 + 2*digits, 2 * sizeof(char));
    return buf + 2;
}

inline char* Utoa_4Digits(char* buf, uint32_t digits)
{
    CC_ASSERT(digits < 10000);
    uint32_t const q = digits / 100;
    uint32_t const r = digits % 100;
    Utoa_2Digits(buf + 0, q);
    Utoa_2Digits(buf + 2, r);
    return buf + 4;
}

inline char* Utoa_8Digits(char* buf, uint32_t digits)
{
    CC_ASSERT(digits < 100000000);
    uint32_t const q = digits / 10000;
    uint32_t const r = digits % 10000;
    Utoa_4Digits(buf + 0, q);
    Utoa_4Digits(buf + 4, r);
    return buf + 8;
}

//==================================================================================================
// DoubleToDecimal
//
// Implements the Grisu2 algorithm for (IEEE) binary to decimal floating-point conversion.
//
// This implementation is a slightly modified version of the reference
// implementation by Florian Loitsch which can be obtained from
// http://florian.loitsch.com/publications (bench.tar.gz)
//
// The license can be found below.
//
// References:
//
// [1]  Loitsch, "Printing Floating-Point Numbers Quickly and Accurately with Integers",
//      Proceedings of the ACM SIGPLAN 2010 Conference on Programming Language Design and Implementation, PLDI 2010
// [2]  Burger, Dybvig, "Printing Floating-Point Numbers Quickly and Accurately",
//      Proceedings of the ACM SIGPLAN 1996 Conference on Programming Language Design and Implementation, PLDI 1996
//==================================================================================================

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

// sizeof(tables) = ??? bytes

struct DiyFp // f * 2^e
{
    static constexpr int SignificandSize = 64; // = q

    uint64_t f = 0;
    int e = 0;

    constexpr DiyFp() = default;
    constexpr DiyFp(uint64_t f_, int e_) : f(f_), e(e_) {}
};

// Returns the number of leading 0-bits in x, starting at the most significant
// bit position.
// If x is 0, the result is undefined.
inline int CountLeadingZeros64(uint64_t x)
{
    CC_ASSERT(x != 0);

#if defined(_MSC_VER) && (defined(_M_ARM) || defined(_M_ARM64)) && !defined(__clang__)

    return static_cast<int>(_CountLeadingZeros64(x));

#elif defined(_MSC_VER) && defined(_M_X64) && !defined(__clang__)

    return static_cast<int>(__lzcnt64(x));

#elif defined(_MSC_VER) && defined(_M_IX86) && !defined(__clang__)

    int lz = static_cast<int>( __lzcnt(static_cast<uint32_t>(x >> 32)) );
    if (lz == 32) {
        lz += static_cast<int>( __lzcnt(static_cast<uint32_t>(x)) );
    }
    return lz;

#elif defined(__GNUC__) || defined(__clang__)

    return __builtin_clzll(x);

#else

    int lz = 0;
    while ((x >> 63) == 0) {
        x <<= 1;
        ++lz;
    }
    return lz;

#endif
}

// Normalize x.
inline DiyFp Normalize(DiyFp x)
{
    int const s = CountLeadingZeros64(x.f);
    return DiyFp(x.f << s, x.e - s);
}

// Normalize x such that the result has the exponent E.
// PRE: e >= x.e and the upper e - x.e bits of x.f must be zero.
inline DiyFp NormalizeTo(DiyFp x, int e)
{
    int const delta = x.e - e;

    CC_ASSERT(delta >= 0);
    CC_ASSERT(((x.f << delta) >> delta) == x.f);

    return DiyFp(x.f << delta, e);
}

// Returns whether the given floating point value is normalized.
inline bool IsNormalized(DiyFp x)
{
    static_assert(DiyFp::SignificandSize == 64, "internal error");

    return x.f >= (uint64_t{1} << 63);
}

// Returns x - y.
// PRE: x.e == y.e and x.f >= y.f
inline DiyFp Subtract(DiyFp x, DiyFp y)
{
    CC_ASSERT(x.e == y.e);
    CC_ASSERT(x.f >= y.f);

    return DiyFp(x.f - y.f, x.e);
}

// Returns x * y.
// The result is rounded (ties up). (Only the upper q bits are returned.)
inline DiyFp Multiply(DiyFp x, DiyFp y)
{
    static_assert(DiyFp::SignificandSize == 64, "internal error");

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

    Uint128 const p = Uint128{x.f} * y.f + (uint64_t{1} << 63);
    return DiyFp(static_cast<uint64_t>(p >> 64), x.e + y.e + 64);

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

    // Note:
    // The 32/64-bit casts here help MSVC to avoid calls to the _allmul
    // library function.

    uint32_t const u_lo = static_cast<uint32_t>(x.f /*& 0xFFFFFFFF*/);
    uint32_t const u_hi = static_cast<uint32_t>(x.f >> 32);
    uint32_t const v_lo = static_cast<uint32_t>(y.f /*& 0xFFFFFFFF*/);
    uint32_t const v_hi = static_cast<uint32_t>(y.f >> 32);

    uint64_t const p0 = uint64_t{u_lo} * v_lo;
    uint64_t const p1 = uint64_t{u_lo} * v_hi;
    uint64_t const p2 = uint64_t{u_hi} * v_lo;
    uint64_t const p3 = uint64_t{u_hi} * v_hi;

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

    Q += uint32_t{1} << (63 - 32); // round, ties up

    uint64_t const h = p3 + p2_hi + p1_hi + (Q >> 32);

    return DiyFp(h, x.e + y.e + 64);

#endif
}

// Decomposes `value` into `f * 2^e`.
// The result is not normalized.
// PRE: `value` must be finite and non-negative, i.e. >= +0.0.
inline DiyFp DiyFpFromFloat(double value)
{
    auto const v = Double(value);

    CC_ASSERT(v.IsFinite());
    CC_ASSERT(!v.SignBit());

    auto const F = v.PhysicalSignificand();
    auto const E = v.PhysicalExponent();

    // If v is denormal:
    //      value = 0.F * 2^(1 - bias) = (          F) * 2^(1 - bias - (p-1))
    // If v is normalized:
    //      value = 1.F * 2^(E - bias) = (2^(p-1) + F) * 2^(E - bias - (p-1))

    return (E == 0) // denormal?
        ? DiyFp(F, Double::MinExponent)
        : DiyFp(F + Double::HiddenBit, static_cast<int>(E) - Double::ExponentBias);
}

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

// Returns the upper boundary of value, i.e. the upper bound of the rounding
// interval for v.
// The result is not normalized.
// PRE: `value` must be finite and non-negative.
inline DiyFp UpperBoundary(double value)
{
    auto const v = DiyFpFromFloat(value);
//  return DiyFp(2*v.f + 1, v.e - 1);
    return DiyFp(4*v.f + 2, v.e - 2);
}

inline bool LowerBoundaryIsCloser(double value)
{
    Double const v(value);

    CC_ASSERT(v.IsFinite());
    CC_ASSERT(!v.SignBit());

    auto const F = v.PhysicalSignificand();
    auto const E = v.PhysicalExponent();
    return F == 0 && E > 1;
}

// Returns the lower boundary of `value`, i.e. the lower bound of the rounding
// interval for `value`.
// The result is not normalized.
// PRE: `value` must be finite and strictly positive.
inline DiyFp LowerBoundary(double value)
{
    CC_ASSERT(Double(value).IsFinite());
    CC_ASSERT(value > 0);

    auto const v = DiyFpFromFloat(value);
    return DiyFp(4*v.f - 2 + (LowerBoundaryIsCloser(value) ? 1 : 0), v.e - 2);
}

struct Boundaries {
    DiyFp v;
    DiyFp m_minus;
    DiyFp m_plus;
};

// Compute the (normalized) DiyFp representing the input number 'value' and its
// boundaries.
// PRE: 'value' must be finite and positive
inline Boundaries ComputeBoundaries(double value)
{
    CC_ASSERT(Double(value).IsFinite());
    CC_ASSERT(value > 0);

    auto const v = DiyFpFromFloat(value);

    // Compute the boundaries of v.
    auto const m_plus = DiyFp(4*v.f + 2, v.e - 2);
    auto const m_minus = DiyFp(4*v.f - 2 + (LowerBoundaryIsCloser(value) ? 1 : 0), v.e - 2);

    // Determine the normalized w = v.
    auto const w = Normalize(v);

    // Determine the normalized w+ = m+.
    // Since e_(w+) == e_(w), one can use NormalizeTo instead of Normalize.
    auto const w_plus = NormalizeTo(m_plus, w.e);

    // Determine w- = m- such that e_(w-) = e_(w+).
    auto const w_minus = NormalizeTo(m_minus, w_plus.e);

    return {w, w_minus, w_plus};
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

struct CachedPower { // c = f * 2^e ~= 10^k
    uint64_t f;
    int e; // binary exponent
    int k; // decimal exponent
};

constexpr int kCachedPowersSize         =   85;
constexpr int kCachedPowersMinDecExp    = -348;
constexpr int kCachedPowersMaxDecExp    =  324;
constexpr int kCachedPowersDecExpStep   =    8;

// Returns the binary exponent of a cached power for a given decimal exponent.
inline int BinaryExponentFromDecimalExponent(int k)
{
    CC_ASSERT(k <=  400);
    CC_ASSERT(k >= -400);

    // log_2(10) ~= [3; 3, 9, 2, 2, 4, 6, 2, 1, 1, 3] = 254370/76573
    // 2^15 * 254370/76573 = 108852.93980907...

//  return (k * 108853) / (1 << 15) - (k < 0) - 63;
//  return ((k * 108853) >> 15) - 63;
    return (k * 108853 - 63 * (1 << 15)) >> 15;
}

#if 0
// Returns the decimal for a cached power with the given binary exponent.
inline int DecimalExponentFromBinaryExponent(int e)
{
    CC_ASSERT(e <=  1265);
    CC_ASSERT(e >= -1392);

    // log_10(2) ~= [0; 3, 3, 9, 2, 2, 4, 6, 2, 1, 1, 3] = 76573/254370
    // 2^18 * 76573/254370 = 78913.20718638...

//  return ((e + 63) * 78913) / (1 << 18) + (e + 63 > 0);
//  return -(((e + 63) * -78913) >> 18);
    return -((e * -78913 - 63 * 78913) >> 18);
}
#endif

inline CachedPower GetCachedPower(int index)
{
    static constexpr uint64_t kSignificands[/*680 bytes*/] = {
        0xFA8FD5A0081C0288, // e = -1220, k = -348, //*
        0xBAAEE17FA23EBF76, // e = -1193, k = -340, //*
        0x8B16FB203055AC76, // e = -1166, k = -332, //*
        0xCF42894A5DCE35EA, // e = -1140, k = -324, //*
        0x9A6BB0AA55653B2D, // e = -1113, k = -316, //*
        0xE61ACF033D1A45DF, // e = -1087, k = -308, //*
        0xAB70FE17C79AC6CA, // e = -1060, k = -300, // >>> double-precision (-1060 + 960 + 64 = -36)
        0xFF77B1FCBEBCDC4F, // e = -1034, k = -292,
        0xBE5691EF416BD60C, // e = -1007, k = -284,
        0x8DD01FAD907FFC3C, // e =  -980, k = -276,
        0xD3515C2831559A83, // e =  -954, k = -268,
        0x9D71AC8FADA6C9B5, // e =  -927, k = -260,
        0xEA9C227723EE8BCB, // e =  -901, k = -252,
        0xAECC49914078536D, // e =  -874, k = -244,
        0x823C12795DB6CE57, // e =  -847, k = -236,
        0xC21094364DFB5637, // e =  -821, k = -228,
        0x9096EA6F3848984F, // e =  -794, k = -220,
        0xD77485CB25823AC7, // e =  -768, k = -212,
        0xA086CFCD97BF97F4, // e =  -741, k = -204,
        0xEF340A98172AACE5, // e =  -715, k = -196,
        0xB23867FB2A35B28E, // e =  -688, k = -188,
        0x84C8D4DFD2C63F3B, // e =  -661, k = -180,
        0xC5DD44271AD3CDBA, // e =  -635, k = -172,
        0x936B9FCEBB25C996, // e =  -608, k = -164,
        0xDBAC6C247D62A584, // e =  -582, k = -156,
        0xA3AB66580D5FDAF6, // e =  -555, k = -148,
        0xF3E2F893DEC3F126, // e =  -529, k = -140,
        0xB5B5ADA8AAFF80B8, // e =  -502, k = -132,
        0x87625F056C7C4A8B, // e =  -475, k = -124,
        0xC9BCFF6034C13053, // e =  -449, k = -116,
        0x964E858C91BA2655, // e =  -422, k = -108,
        0xDFF9772470297EBD, // e =  -396, k = -100,
        0xA6DFBD9FB8E5B88F, // e =  -369, k =  -92,
        0xF8A95FCF88747D94, // e =  -343, k =  -84,
        0xB94470938FA89BCF, // e =  -316, k =  -76,
        0x8A08F0F8BF0F156B, // e =  -289, k =  -68,
        0xCDB02555653131B6, // e =  -263, k =  -60,
        0x993FE2C6D07B7FAC, // e =  -236, k =  -52,
        0xE45C10C42A2B3B06, // e =  -210, k =  -44,
        0xAA242499697392D3, // e =  -183, k =  -36, // >>> single-precision (-183 + 80 + 64 = -39)
        0xFD87B5F28300CA0E, // e =  -157, k =  -28, //
        0xBCE5086492111AEB, // e =  -130, k =  -20, //
        0x8CBCCC096F5088CC, // e =  -103, k =  -12, //
        0xD1B71758E219652C, // e =   -77, k =   -4, //
        0x9C40000000000000, // e =   -50, k =    4, //
        0xE8D4A51000000000, // e =   -24, k =   12, //
        0xAD78EBC5AC620000, // e =     3, k =   20, //
        0x813F3978F8940984, // e =    30, k =   28, //
        0xC097CE7BC90715B3, // e =    56, k =   36, //
        0x8F7E32CE7BEA5C70, // e =    83, k =   44, // <<< single-precision (83 - 196 + 64 = -49)
        0xD5D238A4ABE98068, // e =   109, k =   52,
        0x9F4F2726179A2245, // e =   136, k =   60,
        0xED63A231D4C4FB27, // e =   162, k =   68,
        0xB0DE65388CC8ADA8, // e =   189, k =   76,
        0x83C7088E1AAB65DB, // e =   216, k =   84,
        0xC45D1DF942711D9A, // e =   242, k =   92,
        0x924D692CA61BE758, // e =   269, k =  100,
        0xDA01EE641A708DEA, // e =   295, k =  108,
        0xA26DA3999AEF774A, // e =   322, k =  116,
        0xF209787BB47D6B85, // e =   348, k =  124,
        0xB454E4A179DD1877, // e =   375, k =  132,
        0x865B86925B9BC5C2, // e =   402, k =  140,
        0xC83553C5C8965D3D, // e =   428, k =  148,
        0x952AB45CFA97A0B3, // e =   455, k =  156,
        0xDE469FBD99A05FE3, // e =   481, k =  164,
        0xA59BC234DB398C25, // e =   508, k =  172,
        0xF6C69A72A3989F5C, // e =   534, k =  180,
        0xB7DCBF5354E9BECE, // e =   561, k =  188,
        0x88FCF317F22241E2, // e =   588, k =  196,
        0xCC20CE9BD35C78A5, // e =   614, k =  204,
        0x98165AF37B2153DF, // e =   641, k =  212,
        0xE2A0B5DC971F303A, // e =   667, k =  220,
        0xA8D9D1535CE3B396, // e =   694, k =  228,
        0xFB9B7CD9A4A7443C, // e =   720, k =  236,
        0xBB764C4CA7A44410, // e =   747, k =  244,
        0x8BAB8EEFB6409C1A, // e =   774, k =  252,
        0xD01FEF10A657842C, // e =   800, k =  260,
        0x9B10A4E5E9913129, // e =   827, k =  268,
        0xE7109BFBA19C0C9D, // e =   853, k =  276,
        0xAC2820D9623BF429, // e =   880, k =  284,
        0x80444B5E7AA7CF85, // e =   907, k =  292,
        0xBF21E44003ACDD2D, // e =   933, k =  300,
        0x8E679C2F5E44FF8F, // e =   960, k =  308,
        0xD433179D9C8CB841, // e =   986, k =  316,
        0x9E19DB92B4E31BA9, // e =  1013, k =  324, // <<< double-precision (1013 - 1137 + 64 = -60)
    };

    CC_ASSERT(index >= 0);
    CC_ASSERT(index < kCachedPowersSize);

    int const k = kCachedPowersMinDecExp + index * kCachedPowersDecExpStep;
    int const e = BinaryExponentFromDecimalExponent(k);

    return {kSignificands[index], e, k};
}

// For a normalized DiyFp w = f * 2^e, this function returns a (normalized)
// cached power-of-ten c = f_c * 2^e_c, such that the exponent of the product
// w * c satisfies (Definition 3.2 from [1])
//
//      alpha <= e_c + e + q <= gamma.
//
inline CachedPower GetCachedPowerForBinaryExponent(int e)
{
    CC_ASSERT(e <=  1265);
    CC_ASSERT(e >= -1392);

    // k = ceil((kAlpha - e - 1) * log_10(2))
    int const k = (e * -78913 + ((kAlpha - 1) * 78913 + (1 << 18))) >> 18;
    CC_ASSERT(k >= kCachedPowersMinDecExp);
    CC_ASSERT(k <= kCachedPowersMaxDecExp);

    int const index = static_cast<int>( static_cast<unsigned>(-kCachedPowersMinDecExp + k + (kCachedPowersDecExpStep - 1)) / kCachedPowersDecExpStep );
    CC_ASSERT(index >= 0);
    CC_ASSERT(index < kCachedPowersSize);
    static_cast<void>(kCachedPowersSize);

    auto const cached = GetCachedPower(index);
    CC_ASSERT(kAlpha <= cached.e + e + 64);
    CC_ASSERT(kGamma >= cached.e + e + 64);

    // NB:
    // Actually this function returns c, such that -60 <= e_c + e + 64 <= -34.
    CC_ASSERT(-60 <= cached.e + e + 64);
    CC_ASSERT(-34 >= cached.e + e + 64);

    return cached;
}

inline char* GenerateIntegralDigits(char* buf, uint32_t n)
{
    CC_ASSERT(n <= 798336123);

    uint32_t q;

    if (n >= 100000000)
    {
//L_9_digits:
        q = n / 10000000;
        n = n % 10000000;
        buf = Utoa_2Digits(buf, q);
L_7_digits:
        q = n / 100000;
        n = n % 100000;
        buf = Utoa_2Digits(buf, q);
L_5_digits:
        q = n / 1000;
        n = n % 1000;
        buf = Utoa_2Digits(buf, q);
L_3_digits:
        q = n / 10;
        n = n % 10;
        buf = Utoa_2Digits(buf, q);
L_1_digit:
        buf[0] = static_cast<char>('0' + n);
        buf++;
        return buf;
    }

    if (n >= 10000000)
    {
//L_8_digits:
        q = n / 1000000;
        n = n % 1000000;
        buf = Utoa_2Digits(buf, q);
L_6_digits:
        q = n / 10000;
        n = n % 10000;
        buf = Utoa_2Digits(buf, q);
L_4_digits:
        q = n / 100;
        n = n % 100;
        buf = Utoa_2Digits(buf, q);
L_2_digits:
        buf = Utoa_2Digits(buf, n);
        return buf;
    }

    if (n >=  1000000) goto L_7_digits;
    if (n >=   100000) goto L_6_digits;
    if (n >=    10000) goto L_5_digits;
    if (n >=     1000) goto L_4_digits;
    if (n >=      100) goto L_3_digits;
    if (n >=       10) goto L_2_digits;
    goto L_1_digit;
}

// Modifies the generated digits in the buffer to approach (round towards) w.
//
// Input:
//  * digits of H/10^kappa in [digits, digits + num_digits)
//  * distance    = (H - w) * unit
//  * delta       = (H - L) * unit
//  * rest        = (H - buffer * 10^kappa) * unit
//  * ten_kappa   = 10^kappa * unit
inline void Grisu2Round(char* digits, int num_digits, uint64_t distance, uint64_t delta, uint64_t rest, uint64_t ten_kappa)
{
    CC_ASSERT(num_digits >= 1);
    CC_ASSERT(distance <= delta);
    CC_ASSERT(rest <= delta);
    CC_ASSERT(ten_kappa > 0);

    // By generating the digits of H we got the largest (closest to H) buffer
    // that is still in the interval [L, H]. In the case where w < B <= H we
    // try to decrement the buffer.
    //
    //                                  <---- distance ----->
    //               <--------------------------- delta ---->
    //                                       <---- rest ---->
    //                       <-- ten_kappa -->
    // --------------[------------------+----+--------------]--------------
    //               L                  w    B              H
    //                                       = digits * 10^kappa
    //
    // ten_kappa represents a unit-in-the-last-place in the decimal
    // representation stored in the buffer.
    //
    // There are three stopping conditions:
    // (The position of the numbers is measured relative to H.)
    //
    //  1)  B is already <= w
    //          rest >= distance
    //
    //  2)  Decrementing B would yield a number B' < L
    //          rest + ten_kappa > delta
    //
    //  3)  Decrementing B would yield a number B' < w and farther away from
    //      w than the current number B: w - B' > B - w
    //          rest + ten_kappa > distance &&
    //          rest + ten_kappa - distance >= distance - rest

    // The tests are written in this order to avoid overflow in unsigned
    // integer arithmetic.

    int digit = digits[num_digits - 1] - '0';

    while (rest < distance
        && delta - rest >= ten_kappa
        && (rest + ten_kappa <= distance || rest + ten_kappa - distance < distance - rest))
    {
        CC_ASSERT(digit != 0);
        digit--;
        rest += ten_kappa;
    }

    digits[num_digits - 1] = static_cast<char>('0' + digit);
}

// Generates V = digits * 10^exponent, such that L <= V <= H.
// L and H must be normalized and share the same exponent -60 <= e <= -32.
inline void Grisu2DigitGen(char* digits, int& num_digits, int& exponent, DiyFp L, DiyFp w, DiyFp H)
{
    static_assert(DiyFp::SignificandSize == 64, "internal error");
    static_assert(kAlpha >= -60, "internal error");
    static_assert(kGamma <= -32, "internal error");

    // Generates the digits (and the exponent) of a decimal floating-point
    // number V = digits * 10^exponent in the range [L, H].
    // The DiyFp's w, L and H share the same exponent e, which satisfies
    // alpha <= e <= gamma.
    //
    //                                  <---- distance ----->
    //               <--------------------------- delta ---->
    // --------------[------------------+-------------------]--------------
    //               L                  w                   H
    //
    // This routine generates the digits of H from left to right and stops as
    // soon as V is in [L, H].

    CC_ASSERT(w.e >= kAlpha);
    CC_ASSERT(w.e <= kGamma);
    CC_ASSERT(w.e == L.e);
    CC_ASSERT(w.e == H.e);

    uint64_t distance = Subtract(H, w).f; // (significand of (H - w), implicit exponent is e)
    uint64_t delta    = Subtract(H, L).f; // (significand of (H - L), implicit exponent is e)
    uint64_t rest;
    uint64_t ten_kappa;

    // Split H = f * 2^e into two parts p1 and p2 (note: e < 0):
    //
    //      H = f * 2^e
    //           = ((f div 2^-e) * 2^-e + (f mod 2^-e)) * 2^e
    //           = ((p1        ) * 2^-e + (p2        )) * 2^e
    //           = p1 + p2 * 2^e

    DiyFp const one(uint64_t{1} << -H.e, H.e); // one = 2^-e * 2^e

    uint32_t p1 = static_cast<uint32_t>(H.f >> -one.e); // p1 = f div 2^-e (Since -e >= 32, p1 fits into a 32-bit int.)
    uint64_t p2 = H.f & (one.f - 1);                    // p2 = f mod 2^-e

    CC_ASSERT(p1 >= 4);            // (2^(64-2) - 1) >> 60
    CC_ASSERT(p1 <= 798336123);    // test.cc: FindMaxP1 (depends on index computation in GetCachedPowerForBinaryExponent!)

    // Generate the digits of the integral part p1 = d[n-1]...d[1]d[0]
    //
    //      10^(k-1) <= p1 < 10^k
    //
    //      p1 = (p1 div 10^(k-1)) * 10^(k-1) + (p1 mod 10^(k-1))
    //         = (d[k-1]         ) * 10^(k-1) + (p1 mod 10^(k-1))
    //
    //      H = p1                                             + p2 * 2^e
    //        = d[k-1] * 10^(k-1) + (p1 mod 10^(k-1))          + p2 * 2^e
    //        = d[k-1] * 10^(k-1) + ((p1 mod 10^(k-1)) * 2^-e + p2) * 2^e
    //        = d[k-1] * 10^(k-1) + (                         rest) * 2^e
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
    num_digits = static_cast<int>(GenerateIntegralDigits(digits, p1) - digits);

    if (p2 > delta)
    {
        // The digits of the integral part have been generated (and all of them
        // are significand):
        //
        //      H = d[k-1]...d[1]d[0] + p2 * 2^e
        //        = digits            + p2 * 2^e
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
        //      H = digits + p2 * 2^e
        //        = digits + 10^-m * (d + r * 2^e)
        //        = (digits * 10^m + d) * 10^-m + 10^-m * r * 2^e
        //
        // and stop as soon as 10^-m * r * 2^e <= delta * 2^e

        // unit = 1
        int m = 0;
        for (;;)
        {
            // !!! CC_ASSERT(num_digits < max_digits10) !!!
            CC_ASSERT(num_digits < 17);

            //
            //      H = digits * 10^-m + 10^-m * (d[-m-1] / 10 + d[-m-2] / 10^2 + ...) * 2^e
            //        = digits * 10^-m + 10^-m * (p2                                 ) * 2^e
            //        = digits * 10^-m + 10^-m * (1/10 * (10 * p2)                   ) * 2^e
            //        = digits * 10^-m + 10^-m * (1/10 * ((10*p2 div 2^-e) * 2^-e + (10*p2 mod 2^-e)) * 2^e
            //
            CC_ASSERT(p2 <= UINT64_MAX / 10);
            p2 *= 10;
            uint64_t const d = p2 >> -one.e;     // d = (10 * p2) div 2^-e
            uint64_t const r = p2 & (one.f - 1); // r = (10 * p2) mod 2^-e
            CC_ASSERT(d <= 9);
            //
            //      H = digits * 10^-m + 10^-m * (1/10 * (d * 2^-e + r) * 2^e
            //        = digits * 10^-m + 10^-m * (1/10 * (d + r * 2^e))
            //        = (digits * 10 + d) * 10^(-m-1) + 10^(-m-1) * r * 2^e
            //
            digits[num_digits++] = static_cast<char>('0' + d); // digits := digits * 10 + d
            //
            //      H = buffer * 10^(-m-1) + 10^(-m-1) * r * 2^e
            //
            p2 = r;
            m++;
            //
            //      H = digits * 10^-m + 10^-m * p2 * 2^e
            //

            // Keep the units in sync. (unit *= 10)
            delta    *= 10;
            distance *= 10;

            // Check if enough digits have been generated.
            //
            //      10^-m * p2 * 2^e <= delta * 2^e
            //              p2 * 2^e <= 10^m * delta * 2^e
            //                    p2 <= 10^m * delta
            if (p2 <= delta)
            {
                // V = digits * 10^-m, with L <= V <= H.
                exponent = -m;

                rest = p2;

                // 1 ulp in the decimal representation is now 10^-m.
                // Since delta and distance are now scaled by 10^m, we need to do
                // the same with ulp in order to keep the units in sync.
                //
                //      10^m * 10^-m = 1 = 2^-e * 2^e = ten_m * 2^e
                //
                ten_kappa = one.f; // one.f == 2^-e

                break;
            }
        }
    }
    else // p2 <= delta
    {
        CC_ASSERT((uint64_t{p1} << -one.e) + p2 > delta); // Loop terminates.

        // In this case: Too many digits of p1 might have been generated.
        //
        // Find the largest 0 <= n < k = length, such that
        //
        //      H = (p1 div 10^n) * 10^n + ((p1 mod 10^n) * 2^-e + p2) * 2^e
        //        = (p1 div 10^n) * 10^n + (                     rest) * 2^e
        //
        // and rest <= delta.
        //
        // Compute rest * 2^e = H mod 10^n = p1 + p2 * 2^e = (p1 * 2^-e + p2) * 2^e
        // and check if enough digits have been generated:
        //
        //      rest * 2^e <= delta * 2^e
        //

        int const k = num_digits;
        CC_ASSERT(k >= 0);
        CC_ASSERT(k <= 9);

        rest = p2;

        // 10^n is now 1 ulp in the decimal representation V. The rounding
        // procedure works with DiyFp's with an implicit exponent of e.
        //
        //      10^n = (10^n * 2^-e) * 2^e = ten_kappa * 2^e
        //
        ten_kappa = one.f; // Start with 2^-e

        for (int n = 0; /**/; ++n)
        {
            CC_ASSERT(n <= k - 1);
            CC_ASSERT(rest <= delta);

            // rn = d[n]...d[0] * 2^-e + p2
            uint32_t const dn = static_cast<uint32_t>(digits[k - 1 - n] - '0');
            uint64_t const rn = dn * ten_kappa + rest;

            if (rn > delta)
            {
                num_digits = k - n;
                exponent = n;
                break;
            }

            rest = rn;
            ten_kappa *= 10;
        }
    }

    // The buffer now contains a correct decimal representation of the input
    // number w = buffer * 10^exponent.

    Grisu2Round(digits, num_digits, distance, delta, rest, ten_kappa);
}

// v = buffer * 10^exponent
// length is the length of the buffer (number of decimal digits)
// The buffer must be large enough, i.e. >= max_digits10.
inline void Grisu2(char* digits, int& num_digits, int& exponent, DiyFp m_minus, DiyFp v, DiyFp m_plus)
{
    CC_ASSERT(v.e == m_minus.e);
    CC_ASSERT(v.e == m_plus.e);

    //  --------+-----------------------+-----------------------+--------    (A)
    //          m-                      v                       m+
    //
    //  --------------------+-----------+-----------------------+--------    (B)
    //                      m-          v                       m+
    //
    // First scale v (and m- and m+) such that the exponent is in the range
    // [alpha, gamma].

    auto const cached = GetCachedPowerForBinaryExponent(v.e);

    DiyFp const c_minus_k(cached.f, cached.e); // = c ~= 10^-k

    DiyFp const w       = Multiply(v,       c_minus_k);
    DiyFp const w_minus = Multiply(m_minus, c_minus_k);
    DiyFp const w_plus  = Multiply(m_plus,  c_minus_k);

    // The exponent of the products is = v.e + c_minus_k.e + q and is in the
    // range [alpha, gamma].
    CC_ASSERT(w.e >= kAlpha);
    CC_ASSERT(w.e <= kGamma);

    // Note:
    // The result of Multiply() is **NOT** neccessarily normalized.
    // But since m+ and c are normalized, w_plus.f >= 2^(q - 2).
    CC_ASSERT(w_plus.f >= (uint64_t{1} << (64 - 2)));

    //  ----(---+---)---------------(---+---)---------------(---+---)----
    //          w-                      w                       w+
    //          = c*m-                  = c*v                   = c*m+
    //
    // Multiply rounds its result and c_minus_k is approximated too. w, w- and
    // w+ are now off by a small amount.
    // In fact:
    //
    //      w - v * 10^-k < 1 ulp
    //
    // To account for this inaccuracy, add resp. subtract 1 ulp.
    // Note: ulp(w-) = ulp(w) = ulp(w+).
    //
    //  ----(---+---[---------------(---+---)---------------]---+---)----
    //          w-  L                   w                   H   w+
    //
    // Now any number in [L, H] (bounds included) will round to w when input,
    // regardless of how the input rounding algorithm breaks ties.
    //
    // And DigitGen generates the shortest possible such number in [L, H].
    // Note that this does not mean that Grisu2 always generates the shortest
    // possible number in the interval (m-, m+).
    DiyFp const L(w_minus.f + 1, w_minus.e);
    DiyFp const H(w_plus.f  - 1, w_plus.e );

    Grisu2DigitGen(digits, num_digits, exponent, L, w, H);
    // w = buffer * 10^exponent

    // v = w * 10^k
    exponent += -cached.k; // cached.k = -k
    // v = buffer * 10^exponent
}

//==================================================================================================
// Dtoa
//==================================================================================================

constexpr int kDoubleToDecimalMaxLength = 17;

// v = digits * 10^exponent
// num_digits is the length of the buffer (number of decimal digits)
// PRE: The buffer must be large enough, i.e. >= max_digits10.
// PRE: value must be finite and strictly positive.
inline void DoubleToDigits(char* buffer, int& num_digits, int& exponent, double value)
{
    static_assert(DiyFp::SignificandSize >= std::numeric_limits<double>::digits + 3,
        "Grisu2 requires at least three extra bits of precision");

    CC_ASSERT(Double(value).IsFinite());
    CC_ASSERT(value > 0);

#if 0
    // If the neighbors (and boundaries) of 'value' are always computed for
    // double-precision numbers, all float's can be recovered using strtod
    // (and strtof). However, the resulting decimal representations are not
    // exactly "short".
    //
    // If the neighbors are computed for single-precision numbers, there is a
    // single float (7.0385307e-26f) which can't be recovered using strtod.
    // (The resulting double precision is off by 1 ulp.)
    auto const boundaries = ComputeBoundaries(static_cast<double>(value));
#else
    auto const boundaries = ComputeBoundaries(value);
#endif

    Grisu2(buffer, num_digits, exponent, boundaries.m_minus, boundaries.v, boundaries.m_plus);

    CC_ASSERT(num_digits > 0);
    CC_ASSERT(num_digits <= kDoubleToDecimalMaxLength);
}

inline char* ExponentToString(char* buffer, int value)
{
    CC_ASSERT(value > -1000);
    CC_ASSERT(value <  1000);

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

inline char* FormatFixed(char* buffer, int length, int decimal_point, bool force_trailing_dot_zero)
{
    CC_ASSERT(buffer != nullptr);
    CC_ASSERT(length >= 1);

    if (length <= decimal_point)
    {
        // digits[000]
        // CC_ASSERT(buffer_capacity >= decimal_point + (force_trailing_dot_zero ? 2 : 0));

        std::memset(buffer + length, '0', static_cast<size_t>(decimal_point - length));
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
        // CC_ASSERT(buffer_capacity >= length + 1);

        std::memmove(buffer + (decimal_point + 1), buffer + decimal_point, static_cast<size_t>(length - decimal_point));
        buffer[decimal_point] = '.';
        return buffer + (length + 1);
    }
    else // decimal_point <= 0
    {
        // 0.[000]digits
        // CC_ASSERT(buffer_capacity >= 2 + (-decimal_point) + length);

        std::memmove(buffer + (2 + -decimal_point), buffer, static_cast<size_t>(length));
        buffer[0] = '0';
        buffer[1] = '.';
        std::memset(buffer + 2, '0', static_cast<size_t>(-decimal_point));
        return buffer + (2 + (-decimal_point) + length);
    }
}

inline char* FormatExponential(char* buffer, int length, int exponent)
{
    CC_ASSERT(buffer != nullptr);
    CC_ASSERT(length >= 1);

    if (length == 1)
    {
        // dE+123
        // CC_ASSERT(buffer_capacity >= length + 5);

        //
        // XXX:
        // Should force_trailing_dot_zero apply here?!?!
        //

        buffer += 1;
    }
    else
    {
        // d.igitsE+123
        // CC_ASSERT(buffer_capacity >= length + 1 + 5);

        std::memmove(buffer + 2, buffer + 1, static_cast<size_t>(length - 1));
        buffer[1] = '.';
        buffer += 1 + length;
    }

//  if (exponent != 0)
    {
        buffer[0] = 'e';
        buffer = ExponentToString(buffer + 1, exponent);
    }

    return buffer;
}

inline char* FormatGeneral(char* buffer, int num_digits, int exponent, bool force_trailing_dot_zero)
{
    int const decimal_point = num_digits + exponent;

#if 0

#if 1
    buffer[num_digits] = 'e';
    return ExponentToString(buffer + (num_digits + 1), decimal_point - 1);
#else
    return FormatExponential(buffer, num_digits, decimal_point - 1);
#endif

#else

    // Changing these constants requires changing kDtoaMaxLength (see below) too.
    // XXX:
    // Compute kDtoaMaxLength using these constants...?!
#if 0
    constexpr int const kMinExp = -4;
    constexpr int const kMaxExp = 17; // std::numeric_limits<Fp>::max_digits10;
#else
    constexpr int const kMinExp = -6;
    constexpr int const kMaxExp = 21;
#endif

    bool const use_fixed = kMinExp < decimal_point && decimal_point <= kMaxExp;

    return use_fixed
        ? FormatFixed(buffer, num_digits, decimal_point, force_trailing_dot_zero)
        : FormatExponential(buffer, num_digits, decimal_point - 1);

#endif
}

inline char* PositiveDoubleToString(char* buffer, double value, bool force_trailing_dot_zero = false)
{
    CC_ASSERT(Double(value).IsFinite());
    CC_ASSERT(value > 0);

    int num_digits = 0;
    int exponent = 0;
    DoubleToDigits(buffer, num_digits, exponent, value);

    CC_ASSERT(num_digits <= std::numeric_limits<double>::max_digits10);

    return FormatGeneral(buffer, num_digits, exponent, force_trailing_dot_zero);
}

//==================================================================================================
// DigitsToIEEE
//
// Derived from the double-conversion library:
// https://github.com/google/double-conversion
//
// The original license can be found below.
//
// [1] Clinger, "How to read floating point numbers accurately",
//     PLDI '90 Proceedings of the ACM SIGPLAN 1990 conference on Programming
//     language design and implementation, Pages 92-101
//==================================================================================================

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

// Maximum number of significant digits in decimal representation.
//
// The longest possible double in decimal representation is (2^53 - 1) * 5^1074 / 10^1074,
// which has 767 digits.
// If we parse a number whose first digits are equal to a mean of 2 adjacent
// doubles (that could have up to 768 digits) the result must be rounded to the
// bigger one unless the tail consists of zeros, so we don't need to preserve
// all the digits.
constexpr int kMaxSignificantDigits = 767 + 1;

inline constexpr int Min(int x, int y) { return y < x ? y : x; }
inline constexpr int Max(int x, int y) { return y < x ? x : y; }

inline bool IsDigit(char ch)
{
#if 0
    return static_cast<unsigned>(ch - '0') < 10;
#else
    return '0' <= ch && ch <= '9';
#endif
}

inline int DigitValue(char ch)
{
    CC_ASSERT(IsDigit(ch));
    return ch - '0';
}

//--------------------------------------------------------------------------------------------------
// StrtodFast
//--------------------------------------------------------------------------------------------------

// Double operations detection based on target architecture.
// Linux uses a 80bit wide floating point stack on x86. This induces double
// rounding, which in turn leads to wrong results.
// An easy way to test if the floating-point operations are correct is to
// evaluate: 89255.0/1e22.
// If the floating-point stack is 64 bits wide then the result is equal to
// 89255e-22. The best way to test this, is to create a division-function and to
// compare the output of the division with the expected result. (Inlining must
// be disabled.)
#if !defined(STRTOD_CORRECT_DOUBLE_OPERATIONS)
#if defined(_M_X64)              || \
    defined(_M_ARM)				 || \
    defined(_M_ARM64)			 || \
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
#define STRTOD_CORRECT_DOUBLE_OPERATIONS 1
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
#ifdef _WIN32
// Windows uses a 64bit wide floating point stack.
#define STRTOD_CORRECT_DOUBLE_OPERATIONS 1
#endif
#endif
#endif // !defined(STRTOD_CORRECT_DOUBLE_OPERATIONS)

#if STRTOD_CORRECT_DOUBLE_OPERATIONS

// 2^53 = 9007199254740992.
// Any integer with at most 15 decimal digits will hence fit into a double
// (which has a 53-bit significand) without loss of precision.
constexpr int kMaxExactDoubleIntegerDecimalDigits = 15;

inline bool FastPath(double& result, uint64_t digits, int num_digits, int exponent)
{
    static constexpr int kMaxExactPowerOfTen = 22;
    static constexpr double kExactPowersOfTen[] = {
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
        1.0e+15, // 10^15 < 9007199254740992 = 2^53
        1.0e+16, // 10^16 = 5000000000000000 * 2^1  = (10^15 * 5^1 ) * 2^1
        1.0e+17, // 10^17 = 6250000000000000 * 2^4  = (10^13 * 5^4 ) * 2^4
        1.0e+18, // 10^18 = 7812500000000000 * 2^7  = (10^11 * 5^7 ) * 2^7
        1.0e+19, // 10^19 = 4882812500000000 * 2^11 = (10^8  * 5^11) * 2^11
        1.0e+20, // 10^20 = 6103515625000000 * 2^14 = (10^6  * 5^14) * 2^14
        1.0e+21, // 10^21 = 7629394531250000 * 2^17 = (10^4  * 5^17) * 2^17
        1.0e+22, // 10^22 = 4768371582031250 * 2^21 = (10^1  * 5^21) * 2^21
//      1.0e+23,
    };

    if (num_digits > kMaxExactDoubleIntegerDecimalDigits)
        return false;

    // The significand fits into a double.
    // If 10^exponent (resp. 10^-exponent) fits into a double too then we can
    // compute the result simply by multiplying (resp. dividing) the two
    // numbers.
    // This is possible because IEEE guarantees that floating-point operations
    // return the best possible approximation.

    int const remaining_digits = kMaxExactDoubleIntegerDecimalDigits - num_digits; // 0 <= rd <= 15
    if (-kMaxExactPowerOfTen <= exponent && exponent <= remaining_digits + kMaxExactPowerOfTen)
    {
        CC_ASSERT(digits <= INT64_MAX);
        double d = static_cast<double>(static_cast<int64_t>(digits));
        if (exponent < 0)
        {
            d /= kExactPowersOfTen[-exponent];
        }
        else if (exponent <= kMaxExactPowerOfTen)
        {
            d *= kExactPowersOfTen[exponent];
        }
        else
        {
            // The buffer is short and we can multiply it with 10^remaining_digits
            // and the remaining exponent fits into a double.
            //
            // Eg. 123 * 10^25 = (123*1000) * 10^22

            d *= kExactPowersOfTen[remaining_digits]; // exact
            d *= kExactPowersOfTen[exponent - remaining_digits];
        }
        result = d;
        return true;
    }

    return false;
}

#else // ^^^ STRTOD_CORRECT_DOUBLE_OPERATIONS

inline bool FastPath(double& /*result*/, uint64_t /*digits*/, int /*num_digits*/, int /*exponent*/)
{
    return false;
}

#endif // ^^^ !STRTOD_CORRECT_DOUBLE_OPERATIONS

//--------------------------------------------------------------------------------------------------
// StrtodApprox
//--------------------------------------------------------------------------------------------------

struct DiyFpWithError // value = (x.f + delta) * 2^x.e, where |delta| <= error
{
    // We don't want to deal with fractions and therefore work with a common
    // denominator.
    static constexpr int DenominatorLog = 1;
    static constexpr int Denominator = 1 << DenominatorLog;

    DiyFp x;
    uint32_t error = 0;

    constexpr DiyFpWithError() = default;
    constexpr DiyFpWithError(DiyFp x_, uint32_t error_) : x(x_), error(error_) {}
};

// Normalize x
// and scale the error, so that the error is in ULP(x)
inline void Normalize(DiyFpWithError& num)
{
    int const s = CountLeadingZeros64(num.x.f);

    CC_ASSERT(((num.error << s) >> s) == num.error);

    num.x.f   <<= s;
    num.x.e    -= s;
    num.error <<= s;
}

template <typename T>
inline T ReadInt(char const* str, int len)
{
    CC_ASSERT(len <= std::numeric_limits<T>::digits10);

    T value = 0;

    for (int i = 0; i < len; ++i)
    {
        value = static_cast<unsigned char>(str[i] - '0') + 10 * value;
    }

    return value;
}

// Returns a cached power of ten x ~= 10^k such that
//  k <= e < k + kCachedPowersDecExpStep.
//
// PRE: e >= kCachedPowersMinDecExp
// PRE: e <  kCachedPowersMaxDecExp + kCachedPowersDecExpStep
inline CachedPower GetCachedPowerForDecimalExponent(int e)
{
    CC_ASSERT(e >= kCachedPowersMinDecExp);
    CC_ASSERT(e <  kCachedPowersMaxDecExp + kCachedPowersDecExpStep);

    int const index = static_cast<int>( static_cast<unsigned>(-kCachedPowersMinDecExp + e) / kCachedPowersDecExpStep );
    CC_ASSERT(index >= 0);
    CC_ASSERT(index < kCachedPowersSize);

    auto const cached = GetCachedPower(index);
    CC_ASSERT(e >= cached.k);
    CC_ASSERT(e <  cached.k + kCachedPowersDecExpStep);

    return cached;
}

// Returns 10^k as an exact DiyFp.
// PRE: 1 <= k < kCachedPowersDecExpStep
inline DiyFp GetAdjustmentPowerOfTen(int k)
{
    static_assert(kCachedPowersDecExpStep <= 8, "internal error");

    static constexpr uint64_t kSignificands[] = {
        0x8000000000000000, // * 2^-63   == 10^0 (unused)
        0xA000000000000000, // * 2^-60   == 10^1
        0xC800000000000000, // * 2^-57   == 10^2
        0xFA00000000000000, // * 2^-54   == 10^3
        0x9C40000000000000, // * 2^-50   == 10^4
        0xC350000000000000, // * 2^-47   == 10^5
        0xF424000000000000, // * 2^-44   == 10^6
        0x9896800000000000, // * 2^-40   == 10^7
#if 0
        0xBEBC200000000000, // * 2^-37   == 10^8
        0xEE6B280000000000, // * 2^-34   == 10^9
        0x9502F90000000000, // * 2^-30   == 10^10
        0xBA43B74000000000, // * 2^-27   == 10^11
        0xE8D4A51000000000, // * 2^-24   == 10^12
        0x9184E72A00000000, // * 2^-20   == 10^13
        0xB5E620F480000000, // * 2^-17   == 10^14
        0xE35FA931A0000000, // * 2^-14   == 10^15
#endif
    };

    CC_ASSERT(k > 0);
    CC_ASSERT(k < kCachedPowersDecExpStep);

    int const e = BinaryExponentFromDecimalExponent(k);
    return {kSignificands[k], e};
}

// Max double: 1.7976931348623157 * 10^308, which has 309 digits.
// Any x >= 10^309 is interpreted as +infinity.
constexpr int kMaxDecimalPower = 309;

// Min non-zero double: 4.9406564584124654 * 10^-324
// Any x <= 10^-324 is interpreted as 0.
// Note that 2.5e-324 (despite being smaller than the min double) will be read
// as non-zero (equal to the min non-zero double).
constexpr int kMinDecimalPower = -324;

// Returns the significand size for a given order of magnitude.
//
// If v = f * 2^e with 2^(q-1) <= f < 2^q then (q+e) is v's order of magnitude.
// If v = s * 2^e with 1/2 <= s < 1 then e is v's order of magnitude.
//
// This function returns the number of significant binary digits v will have
// once it's encoded into a 'double'. In almost all cases this is equal to
// Double::SignificandSize. The only exceptions are subnormals. They start with
// leading zeroes and their effective significand-size is hence smaller.
inline int EffectiveSignificandSize(int order)
{
    int const s = order - Double::MinExponent;

    if (s > Double::SignificandSize)
        return Double::SignificandSize;
    if (s < 0)
        return 0;

    return s;
}

// Returns `f * 2^e`.
inline double LoadFloat(uint64_t f, int e)
{
    CC_ASSERT(f <= Double::HiddenBit + Double::SignificandMask);
    CC_ASSERT(e <= Double::MinExponent || (f & Double::HiddenBit) != 0);

    if (e < Double::MinExponent)
        return 0;
    if (e > Double::MaxExponent)
        return std::numeric_limits<double>::infinity();

    bool const is_subnormal = (e == Double::MinExponent && (f & Double::HiddenBit) == 0);

    uint64_t const exponent = is_subnormal
        ? 0
        : static_cast<uint64_t>(e + Double::ExponentBias);

    return Double((exponent << Double::PhysicalSignificandSize) | (f & Double::SignificandMask)).Value();
}

// Use DiyFp's to approximate digits * 10^exponent.
//
// If the function returns true then 'result' is the correct double.
// Otherwise 'result' is either the correct double or the double that is just
// below the correct double.
//
// PRE: num_digits + exponent <= kMaxDecimalPower
// PRE: num_digits + exponent >  kMinDecimalPower
inline bool StrtodApprox(double& result, char const* digits, int num_digits, int exponent)
{
    static_assert(DiyFp::SignificandSize == 64,
        "We use uint64's. This only works if the DiyFp uses uint64's too.");

    CC_ASSERT(num_digits > 0);
    CC_ASSERT(DigitValue(digits[0]) > 0);
//  CC_ASSERT(DigitValue(digits[num_digits - 1]) > 0);
    CC_ASSERT(num_digits + exponent <= kMaxDecimalPower);
    CC_ASSERT(num_digits + exponent >  kMinDecimalPower);

    // Compute an approximation 'input' for B = digits * 10^exponent using DiyFp's.
    // And keep track of the error.
    //
    //                       <-- error -->
    //                               B = digits * 10^exponent
    //  ---------(-----------|-------+---)------------------------------------
    //                       x
    //                       ~= (f * 2^e) * 10^exponent

    int const read_digits = Min(num_digits, std::numeric_limits<uint64_t>::digits10);

    DiyFpWithError input;

    input.x.f = ReadInt<uint64_t>(digits, read_digits);
    input.x.e = 0;
    input.error = 0;

    if (FastPath(result, input.x.f, num_digits, exponent))
        return true;

    constexpr uint32_t ULP = DiyFpWithError::Denominator;
    constexpr uint32_t InitialError = ULP / 2;

    if (read_digits < num_digits)
    {
        // Round.
        input.x.f += (DigitValue(digits[read_digits]) >= 5);
        // The error is <= 1/2 ULP.
        input.error = InitialError;
    }

    // x = f * 2^0

    // Normalize x and scale the error, such that 'error' is in ULP(x).
    Normalize(input);

    // If the input is exact, error == 0.
    // If the input is inexact, we have read 19 digits, i.e., f >= 10^(19-1) > 2^59.
    // The scaling factor in the normalization step above therefore is <= 2^(63-59) = 2^4.
    CC_ASSERT(input.error <= 16 * InitialError);

    // Move the remaining decimals into the (decimal) exponent.
    exponent += num_digits - read_digits;

    // Let x and y be normalized floating-point numbers
    //
    //      x = f_x * 2^e_x,    2^(q-1) <= f_x < 2^q
    //      y = f_y * 2^e_y,    2^(q-1) <= f_y < 2^q
    //
    // Then
    //
    //      z = Multiply(x,y) = f_z * 2^e_z
    //
    // returns the floating-point number closest to the product x*y. The result
    // z is not neccessarily normalized, but the error is bounded by 1/2 ulp,
    // i.e.,
    //
    //      |x*y - z| <= 1/2 ulp
    //
    // or
    //
    //      x*y = (f_z + eps_z) * 2^e_z,    |eps_z| <= 1/2, e_z = e_x + e_y + q.
    //
    // If x and y are approximations to real numbers X and Y, i.e.,
    //
    //      X = (f_x + eps_x) * 2^e_x,      |eps_x| <= err_x,
    //      Y = (f_y + eps_y) * 2^e_y,      |eps_y| <= err_y,
    //
    // then the error introduced by a multiplication Multiply(x,y) is (see [1])
    //
    //      |X*Y - z| <= 1/2 + err_x + err_y + (err_x * err_y - err_x - err_y) / 2^q
    //
    // And if err_x < 1 (or err_y < 1), then
    //
    //      |X*Y - z| <= 1/2 + (err_x + err_y)

    auto const cached = GetCachedPowerForDecimalExponent(exponent);
    auto const cached_power = DiyFp(cached.f, cached.e);

    // Not all powers-of-ten are cached.
    // If cached.k != exponent we need to multiply 'x' by the difference first.
    // This may introduce an additional error.
    if (cached.k != exponent)
    {
        auto const adjustment_exponent = exponent - cached.k;
        auto const adjustment_power = GetAdjustmentPowerOfTen(adjustment_exponent);

        CC_ASSERT(IsNormalized(input.x));
        CC_ASSERT(IsNormalized(adjustment_power));

        input.x = Multiply(input.x, adjustment_power);
        // x ~= digits * 10^adjustment_exponent

        // Adjust error.
        // The adjustment_power is exact (err_y = 0).
        // There is hence only an additional error of (at most) 1/2.

        if (num_digits + adjustment_exponent <= std::numeric_limits<uint64_t>::digits10)
        {
            // x and adjustment_power are exact.
            // The product (digits * 10^adjustment_exponent) fits into an uint64_t.
            // x * adjustment_power is therefore exact, too, and there is no additional error.
        }
        else
        {
            input.error += ULP / 2;

            CC_ASSERT(input.error <= 17 * (ULP / 2));
        }

        // The result of the multiplication might not be normalized.
        // Normalize 'x' again and scale the error.
        Normalize(input);

        // Since both factors are normalized, input.f >= 2^(q-2), and the scaling
        // factor in the normalization step above is bounded by 2^1.
        CC_ASSERT(input.error <= 34 * (ULP / 2));
    }

    CC_ASSERT(IsNormalized(input.x));
    CC_ASSERT(IsNormalized(cached_power));

    input.x = Multiply(input.x, cached_power);
    // x ~= digits * 10^exponent

    // Adjust the error.
    // Since all cached powers have an error of less than 1/2 ulp, err_y = 1/2,
    // and the error is therefore less than 1/2 + (err_x + err_y).

#if 1
    input.error += ULP / 2 + (0 <= exponent && exponent <= 27 ? 0 : ULP / 2);
#else
    input.error += ULP / 2 + ULP / 2;
#endif
    CC_ASSERT(input.error <= 34 * InitialError + 2 * (ULP / 2));

    // The result of the multiplication might not be normalized.
    // Normalize 'x' again and scale the error.
    Normalize(input);

    // Since both factors were normalized, the scaling factor in the
    // normalization step above is bounded by 2^1.
    CC_ASSERT(input.error <= 2 * (34 * InitialError + 2 * (ULP / 2)));

    // We now have an approximation x = f * 2^e ~= digits * 10^exponent.
    //
    //                       <-- error -->
    //                               B = digits * 10^exponent
    //  ---------(-----------|-------+---)------------------------------------
    //                       x
    //                       ~= digits * 10^exponent
    //
    // B = (x.f + delta) * 2^x.e, where |delta| <= error / kULP
    //
    // When converting f * 2^e, which has a q-bit significand, into an IEEE
    // double-precision number, we need to drop some 'excess_bits' bits of
    // precision.

    int const prec = EffectiveSignificandSize(DiyFp::SignificandSize + input.x.e);
    CC_ASSERT(prec >= 0);
    CC_ASSERT(prec <= 53);

    int const excess_bits = DiyFp::SignificandSize - prec;

    // n = excess_bits
    //
    // f = (f div 2^n) * 2^n + (f mod 2^n)
    //   = (p1       ) * 2^n + (p2       )
    //
    //                             f = p1 * 2^n + p2
    //   <--- p2 ------------------>
    //                 <-- error --+-- error -->
    // --|-------------(-----------+------|----)---------------------------|--
    //   p1 * 2^n                                                 (p1 + 1) * 2^n
    //   <------------- half ------------->
    //                  = 2^n / 2
    //
    // The correct double now is either p1 * 2^(e + n) or (p1 + 1) * 2^(e + n).
    // See [1], Theorem 11.
    //
    // In case p2 + error < half, we can safely round down. If p2 - error > half
    // we can safely round up. Otherwise, we are too inaccurate. In this case
    // we round down, so the returned double is either the correct double or the
    // double just below the correct double. In this case we return false, so
    // that the we can fall back to a more precise algorithm.

    CC_ASSERT(excess_bits >= 11);
    CC_ASSERT(excess_bits <= 64);

    uint64_t const p2 = (excess_bits < 64) ? (input.x.f & ((uint64_t{1} << excess_bits) - 1)) : input.x.f;
    uint64_t const half = uint64_t{1} << (excess_bits - 1);

    // Truncate the significand to p = q - n bits and move the discarded bits
    // into the (binary) exponent.
    // (Right shift of >= bit-width is undefined.)
    input.x.f = (excess_bits < 64) ? (input.x.f >> excess_bits) : 0;
    input.x.e += excess_bits;

    // Split up error into high (integral) and low (fractional) parts,
    // since half * kULP might overflow.
    uint32_t const error_hi = input.error / ULP;
    uint32_t const error_lo = input.error % ULP;

    CC_ASSERT(input.error > 0);
    CC_ASSERT(half >= error_hi && half - error_hi <= UINT64_MAX / ULP && (half - error_hi) * ULP >= error_lo);
    CC_ASSERT(half <= UINT64_MAX - error_hi);
    static_cast<void>(error_lo);

    // Note:
    // Since error is non-zero, we can safely use '<=' and '>=' in the
    // comparisons below.

    bool success;
#if 1
    // p2 * U >= half * U + error
    // <=> p2 * U >= half * U + (error_hi * U + error_lo)
    // <=> p2 * U >= (half + error_hi) * U + error_lo
    // <=> p2 >= (half + error_hi) + error_lo / U
    if (p2 > half + error_hi)
#else
    if (p2 * ULP >= half * ULP + input.error)
#endif
    {
        // Round up.
        success = true;

        ++input.x.f;

        // Rounding up may overflow the p-bit significand.
        // But in this case the significand is 2^53 and we don't loose any
        // bits by normalizing 'input' (we just move a factor of 2 into the
        // binary exponent).
        if (input.x.f > Double::HiddenBit + Double::SignificandMask)
        {
            CC_ASSERT(input.x.f == (Double::HiddenBit << 1));

            input.x.f >>= 1;
            input.x.e  += 1;
        }
    }
#if 1
    // p2 * U <= half * U - error
    // <=> half * U >= p2 * U + error
    // <=> half * U >= p2 * U + (error_hi * U + error_lo)
    // <=> half * U >= (p2 + error_hi) * U + error_lo
    // <=> half >= (p2 + error_hi) + error_lo / U
    else if (half > p2 + error_hi)
#else
    else if (p2 * ULP <= half * ULP - input.error)
#endif
    {
        // Round down.
        success = true;
    }
    else
    {
        // Too imprecise.
        // Round down and return false, so that we can fall back to a more
        // precise algorithm.
        success = false;
    }

    result = LoadFloat(input.x.f, input.x.e);
    return success;
}

inline bool ComputeGuess(double& result, char const* digits, int num_digits, int exponent)
{
    CC_ASSERT(num_digits > 0);
    CC_ASSERT(num_digits <= kMaxSignificantDigits);
    CC_ASSERT(DigitValue(digits[0]) > 0);
//  CC_ASSERT(DigitValue(digits[num_digits - 1]) > 0);

    // Any v >= 10^309 is interpreted as +Infinity.
    if (num_digits + exponent > kMaxDecimalPower)
    {
        // Overflow.
        result = std::numeric_limits<double>::infinity();
        return true;
    }

    // Any v <= 10^-324 is interpreted as 0.
    if (num_digits + exponent <= kMinDecimalPower)
    {
        // Underflow.
        result = 0;
        return true;
    }

    return StrtodApprox(result, digits, num_digits, exponent);
}

//--------------------------------------------------------------------------------------------------
// StrtodBignum
//--------------------------------------------------------------------------------------------------

struct DiyInt // bigits * 2^exponent
{
    static constexpr int MaxBits = 64 + 2536 /*log_2(5^(324 - 1 + 769))*/ + 32;
    static constexpr int Capacity = (MaxBits + (32 - 1)) / 32;

    uint32_t bigits[Capacity]; // Significand stored in little-endian form.
    int      size = 0;
    int      exponent = 0;

    DiyInt() = default;
    DiyInt(DiyInt const&) = delete;             // (not needed here)
    DiyInt& operator=(DiyInt const&) = delete;  // (not needed here)
};

inline void AssignZero(DiyInt& x)
{
    x.size = 0;
    x.exponent = 0;
}

inline void AssignU32(DiyInt& x, uint32_t value)
{
    AssignZero(x);

    if (value == 0)
        return;

    x.bigits[0] = value;
    x.size = 1;
}

inline void AssignU64(DiyInt& x, uint64_t value)
{
    AssignZero(x);

    if (value == 0)
        return;

    x.bigits[0] = static_cast<uint32_t>(value);
    x.bigits[1] = static_cast<uint32_t>(value >> 32);
    x.size = (x.bigits[1] == 0) ? 1 : 2;
}

// x := A * x + B
inline void MulAddU32(DiyInt& x, uint32_t A, uint32_t B = 0)
{
    CC_ASSERT(B == 0 || x.exponent == 0);

    if (A == 1 && B == 0)
    {
        return;
    }
    if (A == 0 || x.size == 0)
    {
        AssignU32(x, B);
        return;
    }

    uint32_t carry = B;
    for (int i = 0; i < x.size; ++i)
    {
        uint64_t const p = uint64_t{x.bigits[i]} * A + carry;
        x.bigits[i]      = static_cast<uint32_t>(p);
        carry            = static_cast<uint32_t>(p >> 32);
    }

    if (carry != 0)
    {
        CC_ASSERT(x.size < DiyInt::Capacity);
        x.bigits[x.size++] = carry;
    }
}

inline void AssignDecimalDigits(DiyInt& x, char const* digits, int num_digits)
{
    static constexpr uint32_t kPow10_32[] = {
        1, // (unused)
        10,
        100,
        1000,
        10000,
        100000,
        1000000,
        10000000,
        100000000,
        1000000000, // 10^9
    };

    AssignZero(x);

    while (num_digits > 0)
    {
        int const n = Min(num_digits, 9);
        MulAddU32(x, kPow10_32[n], ReadInt<uint32_t>(digits, n));
        digits     += n;
        num_digits -= n;
    }
}

inline void MulPow2(DiyInt& x, int exp) // aka left-shift
{
    CC_ASSERT(exp >= 0);

    if (x.size == 0)
        return;
    if (exp == 0)
        return;

    int const bigit_shift = static_cast<int>(static_cast<uint32_t>(exp)) / 32;
    int const bit_shift   = static_cast<int>(static_cast<uint32_t>(exp)) % 32;

    if (bit_shift > 0)
    {
        uint32_t carry = 0;
        for (int i = 0; i < x.size; ++i)
        {
            uint32_t const h = x.bigits[i] >> (32 - bit_shift);
            x.bigits[i]      = x.bigits[i] << bit_shift | carry;
            carry            = h;
        }

        if (carry != 0)
        {
            CC_ASSERT(x.size < DiyInt::Capacity);
            x.bigits[x.size++] = carry;
        }
    }

    x.exponent += bigit_shift;
}

inline void MulPow5(DiyInt& x, int exp)
{
    static constexpr uint32_t kPow5_32[] = {
        1, // (unused)
        5,
        25,
        125,
        625,
        3125,
        15625,
        78125,
        390625,
        1953125,
        9765625,
        48828125,
        244140625,
        1220703125, // 5^13
    };

    if (x.size == 0)
        return;

    CC_ASSERT(exp >= 0);
    if (exp == 0)
        return;

    while (exp > 0)
    {
        int const n = Min(exp, 13);
        MulAddU32(x, kPow5_32[n]);
        exp -= n;
    }
}

inline int Compare(DiyInt const& lhs, DiyInt const& rhs)
{
    int const e1 = lhs.exponent;
    int const e2 = rhs.exponent;
    int const n1 = lhs.size + e1;
    int const n2 = rhs.size + e2;

    if (n1 < n2) return -1;
    if (n1 > n2) return +1;

    for (int i = n1 - 1; i >= Min(e1, e2); --i)
    {
        uint32_t const b1 = (i - e1) >= 0 ? lhs.bigits[i - e1] : 0;
        uint32_t const b2 = (i - e2) >= 0 ? rhs.bigits[i - e2] : 0;

        if (b1 < b2) return -1;
        if (b1 > b2) return +1;
    }

    return 0;
}

// Compare digits * 10^exponent with v = f * 2^e.
//
// PRE: num_digits + exponent <= kMaxDecimalPower
// PRE: num_digits + exponent >  kMinDecimalPower
// PRE: num_digits            <= kMaxSignificantDigits
inline int CompareBufferWithDiyFp(char const* digits, int num_digits, int exponent, bool nonzero_tail, DiyFp v)
{
    CC_ASSERT(num_digits > 0);
    CC_ASSERT(num_digits + exponent <= kMaxDecimalPower);
    CC_ASSERT(num_digits + exponent >  kMinDecimalPower);
    CC_ASSERT(num_digits            <= kMaxSignificantDigits);

    DiyInt lhs;
    DiyInt rhs;

    AssignDecimalDigits(lhs, digits, num_digits);
    if (nonzero_tail)
    {
        MulAddU32(lhs, 10, 1);
        exponent--;
    }
    AssignU64(rhs, v.f);

    CC_ASSERT(lhs.size <= (2555 + 31) / 32); // bits <= log_2(10^769) = 2555
    CC_ASSERT(rhs.size <= (  64 + 31) / 32); // bits <= 64

    int lhs_exp5 = 0;
    int rhs_exp5 = 0;
    int lhs_exp2 = 0;
    int rhs_exp2 = 0;

    if (exponent >= 0)
    {
        lhs_exp5 += exponent;
        lhs_exp2 += exponent;
    }
    else
    {
        rhs_exp5 -= exponent;
        rhs_exp2 -= exponent;
    }

    if (v.e >= 0)
    {
        rhs_exp2 += v.e;
    }
    else
    {
        lhs_exp2 -= v.e;
    }

#if 1
    if (lhs_exp5 > 0 || rhs_exp5 > 0)
    {
        MulPow5((lhs_exp5 > 0) ? lhs : rhs, (lhs_exp5 > 0) ? lhs_exp5 : rhs_exp5);
    }
#else
    if (lhs_exp5 > 0) // rhs >= digits
    {
        MulPow5(lhs, lhs_exp5);

        // num_digits + exponent <= kMaxDecimalPower + 1
        CC_ASSERT(lhs.size <= (1030 + 31) / 32);  // 1030 = log_2(10^(309 + 1)))
        CC_ASSERT(rhs.size <= (  64 + 31) / 32);
    }
    else if (rhs_exp5 > 0)
    {
        MulPow5(rhs, rhs_exp5);

        // kMinDecimalPower + 1 <= num_digits + exponent <= kMaxDecimalPower + 1
        // rhs_exp5 = -exponent <= -kMinDecimalPower - 1 + num_digits = 324 - 1 + num_digits <= 324 - 1 + 769
        // rhs_exp5 = -exponent >= -kMaxDecimalPower - 1 + num_digits
        CC_ASSERT(lhs.size <= (2555        + 31) / 32);
        CC_ASSERT(rhs.size <= (  64 + 2536 + 31) / 32); // 2536 = log_2(5^(324 - 1 + 769)) ---- XXX: 2504
    }
#endif

#if 0
    // Cancel common factors of 2.
    int const min_exp2 = Min(lhs_exp2, rhs_exp2);
    lhs_exp2 -= min_exp2;
    rhs_exp2 -= min_exp2;

    MulPow2(lhs, lhs_exp2);
    MulPow2(rhs, rhs_exp2);
#else
#if 1
    int const diff_exp2 = lhs_exp2 - rhs_exp2;
    if (diff_exp2 != 0)
    {
        MulPow2((diff_exp2 > 0) ? lhs : rhs, (diff_exp2 > 0) ? diff_exp2 : -diff_exp2);
    }
#else
    if (lhs_exp2 > rhs_exp2)
    {
        MulPow2(lhs, lhs_exp2 - rhs_exp2);
    }
    else if (rhs_exp2 > lhs_exp2)
    {
        MulPow2(rhs, rhs_exp2 - lhs_exp2);
    }
#endif
#endif

    CC_ASSERT(lhs.size <= (2555        + 32 + 31) / 32);
    CC_ASSERT(rhs.size <= (  64 + 2536 + 32 + 31) / 32);

    return Compare(lhs, rhs);
}

//--------------------------------------------------------------------------------------------------
// DigitsToIEEE
//--------------------------------------------------------------------------------------------------

inline bool SignificandIsEven(double v)
{
//  return (Double(v).PhysicalSignificand() & 1) == 0;
    return (Double(v).bits & 1) == 0;
}

// Returns the next larger double-precision value.
// If v is +Infinity returns v.
inline double NextLarger(double v)
{
    return Double(v).NextValue();
}

// Convert the decimal representation 'digits * 10^exponent' into an IEEE
// double-precision number.
//
// PRE: digits must contain only ASCII characters in the range '0'...'9'.
// PRE: num_digits >= 0
// PRE: num_digits + exponent must not overflow.
inline double DigitsToDouble(char const* digits, int num_digits, int exponent, bool nonzero_tail = false)
{
    CC_ASSERT(num_digits >= 0);
    CC_ASSERT(exponent <= INT_MAX - num_digits);

    // Ignore leading zeros
    while (num_digits > 0 && digits[0] == '0')
    {
        digits++;
        num_digits--;
    }

    // Move trailing zeros into the exponent
    if (!nonzero_tail)
    {
        while (num_digits > 0 && digits[num_digits - 1] == '0')
        {
            num_digits--;
            exponent++;
        }
    }

    if (num_digits > kMaxSignificantDigits)
    {
        // Since trailing zeros have been trimmed above:
        CC_ASSERT(nonzero_tail || DigitValue(digits[num_digits - 1]) > 0);

        nonzero_tail = true;

        // Discard insignificant digits.
        exponent += num_digits - kMaxSignificantDigits;
        num_digits = kMaxSignificantDigits;
    }

    if (num_digits == 0)
    {
        return 0;
    }

    double v;
    if (ComputeGuess(v, digits, num_digits, exponent))
    {
        return v;
    }

    // Now v is either the correct or the next-lower double (i.e. the correct
    // double is v+).
    // Compare B = buffer * 10^exponent with v's upper boundary m+.
    //
    //     v             m+            v+
    //  ---+--------+----+-------------+---
    //              B

    int const cmp = CompareBufferWithDiyFp(digits, num_digits, exponent, nonzero_tail, UpperBoundary(v));
    if (cmp < 0 || (cmp == 0 && SignificandIsEven(v)))
    {
        return v;
    }
    return NextLarger(v);
}

} // namespace charconv_internal
