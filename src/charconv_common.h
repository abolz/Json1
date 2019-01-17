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

#if defined(_M_IX86) || defined(_M_ARM) || defined(__i386__) || defined(__arm__)
#define CC_32_BIT_PLATFORM 1
#endif

#if defined(__GNUC__) && defined(__SIZEOF_INT128__)
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

// If enabled, save ~5KB (bellerophon) + ~9KB (ryu) of constant data,
// such that only ~1.4KB are used.
// Results in slightly more (and slightly slower) code, though.
#ifndef CC_OPTIMIZE_SIZE
#define CC_OPTIMIZE_SIZE 0
#endif

namespace charconv {

//==================================================================================================
// IEEE double-precision inspection
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

inline bool operator==(Double x, Double y) { return x.bits == y.bits; }
inline bool operator!=(Double x, Double y) { return x.bits != y.bits; }

//==================================================================================================
//
//==================================================================================================

struct Uint64x2 {
    uint64_t lo;
    uint64_t hi;
};

inline Uint64x2 Mul128(uint64_t a, uint64_t b)
{
#if CC_HAS_UINT128
    __extension__ using uint128_t = unsigned __int128;

    uint128_t const product = uint128_t{a} * b;

    uint64_t const lo = static_cast<uint64_t>(product);
    uint64_t const hi = static_cast<uint64_t>(product >> 64);

    return {lo, hi};
#elif CC_HAS_64_BIT_INTRINSICS
    uint64_t hi;
    uint64_t lo = _umul128(a, b, &hi);
    return {lo, hi};
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
    uint64_t const lo = (uint64_t{mid2Lo} << 32) + b00Lo;
    return {lo, hi};
#endif
}

} // namespace charconv
