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

#include <cassert>
#include <climits>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

static_assert(std::numeric_limits<double>::is_iec559 &&
              std::numeric_limits<double>::digits == 53 &&
              std::numeric_limits<double>::max_exponent == 1024,
    "The strtod/dtoa implementation requires an IEEE-754 double-precision implementation");

#if defined(_M_IX86) || defined(_M_ARM) || defined(__i386__) || defined(__arm__)
#define CC_32_BIT_PLATFORM 1
#endif

#if defined(__SIZEOF_INT128__)
#define CC_HAS_UINT128 1
#elif defined(_MSC_VER) && defined(_M_X64)
#define CC_HAS_64_BIT_INTRINSICS 1
#endif

#if defined(_MSC_VER)
#include <intrin.h>
#endif

#ifndef CC_ASSERT
#define CC_ASSERT(X) assert(X)
#endif

#ifndef CC_INLINE
#define CC_INLINE inline
#endif

#ifndef CC_FORCE_INLINE
#if defined(__GNUC__)
#define CC_FORCE_INLINE __attribute__((always_inline)) inline
#elif defined(_MSC_VER)
#define CC_FORCE_INLINE __forceinline
#else
#define CC_FORCE_INLINE inline
#endif
#endif

namespace charconv {

//==================================================================================================
// IEEE double-precision inspection
//==================================================================================================

template <typename Dest, typename Source>
CC_INLINE Dest ReinterpretBits(Source source)
{
    static_assert(sizeof(Dest) == sizeof(Source), "size mismatch");

    Dest dest;
    std::memcpy(&dest, &source, sizeof(Source));
    return dest;
}

struct Double
{
    static_assert(std::numeric_limits<double>::is_iec559
                  && (std::numeric_limits<double>::digits == 53 && std::numeric_limits<double>::max_exponent == 1024),
        "IEEE-754 double-precision implementation required");

    using value_type = double;
    using bits_type = uint64_t;

    static constexpr int       SignificandSize         = std::numeric_limits<value_type>::digits; // = p   (includes the hidden bit)
    static constexpr int       PhysicalSignificandSize = SignificandSize - 1;                     // = p-1 (excludes the hidden bit)
    static constexpr int       UnbiasedMinExponent     = 1;
    static constexpr int       UnbiasedMaxExponent     = 2 * std::numeric_limits<value_type>::max_exponent - 1 - 1;
    static constexpr int       ExponentBias            = 2 * std::numeric_limits<value_type>::max_exponent / 2 - 1 + (SignificandSize - 1);
    static constexpr int       MinExponent             = UnbiasedMinExponent - ExponentBias;
    static constexpr int       MaxExponent             = UnbiasedMaxExponent - ExponentBias;
    static constexpr bits_type HiddenBit               = bits_type{1} << (SignificandSize - 1);   // = 2^(p-1)
    static constexpr bits_type SignificandMask         = HiddenBit - 1;                           // = 2^(p-1) - 1
    static constexpr bits_type ExponentMask            = bits_type{2 * std::numeric_limits<value_type>::max_exponent - 1} << PhysicalSignificandSize;
    static constexpr bits_type SignMask                = ~(~bits_type{0} >> 1);

    bits_type /*const*/ bits;

    explicit Double(bits_type bits_) : bits(bits_) {}
    explicit Double(value_type value) : bits(ReinterpretBits<bits_type>(value)) {}

    bits_type PhysicalSignificand() const {
        return bits & SignificandMask;
    }

    bits_type PhysicalExponent() const {
        return (bits & ExponentMask) >> PhysicalSignificandSize;
    }

    // Returns whether x is zero, subnormal or normal (not infinite or NaN).
    bool IsFinite() const {
        return (bits & ExponentMask) != ExponentMask;
    }

    // Returns whether x is infinite.
    bool IsInf() const {
        return (bits & ExponentMask) == ExponentMask && (bits & SignificandMask) == 0;
    }

    // Returns whether x is a NaN.
    bool IsNaN() const {
        return (bits & ExponentMask) == ExponentMask && (bits & SignificandMask) != 0;
    }

    // Returns whether x is +0 or -0 or subnormal.
    bool IsSubnormalOrZero() const {
        return (bits & ExponentMask) == 0;
    }

