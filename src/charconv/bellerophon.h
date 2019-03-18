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

#include "common.h"
#include "pow5.h"

namespace charconv {
namespace bellerophon {

//==================================================================================================
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
constexpr int kDoubleMaxSignificantDigits = 767 + 1;

// Max double: 1.7976931348623157 * 10^308, which has 309 digits.
// Any x >= 10^309 is interpreted as +infinity.
constexpr int kDoubleMaxDecimalPower = 309;

// Min non-zero double: 4.9406564584124654 * 10^-324
// Any x <= 10^-324 is interpreted as 0.
// Note that 2.5e-324 (despite being smaller than the min double) will be read
// as non-zero (equal to the min non-zero double).
constexpr int kDoubleMinDecimalPower = -324;

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

template <typename T>
inline T ReadInt(char const* str, int len)
{
    CC_ASSERT(len <= std::numeric_limits<T>::digits10);

    T value = 0;

    for (int i = 0; i < len; ++i)
    {
        CC_ASSERT(IsDigit(str[i]));
        uint8_t const digit = static_cast<uint8_t>(str[i] - '0');
        value = 10 * value + digit;
    }

    return value;
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
#if !defined(CC_CORRECT_DOUBLE_OPERATIONS)
#if defined(__aarch64__)            \
    || defined(__AARCH64EL__)       \
    || defined(__alpha__)           \
    || defined(__arm__)             \
    || defined(__arm64__)           \
    || defined(__ARMEL__)           \
    || defined(__avr32__)           \
    || defined(__hppa__)            \
    || defined(__ia64__)            \
    || defined(__mips__)            \
    || defined(__powerpc__)         \
    || defined(__ppc__)             \
    || defined(__ppc64__)           \
    || defined(__riscv)             \
    || defined(__s390__)            \
    || defined(__SH4__)             \
    || defined(__sparc__)           \
    || defined(__sparc)             \
    || defined(__x86_64__)          \
    || defined(_ARCH_PPC)           \
    || defined(_ARCH_PPC64)         \
    || defined(_M_ARM)              \
    || defined(_M_ARM64)            \
    || defined(_M_X64)              \
    || defined(_MIPS_ARCH_MIPS32R2) \
    || defined(_POWER)
#define CC_CORRECT_DOUBLE_OPERATIONS 1
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
#ifdef _WIN32
// Windows uses a 64bit wide floating point stack.
#define CC_CORRECT_DOUBLE_OPERATIONS 1
#endif
#endif
#endif // !defined(CC_CORRECT_DOUBLE_OPERATIONS)

#if CC_CORRECT_DOUBLE_OPERATIONS

// 2^53 = 9007199254740992.
// Any integer with at most 15 decimal digits will hence fit into a double
// (which has a 53-bit significand) without loss of precision.
constexpr int kMaxExactDoubleIntegerDecimalDigits = 15;
constexpr int kMaxExactDoublePowerOfTen = 22;

inline bool UseFastPath(int num_digits, int exponent)
{
    if (num_digits > kMaxExactDoubleIntegerDecimalDigits)
        return false;
    if (exponent < -kMaxExactDoublePowerOfTen)
        return false;
    if (exponent > (kMaxExactDoubleIntegerDecimalDigits - num_digits) + kMaxExactDoublePowerOfTen)
        return false;

    return true;
}

inline double FastPath(uint64_t digits, int num_digits, int exponent)
{
    CC_ASSERT(UseFastPath(num_digits, exponent));

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
    };

    // The significand fits into a double.
    // Since 10^exponent (resp. 10^-exponent) fits into a double too, we can
    // compute the result simply by multiplying (resp. dividing) the two
    // numbers.
    // This is possible because IEEE guarantees that floating-point operations
    // return the best possible approximation.

    int const remaining_digits = kMaxExactDoubleIntegerDecimalDigits - num_digits; // 0 <= rd <= 15

    CC_ASSERT(digits <= INT64_MAX);
    double d = static_cast<double>(static_cast<int64_t>(digits));
    if (exponent < 0)
    {
        d /= kExactPowersOfTen[-exponent];
    }
    else if (exponent <= kMaxExactDoublePowerOfTen)
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

    return d;
}

#else // ^^^ CC_CORRECT_DOUBLE_OPERATIONS

inline bool UseFastPath(int /*num_digits*/, int /*exponent*/)
{
    return false;
}

inline double FastPath(uint64_t /*digits*/, int /*num_digits*/, int /*exponent*/)
{
    return 0;
}

#endif // ^^^ !CC_CORRECT_DOUBLE_OPERATIONS

//--------------------------------------------------------------------------------------------------
// StrtodApprox
//--------------------------------------------------------------------------------------------------

struct DiyFp // f * 2^e
{
    static constexpr int SignificandSize = 64; // = q

