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

} // namespace charconv
