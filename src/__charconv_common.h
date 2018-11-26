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

#ifndef CC_ASSERT
#define CC_ASSERT(X) assert(X)
#endif

// If enabled, save ~5KB (bellerophon) + ~9KB (ryu) of constant data,
// such that only ~1.4KB are used.
// Results in slightly more (and slightly slower) code, though.
#ifndef CC_OPTIMIZE_SIZE
#define CC_OPTIMIZE_SIZE 0
#endif

#ifndef CC_SINGLE_PRECISION
#define CC_SINGLE_PRECISION 0
#endif

namespace charconv {

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

template <typename FloatType>
struct IEEEFloatingPoint
{
    static_assert(std::numeric_limits<FloatType>::is_iec559
                  && ((std::numeric_limits<FloatType>::digits == 53 && std::numeric_limits<FloatType>::max_exponent == 1024) ||
                      (std::numeric_limits<FloatType>::digits == 24 && std::numeric_limits<FloatType>::max_exponent == 128)),
        "IEEE-754 double- or single-precision implementation required");

    using value_type = FloatType;
    using bits_type = std::conditional_t<sizeof(FloatType) * CHAR_BIT == 32, uint32_t, uint64_t>;

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

    explicit IEEEFloatingPoint(bits_type bits_) : bits(bits_) {}
    explicit IEEEFloatingPoint(value_type value) : bits(ReinterpretBits<bits_type>(value)) {}

    bits_type PhysicalSignificand() const {
        return bits & SignificandMask;
    }

    bits_type PhysicalExponent() const {
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

using Double = IEEEFloatingPoint<double>;
#if CC_SINGLE_PRECISION
using Single = IEEEFloatingPoint<float>;
#endif

} // namespace charconv
