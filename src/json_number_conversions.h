// Copyright 2018 Alexander Bolz
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

#include "charconv/common.h"

#include <cassert>
#include <cstdint>
#include <climits>
#include <limits>

#ifndef JSON_ASSERT
#define JSON_ASSERT(X) assert(X)
#endif

//==================================================================================================
//
//==================================================================================================

namespace json {

#if defined(__GNUC__) // && !defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#endif

struct int53_t;
struct uint53_t;

struct int53_t
{
    union {
        int64_t s : 53;
        uint64_t u : 53;
    };
    int53_t(signed int v) : s(v) {}
#if LONG_MAX == INT_MAX
    int53_t(signed long v) : s(v) {}
#else
    explicit int53_t(signed long v) : s(v) {}
#endif
    explicit int53_t(signed long long v) : s(v) {}
    explicit int53_t(uint53_t v);

    explicit operator signed int() const { return static_cast<signed int >(s); }
#if LONG_MAX == INT_MAX
    explicit operator signed long() const { return static_cast<signed long>(s); }
#else
    operator signed long() const { return s; }
#endif
    operator signed long long() const { return s; }
};

struct uint53_t
{
    union {
        int64_t s : 53;
        uint64_t u : 53;
    };
    uint53_t(unsigned int v) : u(v) {}
#if LONG_MAX == INT_MAX
    uint53_t(unsigned long v) : u(v) {}
#else
    explicit uint53_t(unsigned long v) : u(v) {}
#endif
    explicit uint53_t(unsigned long long v) : u(v) {}
    explicit uint53_t(int53_t v);

    explicit operator unsigned int() const { return static_cast<unsigned int >(u); }
#if LONG_MAX == INT_MAX
    explicit operator unsigned long() const { return static_cast<unsigned long>(u); }
#else
    operator unsigned long() const { return u; }
#endif
    operator unsigned long long() const { return u; }
};

inline int53_t::int53_t(uint53_t v) : u(v.u) {}
inline uint53_t::uint53_t(int53_t v) : s(v.s) {}

#if defined(__GNUC__) // && !defined(__clang__)
#pragma GCC diagnostic pop
#endif

} // namespace json

//==================================================================================================
// Number conversions
//==================================================================================================

