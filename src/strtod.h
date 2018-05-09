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

// Derived from the double-conversion library:
// https://github.com/google/double-conversion
//
// The original license can be found at the end of this file.

#pragma once

#include "dtoa.h"

namespace base_conv {

// Maximum number of significant digits in decimal representation.
// The longest possible double in decimal representation is
// (2^53 - 1) * 5^1074 / 10^1074, which has (767 digits).
// If we parse a number whose first digits are equal to a mean of 2 adjacent
// doubles (that could have up to 768 digits) the result must be rounded to the
// bigger one unless the tail consists of zeros, so we don't need to preserve
// all the digits.
constexpr int kMaxSignificantDigits = 767 + 1;

// To simplify the implementation we use an extra digit to indicate whether the
// tail consists of zeros or not. If the tail consists of zeros, we ignore the
// extra digit, otherwise it will be set to a non-zero value (which is enough
// to guarantee correct rounding.)
constexpr int kMaxSignificantDecimalDigits = kMaxSignificantDigits + 1;

//==================================================================================================
// DecimalToDouble
//==================================================================================================

namespace impl {

inline constexpr int Min(int x, int y) { return y < x ? y : x; }
inline constexpr int Max(int x, int y) { return y < x ? x : y; }

inline int DigitValue(char ch)
{
    DTOA_ASSERT(ch >= '0');
    DTOA_ASSERT(ch <= '9');
    return ch - '0';
}

// Returns `f * 2^e`.
// Note: the significand is truncated to Double::Precision bits.
inline double DoubleFromDiyFp(uint64_t f, int e)
{
    while (f > Double::HiddenBit + Double::SignificandMask) {
        f >>= 1;
        DTOA_ASSERT(e != INT_MAX);
        e++;
    }
    if (e > Double::MaxExponent) {
        return std::numeric_limits<double>::infinity();
    }
    if (e < Double::MinExponent) {
        return 0.0;
    }
    while (e > Double::MinExponent && (f & Double::HiddenBit) == 0) {
        f <<= 1;
        e--;
    }

    uint64_t const exponent = (e == Double::MinExponent && (f & Double::HiddenBit) == 0)
        ? 0 // (subnormal)
        : static_cast<uint64_t>(e + Double::ExponentBias);

    uint64_t const bits = (exponent << Double::SignificandSize) | (f & Double::SignificandMask);
    return ReinterpretBits<double>(bits);
}

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
#define DTOA_CORRECT_DOUBLE_OPERATIONS 1
#elif defined(_M_IX86) || defined(__i386__) || defined(__i386)
#if defined(_WIN32)
// Windows uses a 64bit wide floating point stack.
#define DTOA_CORRECT_DOUBLE_OPERATIONS 1
#endif // _WIN32
#elif defined(__mc68000__)
// Not supported.
#else
#error Target architecture was not detected as supported by Double-Conversion.
#endif

#if !DTOA_CORRECT_DOUBLE_OPERATIONS

inline bool StrtodFast(char const* /*digits*/, int /*num_digits*/, int /*exponent*/, double& /*result*/)
{
    return false;
}

#else // ^^^ !DTOA_CORRECT_DOUBLE_OPERATIONS

// 2^53 = 9007199254740992.
// Any integer with at most 15 decimal digits will hence fit into a double
// (which has a 53bit significand) without loss of precision.
constexpr int kMaxExactDoubleIntegerDecimalDigits = 15;

inline double ReadDouble_unsafe(char const* digits, int num_digits)
{
    DTOA_ASSERT(num_digits >= 0);
    DTOA_ASSERT(num_digits <= kMaxExactDoubleIntegerDecimalDigits);

    int64_t result = 0;

    for (int i = 0; i < num_digits; ++i)
    {
        result = 10 * result + DigitValue(digits[i]);
    }

    return static_cast<double>(result);
}

inline bool StrtodFast(char const* digits, int num_digits, int exponent, double& result)
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
        auto d = ReadDouble_unsafe(digits, num_digits);
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
            // The buffer is short and we can multiply it with
            // 10^remaining_digits and the remaining exponent fits into a double.
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

#endif // DTOA_CORRECT_DOUBLE_OPERATIONS

// 2^64 = 18446744073709551616 > 10^19
// Any integer with at most 19 decimal digits will hence fit into an uint64_t.
constexpr int kMaxUint64DecimalDigits = 19;

// Reads a (rounded) DiyFp from the buffer.
//
// If read_digits == buffer_length then the returned DiyFp is accurate.
// Otherwise it has been rounded and has an error of at most 1/2 ulp.
//
// The returned DiyFp is not normalized.
inline DiyFp ReadDiyFp(char const* digits, int num_digits, int& read_digits)
{
    DTOA_ASSERT(num_digits > 0);
    DTOA_ASSERT(DigitValue(digits[num_digits - 1]) > 0); // The buffer has been trimmed.

    uint64_t significand = 0;

    int const max_len = num_digits < kMaxUint64DecimalDigits ? num_digits : kMaxUint64DecimalDigits;
    int i = 0;
    for ( ; i < max_len; ++i)
    {
        significand = 10 * significand + static_cast<uint32_t>(DigitValue(digits[i]));
    }

    if (i < num_digits)
    {
        if (DigitValue(digits[i]) >= 5)
        {
            // Round up.
            DTOA_ASSERT(significand != UINT64_MAX);
            ++significand;
        }
    }

    read_digits = i;
    return {significand, 0};
}

// Returns a cached power of ten x ~= 10^k such that
//  k <= e < k + kCachedPowersDecExpStep.
//
// PRE: e >= kCachedPowersMinDecExp
// PRE: e <  kCachedPowersMaxDecExp + kCachedPowersDecExpStep
inline CachedPower GetCachedPowerForDecimalExponent(int e)
{
    DTOA_ASSERT(e >= kCachedPowersMinDecExp);
    DTOA_ASSERT(e <  kCachedPowersMaxDecExp + kCachedPowersDecExpStep);

    int const index = (-kCachedPowersMinDecExp + e) / kCachedPowersDecExpStep;
    DTOA_ASSERT(index >= 0);
    DTOA_ASSERT(index < kCachedPowersSize);

    auto const cached = GetCachedPower(index);
    DTOA_ASSERT(e >= cached.k);
    DTOA_ASSERT(e <  cached.k + kCachedPowersDecExpStep);

    return cached;
}

// Returns 10^exponent as an exact DiyFp.
// PRE: 1 <= exponent < kCachedPowersDecExpStep
inline DiyFp GetAdjustmentPowerOfTen(int exponent)
{
    static_assert(kCachedPowersDecExpStep <= 8, "internal error");
    static constexpr DiyFp kPowers[] = {
        { 0x8000000000000000, -63 }, // == 10^0 (unused)
        { 0xA000000000000000, -60 }, // == 10^1
        { 0xC800000000000000, -57 }, // == 10^2
        { 0xFA00000000000000, -54 }, // == 10^3
        { 0x9C40000000000000, -50 }, // == 10^4
        { 0xC350000000000000, -47 }, // == 10^5
        { 0xF424000000000000, -44 }, // == 10^6
        { 0x9896800000000000, -40 }, // == 10^7
    };

    DTOA_ASSERT(exponent > 0);
    DTOA_ASSERT(exponent < kCachedPowersDecExpStep);
    return kPowers[exponent];
}

// Max double: 1.7976931348623157 x 10^308
// Min non-zero double: 4.9406564584124654 x 10^-324
// Any x >= 10^309 is interpreted as +infinity.
// Any x <= 10^-324 is interpreted as 0.
// Note that 2.5e-324 (despite being smaller than the min double) will be read
// as non-zero (equal to the min non-zero double).
constexpr int kMaxDecimalPower =  309;
constexpr int kMinDecimalPower = -324;

// Returns the significand size for a given order of magnitude.
//
// If v = f * 2^e with 2^(q-1) <= f <= 2^q then (q+e) is v's order of magnitude.
// If v = s * 2^e with 1/2 <= s < 1 then e is v's order of magnitude.
//
// This function returns the number of significant binary digits v will have
// once it's encoded into a 'double'. In almost all cases this is equal to
// Double::Precision. The only exceptions are subnormals. They start with
// leading zeroes and their effective significand-size is hence smaller.
inline int EffectiveSignificandSize(int order)
{
    if (order >= Double::MinExponent + Double::Precision)
        return Double::Precision;

    if (order <= Double::MinExponent)
        return 0;

    return order - Double::MinExponent;
}

// Use DiyFp's to approximate digits * 10^exponent.
//
// If the function returns true then 'result' is the correct double.
// Otherwise 'result' is either the correct double or the double that is just
// below the correct double.
//
// PRE: num_digits + exponent <= kMaxDecimalPower
// PRE: num_digits + exponent >  kMinDecimalPower
inline bool StrtodApprox(char const* digits, int num_digits, int exponent, double& result)
{
    static_assert(DiyFp::Precision == 64,
        "We use uint64_ts. This only works if the DiyFp uses uint64_ts too.");

    DTOA_ASSERT(num_digits + exponent <= kMaxDecimalPower);
    DTOA_ASSERT(num_digits + exponent >  kMinDecimalPower);

    //
    // Compute an approximation 'input' for B = digits * 10^exponent
    // using DiyFp's.
    // And keep track of the error.
    //
    //                       <-- error -->
    //                               B = digits * 10^exponent
    //  ---------(-----------|-------+---)------------------------------------
    //                       input
    //                       ~= digits * 10^exponent
    //

    int read_digits;
    auto input = ReadDiyFp(digits, num_digits, read_digits);

    int const remaining_decimals = num_digits - read_digits; // 0 <= rd < num_digits

    // Move the remaining decimals into the exponent.
    exponent += remaining_decimals;

    if (exponent < kCachedPowersMinDecExp)
    {
        result = 0.0;
        return true;
    }

    // Since we may have dropped some digits, 'input' may not be accurate.
    // If 'remaining_decimals' is different from 0 than we did round and the
    // error is at most 1/2 ulp (unit in the last place).
    // We don't want to deal with fractions and therefore work with fixed-point
    // numbers.
    constexpr int kDenominatorLog = 3;
    constexpr int kDenominator = 1 << kDenominatorLog;

    // error is in ULP(input); scaled by kDenominator
    uint64_t error = (remaining_decimals == 0) ? 0 : kDenominator / 2;

    // Normalize 'input' (i.e. multiply by a power of 2) and scale the error
    // accordingly.
    {
        int const old_e = input.e;
        input = Normalize(input);   // input = (f * 2^s) * 2^(e - s)
        int const s = old_e - input.e;
        error <<= s;                // error * 2^e = (error * 2^s) * 2^(e - s)
    }

    // input = f * 2^e, where 2^(q-1) <= f < 2^q

    // Multiply input by 10^exponent.
    // And keep track of the error.
    //
    // The error introduced by a multiplication of a * b equals
    //      error = error_a + error_b + error_ab / 2^q + 1/2,
    // where
    //      error_ab = error_a * error_b,
    //
    // The resulting error is in ULP(a*b), error_a and error_b are in ULP(a)
    // and ULP(b) resp.
    // All values are scaled by kDenominator.

    auto const cached = GetCachedPowerForDecimalExponent(exponent);
    auto const cached_power = DiyFp(cached.f, cached.e);

    // Not all powers-of-ten are cached. If cached.k != exponent
    // we need to multiply `input` by the difference first. This may introduce
    // an additional error.
    if (cached.k != exponent)
    {
        auto const adjustment_exponent = exponent - cached.k;
        auto const adjustment_power = GetAdjustmentPowerOfTen(adjustment_exponent);

        auto const input_f_old = input.f;
        input = Multiply(input, adjustment_power);
        // input ~= digits * 10^adjustment_exponent

        // Adjust error.
        // The adjustment_power is exact (error_b = error_ab = 0).
        // There is hence only an additional error of (at most) 1/2.

        if (num_digits + adjustment_exponent <= kMaxUint64DecimalDigits)
        {
            // input and adjustment_power are exact.
            // The product digits * 10^adjustment_exponent fits into an uint64_t.
            // input * adjustment_power is therefore exact, too.

            // (Assert the lower 64-bits of the product are 0.)
            DTOA_ASSERT(input_f_old * adjustment_power.f == 0);
            static_cast<void>(input_f_old);
        }
        else
        {
            error += kDenominator / 2;
        }
    }

    input = Multiply(input, cached_power);
    // input ~= digits * 10^exponent

    // Adjust the error.
    // Substituting 'a' with 'input' and 'b' with 'cached_power' we have
    //   error_b  = 0.5  (all cached powers have an error of less than 1/2 ulp),
    //   error_ab = 0 or 1/kDenominator > error_a*error_b/2^64
    uint64_t const error_a = error;
    uint64_t const error_b = kDenominator / 2;
    uint64_t const error_ab = (error_a == 0 ? 0 : 1); // We round up to 1.

    error = error_a + error_b + error_ab + kDenominator / 2;

    // The result of the multiplication might not be normalized.
    // Normalize 'input' again and scale the error.
    {
        int const old_e = input.e;
        input = Normalize(input);   // input = (f * 2^s) * 2^(e - s)
        int const s = old_e - input.e;
        error <<= s;                // error * 2^e = (error * 2^s) * 2^(e - s)
    }

    //
    // We now have an approximation input = f * 2^e ~= digits * 10^exponent.
    //
    //                       <-- error -->
    //                               B = digits * 10^exponent
    //  ---------(-----------|-------+---)------------------------------------
    //                       input
    //                       ~= digits * 10^exponent
    //
    // When converting f * 2^e, which has a q-bit significand, into an IEEE
    // double-precision number, we need to drop some 'extra_bits' bits of
    // precision.
    //

    int const order_of_magnitude = DiyFp::Precision + input.e;

    int const effective_significand_size = EffectiveSignificandSize(order_of_magnitude);
    DTOA_ASSERT(effective_significand_size >= 0);
    DTOA_ASSERT(effective_significand_size <= 53);

    int extra_bits = DiyFp::Precision - effective_significand_size;
    DTOA_ASSERT(extra_bits >= 11);
    DTOA_ASSERT(extra_bits <= 64);

    if (extra_bits + kDenominatorLog >= DiyFp::Precision)
    {
        // This can only happen for very small subnormals. In this case 'half'
        // (see below) multiplied by kDenominator exceeds the range of an
        // uint64_t.
        // Simply shift everything to the right.
        // And adjust the error.

        // input = f * 2^e ~= (f div 2^s) * 2^(e + s)
        int const s = extra_bits - (DiyFp::Precision - kDenominatorLog - 1);
        input.f >>= s;
        input.e  += s;

        // error * 2^e ~= (error div 2^s) * 2^(e + s)
        error >>= s;

        // We add kDenominator for the lost precision of input.f
        error += kDenominator;  // kDenominator = 1 ulp
        // We add 1 for the lost precision of error,
        error += 1;             // 1 = 1/kDenominator ulp

        extra_bits = DiyFp::Precision - kDenominatorLog - 1;
    }

    //
    // f = (f div 2^p) * 2^p + (f mod 2^p)
    //   = (p1       ) * 2^p + (p2       )
    //
    //                             f = p1 * 2^p + p2
    //   <--- p2 ------------------>
    //                 <-- error --+-- error -->
    // --|-------------(-----------+------|----)---------------------------|--
    //   p1 * 2^p                                                 (p1 + 1) * 2^p
    //   <------------- half ------------->
    //                  = 2^p / 2
    //
    // The correct double now is either p1 * 2^(e + p) or (p1 + 1) * 2^(e + p).
    // In case p2 + error < half, we can safely round down. If p2 - error > half
    // we can safely round up. Otherwise, we are too inaccurate. In this case
    // we round down, so the returned double is either the correct double or the
    // double just below the correct double. In this case we return false, so
    // that the we can fall back to a more precise algorithm.
    //

    DTOA_ASSERT(extra_bits >= 11);
    DTOA_ASSERT(extra_bits <  64);
    DTOA_ASSERT(extra_bits <  DiyFp::Precision - kDenominatorLog);

    uint64_t p2   = input.f & ((uint64_t{1} << extra_bits) - 1);
    uint64_t half = uint64_t{1} << (extra_bits - 1);
    // 'error' is scaled by kDenominator.
    // In order to compare 'p2' and 'half' with 'error', these values need to be
    // scaled, too.
    DTOA_ASSERT(p2   <= UINT64_MAX / kDenominator);
    DTOA_ASSERT(half <= UINT64_MAX / kDenominator);
    p2   *= kDenominator;
    half *= kDenominator;

    // Truncate the significand to p = Float::Precision bits and move the
    // remaining bits into the exponent.
    input.f >>= extra_bits;
    input.e  += extra_bits;

    DTOA_ASSERT(error > 0);
    DTOA_ASSERT(half >= error);
    DTOA_ASSERT(half <= UINT64_MAX - error);

    // Since 'error' is non-zero, we can safely use '<=' and '>=' in the
    // comparisons below.

    bool success;
    if (p2 >= half + error)
    {
        // Round up.
        DTOA_ASSERT(input.f != UINT64_MAX);
        ++input.f;
        success = true;
    }
    else if (p2 <= half - error)
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

    result = DoubleFromDiyFp(input.f, input.e);
    return success;
}

struct DiyInt // bigits * 2^exponent
{
    static constexpr int MaxBits = 64 + 2536 /*log_2(5^(324 - 1 + 769))*/ + 32;
    static constexpr int BigitSize = 32;
    static constexpr int Capacity = (MaxBits + (BigitSize - 1)) / BigitSize;

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

inline void AssignU64(DiyInt& x, uint64_t value)
{
    AssignZero(x);

    if (value == 0)
        return;

    x.bigits[0] = static_cast<uint32_t>(value);
    x.bigits[1] = static_cast<uint32_t>(value >> DiyInt::BigitSize);
    x.size = (x.bigits[1] == 0) ? 1 : 2;
}

inline void MulU32(DiyInt& x, uint32_t value)
{
    if (value == 1)
        return;
    if (value == 0) {
        AssignZero(x);
        return;
    }

    if (x.size == 0)
        return;

    uint32_t carry = 0;
    for (int i = 0; i < x.size; ++i)
    {
        uint64_t const p = uint64_t{x.bigits[i]} * value + carry;
        x.bigits[i]      = static_cast<uint32_t>(p);
        carry            = static_cast<uint32_t>(p >> DiyInt::BigitSize);
    }

    if (carry != 0)
    {
        DTOA_ASSERT(x.size < DiyInt::Capacity);
        x.bigits[x.size++] = carry;
    }
}

inline void AddU32(DiyInt& x, uint32_t value)
{
    DTOA_ASSERT(x.exponent == 0);

    if (value == 0)
        return;

    uint32_t carry = value;
    for (int i = 0; i < x.size; ++i)
    {
        uint64_t const p = uint64_t{x.bigits[i]} + carry;
        x.bigits[i]      = static_cast<uint32_t>(p);
        carry            = static_cast<uint32_t>(p >> DiyInt::BigitSize);
    }

    if (carry != 0)
    {
        DTOA_ASSERT(x.size < DiyInt::Capacity);
        x.bigits[x.size++] = static_cast<uint32_t>(carry);
    }
}

inline uint32_t ReadU32(char const* digits, int num_digits)
{
    uint32_t result = 0;

    for (int i = 0; i < num_digits; ++i)
    {
        result = 10 * result + static_cast<uint32_t>(DigitValue(digits[i]));
    }

    return result;
}

inline void AssignDecimalDigits(DiyInt& x, char const* digits, int num_digits)
{
    static constexpr uint32_t kPow10[] = {
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

#if 0
    while (num_digits >= 9)
    {
        MulU32(x, 1000000000);
        AddU32(x, ReadU32(digits, 9));
        digits     += 9;
        num_digits -= 9;
    }
    if (num_digits > 0)
    {
        MulU32(x, kPow10[num_digits]);
        AddU32(x, ReadU32(digits, num_digits));
    }
#else
    while (num_digits > 0)
    {
        int const n = Min(num_digits, 9);
        MulU32(x, kPow10[n]);
        AddU32(x, ReadU32(digits, n));
        digits     += n;
        num_digits -= n;
    }
#endif
}

inline void MulPow2(DiyInt& x, int exp) // aka left-shift
{
    if (x.size == 0)
        return;

    DTOA_ASSERT(exp >= 0);
    if (exp == 0)
        return;

    int const bigit_shift = exp / DiyInt::BigitSize;
    int const bit_shift   = exp % DiyInt::BigitSize;

    uint32_t carry = 0;
    for (int i = 0; i < x.size; ++i)
    {
        uint32_t const h = x.bigits[i] >> (DiyInt::BigitSize - bit_shift);
        x.bigits[i]      = x.bigits[i] << bit_shift | carry;
        carry            = h;
    }

    if (carry != 0)
    {
        DTOA_ASSERT(x.size < DiyInt::Capacity);
        x.bigits[x.size++] = carry;
    }

    x.exponent += bigit_shift;
}

inline void MulPow5(DiyInt& x, int exp)
{
    static constexpr uint32_t kPow5[] = {
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

    DTOA_ASSERT(exp >= 0);
    if (exp == 0)
        return;

#if 0
    while (exp >= 13)
    {
        MulU32(x, 1220703125);
        exp -= 13;
    }
    if (exp > 0)
    {
        MulU32(x, kPow5[exp]);
    }
#else
    while (exp > 0)
    {
        int const n = Min(exp, 13);
        MulU32(x, kPow5[n]);
        exp -= n;
    }
#endif
}

inline int Compare(DiyInt const& lhs, DiyInt const& rhs)
{
    int const e1 = lhs.exponent;
    int const e2 = rhs.exponent;
    int const n1 = lhs.size + e1;
    int const n2 = rhs.size + e2;

    if (n1 < n2)
        return -1;
    if (n1 > n2)
        return +1;

    for (int i = n1 - 1; i >= Min(e1, e2); --i)
    {
        uint32_t const b1 = (i - e1) >= 0 ? lhs.bigits[i - e1] : 0;
        uint32_t const b2 = (i - e2) >= 0 ? rhs.bigits[i - e2] : 0;

        if (b1 < b2)
            return -1;
        if (b1 > b2)
            return +1;
    }

    return 0;
}

// Compare digits * 10^exponent with v = f * 2^e.
//
// PRE: num_digits + exponent <= kMaxDecimalPower + 1
// PRE: num_digits + exponent >  kMinDecimalPower
// PRE: num_digits            <= kMaxSignificantDecimalDigits
inline int CompareBufferWithDiyFp(char const* digits, int num_digits, int exponent, DiyFp v)
{
    DTOA_ASSERT(num_digits + exponent <= kMaxDecimalPower + 1);
    DTOA_ASSERT(num_digits + exponent >= kMinDecimalPower + 1);
    DTOA_ASSERT(num_digits            <= kMaxSignificantDecimalDigits);

    DiyInt lhs;
    DiyInt rhs;

    AssignDecimalDigits(lhs, digits, num_digits);
    AssignU64(rhs, v.f);

    DTOA_ASSERT(lhs.size <= (2555 + 31) / 32); // bits <= log_2(10^(num_digits - 1)) <= log_2(10^769) = 2555
    DTOA_ASSERT(rhs.size <= (  64 + 31) / 32); // bits <= 64

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

    if (lhs_exp5 > 0) // rhs >= digits
    {
        MulPow5(lhs, lhs_exp5);

        // num_digits + exponent <= kMaxDecimalPower + 1
        DTOA_ASSERT(lhs.size <= (1030 + 31) / 32);  // 1030 = log_2(10^(309 + 1)))
        DTOA_ASSERT(rhs.size <= (  64 + 31) / 32);
    }
    else if (rhs_exp5 > 0)
    {
        MulPow5(rhs, rhs_exp5);

        // kMinDecimalPower + 1 <= num_digits + exponent <= kMaxDecimalPower + 1
        // rhs_exp5 = -exponent <= -kMinDecimalPower - 1 + num_digits = 324 - 1 + num_digits <= 324 - 1 + 769
        // rhs_exp5 = -exponent >= -kMaxDecimalPower - 1 + num_digits
        DTOA_ASSERT(lhs.size <= (2555        + 31) / 32);
        DTOA_ASSERT(rhs.size <= (  64 + 2536 + 31) / 32); // 2536 = log_2(5^(324 - 1 + 769)) ---- XXX: 2504
    }

#if 0
    MulPow2(lhs, lhs_exp2);
    MulPow2(rhs, rhs_exp2);
#else
    // Cancel common factors of 2.
    if (lhs_exp2 > rhs_exp2)
    {
        MulPow2(lhs, lhs_exp2 - rhs_exp2);
    }
    else if (rhs_exp2 > lhs_exp2)
    {
        MulPow2(rhs, rhs_exp2 - lhs_exp2);
    }
#endif

    DTOA_ASSERT(lhs.size <= (2555        + 32 + 31) / 32);
    DTOA_ASSERT(rhs.size <= (  64 + 2536 + 32 + 31) / 32);

    //printf("lhs[%3d + %3d = %3d] -- rhs[%3d + %3d = %3d]\n", lhs.size, lhs.exponent, lhs.size + lhs.exponent, rhs.size, rhs.exponent, rhs.size + rhs.exponent);

    return Compare(lhs, rhs);
}

// Returns whether the significand f of v = f * 2^e is even.
inline bool SignificandIsEven(double v)
{
//  return (DiyFpFromDouble(v).f & 1) == 0;
    return (Double(v).PhysicalSignificand() & 1) == 0;
}

// Returns the next larger double-precision value.
// If v is +Infinity returns v.
inline double NextFloat(double v)
{
    return Double(v).NextValue();
}

inline bool ComputeGuess(double& result, char const* digits, int num_digits, int exponent)
{
    if (num_digits == 0) {
        result = 0;
        return true;
    }

    DTOA_ASSERT(num_digits <= kMaxSignificantDecimalDigits);
    DTOA_ASSERT(DigitValue(digits[0]) > 0);
    DTOA_ASSERT(DigitValue(digits[num_digits - 1]) > 0);

    // Any v >= 10^309 is interpreted as +Infinity.
    if (num_digits + exponent >= kMaxDecimalPower + 1) {
        result = std::numeric_limits<double>::infinity();
        return true;
    }

    // Any v <= 10^-324 is interpreted as 0.
    if (num_digits + exponent <= kMinDecimalPower) {
        result = 0;
        return true;
    }

    if (StrtodFast(digits, num_digits, exponent, result)) {
        return true;
    }
    if (StrtodApprox(digits, num_digits, exponent, result)) {
        return true;
    }
    if (result == std::numeric_limits<double>::infinity()) {
        return true;
    }

    return false;
}

inline int CountLeadingZeros(char const* digits, int num_digits)
{
    DTOA_ASSERT(num_digits >= 0);

    int i = 0;
    for ( ; i < num_digits && digits[i] == '0'; ++i)
    {
    }
    return i;
}

inline int CountTrailingZeros(char const* digits, int num_digits)
{
    DTOA_ASSERT(num_digits >= 0);

    int i = num_digits;
    for ( ; i > 0 && digits[i - 1] == '0'; --i)
    {
    }
    return num_digits - i;
}

} // namespace impl

// Convert the decimal representation 'digits * 10^exponent' into an IEEE
// double-precision number.
//
// PRE: digits must contain only ASCII characters in the range '0'...'9'.
// PRE: num_digits >= 0
// PRE: num_digits <= kMaxSignificandDecimalDigits
// PRE: num_digits + exponent must not overflow.
inline double DecimalToDouble(char const* digits, int num_digits, int exponent)
{
    DTOA_ASSERT(num_digits >= 0);
    DTOA_ASSERT(exponent <= INT_MAX - num_digits);

#if 1
    // Ignore leading zeros
    int const lz = impl::CountLeadingZeros(digits, num_digits);
    digits     += lz;
    num_digits -= lz;

    // Move trailing zeros into the exponent
    int const tz = impl::CountTrailingZeros(digits, num_digits);
    num_digits -= tz;
    exponent   += tz;
#endif

    double v;
    if (impl::ComputeGuess(v, digits, num_digits, exponent)) {
        return v;
    }

    // Now v is either the correct or the next-lower double (i.e. the correct double is v+).
    // Compare B = buffer * 10^exponent with v's upper boundary m+.
    //
    //     v             m+            v+
    //  ---+--------+----+-------------+---
    //              B

    int const cmp = impl::CompareBufferWithDiyFp(digits, num_digits, exponent, impl::UpperBoundary(v));
    if (cmp < 0 || (cmp == 0 && impl::SignificandIsEven(v))) {
        return v;
    }
    return impl::NextFloat(v);
}

//==================================================================================================
// StringToDouble
//==================================================================================================

namespace impl {

inline double SignedZero(bool negative)
{
    return negative ? -0.0 : 0.0;
}

} // namespace impl

inline bool StringToDouble(double& result, char const* next, char const* last)
{
    // Inputs larger than kMaxInt (currently) can not be handled.
    // To avoid overflow in integer arithmetic.
    constexpr int const kMaxInt = INT_MAX / 4;

    if (next == last) {
        result = 0; // [Recover.]
        return true;
    }

    if (last - next >= kMaxInt) {
        result = std::numeric_limits<double>::quiet_NaN();
        return false;
    }

    constexpr int const kBufferSize = kMaxSignificantDigits + 1;

    char digits[kBufferSize];
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
            result = impl::SignedZero(is_neg); // Recover.
            return false;
        }
    }
    else if (/*allow_leading_plus &&*/ *next == '+')
    {
        ++next;
        if (next == last)
        {
            result = impl::SignedZero(is_neg); // Recover.
            return false;
        }
    }