    uint64_t f = 0;
    int e = 0;

    constexpr DiyFp() = default;
    constexpr DiyFp(uint64_t f_, int e_) : f(f_), e(e_) {}
};

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

    auto const p = Mul128(x.f, y.f);
    auto const h = p.hi + (p.lo >> 63); // round, ties up: [h, l] += 2^q / 2

    return DiyFp(h, x.e + y.e + 64);
}

// Decomposes `value` into `f * 2^e`.
// The result is not normalized.
// PRE: `value` must be finite and non-negative, i.e. >= +0.0.
inline DiyFp DiyFpFromFloat(Double v)
{
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
inline DiyFp UpperBoundary(Double value)
{
    auto const v = DiyFpFromFloat(value);
//  return DiyFp(2*v.f + 1, v.e - 1);
    return DiyFp(4*v.f + 2, v.e - 2);
}

struct DiyFpWithError // value = (x.f + delta) * 2^x.e, where |delta| <= error
{
    DiyFp x;
    uint32_t error = 0;

    constexpr DiyFpWithError() = default;
    constexpr DiyFpWithError(DiyFp x_, uint32_t error_) : x(x_), error(error_) {}
};

// Normalize x
inline int Normalize(DiyFp& x)
{
    int const s = CountLeadingZeros64(x.f);

    x.f <<= s;
    x.e  -= s;

    return s;
}

// Normalize input.x
// and scale the error, so that the error is in ULP(x)
inline int Normalize(DiyFpWithError& num)
{
    int const s = CountLeadingZeros64(num.x.f);
    CC_ASSERT(s < 32);
    CC_ASSERT(((num.error << s) >> s) == num.error);

    num.x.f   <<= s;
    num.x.e    -= s;
    num.error <<= s;

    return s;
}

// Returns a cached power of ten x ~= 10^n such that
//  n <= k < n + kCachedPowersDecExpStep.
//
// PRE: k >= kCachedPowersMinDecExp
// PRE: k <  kCachedPowersMaxDecExp + kCachedPowersDecExpStep
inline DiyFp GetCachedPowerForDecimalExponent(int k)
{
    // 10^k = 2^k * 5^k
    auto const pow = ComputePow5(k);
    //
    // FIXME:
    //
    // This trashes 64 bits of the cached power (for now good reason).
    // Find a way to make use of these additional bits!!!
    //
	auto const f = pow.hi + (pow.lo >> 63); // Round, ties towards infinity.
    auto const e = FloorLog2Pow10(k) + 1 - 64;

    return {f, e};
}

// Returns x * 10^k.
inline DiyFp MultiplyPow10(DiyFp x, int k)
{
    auto const pow = GetCachedPowerForDecimalExponent(k);

    CC_ASSERT(IsNormalized(x));
    CC_ASSERT(IsNormalized(pow));

    return Multiply(x, pow);
}

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
// PRE: f <= 2^p - 1
inline double DoubleFromDiyFp(DiyFp x)
{
    CC_ASSERT(x.f <= Double::HiddenBit + Double::SignificandMask);
    CC_ASSERT(x.e <= Double::MinExponent || (x.f & Double::HiddenBit) != 0);

    if (x.e < Double::MinExponent)
        return 0;
    if (x.e > Double::MaxExponent)
        return std::numeric_limits<double>::infinity();

    bool const is_subnormal = (x.e == Double::MinExponent && (x.f & Double::HiddenBit) == 0);

    CC_ASSERT(x.e >= -Double::ExponentBias);
    uint64_t const E = is_subnormal ? 0 : static_cast<unsigned>(x.e + Double::ExponentBias);
    uint64_t const F = x.f & Double::SignificandMask;

    return Double((E << Double::PhysicalSignificandSize) | F).Value();
}

// Maximum number of decimal digits in |significand|.
constexpr int kMaxDigitsInSignificand = 19;

struct StrtodApproxResult {
    double value;
    bool is_correct;
};

// Use DiyFp's to approximate a number of the "D * 10^exponent".
//
// |num_digits| is the number of decimal digits in "D".
// |significand| contains the most significant decimal digits of D and must be rounded (towards +infinity)
// if num_digits >= kMaxDigitsInSignificand.
//
// If |return.is_correct| is true, then |return.value| is the correct double.
// Otherwise |return.value| is either the correct double or the double that is just
// below the correct double.
//
// PRE: |num_digits| + |exponent| must not overflow.
// PRE: |num_digits| + |exponent| <= kDoubleMaxDecimalPower
// PRE: |num_digits| + |exponent| >  kDoubleMinDecimalPower
inline StrtodApproxResult StrtodApprox(uint64_t significand, int num_digits, int exponent)
{
    static_assert(DiyFp::SignificandSize == 64,
        "We use uint64's. This only works if the DiyFp uses uint64's too.");

    CC_ASSERT(significand != 0);
//  CC_ASSERT(num_digits_in_significand > 0);
//  CC_ASSERT(num_digits_in_significand <= 19);
    CC_ASSERT(num_digits > 0);
//  CC_ASSERT(num_digits_in_significand <= num_digits);
    CC_ASSERT(num_digits + exponent <= kDoubleMaxDecimalPower);
    CC_ASSERT(num_digits + exponent >  kDoubleMinDecimalPower);
//  CC_ASSERT(num_digits            <= kDoubleMaxSignificantDigits);

    // Compute an approximation 'input' for B = digits * 10^exponent using DiyFp's.
    // And keep track of the error.
    //
    //                       <-- error -->
    //                               B = digits * 10^exponent
    //  ---------(-----------|-------+---)------------------------------------
    //                       x
    //                       ~= (f * 2^e) * 10^exponent

    DiyFpWithError input;

    input.x.f = significand;
    input.x.e = 0;
    input.error = 0;

    if (UseFastPath(num_digits, exponent))
    {
        return {FastPath(input.x.f, num_digits, exponent), true};
    }

    // We don't want to deal with fractions and therefore work with a common denominator.
    constexpr uint32_t ULP = 2;

//  if (num_digits_in_significand < num_digits)
    if (num_digits > kMaxDigitsInSignificand)
    {
        // We assume |significand| is rounded (towards +infinity),
        // so the error is <= 1/2 ULP.
        input.error = ULP / 2;

        // Normalize x and scale the error, such that 'error' is in ULP(x).
        Normalize(input);

        // Move the remaining decimals into the (decimal) exponent.
        exponent += num_digits - kMaxDigitsInSignificand;
    }
    else
    {
        CC_ASSERT(input.error == 0);

        // The error is == 0.
        // Only scale input.x to avoid UB when shifting a 32-bit unsigned integer
        // by more than 32 bits.
        Normalize(input.x);
    }

    // If the input is exact, error == 0.
    // If the input is inexact, we have read 19 digits, i.e., f >= 10^(19-1) > 2^59.
    // The scaling factor in the normalization step above therefore is <= 2^(63-59) = 2^4.
    CC_ASSERT(input.error <= 16 * (ULP / 2));

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

    input.x = MultiplyPow10(input.x, exponent);
    // x ~= digits * 10^exponent

    // Adjust the error.
    // Since all cached powers have an error of less than 1/2 ulp, err_y = 1/2,
    // and the error is therefore less than 1/2 + (err_x + err_y).
    input.error += ULP / 2 + ULP / 2;

    CC_ASSERT(input.error <= 18 * (ULP / 2));

    // The result of the multiplication might not be normalized.
    // Normalize 'x' again and scale the error.
    Normalize(input);

    // Since both factors were normalized, the scaling factor in the
    // normalization step above is bounded by 2^1.
    CC_ASSERT(input.error <= 36 * (ULP / 2));

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

    uint64_t const half = uint64_t{1} << (excess_bits - 1);

    CC_ASSERT(input.error > 0);

    uint64_t p1;
    uint64_t p2;
    if (excess_bits < 64)
    {
        p1 = input.x.f >> excess_bits;
//      p2 = input.x.f & ((uint64_t{1} << excess_bits) - 1);
        p2 = input.x.f & (2*half - 1);
    }
    else
    {
        p1 = 0;
        p2 = input.x.f;
    }

    DiyFp result;
    result.f = p1;
    result.e = input.x.e + excess_bits; // Move the discarded bits into the exponent.

    // Note:
    // Since error is non-zero, we can safely use '<=' and '>=' in the
    // comparisons below.

    // The following code assumes ULP = 2.
    static_assert(ULP == 2, "internal error");

    // We need to check whether:
    //
    //  p2 * ULP >= half * ULP + error
    //      <=> (p2 - half) * ULP >= error
    //
    // But half * ULP might overflow (in case half = 2^63).
    // So write this as
    //
    //  p2 >= half && (p2 - half) * ULP >= error.
    //
    // Here, (p2 - half) * 2 will not overflow:
    // If half <= 2^62 < 2^63, we have p2 <= 2^63 - 1, so (p2 - half) <= 2^63 - 1,
    // and the multiplication will not overflow.
    // If half = 2^63, we have p2 != 0, since digits is trimmed and non-zero, so
    // (p2 - half) <= 2^64 - 1 - 2^63 = 2^63 - 1, and the multiplication will not overflow.
    //
    // Likewise, for the test
    //
    //  p2 * ULP <= half * ULP - error
    //      <=> (half - p2) * ULP >= error
    //
    // since either half < 2^63 - 1 or p2 != 0.

    CC_ASSERT(p2 < half || (p2 - half) <= UINT64_MAX / ULP);
    CC_ASSERT(p2 > half || (half - p2) <= UINT64_MAX / ULP);

    bool success;
    if (p2 >= half && (p2 - half) * ULP >= input.error)
    {
        success = true;

        // Round up.
        ++result.f;

        // Rounding up may overflow the p-bit significand.
        // But in this case the significand is 2^53 and we don't loose any
        // bits by normalizing 'input' (we just move a factor of 2 into the
        // binary exponent).
        if (result.f > Double::HiddenBit + Double::SignificandMask)
        {
            CC_ASSERT(result.f == (Double::HiddenBit << 1));

            result.f >>= 1;
            result.e  += 1;
        }
    }
    else if (half >= p2 && (half - p2) * ULP >= input.error)
    {
        success = true;

        // Round down,
        // ie. do nothing.
    }
    else
    {
        // Too imprecise.
        // Round down and return false, so that we can fall back to a more
        // precise algorithm.
        success = false;
    }

    return {DoubleFromDiyFp(result), success};
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
    CC_ASSERT(x.size >= 0);
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

inline void AssignDecimalInteger(DiyInt& x, char const* digits, int num_digits)
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

CC_NEVER_INLINE int AssignDecimalFraction(DiyInt& x, char const* next, char const* last, int& exponent)
{
    // digits, stored in [next, last) is of the form "xxx.yyy"
    // This function removes the decimal point, leading and trailing zeros, and adjusts the exponent.
    // Then assigns the decimal integer "xxxyyy" to |x|.

    CC_ASSERT(next != last);

    char digits[kDoubleMaxSignificantDigits + 1];
    int num_digits = 0;
    bool zero_tail = true;

    // Skip leading zeros in "xxx"
    while (next != last && *next == '0')
    {
        ++next;
    }

// int

    while (next != last && IsDigit(*next))
    {
        if (num_digits < kDoubleMaxSignificantDigits)
        {
            digits[num_digits] = *next;
            ++num_digits;
        }
        else
        {
            ++exponent;
            zero_tail &= (*next == '0');
        }
        ++next;
    }

// frac

    if (next != last && *next == '.')
    {
        ++next;

        if (num_digits == 0)
        {
            // 0.yyy
            // Skip leading zeros in "yyy" and adjust the exponent
            while (next != last && *next == '0')
            {
                --exponent;
                ++next;
            }
        }

        while (next != last && IsDigit(*next))
        {
            if (num_digits < kDoubleMaxSignificantDigits)
            {
                digits[num_digits] = *next;
                ++num_digits;
                --exponent;
            }
            else
            {
                zero_tail &= (*next == '0');
            }
            ++next;
        }
    }

    const int result = num_digits;
    if (!zero_tail)
    {
        digits[num_digits] = '1';
        ++num_digits;
        --exponent;
    }

    AssignDecimalInteger(x, digits, num_digits);

    return result;
}

inline void MulPow2(DiyInt& x, int exp) // aka left-shift
{
    CC_ASSERT(x.size >= 0);
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

// Compare (lhs * 10^lhs_decimal_exponent) with (rhs * 2^rhs_binary_exponent).
// Modifies lhs and/or rhs.
inline int MulCompare(DiyInt& lhs, int lhs_decimal_exponent, DiyInt& rhs, int rhs_binary_exponent)
{
    CC_ASSERT(lhs.size <= (2555 + 31) / 32); // bits <= log_2(10^769) = 2555
    CC_ASSERT(rhs.size <= (  64 + 31) / 32); // bits <= 64

    int lhs_exp5 = 0;
    int rhs_exp5 = 0;
    int lhs_exp2 = 0;
    int rhs_exp2 = 0;

    if (lhs_decimal_exponent >= 0)
    {
        lhs_exp5 += lhs_decimal_exponent;
        lhs_exp2 += lhs_decimal_exponent;
    }
    else
    {
        rhs_exp5 -= lhs_decimal_exponent;
        rhs_exp2 -= lhs_decimal_exponent;
    }

    if (rhs_binary_exponent >= 0)
    {
        rhs_exp2 += rhs_binary_exponent;
    }
    else
    {
        lhs_exp2 -= rhs_binary_exponent;
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

        // num_digits + exponent <= kDoubleMaxDecimalPower + 1
        CC_ASSERT(lhs.size <= (1030 + 31) / 32);  // 1030 = log_2(10^(309 + 1)))
        CC_ASSERT(rhs.size <= (  64 + 31) / 32);
    }
    else if (rhs_exp5 > 0)
    {
        MulPow5(rhs, rhs_exp5);

        // kDoubleMinDecimalPower + 1 <= num_digits + exponent <= kDoubleMaxDecimalPower + 1
        // rhs_exp5 = -exponent <= -kDoubleMinDecimalPower - 1 + num_digits = 324 - 1 + num_digits <= 324 - 1 + 769
        // rhs_exp5 = -exponent >= -kDoubleMaxDecimalPower - 1 + num_digits
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

// Compare digits * 10^exponent with v = f * 2^e.
//
// PRE: num_digits + exponent <= kDoubleMaxDecimalPower
// PRE: num_digits + exponent >  kDoubleMinDecimalPower
// PRE: num_digits            <= kDoubleMaxSignificantDigits
inline int CompareBufferWithDiyFp(char const* digits, int num_digits, int exponent, bool nonzero_tail, DiyFp v)
{
    CC_ASSERT(num_digits > 0);
    CC_ASSERT(num_digits + exponent <= kDoubleMaxDecimalPower);
    CC_ASSERT(num_digits + exponent >  kDoubleMinDecimalPower);
    CC_ASSERT(num_digits            <= kDoubleMaxSignificantDigits);

    DiyInt lhs;
    DiyInt rhs;

    AssignDecimalInteger(lhs, digits, num_digits);
    if (nonzero_tail)
    {
        MulAddU32(lhs, 10, 1);
        exponent--;
    }
    AssignU64(rhs, v.f);

    return MulCompare(lhs, exponent, rhs, v.e);
}

//--------------------------------------------------------------------------------------------------
// DigitsToDouble
//--------------------------------------------------------------------------------------------------

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

    if (num_digits > kDoubleMaxSignificantDigits)
    {
        // Since trailing zeros have been trimmed above:
        CC_ASSERT(nonzero_tail || DigitValue(digits[num_digits - 1]) > 0);

        nonzero_tail = true;

        // Discard insignificant digits.
        exponent += num_digits - kDoubleMaxSignificantDigits;
        num_digits = kDoubleMaxSignificantDigits;
    }

    if (num_digits == 0)
    {
        return 0;
    }

    // Any v >= 10^309 is interpreted as +Infinity.
    if (num_digits + exponent > kDoubleMaxDecimalPower)
    {
        return std::numeric_limits<double>::infinity();
    }

    // Any v <= 10^-324 is interpreted as 0.
    if (num_digits + exponent <= kDoubleMinDecimalPower)
    {
        return 0.0;
    }

    // Read up to 19 digits of the significand.
    uint64_t significand = ReadInt<uint64_t>(digits, Min(num_digits, kMaxDigitsInSignificand));
    if (num_digits > kMaxDigitsInSignificand)
    {
        // Round towards +infinity.
        significand += (digits[kMaxDigitsInSignificand] >= '5');
    }

    // Compute an approximation to digits * 10^exponent using the most significant digits.
    const StrtodApproxResult guess = StrtodApprox(significand, num_digits, exponent);
    if (guess.is_correct)
    {
        return guess.value;
    }

    Double v(guess.value);

    // Now v is either the correct or the next-lower double (i.e. the correct double is v+).
    // Compare B = buffer * 10^exponent with v's upper boundary m+.
    //
    //     v             m+            v+
    //  ---+--------+----+-------------+---
    //              B

    int const cmp = CompareBufferWithDiyFp(digits, num_digits, exponent, nonzero_tail, UpperBoundary(v));
    if (cmp < 0 || (cmp == 0 && (v.bits & 1) == 0))
    {
        return guess.value;
    }

    return v.NextValue();
}

} // namespace bellerophon
} // namespace charconv
