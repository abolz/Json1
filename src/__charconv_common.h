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
#include <cstdint>
#include <cstring>
#include <limits>

#ifndef CC_ASSERT
#define CC_ASSERT(X) assert(X)
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

template <typename FloatType, typename BitsType>
struct IEEEFloatingPoint
{
    static_assert(std::numeric_limits<FloatType>::is_iec559
                  && ((std::numeric_limits<FloatType>::digits == 53 && std::numeric_limits<FloatType>::max_exponent == 1024) ||
                      (std::numeric_limits<FloatType>::digits == 24 && std::numeric_limits<FloatType>::max_exponent == 128)),
        "IEEE-754 double- or single-precision implementation required");

    static constexpr int      SignificandSize         = std::numeric_limits<FloatType>::digits; // = p   (includes the hidden bit)
    static constexpr int      PhysicalSignificandSize = SignificandSize - 1;                    // = p-1 (excludes the hidden bit)
    static constexpr int      UnbiasedMinExponent     = 1;
    static constexpr int      UnbiasedMaxExponent     = 2 * std::numeric_limits<FloatType>::max_exponent - 1 - 1;
    static constexpr int      ExponentBias            = 2 * std::numeric_limits<FloatType>::max_exponent / 2 - 1 + (SignificandSize - 1);
    static constexpr int      MinExponent             = UnbiasedMinExponent - ExponentBias;
    static constexpr int      MaxExponent             = UnbiasedMaxExponent - ExponentBias;
    static constexpr BitsType HiddenBit               = BitsType{1} << (SignificandSize - 1);   // = 2^(p-1)
    static constexpr BitsType SignificandMask         = HiddenBit - 1;                          // = 2^(p-1) - 1
    static constexpr BitsType ExponentMask            = BitsType{2 * std::numeric_limits<FloatType>::max_exponent - 1} << PhysicalSignificandSize;
    static constexpr BitsType SignMask                = ~(~BitsType{0} >> 1);

    BitsType /*const*/ bits;

    explicit IEEEFloatingPoint(BitsType bits_) : bits(bits_) {}
    explicit IEEEFloatingPoint(FloatType value) : bits(ReinterpretBits<BitsType>(value)) {}

    BitsType PhysicalSignificand() const {
        return bits & SignificandMask;
    }

    BitsType PhysicalExponent() const {
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

    FloatType Value() const {
        return ReinterpretBits<FloatType>(bits);
    }

    FloatType AbsValue() const {
        return ReinterpretBits<FloatType>(bits & ~SignMask);
    }

    FloatType NextValue() const {
        CC_ASSERT(!SignBit());
        return ReinterpretBits<FloatType>(IsInf() ? bits : bits + 1);
    }
};

using Double = IEEEFloatingPoint<double, uint64_t>;
#if CC_SINGLE_PRECISION
using Single = IEEEFloatingPoint<float, uint32_t>;
#endif

} // namespace charconv