    // Returns whether x is subnormal (not zero or normal or infinite or NaN).
    bool IsSubnormal() const {
        return (bits & ExponentMask) == 0 && (bits & SignificandMask) != 0;
    }

    // Returns whether x is +0 or -0.
    bool IsZero() const {
        return (bits & ~SignMask) == 0;
    }

    // Returns whether x is -0.
    bool IsMinusZero() const {
        return bits == SignMask;
    }

    // Returns whether x has negative sign. Applies to zeros and NaNs as well.
    bool SignBit() const {
        return (bits & SignMask) != 0;
    }

    value_type Value() const {
        return ReinterpretBits<value_type>(bits);
    }

    value_type AbsValue() const {
        return ReinterpretBits<value_type>(bits & ~SignMask);
    }

    value_type NextValue() const {
        CC_ASSERT(!SignBit());
        return ReinterpretBits<value_type>(IsInf() ? bits : bits + 1);
    }
};

CC_INLINE bool operator==(Double x, Double y) { return x.bits == y.bits; }
CC_INLINE bool operator!=(Double x, Double y) { return x.bits != y.bits; }

//==================================================================================================
//
//==================================================================================================

struct Uint64x2 {
    uint64_t hi;
    uint64_t lo;
};

CC_FORCE_INLINE Uint64x2 Mul128(uint64_t a, uint64_t b)
{
#if CC_HAS_UINT128
    __extension__ using uint128_t = unsigned __int128;

    uint128_t const product = uint128_t{a} * b;

    uint64_t const lo = static_cast<uint64_t>(product);
    uint64_t const hi = static_cast<uint64_t>(product >> 64);
    return {hi, lo};
#elif CC_HAS_64_BIT_INTRINSICS
    uint64_t hi;
    uint64_t lo = _umul128(a, b, &hi);
    return {hi, lo};
#else
    uint32_t const aLo = static_cast<uint32_t>(a);
    uint32_t const aHi = static_cast<uint32_t>(a >> 32);
    uint32_t const bLo = static_cast<uint32_t>(b);
    uint32_t const bHi = static_cast<uint32_t>(b >> 32);

    uint64_t const b00 = uint64_t{aLo} * bLo;
    uint64_t const b01 = uint64_t{aLo} * bHi;
    uint64_t const b10 = uint64_t{aHi} * bLo;
    uint64_t const b11 = uint64_t{aHi} * bHi;

    uint32_t const b00Lo = static_cast<uint32_t>(b00);
    uint32_t const b00Hi = static_cast<uint32_t>(b00 >> 32);

    uint64_t const mid1 = b10 + b00Hi;
    uint32_t const mid1Lo = static_cast<uint32_t>(mid1);
    uint32_t const mid1Hi = static_cast<uint32_t>(mid1 >> 32);

    uint64_t const mid2 = b01 + mid1Lo;
    uint32_t const mid2Lo = static_cast<uint32_t>(mid2);
    uint32_t const mid2Hi = static_cast<uint32_t>(mid2 >> 32);

    uint64_t const hi = b11 + mid1Hi + mid2Hi;
    uint64_t const lo = (uint64_t{mid2Lo} << 32) | b00Lo;
    return {hi, lo};
#endif
}

CC_FORCE_INLINE uint64_t ShiftRight128(uint64_t lo, uint64_t hi, unsigned char dist)
{
    // For the __shiftright128 intrinsic, the shift value is always modulo 64.
    // In the current implementation of the double-precision version of Ryu, the
    // shift value is always < 64.
    // Check this here in case a future change requires larger shift values. In
    // this case this function needs to be adjusted.
    CC_ASSERT(dist >= 56); // 56: MulShiftAll fallback, 57: otherwise.
    CC_ASSERT(dist <= 63);

#if CC_HAS_UINT128
    __extension__ using uint128_t = unsigned __int128;

    return static_cast<uint64_t>(((uint128_t{hi} << 64) | lo) >> dist);
#elif CC_HAS_64_BIT_INTRINSICS
    return __shiftright128(lo, hi, dist);
#else
#if CC_32_BIT_PLATFORM
    // Avoid a 64-bit shift by taking advantage of the range of shift values.
    // We know that 0 <= 64 - dist < 32 and 0 <= dist - 32 < 32.
    const unsigned char lshift = -dist & 31; // == (64 - dist) % 32
    const unsigned char rshift =  dist & 31; // == (dist - 32) % 32
#if defined(_MSC_VER) && !defined(__clang__)
    return __ll_lshift(hi, lshift) | (static_cast<uint32_t>(lo >> 32) >> rshift);
#else
    return (hi << lshift) | (static_cast<uint32_t>(lo >> 32) >> rshift);
#endif
#else // ^^^ CC_32_BIT_PLATFORM ^^^
    return (hi << (64 - dist)) | (lo >> dist);
#endif // ^^^ not CC_32_BIT_PLATFORM ^^^
#endif
}

// TODO:
// Technically, right shifting negative integers is implementation-defined.

// Returns: floor(log_2(5^e))
CC_INLINE int FloorLog2Pow5(int e)
{
    CC_ASSERT(e >= -1764);
    CC_ASSERT(e <=  1763);
    return (e * 1217359) >> 19;
}

// Returns: ceil(log_2(5^e))
CC_INLINE int CeilLog2Pow5(int e)
{
    CC_ASSERT(e >= -1764);
    CC_ASSERT(e <=  1763);
    return (e * 1217359 + ((1 << 19) - 1)) >> 19;
}

// Returns: floor(log_2(10^e))
// Equivalent to: e + FloorLog2Pow5(e)
CC_INLINE int FloorLog2Pow10(int e)
{
    CC_ASSERT(e >= -1233);
    CC_ASSERT(e <=  1232);
    return (e * 1741647) >> 19;
}

// Returns: ceil(log_2(10^e))
// Equivalent to: e + CeilLog2Pow5(e)
CC_INLINE int CeilLog2Pow10(int e)
{
    CC_ASSERT(e >= -1233);
    CC_ASSERT(e <=  1232);
    return (e * 1741647 + ((1 << 19) - 1)) >> 19;
}

// Returns: floor(log_5(2^e))
CC_INLINE int FloorLog5Pow2(int e)
{
    CC_ASSERT(e >= -1831);
    CC_ASSERT(e <=  1831);
    return (e * 225799) >> 19;
}

// Returns: ceil(log_5(2^e))
CC_INLINE int CeilLog5Pow2(int e)
{
    CC_ASSERT(e >= -1831);
    CC_ASSERT(e <=  1831);
    return (e * 225799 + ((1 << 19) - 1)) >> 19;
}

// Returns: floor(log_5(10^e))
// Equivalent to: e + FloorLog5Pow2(e)
CC_INLINE int FloorLog5Pow10(int e)
{
    CC_ASSERT(e >= -1831);
    CC_ASSERT(e <=  1831);
    return (e * 750087) >> 19;
}

// Returns: ceil(log_5(10^e))
// Equivalent to: e + CeilLog5Pow2(e)
CC_INLINE int CeilLog5Pow10(int e)
{
    CC_ASSERT(e >= -1831);
    CC_ASSERT(e <=  1831);
    return (e * 750087 + ((1 << 19) - 1)) >> 19;
}

// Returns: floor(log_10(2^e))
CC_INLINE int FloorLog10Pow2(int e)
{
    CC_ASSERT(e >= -2620);
    CC_ASSERT(e <=  2620);
    return (e * 315653) >> 20;
}

// Returns: ceil(log_10(2^e))
CC_INLINE int CeilLog10Pow2(int e)
{
    CC_ASSERT(e >= -2620);
    CC_ASSERT(e <=  2620);
    return (e * 315653 + ((1 << 20) - 1)) >> 20;
}

// Returns: floor(log_10(5^e))
CC_INLINE int FloorLog10Pow5(int e)
{
    CC_ASSERT(e >= -2620);
    CC_ASSERT(e <=  2620);
    return (e * 732923) >> 20;
}

// Returns: ceil(log_10(5^e))
CC_INLINE int CeilLog10Pow5(int e)
{
    CC_ASSERT(e >= -2620);
    CC_ASSERT(e <=  2620);
    return (e * 732923 + ((1 << 20) - 1)) >> 20;
}

} // namespace charconv
