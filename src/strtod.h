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

#include <cstdio>

#include "dtoa.h" // DiyFp, GetCachedPower

// If 0, the result of StringToIeee might be off by 1 ULP
#define DTOA_STRTOD_FULL_PRECISION 1

namespace base_conv {

inline constexpr int Min(int x, int y) { return y < x ? y : x; }
inline constexpr int Max(int x, int y) { return y < x ? x : y; }

// Returns `f * 2^e`.
// Note: the significand is truncated to Float::precision bits.
template <typename Float>
inline Float FloatFromDiyFp(DiyFp v)
{
    using Traits = IEEE<Float>;
    using Bits = typename Traits::bits_type;

    auto f = v.f;
    auto e = v.e;

    while (f > Traits::HiddenBit + Traits::SignificandMask) {
        f >>= 1;
        DTOA_ASSERT(e != INT_MAX);
        e++;
    }
    if (e > Traits::MaxExponent) {
        return std::numeric_limits<Float>::infinity();
    }
    if (e < Traits::MinExponent) {
        return 0.0;
    }
    while (e > Traits::MinExponent && (f & Traits::HiddenBit) == 0) {
        f <<= 1;
        e--;
    }

    Bits const exponent = (e == Traits::MinExponent && (f & Traits::HiddenBit) == 0)
        ? 0
        : static_cast<Bits>(e + Traits::ExponentBias);

    Bits const bits = (exponent << Traits::PhysicalSignificandSize) | (f & Traits::SignificandMask);
    return ReinterpretBits<Float>(bits);
}

//--------------------------------------------------------------------------------------------------
// StringToDoubleFast
//--------------------------------------------------------------------------------------------------

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

inline bool StringToIeeeFast(char const* /*buffer*/, int /*buffer_length*/, int /*exponent*/, double& /*result*/)
{
    // On x86 the floating-point stack can be 64 or 80 bits wide. If it is
    // 80 bits wide then double-rounding occurs and the result is not accurate.
    return false;
}

#else // ^^^ !DTOA_CORRECT_DOUBLE_OPERATIONS

// 2^53 = 9007199254740992.
// Any integer with at most 15 decimal digits will hence fit into a double
// (which has a 53bit significand) without loss of precision.
constexpr int kMaxExactDoubleIntegerDecimalDigits = 15;

inline double ReadDouble_unsafe(char const* buffer, int buffer_length)
{
    DTOA_ASSERT(buffer_length >= 0);
    DTOA_ASSERT(buffer_length <= kMaxExactDoubleIntegerDecimalDigits);

    int64_t result = 0;

    for (int i = 0; i < buffer_length; ++i)
    {
        DTOA_ASSERT(buffer[i] >= '0');
        DTOA_ASSERT(buffer[i] <= '9');
        result = 10 * result + (buffer[i] - '0');
    }

    return static_cast<double>(result);
}

inline bool StringToIeeeFast(char const* buffer, int buffer_length, int exponent, double& result)
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

    if (buffer_length > kMaxExactDoubleIntegerDecimalDigits)
        return false;

    // The significand fits into a double.
    // If 10^exponent (resp. 10^-exponent) fits into a double too then we can
    // compute the result simply by multiplying (resp. dividing) the two
    // numbers.
    // This is possible because IEEE guarantees that floating-point operations
    // return the best possible approximation.