namespace json {
namespace numbers {

/*never*/inline uint32_t ToUint32(double value)
{
    //
    // https://tc39.github.io/ecma262/#sec-touint32
    //
    // The abstract operation ToUint32 converts argument to one of 2^32 integer values in the range 0 through 2^32 - 1, inclusive.
    // This abstract operation functions as follows:
    //
    //  1.  Let number be ? ToNumber(argument).
    //  2.  If number is NaN, +0, -0, +Infinity, or -Infinity, return +0.
    //  3.  Let int be the mathematical value that is the same sign as number and whose magnitude is floor(abs(number)).
    //  4.  Let int32bit be int modulo 2^32.
    //  5.  Return int32bit.
    //

    using charconv::Double;
    Double const d(value);

    uint64_t const F = d.PhysicalSignificand();
    uint64_t const E = d.PhysicalExponent();

    // Assume that x is a normalized floating-point number.
    // The special cases subnormal/zero and nan/inf are actually handled below
    // in the branches 'exponent <= -p' and 'exponent >= 32'.

    auto const significand = Double::HiddenBit | F;
    auto const exponent = static_cast<int>(E) - Double::ExponentBias;

    uint32_t bits32; // = floor(abs(x)) mod 2^32
    if (exponent <= -Double::SignificandSize)
    {
        // x = significand / 2^-exponent < 1.
        return 0u;
    }
    else if (exponent < 0)
    {
        // x = significand / 2^-exponent.
        // Discard the fractional bits (floor).
        // Since 0 < -exponent < 53, the (64-bit) right shift is well defined.
        bits32 = static_cast<uint32_t>(significand >> -exponent);
    }
    else if (exponent < 32)
    {
        // x = significand * 2^exponent.
        // Since 0 <= exponent < 32, the (32-bit) left shift is well defined.
        bits32 = static_cast<uint32_t>(significand) << exponent;
    }
    else
    {
        // x = significand * 2^exponent.
        // The lower 32 bits of x are definitely 0.
        return 0u;
    }

    return d.SignBit() ? 0 - bits32 : bits32;
}

// TODO:
// Technically, casting unsigned to signed integers is implementation-defined (if out of range).

inline int32_t ToInt32(double value)
{
    return static_cast<int32_t>(ToUint32(value));
}

inline uint16_t ToUint16(double value)
{
    return static_cast<uint16_t>(ToUint32(value));
}

inline int16_t ToInt16(double value)
{
    return static_cast<int16_t>(ToUint16(value));
}

inline uint8_t ToUint8(double value)
{
    return static_cast<uint8_t>(ToUint32(value));
}

inline int8_t ToInt8(double value)
{
    return static_cast<int8_t>(ToUint8(value));
}

/*never*/ inline uint8_t ToUint8Clamp(double value)
{
    //
    // https://tc39.github.io/ecma262/#sec-touint8clamp
    //
    // The abstract operation ToUint8Clamp converts argument to one of 2^8 integer values in the
    // range 0 through 255, inclusive.
    // This abstract operation functions as follows:
    //
    //  1.  Let number be ? ToNumber(argument).
    //  2.  If number is NaN, return +0.
    //  3.  If number <= 0, return +0.
    //  4.  If number >= 255, return 255.
    //  5.  Let f be floor(number).
    //  6.  If f + 0.5 < number, return f + 1.
    //  7.  If number < f + 0.5, return f.
    //  8.  If f is odd, return f + 1.
    //  9.  Return f.
    //

#if 1
    // The following test is equivalent to (std::isnan(value) || value <= 0.5),
    // i.e. it catches NaN's and subnormal numbers.
    if (!(value > 0.5))
        return 0;
    if (value > 254.5)
        return 255;

    // Truncate to integer.
    int32_t t = static_cast<int32_t>(value);

    // Then do the rounding.
    // Note: (value - t) is exact here (as would be (t + 0.5)).
    double const fraction = value - t;

    // Round towards +Inf
    t += (fraction >= 0.5);
    if (fraction == 0.5)
    {
        // Round to nearest-even
        t &= ~1;
    }

    JSON_ASSERT(t >= 0);
    JSON_ASSERT(t <= 255);
    return static_cast<uint8_t>(static_cast<uint32_t>(t));
#else
    using Double = charconv::Double;
    Double const d(value);

    if (d.bits <= 0x3FE0000000000000ull) // +0.0 <= x <= 1/2
    {
        return 0;
    }
    else if (d.bits <= 0x406FD00000000000ull) // 0.5 < x <= 254.5
    {
        // x = significand * 2^exponent is a normalized floating-point number.
        uint64_t const F = d.PhysicalSignificand();
        uint64_t const E = d.PhysicalExponent();

        auto const significand = Double::HiddenBit | F;
        auto const exponent = static_cast<int>(E) - Double::ExponentBias;
        JSON_ASSERT(exponent >= -1 - (Double::SignificandSize - 1));
        JSON_ASSERT(exponent <   8 - (Double::SignificandSize - 1));
        // 44 < -exponent <= 53

        uint64_t const high = uint64_t{1} << -exponent;
        uint64_t const mask = high - 1;
        uint64_t const half = high >> 1;
        uint64_t const fraction = significand & mask;

        uint32_t t = static_cast<uint32_t>(significand >> -exponent);

        // Round towards +Inf
        t += (fraction >= half);
        if (fraction == half)
        {
            // Round towards nearest-even.
            t &= ~1u;
        }

        JSON_ASSERT(t <= 255);
        return static_cast<uint8_t>(t);
    }
    else if (d.bits <= 0x7FF0000000000000ull) // 254.5 < x <= +Inf
    {
        return 255;
    }
    else // x <= -0.0 or NaN(x)
    {
        return 0;
    }
#endif
}

/*never*/inline uint53_t ToUint53(double value)
{
    using charconv::Double;
    Double const d(value);

    uint64_t const F = d.PhysicalSignificand();
    uint64_t const E = d.PhysicalExponent();

    auto const significand = Double::HiddenBit | F;
    auto const exponent = static_cast<int>(E) - Double::ExponentBias;

    uint64_t bits64;
    if (exponent <= -Double::SignificandSize)
    {
        return 0u;
    }
    else if (exponent < 0)
    {
        bits64 = significand >> -exponent;
    }
    else if (exponent < 53)
    {
        bits64 = (significand << exponent) & ((uint64_t{1} << 53) - 1);
    }
    else
    {
        return 0u;
    }

    return static_cast<uint53_t>(d.SignBit() ? (0 - bits64) : bits64);
}

inline int53_t ToInt53(double value)
{
    return static_cast<int53_t>(ToUint53(value));
}

// Returns whether x is an integer and in the range [-2^53+1, 2^53-1]
// and stores the integral value of x in result.
inline int64_t DoubleToInt53(double x, bool& is_int53)
{
    using charconv::Double;

    constexpr double kMinInt = -9007199254740991.0; // -2^53 + 1
    constexpr double kMaxInt =  9007199254740991.0; //  2^53 - 1

    if (x < kMinInt || x > kMaxInt || Double(x).IsMinusZero())
    {
        is_int53 = false;
        return 0;
    }

    int64_t const i = static_cast<int64_t>(x);
    is_int53 = (x == static_cast<double>(i));
    return i;
}

} // namespace numbers
} // namespace json