    if (*next == '0')
    {
        ++next;
        if (next == last)
        {
            result = impl::SignedZero(is_neg);
            return true;
        }
    }
    else if ('1' <= *next && *next <= '9')
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
            if (*next < '0' || *next > '9')
            {
                break;
            }
        }
    }
#if 0
    else if (allow_leading_dot && *next == '.')
    {
    }
#endif
    else
    {
        if (/*allow_nan_inf &&*/ last - next >= 3 && std::memcmp(next, "NaN", 3) == 0)
        {
            result = std::numeric_limits<double>::quiet_NaN();
            return true;
        }

        if (/*allow_nan_inf &&*/ last - next >= 8 && std::memcmp(next, "Infinity", 8) == 0)
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
                    result = impl::SignedZero(is_neg);
                    return true;
                }

                // Move this 0 into the exponent.
                --exponent;
            }
        }

        // There is a fractional part.
        // We don't emit a '.', but adjust the exponent instead.
        while ('0' <= *next && *next <= '9')
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
            result = std::numeric_limits<double>::quiet_NaN();
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
                result = std::numeric_limits<double>::quiet_NaN();
                return false;
            }
        }

        if (*next < '0' || *next > '9')
        {
            // XXX:
            // Recover? Parse as if exponent = 0?
            result = std::numeric_limits<double>::quiet_NaN();
            return false;
        }

        int num = 0;
        for (;;)
        {
            int const digit = *next - '0';

//          if (num > kMaxInt / 10 || digit > kMaxInt - 10 * num)
            if (num > kMaxInt / 10 - 9)
            {
                // Overflow.
                // Skip the rest of the exponent (ignored).
                for (++next; next != last && '0' <= *next && *next <= '9'; ++next)
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
            if (*next < '0' || *next > '9')
            {
                break; // trailing junk
            }
        }

        exponent += exp_is_neg ? -num : num;
    }

L_parsing_done:
    if (nonzero_tail)
    {
        // Set the last digit to be non-zero.
        // This is sufficient to guarantee correct rounding.
        DTOA_ASSERT(num_digits == kMaxSignificantDigits);
        DTOA_ASSERT(num_digits < kBufferSize);
        digits[num_digits++] = '1';
        --exponent;
    }

    double const value = DecimalToDouble(digits, num_digits, exponent);
    DTOA_ASSERT(!impl::Double(value).SignBit());

    result = is_neg ? -value : value;
    return true;
}

} // namespace base_conv

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