    int const remaining_digits = kMaxExactDoubleIntegerDecimalDigits - buffer_length; // 0 <= rd <= 15
    if (-kMaxExactPowerOfTen <= exponent && exponent <= remaining_digits + kMaxExactPowerOfTen)
    {
        auto d = ReadDouble_unsafe(buffer, buffer_length);
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

#if 1 // !DTOA_CORRECT_DOUBLE_OPERATIONS
inline bool StringToIeeeFast(char const* /*buffer*/, int /*buffer_length*/, int /*exponent*/, float& /*result*/)
{
    // On x86 the floating-point stack can be 64 or 80 bits wide. If it is
    // 80 bits wide then double-rounding occurs and the result is not accurate.
    return false;
}
#else
// 2^24 = 16777216.
// Any integer with at most 7 decimal digits will hence fit into a single
// (which has a 24bit significand) without loss of precision.
constexpr int kMaxExactSingleIntegerDecimalDigits = 7;

inline float ReadSingle_unsafe(char const* buffer, int buffer_length)
{
    DTOA_ASSERT(buffer_length >= 0);
    DTOA_ASSERT(buffer_length <= kMaxExactSingleIntegerDecimalDigits);

    int32_t result = 0;

    for (int i = 0; i < buffer_length; ++i)
    {
        DTOA_ASSERT(buffer[i] >= '0');
        DTOA_ASSERT(buffer[i] <= '9');
        result = 10 * result + (buffer[i] - '0');
    }

    return static_cast<float>(result);
}

inline bool StringToIeeeFast(char const* buffer, int buffer_length, int exponent, float& result)
{
    static constexpr int kMaxExactPowerOfTen = 10;
    static constexpr float kExactPowersOfTen[] = {
        1.0e+00f,
        1.0e+01f,
        1.0e+02f,
        1.0e+03f,
        1.0e+04f,
        1.0e+05f,
        1.0e+06f,
        1.0e+07f,
        1.0e+08f,
        1.0e+09f,
        1.0e+10f,
//      1.0e+11f,
    };

    if (buffer_length > kMaxExactSingleIntegerDecimalDigits)
        return false;

    int const remaining_digits = kMaxExactSingleIntegerDecimalDigits - buffer_length;
    if (-kMaxExactPowerOfTen <= exponent && exponent <= remaining_digits + kMaxExactPowerOfTen)
    {
        auto d = ReadSingle_unsafe(buffer, buffer_length);
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
            d *= kExactPowersOfTen[remaining_digits]; // exact
            d *= kExactPowersOfTen[exponent - remaining_digits];
        }
        result = d;
        return true;
    }

    return false;
}
#endif

#endif // DTOA_CORRECT_DOUBLE_OPERATIONS

//--------------------------------------------------------------------------------------------------
// StringToDoubleApprox
//--------------------------------------------------------------------------------------------------

// 2^64 = 18446744073709551616 > 10^19
// Any integer with at most 19 decimal digits will hence fit into an uint64_t.
constexpr int kMaxUint64DecimalDigits = 19;

// Reads a (rounded) DiyFp from the buffer.
//
// If read_digits == buffer_length then the returned DiyFp is accurate.
// Otherwise it has been rounded and has an error of at most 1/2 ulp.
//
// The returned DiyFp is not normalized.
inline DiyFp ReadDiyFp(char const* buffer, int buffer_length, int& read_digits)
{
    DTOA_ASSERT(buffer_length > 0);
    DTOA_ASSERT(buffer[buffer_length - 1] != '0'); // The buffer has been trimmed.

    uint64_t significand = 0;

    int const max_len = buffer_length < kMaxUint64DecimalDigits ? buffer_length : kMaxUint64DecimalDigits;
    int i = 0;
    for ( ; i < max_len; ++i)
    {
        DTOA_ASSERT(buffer[i] >= '0');
        DTOA_ASSERT(buffer[i] <= '9');
        significand = 10 * significand + static_cast<uint32_t>(buffer[i] - '0');
    }

    if (i < buffer_length)
    {
        DTOA_ASSERT(buffer[i] >= '0');
        DTOA_ASSERT(buffer[i] <= '9');
        if (buffer[i] >= '5')
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
#if 0
        { 0xBEBC200000000000, -37 }, // == 10^8
        { 0xEE6B280000000000, -34 }, // == 10^9
        { 0x9502F90000000000, -30 }, // == 10^10
        { 0xBA43B74000000000, -27 }, // == 10^11
        { 0xE8D4A51000000000, -24 }, // == 10^12
        { 0x9184E72A00000000, -20 }, // == 10^13
        { 0xB5E620F480000000, -17 }, // == 10^14
        { 0xE35FA931A0000000, -14 }, // == 10^15
        { 0x8E1BC9BF04000000, -10 }, // == 10^16
        { 0xB1A2BC2EC5000000,  -7 }, // == 10^17
        { 0xDE0B6B3A76400000,  -4 }, // == 10^18
        { 0x8AC7230489E80000,   0 }, // == 10^19
        { 0xAD78EBC5AC620000,   3 }, // == 10^20
        { 0xD8D726B7177A8000,   6 }, // == 10^21
        { 0x878678326EAC9000,  10 }, // == 10^22
        { 0xA968163F0A57B400,  13 }, // == 10^23
        { 0xD3C21BCECCEDA100,  16 }, // == 10^24
        { 0x84595161401484A0,  20 }, // == 10^25
        { 0xA56FA5B99019A5C8,  23 }, // == 10^26
        { 0xCECB8F27F4200F3A,  26 }, // == 10^27
#endif
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
// If v = f*2^e with 2^p-1 <= f <= 2^p then p+e is v's order of magnitude.
//
// This function returns the number of significant binary digits v will have
// once it's encoded into a double. In almost all cases this is equal to
// SignificandSize. The only exceptions are denormals. They start with
// leading zeroes and their effective significand-size is hence smaller.
template <typename Float>
inline int SignificandSizeForOrderOfMagnitude(int order)
{
    using Traits = IEEE<Float>;

    if (order >= Traits::MinExponent + Traits::SignificandSize)
        return Traits::SignificandSize;

    if (order <= Traits::MinExponent)
        return 0;

    return order - Traits::MinExponent;
}

// If the function returns true then the result is the correct double.
// Otherwise it is either the correct double or the double that is just below
// the correct double.
//
// PRE: buffer_length + exponent <= kMaxDecimalPower
// PRE: buffer_length + exponent >  kMinDecimalPower
template <typename Float>
inline bool StringToIeeeApprox(char const* buffer, int buffer_length, int exponent, Float& result)
{
    // TODO:
    // Understand this...

    static_assert(DiyFp::SignificandSize == 64,
        "We use uint64_ts. This only works if the DiyFp uses uint64_ts too.");

    DTOA_ASSERT(buffer_length + exponent <= kMaxDecimalPower);
    DTOA_ASSERT(buffer_length + exponent >  kMinDecimalPower);

    // 1)
    //
    // Compute an approximation `input` of B using DiyFp's.
    //
    //                       <-- error -->
    //                               B = buffer * 10^exponent
    //  ---------[-----------|-------+---]------------------------------------
    //                       input
    //                       ~= buffer * 10^exponent

    int read_digits;
    auto input = ReadDiyFp(buffer, buffer_length, read_digits);

    int const remaining_decimals = buffer_length - read_digits; // 0 <= rd < buffer_length

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

        input = Multiply(input, adjustment_power);
        // input ~= digits * 10^adjustment_exponent

        // Adjust error.
        // The adjustment_power is exact (error_b = error_ab = 0).
        // There is hence only an additional error of (at most) 1/2.
        if (buffer_length + adjustment_exponent <= kMaxUint64DecimalDigits)
        {
            // input and adjustment_power are exact.
            // The product buffer * 10^adjustment_exponent fits into an uint64_t.
            // input * adjustment_power is therefore exact, too.
        }
        else
        {
            error += kDenominator / 2;
        }
    }

    input = Multiply(input, cached_power);
    // input ~= digits * 10^exponent

    // Adjust error.
    // Substituting a with 'input' and b with 'cached_power' we have
    //   error_b  = 0.5  (all cached powers have an error of less than 0.5 ulp),
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

    // 2)
    //
    // We now have an approximation B = buffer * 10^exponent.
    //
    //                       <-- error -->
    //                               B = buffer * 10^exponent
    //  ---------[-----------|-------+---]------------------------------------
    //                       input
    //                       ~= buffer * 10^exponent

    // See if the double's significand changes if we add/subtract the error.

    // For v = f * 2^e, where 2^(q-1) <= f < 2^q, the order of magnitude is q+e.
    int const order_of_magnitude = DiyFp::SignificandSize + input.e;

    // Determine the number of significand digits needed the DiyFp `input` will
    // have once encoded as an IEEE double-precision number.
    int const effective_significand_size = SignificandSizeForOrderOfMagnitude<Float>(order_of_magnitude);
    DTOA_ASSERT(effective_significand_size >= 0);
    DTOA_ASSERT(effective_significand_size <= 53);

    // Determine the effective number of extra bits of precision we have and see
    // if these extra bits of precision are sufficient to ensure that the
    // rounded `input` will be correctly rounded.
    // (When converting 'input' to double, the lower 'extra_bits' will be discarded.)
    int extra_bits = DiyFp::SignificandSize - effective_significand_size;
    DTOA_ASSERT(extra_bits >= 11);
    DTOA_ASSERT(extra_bits <= 64);

    if (extra_bits + kDenominatorLog >= DiyFp::SignificandSize)
    {
        // This can only happen for very small denormals. In this case
        // 'half' (see below) multiplied by kDenominator exceeds the range of an uint64.
        // Simply shift everything to the right.

        // input = f * 2^e ~= floor(f / 2^s) * 2^(e + s)
        int const s = (extra_bits + kDenominatorLog) - DiyFp::SignificandSize + 1;
        input.f >>= s;
        input.e  += s;

        // error * 2^e ~= floor(error / 2^s) * 2^(e + s)
        error >>= s;

        // We add kDenominator for the lost precision of input.f
        error += kDenominator;  // kDenominator = 1 ulp
        // We add 1 for the lost precision of error,
        error += 1;             // 1 = 1/kDenominator ulp

        //extra_bits -= s;
        extra_bits = DiyFp::SignificandSize - kDenominatorLog - 1;
    }

    DTOA_ASSERT(extra_bits >= 11);
    DTOA_ASSERT(extra_bits <  DiyFp::SignificandSize - kDenominatorLog);

    // Split the significand f of input into an integral and a fractional part:
    //  f = iiii.ffff = int * 2^extra_bits + frac
    // where
    //  frac < 2^extra_bits

    uint64_t frac = input.f & ((uint64_t{1} << extra_bits) - 1);
    uint64_t half = uint64_t{1} << (extra_bits - 1);
    // error is scaled by kDenominator.
    // In order to compare 'frac' and 'half' with error, these values need to be
    // scaled, too.
    frac *= kDenominator;
    half *= kDenominator;

    DTOA_ASSERT(half <= UINT64_MAX - error);

    // -+-----------+-----------+----------------------------
    //  h=           f

    // Round the significand to p bits: f = round(f / 2^prec) * 2^prec, move the
    // factor of 2^prec into the exponent.
    input.f >>= extra_bits;
    input.e  += extra_bits;
    if (frac >= half + error)
    {
        // Round up.
        DTOA_ASSERT(input.f != UINT64_MAX);
        ++input.f;
    }
    else
    {
        // Round down.

        // If the last_bits are too close to the half-way case than we are too
        // inaccurate and round down. In this case we return false so that we can
        // fall back to a more precise algorithm.
    }

    result = FloatFromDiyFp<Float>(input); // (Truncate to 53-bits and convert to double-precision.)

    if (half - error < frac && frac < half + error)
    {
        // Too imprecise. The caller will have to fall back to a slower version.
        // However the returned number is guaranteed to be either the correct
        // double, or the double just below the correct double.
        return false;
    }

    return true;
}

//--------------------------------------------------------------------------------------------------
// StringToIeee
//--------------------------------------------------------------------------------------------------

// Maximum number of significant digits in decimal representation.
// The longest possible double in decimal representation is
// (2^53 - 1) * 2^(1-1023-52) that is (2^53 - 1) * 5^1074 / 10^1074 (768 digits). If
// we parse a number whose first digits are equal to a mean of 2 adjacent
// doubles (that could have up to 769 digits) the result must be rounded to the
// bigger one unless the tail consists of zeros, so we don't need to preserve
// all the digits.
//constexpr int kMaxSignificantDigits = 772;
constexpr int kMaxSignificantDigits = 767 + 1;

// Maximum number of significant digits in the decimal representation.
// In fact the value is 772 (see conversions.cc), but to give us some margin
// we round up to 780.
constexpr int kMaxSignificantDecimalDigits = 780;

#if DTOA_STRTOD_FULL_PRECISION

struct DiyInt // bigits * 2^exponent
{
    static constexpr int MaxSignificantBits = 1074 + 2552;
    static constexpr int BigitSize = 32;
    static constexpr int Capacity = (MaxSignificantBits + (BigitSize - 1)) / BigitSize;

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
        DTOA_ASSERT(x.size + 1 <= DiyInt::Capacity);
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
        DTOA_ASSERT(x.size + 1 <= DiyInt::Capacity);
        x.bigits[x.size++] = static_cast<uint32_t>(carry);
    }
}

inline uint32_t ReadU32(char const* buffer, int num_digits)
{
    uint32_t result = 0;

    for (int i = 0; i < num_digits; ++i)
    {
        DTOA_ASSERT(buffer[i] >= '0');
        DTOA_ASSERT(buffer[i] <= '9');
        result = 10 * result + static_cast<uint32_t>(buffer[i] - '0');
    }

    return result;
}

inline void AssignDecimalString(DiyInt& x, char const* buffer, int length)
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
    while (length >= 9)
    {
        MulU32(x, 1000000000);
        AddU32(x, ReadU32(buffer, 9));
        length -= 9;
        buffer += 9;
    }
    if (length > 0)
    {
        MulU32(x, kPow10[length]);
        AddU32(x, ReadU32(buffer, length));
    }
#else
    while (length > 0)
    {
        int const n = Min(length, 9);
        MulU32(x, kPow10[n]);
        AddU32(x, ReadU32(buffer, n));
        length -= n;
        buffer += n;
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
        DTOA_ASSERT(x.size + 1 <= DiyInt::Capacity);
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

#if 1
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
    int const n1 = lhs.size + lhs.exponent;
    int const n2 = rhs.size + rhs.exponent;

    if (n1 < n2)
        return -1;
    if (n1 > n2)
        return +1;

    int ix = (n1 - 1) - lhs.exponent;
    int iy = (n2 - 1) - rhs.exponent;
    for ( ; ix >= 0 || iy >= 0; --ix, --iy)
    {
        uint32_t const bigit_x = (ix >= 0) ? lhs.bigits[ix] : 0;
        uint32_t const bigit_y = (iy >= 0) ? rhs.bigits[iy] : 0;

        if (bigit_x < bigit_y)
            return -1;
        if (bigit_x > bigit_y)
            return +1;
    }

    return 0;
}

// Compate buffer * 10^exponent with v = f * 2^e
//
// PRE: buffer_length + exponent <= kMaxDecimalPower + 1
// PRE: buffer_length + exponent >  kMinDecimalPower
// PRE: buffer_length            <= kMaxSignificantDecimalDigits
inline int CompareBufferWithDiyFp(char const* buffer, int buffer_length, int exponent, DiyFp v)
{
    DTOA_ASSERT(buffer_length + exponent <= kMaxDecimalPower + 1);
    DTOA_ASSERT(buffer_length + exponent > kMinDecimalPower);
    DTOA_ASSERT(buffer_length <= kMaxSignificantDecimalDigits);

    DiyInt lhs;
    DiyInt rhs;

    AssignDecimalString(lhs, buffer, buffer_length);
    AssignU64(rhs, v.f);

#if 1
    int lhs_exp2 = 0;
    int rhs_exp2 = 0;

    if (exponent >= 0)
        lhs_exp2 += exponent;
    else
        rhs_exp2 -= exponent;

    if (v.e >= 0)
        rhs_exp2 += v.e;
    else
        lhs_exp2 -= v.e;

    // Remove common factors of 2.
    int const min_exp2 = Min(lhs_exp2, rhs_exp2);
    lhs_exp2 -= min_exp2;
    rhs_exp2 -= min_exp2;

    auto& m5 = (exponent >= 0) ? lhs : rhs;
    MulPow5(m5, (exponent >= 0) ? exponent : -exponent);

    MulPow2(lhs, lhs_exp2);
    MulPow2(rhs, rhs_exp2);
#else
    int lhs_exp5 = 0;
    int rhs_exp5 = 0;
    int lhs_exp2 = 0;
    int rhs_exp2 = 0;

    if (exponent >= 0) {
        lhs_exp5 += exponent;
        lhs_exp2 += exponent;
    } else {
        rhs_exp5 -= exponent;
        rhs_exp2 -= exponent;
    }

    if (v.e >= 0)
        rhs_exp2 += v.e;
    else
        lhs_exp2 -= v.e;

    // Remove common factors of 2.
    int const min_exp2 = Min(lhs_exp2, rhs_exp2);
    lhs_exp2 -= min_exp2;
    rhs_exp2 -= min_exp2;

    MulPow5(lhs, lhs_exp5);
    MulPow5(rhs, rhs_exp5);

    MulPow2(lhs, lhs_exp2);
    MulPow2(rhs, rhs_exp2);
#endif

    return Compare(lhs, rhs);
}

// Returns whether the significand f of v = f * 2^e is even.
template <typename Float>
inline bool SignificandIsEven(Float v)
{
    using Traits = IEEE<Float>;

    auto const bits = ReinterpretBits<typename Traits::bits_type>(v);
    return (bits & 1) == 0;
}

// Returns the next larger double-precision value.
// If v is +Infinity returns v.
template <typename Float>
inline Float NextFloat(Float v)
{
    return std::nextafter(v, std::numeric_limits<Float>::infinity());
}

#endif // DTOA_STRTOD_FULL_PRECISION

template <typename Float>
inline bool ComputeGuess(Float& result, char const* buffer, int buffer_length, int exponent)
{
    if (buffer_length == 0) {
        result = 0;
        return true;
    }

    DTOA_ASSERT(buffer_length <= kMaxSignificantDecimalDigits);
    DTOA_ASSERT(buffer[0] != '0');
    DTOA_ASSERT(buffer[buffer_length - 1] != '0');

    // Any v >= 10^309 is interpreted as +Infinity.
    if (buffer_length + exponent >= kMaxDecimalPower + 1) {
        result = std::numeric_limits<Float>::infinity();
        return true;
    }

    // Any v <= 10^-324 is interpreted as 0.
    if (buffer_length + exponent <= kMinDecimalPower) {
        result = 0;
        return true;
    }

    if (StringToIeeeFast(buffer, buffer_length, exponent, result)) {
        return true;
    }
    if (StringToIeeeApprox(buffer, buffer_length, exponent, result)) {
        return true;
    }
    if (result == std::numeric_limits<Float>::infinity()) {
        return true;
    }

    return false;
}

template <typename Float>
inline Float StringToIeee(char const* buffer, int buffer_length, int exponent)
{
    Float v;
    if (ComputeGuess(v, buffer, buffer_length, exponent)) {
        return v;
    }

#if DTOA_STRTOD_FULL_PRECISION
    // Now v is either the correct or the next-lower double (i.e. the correct double is v+).
    // Compare B = buffer * 10^exponent with v's upper boundary m+.
    //
    //     v             m+            v+
    //  ---+--------+----+-------------+---
    //              B

    int const cmp = CompareBufferWithDiyFp(buffer, buffer_length, exponent, UpperBoundary(v));
    if (cmp < 0 || (cmp == 0 && SignificandIsEven(v))) {
        return v;
    }
    return NextFloat(v);
#else
    return v;
#endif
}

//--------------------------------------------------------------------------------------------------
// Strtod
//--------------------------------------------------------------------------------------------------
#if 0

#if 0
inline int DigitValue(char ch)
{
#define N -1
    static constexpr int8_t const kDigitValue[256] = {
    //  NUL   SOH   STX   ETX   EOT   ENQ   ACK   BEL   BS    HT    LF    VT    FF    CR    SO    SI
        N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,
    //  DLE   DC1   DC2   DC3   DC4   NAK   SYN   ETB   CAN   EM    SUB   ESC   FS    GS    RS    US
        N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,
    //  space !     "     #     $     %     &     '     (     )     *     +     ,     -     .     /
        N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,
    //  0     1     2     3     4     5     6     7     8     9     :     ;     <     =     >     ?
        0,    1,    2,    3,    4,    5,    6,    7,    8,    9,    N,    N,    N,    N,    N,    N,
    //  @     A     B     C     D     E     F     G     H     I     J     K     L     M     N     O
        N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,
    //  P     Q     R     S     T     U     V     W     X     Y     Z     [     \     ]     ^     _
        N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,
    //  `     a     b     c     d     e     f     g     h     i     j     k     l     m     n     o
        N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,
    //  p     q     r     s     t     u     v     w     x     y     z     {     |     }     ~     DEL
        N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,
        N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,
        N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,
        N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,
        N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,
        N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,
        N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,
        N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,
        N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,    N,
    };
#undef N

    return kDigitValue[static_cast<unsigned char>(ch)];
}
#else
inline int DigitValue(char ch)
{
    if ('0' <= ch && ch <= '9')
        return ch - '0';
    return -1;
}
#endif

inline bool IsDigit(char ch)
{
    return DigitValue(ch) >= 0;
}

inline int CountTrailingZeros(char const* buffer, int buffer_length)
{
    DTOA_ASSERT(buffer_length >= 0);

    int i = buffer_length;
    for ( ; i > 0; --i)
    {
        if (buffer[i - 1] != '0')
            break;
    }

    return buffer_length - i;
}

template <typename Float>
inline Float SignedZero(bool negative)
{
    return negative ? -static_cast<Float>(0) : static_cast<Float>(0);
}

template <typename Float>
inline bool Strtod(Float& result, char const* next, char const* last)
{
    // Inputs larger than kMaxInt (currently) can not be handled.
    // To avoid overflow in integer arithmetic.
    constexpr int const kMaxInt = INT_MAX / 4;

    if (next == last)
    {
        result = 0; // [Recover.]
        return true;
    }

    if (last - next >= kMaxInt)
    {
        return false;
    }

    constexpr int const kBufferSize = kMaxSignificantDigits + 1;

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
            result = SignedZero<Float>(is_neg); // Recover.
            return false;
        }
    }
    else if (/*allow_leading_plus &&*/ *next == '+')
    {
        ++next;
        if (next == last)
        {
            result = SignedZero<Float>(is_neg); // Recover.
            return false;
        }
    }

    if (*next == '0')
    {
        ++next;
        if (next == last)
        {
            result = SignedZero<Float>(is_neg);
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
    else if (allow_leading_dot && *next == '.')
    {
    }
#endif
    else
    {
#if 0
        if (allow_nan_inf && last - next >= 3 && std::memcmp(next, "NaN", 3) == 0)
        {
            result = std::numeric_limits<double>::quiet_NaN();
            return true;
        }

        if (allow_nan_inf && last - next >= 8 && std::memcmp(next, "Infinity", 8) == 0)
        {
            result = is_neg ? -std::numeric_limits<double>::infinity() : std::numeric_limits<double>::infinity();
            return true;
        }
#endif

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
                    result = SignedZero<Float>(is_neg);
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
            DTOA_ASSERT(IsDigit(*next));
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
    if (/*allow_trailing_junk &&*/ next != last)
    {
        return false; // trailing junk
    }
#endif

    if (nonzero_digit_dropped)
    {
        // Set the last digit to be non-zero.
        // This is sufficient to guarantee correct rounding.
        DTOA_ASSERT(length == kMaxSignificantDigits);
        DTOA_ASSERT(length < kBufferSize);
        buffer[length++] = '1';
        --exponent;
    }
    else
    {
        int const num_zeros = CountTrailingZeros(buffer, length);
        length   -= num_zeros;
        exponent += num_zeros;
    }

    auto value = StringToIeee<Float>(buffer, length, exponent);
    result = is_neg ? -value : value;
    return true;
}

#endif

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
